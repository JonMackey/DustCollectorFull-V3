/*
*	GateSets.cpp, Copyright Jonathan Mackey 2020
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
#include "GateSets.h"
#include "DataStream.h"
#include <string.h>

#ifndef __MACH__
#include <Arduino.h>
#include "SdFat.h"
#include "sdios.h"
#include <EEPROM.h>
#else
#include <stdio.h>
#endif
#include "BMP280Utils.h"
#include "UnixTime.h"
#include "Gates.h"
#include "DCConfig.h"

// There are up to 32 gates.  The stream contains space for 32 gate sets + the root.
// Each SGateSetLink is 14 bytes.  33 * 14 = 462
DataStream_E	gateSetsDataStream((void *)((sizeof(SGateLink) * (DCConfig::kMaxGates +1)) +
							DCConfig::kGatesDataAddr), sizeof(SGateSetLink) * (DCConfig::kMaxGateSets +1));

/******************************** GateSets ********************************/
GateSets::GateSets(void)
	: mGateSets(0), mCurrentIndex(0), mCount(0)
{
}

#if 0
char* UInt16ToBinaryStr(
	uint32_t	inValue,
	char*		inBuffer);
	
/***************************** UInt32ToBinaryStr ******************************/
char* UInt32ToBinaryStr(
	uint32_t	inValue,
	char*		inBuffer)
{
	uint32_t	nMask = 0x80000000;
	char*	buffPtr = inBuffer;
	for (; nMask != 0; nMask >>= 1)
	{
		*(buffPtr++) = (inValue & nMask) ? '1':'0';
		if (!(nMask & 0x1110))
		{
			continue;
		}
		*(buffPtr++) = ' ';
	}
	*buffPtr = 0;
	return(buffPtr);
}

/************************************ Dump ************************************/
void GateSets::Dump(void)
{
	SGateSetRoot	root;
	ReadGateSet(0, &root);
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

	SGateSetLink	link;
	char			maskStr[20];
	char	cleanStr[15];
	char	dirtyStr[15];
	for (uint8_t i = 1; i < 4; i++)
	{
		ReadGateSet(i, &link);
		UInt32ToBinaryStr(link.gatesMask, maskStr);
		BMP280Utils::Int32ToDec22Str(link.clean, cleanStr);
		BMP280Utils::Int32ToDec22Str(link.dirty, dirtyStr);
#ifdef __MACH__
		fprintf(stderr, "[%hd] prev = %hd, next = %hd, gatesMask = %sb, clean = %s, dirty = %s\n",
							i, link.prev, link.next, maskStr, cleanStr, dirtyStr);
#else
		Serial.print('[');
		Serial.print(i);
		Serial.print(F("] prev = "));
		Serial.print(link.prev);
		Serial.print(F(", next = "));
		Serial.print(link.next);
		Serial.print(F(", gatesMask = "));
		Serial.print(maskStr);
		Serial.print(F("b, clean = "));
		Serial.print(cleanStr);
		Serial.print(F(", dirty = "));
		Serial.println(dirtyStr);
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
void GateSets::begin(void)
{
	mGateSets = &gateSetsDataStream;
	EEPROM.get(DCConfig::kDefaultCleanDeltaAddr, mDefaultCleanDelta);
	EEPROM.get(DCConfig::kkDefaultDirtyDeltaAddr, mDefaultDirtyDelta);
	// If the EEPROM is unitialized THEN use the hard-coded defaults.
	if (mDefaultCleanDelta > 0xFFFF)
	{
		mDefaultCleanDelta = DCConfig::kDefaultCleanDelta;
	}
	if (mDefaultDirtyDelta > 0xFFFF)
	{
		mDefaultDirtyDelta = DCConfig::kDefaultDirtyDelta;
	}
	if (mGateSets)
	{
		SGateSetRoot	root;
		ReadGateSet(0, &root);

		if (root.tail)
		{
			if (root.tail != 0xFF)
			{
				uint8_t	count = 0;
				uint8_t	prev = root.tail;
				while (prev)
				{
					count++;
					ReadGateSet(prev, &mCurrent);
					if (mCurrent.clean < mDefaultCleanDelta)
					{
						mDefaultCleanDelta = mCurrent.clean;
					}
					if (mCurrent.dirty < mDefaultDirtyDelta)
					{
						mDefaultDirtyDelta = mCurrent.dirty;
					}
					prev = mCurrent.prev;
				}
				mCurrentIndex = root.head;
				mCount = count;
			} else
			{
				RemoveAllGateSets();
			}
		} // Else there are no gates
	}
}

/******************************** GetNextIndex ********************************/
/*
*	Gets the next sorted physical index without loading
*/
uint8_t GateSets::GetNextIndex(
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
			SGateSetRoot	root;
			ReadGateSet(0, &root);
			index = root.head;
		}
	}
	return(index);
}

