/*
*	Gates.cpp, Copyright Jonathan Mackey 2020
*	Class to manage blast gate sensors.
*
*	GNU license:
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*	Please maintain this license information along with authorship and copyright
*	notices in any redistribution of this code.
*
*/
#include "Gates.h"
#include "DataStream.h"
#include <string.h>

#ifndef __MACH__
#include <Arduino.h>
#include "SdFat.h"
#include "sdios.h"
#else
#include <stdio.h>
#endif
#include "CSVUtils.h"
#include "UnixTime.h"
#include "DCConfig.h"

// There are up to 32 gates.  The stream contains space for 32 gates + the root.
// Each SGateLink is 22 bytes.  33 * 22 = 726
DataStream_E	gatesDataStream((void *)DCConfig::kGatesDataAddr, sizeof(SGateLink) * 33);

/******************************** Gates ********************************/
Gates::Gates(void)
	: mGates(nullptr), mCurrentIndex(0), mCount(0)
{
}

#if 0
/************************************ Dump ************************************/
void Gates::Dump(void)
{
	SGateRoot	root;
	ReadGate(0, &root);
#ifdef __MACH__
	fprintf(stderr, "root tail = %hd, head = %hd, freeHead = %hd\n", root.tail, root.head, root.freeHead);
#else
	Serial.print(F("root tail = "));
	Serial.print(root.tail);
	Serial.print(F(", head = "));
	Serial.print(root.head);
	Serial.print(F(", freeHead = "));
	Serial.println(root.freeHead);
#endif

	SGateLink	link;
	for (uint8_t i = 1; i < 4; i++)
	{
		ReadGate(i, &link);
#ifdef __MACH__
		fprintf(stderr, "[%hd] prev = %hd, next = %hd, name = \"%s\"\n", i, link.prev, link.next, link.name);
#else
		Serial.print('[');
		Serial.print(i);
		Serial.print(F("] prev = "));
		Serial.print(link.prev);
		Serial.print(F(", next = "));
		Serial.print(link.next);
		Serial.print(F(", name = \""));
		Serial.print(link.name);
		Serial.println('\"');
#endif
	}
#ifdef __MACH__
	fprintf(stderr, "\n");
#else
	Serial.println();
#endif
}
#endif

/*********************************** begin ************************************/
void Gates::begin(void)
{
	mGates = &gatesDataStream;
	if (mGates)
	{
		SGateRoot	root;
		ReadGate(0, &root);

		if (root.tail)
		{
			if (root.tail != 0xFF)
			{
				uint8_t	count = 1;
				ReadGate(root.tail, &mCurrent);
				while (mCurrent.prev)
				{
					count++;
					ReadGate(mCurrent.prev, &mCurrent);
				}
				mCurrentIndex = root.head;
				mCount = count;
			} else
			{
				RemoveAllGates();
			}
		} // Else there are no gates
	}
}

/******************************** GetNextIndex ********************************/
/*
*	Gets the next sorted physical index without loading
*/
uint8_t Gates::GetNextIndex(
	bool	inWrap) const
{
	uint8_t	index = 0;
	if (mCurrentIndex != 0)
	{
		if (mCurrent.next)
		{
			index = mCurrent.next;
		} else if (inWrap &&
			mCurrent.prev)
		{
			SGateRoot	root;
			ReadGate(0, &root);
			index = root.head;
		}
	}
	return(index);
}

/************************************ Next ************************************/
bool Gates::Next(
	bool	inWrap)
{
	uint8_t	nextindex = GetNextIndex(inWrap);
	bool	success = nextindex != 0;
	if (success)
	{
		GoToGate(nextindex);
	}
	return(success);
}

/****************************** GetPreviousIndex ******************************/
uint8_t Gates::GetPreviousIndex(
	bool	inWrap) const
{
	uint8_t	index = 0;
	if (mCurrentIndex != 0)
	{
		if (mCurrent.prev)
		{
			index = mCurrent.prev;
		} else if (inWrap &&
			mCurrent.next)
		{
			SGateRoot	root;
			ReadGate(0, &root);
			index = root.tail;
		}
	}
	return(index);
}

/********************************** Previous **********************************/
bool Gates::Previous(
	bool	inWrap)
{
	uint8_t	prevIndex = GetPreviousIndex(inWrap);
	bool	success = prevIndex != 0;
	if (success)
	{
		GoToGate(prevIndex);
	}
	return(success);
}

