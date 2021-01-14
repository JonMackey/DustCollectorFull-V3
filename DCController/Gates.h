/*
*	Gates.h, Copyright Jonathan Mackey 2020
*	Class to manage blast gate sensors.
*
*	The requirement I have is that it not be tied to any specific storage AND
*	when adding/removing sensors, the existing sensors will not move.  This
*	is implemented using a generic stream, which means it can be stored anywhere.
*	It's also a simple linked list, so minimal changes are needed to add and
*	remove sensors.
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
#ifndef Gates_h
#define Gates_h

#include <inttypes.h>

class DataStream;

/*
*	next and prev below, when multiplied by the size of SGateLink, is a
*	physical offset from the start of the data stream.
*	The first location (index 0) is the root.
*
*	The gate names are all uppercase.  strcmp is used for making comparisons.
*
*	Any change in the size of SGateLink is reflected in SGateRoot by the size
*	of SGateRoot.unused[].
*	The sizeof(SGateRoot) must equal the sizeof(SGateLink)
*/
typedef struct
{
	uint8_t		prev;		// Index of the previous gate.  0 if head.
	uint8_t		next;		// Index of the next gate.  0 if tail.
	char		name[20];	// Gate name
} SGateLink;

typedef struct
{
	uint8_t	tail;		// Index of the last gate.
	uint8_t	head;		// Index of the first gate.  0xFFFF if none.
	/*
	*	freeHead is the gate of the first free gate.  This will only be
	*	non-zero when a gate is removed within the set of defined gates.
	*	The length of the data stream determines the capacity.  freeHead only
	*	indicates that there is fragmentation within the set of gates.
	*/
	uint8_t	freeHead;
	// The root must be the same size as SGateLink
	uint8_t		unused[sizeof(SGateLink) -3];
} SGateRoot;

class Gates
{
public:
							Gates(void);

	void					begin(void);
	uint8_t					GetCount(void) const
								{return(mCount);}
	const SGateLink&		GetCurrent(void) const			// Loaded Gate
								{return(mCurrent);}
	uint8_t					GetCurrentIndex(void) const		// Unsorted physical record index
								{return(mCurrentIndex);}
	int8_t					GetLogicalIndex(void) const;	// Sorted logical index
	bool					IsValidIndex(
								uint8_t					inRecIndex);	// Unsorted physical record index
							/*
							*	Load the next sorted location
							*/
	bool					Next(
								bool					inWrap = true);
							/*
							*	Gets the next sorted physical index without loading
							*/
	uint8_t					GetNextIndex(
								bool					inWrap = true) const;
							/*
							*	Load the previous sorted gate
							*/
	bool					Previous(
								bool					inWrap = true);
							/*
							*	Gets the previous sorted physical index without loading
							*/
	uint8_t					GetPreviousIndex(
								bool					inWrap = true) const;
	void					GoToGate(
								uint8_t					inRecIndex);	// Unsorted physical record index
	bool					GoToNthGate(
								uint8_t					inLogIndex);	// Sorted logical index
	uint8_t					Add(
								SGateLink&				inGate);
	uint8_t					AddNew(void);
	bool					RemoveCurrent(void);
							/*
							*	Called when the stream is uninitialized and when
							*	resetting/clearing all gates for debugging.
							*/
	void					RemoveAllGates(void);
	
#ifndef __MACH__
	bool					LoadFromSD(void);
	bool					SaveToSD(void);
#endif
//	void					Dump(void);

							// Returns a mask where each bit represents a
							// registered gate index (bit 0 = physical index 1)
	uint32_t				GatesMask(void);
protected:
	DataStream*		mGates;
	uint32_t		mOpenGates;
	SGateLink		mCurrent;
	uint8_t			mCurrentIndex;
	uint8_t			mCount;
	
	void					ReadGate(
								uint8_t					inIndex,	// Physical record index
								void*					inGate) const;
	void					WriteGate(
								uint8_t					inIndex,	// Physical record index
								const void*				inGate) const;
	bool					GoToRelativeGate(
								int16_t					inRelLogIndex);	// Relative sorted logical index
};

#endif // Gates_h