/************************************ Next ************************************/
bool GateSets::Next(
	bool	inWrap)
{
	uint8_t	nextindex = GetNextIndex(inWrap);
	bool	success = nextindex != 0;
	if (success)
	{
		GoToGateSet(nextindex);
	}
	return(success);
}

/****************************** GetPreviousIndex ******************************/
uint8_t GateSets::GetPreviousIndex(
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
			SGateSetRoot	root;
			ReadGateSet(0, &root);
			index = root.tail;
		}
	}
	return(index);
}

/********************************** Previous **********************************/
bool GateSets::Previous(
	bool	inWrap)
{
	uint8_t	prevIndex = GetPreviousIndex(inWrap);
	bool	success = prevIndex != 0;
	if (success)
	{
		GoToGateSet(prevIndex);
	}
	return(success);
}

/******************************** GoToGateSet *********************************/
/*
*	Does no bounds checking.
*/
void GateSets::GoToGateSet(
	uint8_t	inRecIndex)
{
	if (mCurrentIndex != inRecIndex)
	{
		mCurrentIndex = inRecIndex;
		ReadGateSet(inRecIndex, &mCurrent);
	}
}

/********************************* CountBits **********************************/
uint8_t GateSets::CountBits(
	uint32_t	inValue)
{
	uint8_t	bitCount = 0;
	for (uint32_t value = inValue; value; value >>= 1)
	{
		if ((value & 1) == 0)
		{
			continue;
		}
		bitCount++;
	}
	return(bitCount);
}

/****************************** GateStateChanged ******************************/
/*
*	Called by Gates::SetGateState() whenever a gate is open or closed.
*/
void GateSets::GateStateChanged(
	uint32_t	inOpenGates)
{
	GoToNearestGateSet(inOpenGates);
}

/**************************** GoToGateSetWithMask *****************************/
/*
*	Returns true if there is a set with this mask.
*/
bool GateSets::GoToGateSetWithMask(
	uint32_t	inGateMask)
{
	SGateSetRoot	root;
	ReadGateSet(0, &root);
	bool	success = false;

	if (root.head != 0)
	{
		GoToGateSet(root.head);
		while (mCurrent.gatesMask != inGateMask && mCurrent.next)
		{
			GoToGateSet(mCurrent.next);
		}
		success = mCurrent.gatesMask == inGateMask;
	}
	return(success);
}

/******************************** SaveCleanSet ********************************/
/*
*	Updates the existing set with inGateMask or adds a new set if no set with
*	inGateMask exists.
*/
bool GateSets::SaveCleanSet(
	uint32_t	inGateMask,
	uint32_t	inCleanDelta)
{
	bool success = true;
	if (inGateMask)
	{
		success = GoToGateSetWithMask(inGateMask);
		if (inCleanDelta < mDefaultCleanDelta)
		{
			mDefaultCleanDelta = inCleanDelta;
		}
		if (success)
		{
			mCurrent.clean = inCleanDelta;
		} else
		{
			SGateSetLink	gateSetLink = {0,0, inGateMask, inCleanDelta, mDefaultDirtyDelta};
			success = Add(gateSetLink) != 0;
		}
	/*
	*	Else, if there are no gates open, the assumption is that there are no
	*	sensors.  In this case the default is updated.
	*/
	} else
	{
		mDefaultCleanDelta = inCleanDelta;
		EEPROM.put(DCConfig::kDefaultCleanDeltaAddr, inCleanDelta);
	}
	return(success);
}