/********************************** GoToGate **********************************/
/*
*	Does no bounds checking.
*/
void Gates::GoToGate(
	uint8_t	inRecIndex)
{
	if (mCurrentIndex != inRecIndex)
	{
		mCurrentIndex = inRecIndex;
		ReadGate(inRecIndex, &mCurrent);
	}
}

/******************************** GoToNthGate *********************************/
/*
*	Returns true if there is an Nth record.
*/
bool Gates::GoToNthGate(
	uint8_t	inLogIndex)
{
	SGateRoot	root;
	ReadGate(0, &root);
	bool	success = root.head != 0;
	if (success)
	{
		GoToGate(root.head);
		
		for (uint8_t i = 0; i < inLogIndex; i++)
		{
			if (mCurrent.next)
			{
				GoToGate(mCurrent.next);
			} else
			{
				success = false;
				break;
			}
		}
	}
	return(success);
}

/****************************** GoToRelativeGate ******************************/
/*
*	Starting from the current gate, go to the logical position relative to
*	the current gate by inRelLogIndex. 
*/
bool Gates::GoToRelativeGate(
	int16_t	inRelLogIndex)
{
	bool success = true;
	if (inRelLogIndex > 0)
	{
		do
		{
			success = Next(false);
			inRelLogIndex--;
		} while (success && inRelLogIndex);
	} else if (inRelLogIndex < 0)
	{
		do
		{
			success = Previous(false);
			inRelLogIndex++;
		} while (success && inRelLogIndex);
	}
	return(success);
}

/****************************** GetLogicalIndex *******************************/
/*
*	Starting from the root head, count indexes till the current gate is hit.
*	-1 is returned if there is no current.  This should only happen when there
*	are no gates.
*/
int8_t Gates::GetLogicalIndex(void) const
{
	int8_t	logIndex = 0;
	if (mCurrentIndex)
	{
		SGateLink	thisGate;
		uint8_t	next = 0;
		uint8_t	currentPrev = mCurrent.prev;
		while (next != currentPrev)
		{
			logIndex++;
			ReadGate(next, &thisGate);
			next = thisGate.next;
		}
	} else
	{
		logIndex = -1;
	}
	return(logIndex);
}

/*********************************** AddNew ***********************************/
/*
*	Adds a new untitled gate.
*	The new gate becomes the current gate.
*/
uint8_t Gates::AddNew(void)
{
	SGateLink	newGate;
	
	uint8_t	count = (uint8_t)mCount + 1;
	newGate.name[0] = (count / 10) + '0';
	newGate.name[1] = (count % 10) + '0';
	newGate.name[2] = 0;
	return(Add(newGate));
}

/************************************ Add *************************************/
/*
*	The added gate becomes the current gate.
*/
uint8_t Gates::Add(
	SGateLink&	inGate)
{
	int16_t leftIndex = 0;
	int16_t	currLogIndex = GetLogicalIndex();
	if (currLogIndex >= 0)
	{
		int16_t current = 0;
		int16_t rightIndex = mCount -1;
		const char*	gateName = inGate.name;
		while (leftIndex <= rightIndex)
		{
			current = (leftIndex + rightIndex) / 2;
			GoToRelativeGate(current - currLogIndex);
			currLogIndex = current;
			
			int	cmpResult = strcmp(mCurrent.name, gateName);
			if (cmpResult == 0)
			{
				leftIndex = current;
				break;
			} else if (cmpResult > 0)
			{
				rightIndex = current - 1;
			} else
			{
				leftIndex = current + 1;
			}
		}
	}
	SGateRoot	root;
	ReadGate(0, &root);
	uint8_t	newIndex = root.freeHead;
	if (newIndex)
	{
		SGateLink	freeGate;
		ReadGate(newIndex, &freeGate);
		root.freeHead = freeGate.next;
		WriteGate(0, &root);
		mCount++;
	} else
	{
		mGates->Seek(0, DataStream::eSeekEnd);
		uint8_t	maxGates = mGates->GetPos()/sizeof(SGateLink)-1;
		if (maxGates > mCount)
		{
			mCount++;
			newIndex = mCount;
		}
	}

	if (newIndex)
	{
		if (mCurrentIndex)
		{
			leftIndex--;
			/*
			*	If the new gate it to be inserted after an existing gate...
			*/
			if (leftIndex >= 0)
			{
				GoToRelativeGate(leftIndex - currLogIndex);
				inGate.prev = mCurrentIndex;
				inGate.next = mCurrent.next;
				mCurrent.next = newIndex;
				WriteGate(mCurrentIndex, &mCurrent);
				if (inGate.next != 0)
				{
					GoToGate(inGate.next);
					mCurrent.prev = newIndex;
					WriteGate(inGate.next, &mCurrent);
				} else
				{
					root.tail = newIndex;
					WriteGate(0, &root);
				}
			/*
			*	Else, this is the new head.
			*	Load and update the root and the previous head gate.
			*/
			} else
			{
				GoToGate(root.head);
				inGate.prev = 0;
				inGate.next = root.head;
				root.head = newIndex;
				mCurrent.prev = newIndex;
				WriteGate(mCurrentIndex, &mCurrent);
				WriteGate(0, &root);
			}
		/*
		*	Else the list is empty.
		*	Initialize a new list.
		*/
		} else
		{
			inGate.prev = 0;
			inGate.next = 0;
			root.head = newIndex;
			root.tail = newIndex;
			WriteGate(0, &root);
		}
		WriteGate(newIndex, &inGate);
		GoToGate(newIndex);
	}
	return(newIndex);
}