/******************************** SaveDirtySet ********************************/
/*
*	Updates the existing set with inGateMask or adds a new set if no set with
*	inGateMask exists.
*/
bool GateSets::SaveDirtySet(
	uint32_t	inGateMask,
	uint32_t	inDirtyDelta)
{
	bool success = true;
	if (inGateMask)
	{
		success = GoToGateSetWithMask(inGateMask);
		if (inDirtyDelta < mDefaultDirtyDelta)
		{
			mDefaultDirtyDelta = inDirtyDelta;
		}
		if (success)
		{
			mCurrent.dirty = inDirtyDelta;
		} else
		{
			SGateSetLink	gateSetLink = {0,0, inGateMask, mDefaultCleanDelta, inDirtyDelta};
			success = Add(gateSetLink) != 0;
		}
	/*
	*	Else, if there are no gates open, the assumption is that there are no
	*	sensors.  In this case the default is updated.
	*/
	} else
	{
		mDefaultDirtyDelta = inDirtyDelta;
		EEPROM.get(DCConfig::kkDefaultDirtyDeltaAddr, inDirtyDelta);
	}
	return(success);
}

/**************************** CurrentDirtyPressure ****************************/
uint32_t GateSets::CurrentDirtyPressure(void) const
{
	return(mCount ? mCurrent.dirty : mDefaultDirtyDelta);
}

/**************************** CurrentCleanPressure ****************************/
uint32_t GateSets::CurrentCleanPressure(void) const
{
	return(mCount ? mCurrent.clean : mDefaultCleanDelta);
}

/***************************** GoToNearestGateSet *****************************/
/*
*	Ideally there should be a set for every gate combination commonly used. This
*	routine is called when a gate is opened or closed.  In general you close and
*	open gates to setup a particular use case.  The intermediate gate
*	combinations may never actually be used but the controller still needs some
*	sane limits, even if the combination is only brief.
*
*	When the desired gate set doesn't exist, look for the set that has the
*	closest number of gates.  Of these "closest number" sets, favor the set that
*	has the most gates in common.
*/
bool GateSets::GoToNearestGateSet(
	uint32_t	inGateMask)
{
	bool	success = GoToGateSetWithMask(inGateMask);
	if (!success)
	{
		uint8_t	gatesInSet = CountBits(inGateMask);
		uint8_t	deltaIndex = 0;
		uint8_t	delta = 0xFF;
		uint8_t	gatesInCommon = 0;
		
		SGateSetRoot	root;
		ReadGateSet(0, &root);

		if (root.head != 0)
		{
			GoToGateSet(root.head);
			while (true)
			{
				uint8_t	gatesInCurrentSet = CountBits(mCurrent.gatesMask);
				uint8_t	currentGatesInCommon = CountBits(mCurrent.gatesMask & inGateMask);
				uint8_t	currentDelta = abs(gatesInCurrentSet - gatesInSet);
				if (currentDelta < delta)
				{
					delta = currentDelta;
					deltaIndex = mCurrentIndex;
					gatesInCommon = currentGatesInCommon;
				} else if (currentDelta == delta &&
					currentGatesInCommon > gatesInCommon)
				{
					gatesInCommon = currentGatesInCommon;
					deltaIndex = mCurrentIndex;
				}
				if (mCurrent.next)
				{
					GoToGateSet(mCurrent.next);
				} else
				{
					break;
				}
			}
		}
		if (deltaIndex != 0)
		{
			success = true;
			GoToGateSet(deltaIndex);
		}
	}
	return(success);
}

/******************************* GoToNthGateSet *******************************/
/*
*	Returns true if there is an Nth record.
*/
bool GateSets::GoToNthGateSet(
	uint8_t	inLogIndex)
{
	SGateSetRoot	root;
	ReadGateSet(0, &root);
	bool	success = root.head != 0;
	if (success)
	{
		GoToGateSet(root.head);
		
		for (uint8_t i = 0; i < inLogIndex; i++)
		{
			if (mCurrent.next)
			{
				GoToGateSet(mCurrent.next);
			} else
			{
				success = false;
				break;
			}
		}
	}
	return(success);
}

/**************************** GoToRelativeGateSet ****************************/
/*
*	Starting from the current gate set, go to the logical position relative to
*	the current gate set by inRelLogIndex. 
*/
bool GateSets::GoToRelativeGateSet(
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
*	Starting from the root head, count indexes till the current gate set is hit.
*	-1 is returned if there is no current.  This should only happen when there
*	are no gate sets.
*/
int8_t GateSets::GetLogicalIndex(void) const
{
	int8_t	logIndex = 0;
	if (mCurrentIndex)
	{
		SGateSetLink	thisGateSet;
		uint8_t	next = 0;
		uint8_t	currentPrev = mCurrent.prev;
		while (next != currentPrev)
		{
			logIndex++;
			ReadGateSet(next, &thisGateSet);
			next = thisGateSet.next;
		}
	} else
	{
		logIndex = -1;
	}
	return(logIndex);
}

/************************************ Add *************************************/
/*
*	The added gate set becomes the current gate set.
*/
uint8_t GateSets::Add(
	SGateSetLink&	inGateSet)
{
	int16_t leftIndex = 0;
	int16_t	currLogIndex = GetLogicalIndex();
	if (currLogIndex >= 0)
	{
		int16_t current = 0;
		int16_t rightIndex = mCount -1;
		uint32_t	gateMask = inGateSet.gatesMask;
		while (leftIndex <= rightIndex)
		{
			current = (leftIndex + rightIndex) / 2;
			GoToRelativeGateSet(current - currLogIndex);
			currLogIndex = current;
			
			int	cmpResult = mCurrent.gatesMask - gateMask;
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
	SGateSetRoot	root;
	ReadGateSet(0, &root);
	uint8_t	newIndex = root.freeHead;
	if (newIndex)
	{
		SGateSetLink	freeGate;
		ReadGateSet(newIndex, &freeGate);
		root.freeHead = freeGate.next;
		WriteGateSet(0, &root);
		mCount++;
	} else
	{
		mGateSets->Seek(0, DataStream::eSeekEnd);
		uint8_t	maxGateSets = mGateSets->GetPos()/sizeof(SGateSetLink)-1;
		if (maxGateSets > mCount)
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
			*	If the new gate it to be inserted after an existing gate set...
			*/
			if (leftIndex >= 0)
			{
				GoToRelativeGateSet(leftIndex - currLogIndex);
				inGateSet.prev = mCurrentIndex;
				inGateSet.next = mCurrent.next;
				mCurrent.next = newIndex;
				WriteGateSet(mCurrentIndex, &mCurrent);
				if (inGateSet.next != 0)
				{
					GoToGateSet(inGateSet.next);
					mCurrent.prev = newIndex;
					WriteGateSet(inGateSet.next, &mCurrent);
				} else
				{
					root.tail = newIndex;
					WriteGateSet(0, &root);
				}
			/*
			*	Else, this is the new head.
			*	Load and update the root and the previous head gate set.
			*/
			} else
			{
				GoToGateSet(root.head);
				inGateSet.prev = 0;
				inGateSet.next = root.head;
				root.head = newIndex;
				mCurrent.prev = newIndex;
				WriteGateSet(mCurrentIndex, &mCurrent);
				WriteGateSet(0, &root);
			}
		/*
		*	Else the list is empty.
		*	Initialize a new list.
		*/
		} else
		{
			inGateSet.prev = 0;
			inGateSet.next = 0;
			root.head = newIndex;
			root.tail = newIndex;
			WriteGateSet(0, &root);
		}
		WriteGateSet(newIndex, &inGateSet);
		GoToGateSet(newIndex);
	}
	return(newIndex);
}

/******************************* RemoveCurrent ********************************/
/*
*	Removes the current gate set by adding it to the free linked list.
*	Returns true if the current gate set was removed.
*	If there is a next gate set, the next gate set becomes the current else
*	the previous becomes the current, if one exists.
*	If no current then the current index is 0 (root).
*/
bool GateSets::RemoveCurrent(void)
{
	bool	success = mCurrentIndex != 0;
	if (success)
	{
		// Save the current prev and next indexes
		uint8_t	prev = mCurrent.prev;
		uint8_t	next = mCurrent.next;
		// Load the root
		SGateSetRoot	root;
		ReadGateSet(0, &root);
	
		// Add current to the free list as the new head.
		mCurrent.next = root.freeHead;
		mCurrent.prev = 0;	// The free list is one way.
		WriteGateSet(mCurrentIndex, &mCurrent);
		root.freeHead = mCurrentIndex;

		/*
		*	If the previous gate isn't the root THEN
		*	update the previous gate's next field.
		*/
		if (prev != 0)
		{
			ReadGateSet(prev, &mCurrent);
			mCurrent.next = next;
			WriteGateSet(prev, &mCurrent);
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
			ReadGateSet(next, &mCurrent);
			mCurrent.prev = prev;
			WriteGateSet(next, &mCurrent);
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
		WriteGateSet(0, &root);
		mCount--;
	}
	return(success);
}

/******************************* RemoveAllGateSets *******************************/
void GateSets::RemoveAllGateSets(void)
{
	SGateSetRoot	root = {0,0,0};
	mCurrentIndex = 0;
	mCount = 0;
	WriteGateSet(0, &root);
}

/************************ RemoveGateSetsContainingGate ************************/
void GateSets::RemoveGateSetsContainingGate(
	uint32_t	inGateMask)
{
	if (inGateMask > 0 &&
		mCount > 0)
	{
		SGateSetRoot	root;
		ReadGateSet(0, &root);
		uint8_t		next = root.head;
		
		while (next)
		{
			GoToGateSet(next);
			next = mCurrent.next;
			if (mCurrent.gatesMask & inGateMask)
			{
				RemoveCurrent();
			}
		}
	}
}

/******************************** IsValidIndex ********************************/
/*
*	Returns true if the passed physical record index is valid.  In order to
*	determine this the link list needs to be walked till the index is found.
*	You can't simply check to see if it's less than the logical count because
*	physical indexes don't move when a gate is removed, the index is only added
*	to the freeHead chain.
*/
bool GateSets::IsValidIndex(
	uint8_t	inRecIndex)
{
	bool isValid = false;
	if (inRecIndex > 0)
	{
		SGateSetRoot	root;
		ReadGateSet(0, &root);
		SGateSetLink	thisGateSet;
		uint8_t	next = root.head;
		while (next)
		{
			if (next != inRecIndex)
			{
				ReadGateSet(next, &thisGateSet);
				next = thisGateSet.next;
				continue;
			}
			isValid = true;
			break;
		}
	}
	return(isValid);
}

/********************************** ReadGateSet **********************************/
void GateSets::ReadGateSet(
	uint8_t		inIndex,
	void*		inGateSet) const
{
	mGateSets->Seek(inIndex*sizeof(SGateSetLink), DataStream::eSeekSet);
	mGateSets->Read(sizeof(SGateSetLink), inGateSet);
}

/********************************** WriteGateSet *********************************/
void GateSets::WriteGateSet(
	uint8_t		inIndex,
	const void*	inGateSet) const
{
	mGateSets->Seek(inIndex*sizeof(SGateSetLink), DataStream::eSeekSet);
	mGateSets->Write(sizeof(SGateSetLink), inGateSet);
}

const char kGateSetsFilename[] = "GateSets.txt";
/********************************** SaveToSD **********************************/
/*
*	This routine overwrites or creates the text file named GateSets.txt.
*	Each set contains an ID followed by the names of the gates in the set.
*	
*	Example:
*	[0] PLANER, SAW, JOINTER
*	[1] PLANER, BELT SANDER, JOINTER
*	[2] PLANER, SAW, BAND SAW
*	
*/
bool GateSets::SaveToSD(
	Gates&	inGates)
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
			success = file.open(kGateSetsFilename, O_WRONLY | O_TRUNC | O_CREAT);
			if (success)
			{
				if (GetCount())
				{
					uint8_t	savedGateIndex = inGates.GetCurrentIndex();
					GoToGateSet(0);
					do	
					{
						file.print('[');
						file.print(mCurrentIndex);
						file.print(']');
						uint8_t	gateIndex = 1;
						for (uint32_t gatesMask = mCurrent.gatesMask; gatesMask != 0; gatesMask >>= 1)
						{
							if (gatesMask & 1)
							{
								inGates.GoToGate(gateIndex);
								file.print(F("\n\t"));
								file.print(inGates.GetCurrentIndex());
								file.print('-');
								file.print(inGates.GetCurrent().name);
							}
							gateIndex++;
						}
						file.print('\n');
					} while(Next(false));
					inGates.GoToGate(savedGateIndex);
				} else
				{
					file.print(F("No gate sets defined.\n"));
				}
				file.close();
			}
		} else
		{
			sd.initErrorHalt();
		}
	}
	return(success);
}