/******************************* RemoveCurrent ********************************/
/*
*	Removes the current gate by adding it to the free linked list.
*	Returns true if the current gate was removed.
*	If there is a next gate, the next gate becomes the current else
*	the previous becomes the current, if one exists.
*	If no current then the current index is 0 (root).
*/
bool Gates::RemoveCurrent(void)
{
	bool	success = mCurrentIndex != 0;
	if (success)
	{
		// Save the current prev and next indexes
		uint8_t	prev = mCurrent.prev;
		uint8_t	next = mCurrent.next;
		// Load the root
		SGateRoot	root;
		ReadGate(0, &root);
	
		// Add current to the free list as the new head.
		mCurrent.next = root.freeHead;
		mCurrent.prev = 0;	// The free list is one way.
		WriteGate(mCurrentIndex, &mCurrent);
		root.freeHead = mCurrentIndex;

		/*
		*	If the previous gate isn't the root THEN
		*	update the previous gate's next field.
		*/
		if (prev != 0)
		{
			ReadGate(prev, &mCurrent);
			mCurrent.next = next;
			WriteGate(prev, &mCurrent);
		/*
		*	Else, update the root's head.
		*/
		} else
		{
			root.head = next;
		}
		/*
		*	If there is a next gate THEN
		*	update the next gate's prev field.
		*	The next gate becomes the new current gate.
		*/
		if (next != 0)
		{
			ReadGate(next, &mCurrent);
			mCurrent.prev = prev;
			WriteGate(next, &mCurrent);
			mCurrentIndex = next;
		/*
		*	Else update the root tail to point to the prev gate.
		*	The prev gate becomes the new current gate.
		*/
		} else
		{
			root.tail = prev;
			mCurrentIndex = prev;
		}
		
		// Write the updated root.
		WriteGate(0, &root);
		mCount--;
	}
	return(success);
}

/******************************* RemoveAllGates *******************************/
void Gates::RemoveAllGates(void)
{
	SGateRoot	root = {0,0,0};
	mCurrentIndex = 0;
	mCount = 0;
	WriteGate(0, &root);
}

/******************************** IsValidIndex ********************************/
/*
*	Returns true if the passed physical record index is valid.  In order to
*	determine this the link list needs to be walked till the index is found.
*	You can't simply check to see if it's less than the logical count because
*	physical indexes don't move when a gate is removed, the index is only added
*	to the freeHead chain.
*/
bool Gates::IsValidIndex(
	uint8_t	inRecIndex)
{
	bool isValid = false;
	if (inRecIndex > 0)
	{
		SGateRoot	root;
		ReadGate(0, &root);
		SGateLink	thisGate;
		uint8_t	next = root.head;
		while (next)
		{
			if (next != inRecIndex)
			{
				ReadGate(next, &thisGate);
				next = thisGate.next;
				continue;
			}
			isValid = true;
			break;
		}
	}
	return(isValid);
}

/********************************** ReadGate **********************************/
void Gates::ReadGate(
	uint8_t		inIndex,
	void*		inGate) const
{
	mGates->Seek(inIndex*sizeof(SGateLink), DataStream::eSeekSet);
	mGates->Read(sizeof(SGateLink), inGate);
}

/********************************** WriteGate *********************************/
void Gates::WriteGate(
	uint8_t		inIndex,
	const void*	inGate) const
{
	mGates->Seek(inIndex*sizeof(SGateLink), DataStream::eSeekSet);
	mGates->Write(sizeof(SGateLink), inGate);
}

/********************************* GatesMask **********************************/
/*
*	Returns a mask representing all of the registered gate physical indexes.
*	Note: Bit 0 is physical index 1, Bit 1 is physical index 2, etc..
*
*	It is possible for a removed gate to leave a gap in the physical indexes.
*	if this gap wasn't possible then there would be no need for this routine.
*/
uint32_t Gates::GatesMask(void)
{
	uint32_t	gatesMask = 0;
	if (mCount)
	{
		uint16_t	savedCurrent = mCurrentIndex;
		GoToNthGate(0);
		do	
		{
			gatesMask |= ((uint32_t)1 << (mCurrentIndex -1));
		} while(Next(false));
		GoToGate(savedCurrent);
	}
	return(gatesMask);
}

#ifndef __MACH__

const char kGatesFilename[] = "Gates.csv";

/********************************* LoadFromSD *********************************/
/*
*	This routine attemps to open the CSV file named Gates.csv.  The original
*	file should be created using SaveToSD().  The csv contains two fields, ID
*	and Name.  Only the Name field should be edited.  The ID field should not be
*	modified.  The ID field is used to associate the edited name with the gate. 
*	Only the character set defined for the font used should be used in the name
*	field.
*/
bool Gates::LoadFromSD(void)
{
	SdFat sd;
	bool	success = sd.begin(DCConfig::kSDSelectPin);
	if (success)
	{
		SdFile file;
		success = file.open(kGatesFilename, O_RDONLY);
		if (success)
		{
			SGateLink	link;
			CSVUtils	csv(&file);
			uint8_t		id;
			
			char thisChar = csv.SkipLine();	// Skip the csv header line.
			
			while (thisChar != 0)
			{
				if ((thisChar = csv.ReadUint8(&id)) == ',' &&
					IsValidIndex(id) &&
					((thisChar = csv.ReadStr(sizeof(link.name), link.name)) == '\n' || thisChar == 0))
				{
					/*
					*	Remove and add this gate to update the sort by name.
					*	The physical index is the gate ID.
					*	Load the gate with this id
					*/
					GoToGate(id);
					/*
					*	RemoveCurrent places the gate link as the freeHead.
					*/
					RemoveCurrent();
					/*
					*	Add will reuse the freeHead.  This means that the
					*	physical index (i.e. id) will remain the same.
					*/
					Add(link);
				}
			}
			file.close();
		}
	} else
	{
		sd.initErrorHalt();
	}
	return(success);
}

/********************************** SaveToSD **********************************/
/*
	This routine overwrites or creates the CSV file named Gates.csv.  The csv
	contains two fields, ID and Name.  Only the Name field should be edited by
	the user.  Only the character set defined for the font used should be used
	in the name field.
*/
bool Gates::SaveToSD(void)
{
	bool	success = mCount > 0;
	if (success)
	{
		SdFat sd;
		bool	success = sd.begin(DCConfig::kSDSelectPin);
		if (success)
		{
			SdFile::dateTimeCallback(UnixTime::SDFatDateTimeCB);
			SdFile file;
			success = file.open(kGatesFilename, O_WRONLY | O_TRUNC | O_CREAT);
			if (success)
			{
				CSVUtils	csv(&file);
				file.println(F("ID,Name"));
				uint16_t	savedCurrent = mCurrentIndex;
				GoToNthGate(0);
				char	quotedStr[50];
				do	
				{
					file.print(mCurrentIndex);
					file.write(',');
					file.println(csv.QuoteForCSV(mCurrent.name, quotedStr));
				} while(Next(false));
				GoToGate(savedCurrent);
				file.close();
			}
		} else
		{
			sd.initErrorHalt();
		}
	}
	return(success);
}

#if 0
SDFatDateTimeCB is now supplied by UnixTime due to the addition of an RTC
to the project.
#include "CompileTime.h"
/****************************** SDFatDateTimeCB *******************************/
/*
*	SDFat date time callback.
*/
void Gates::SDFatDateTimeCB(
	uint16_t*	outDate,
	uint16_t*	outTime)
{
	// The time macros come from CompileTime.h
	*outDate = ((uint16_t)(__TIME_YEARS__ - 1980) << 9 | (uint16_t)__TIME_MONTH__ << 5 | __TIME_DAYS__);
	*outTime = ((uint16_t)__TIME_HOURS__ << 11 | (uint16_t)__TIME_MINUTES__ << 5 | __TIME_SECONDS__ >> 1);
}
#endif
#endif