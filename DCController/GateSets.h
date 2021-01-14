/*
*	GateSets.h, Copyright Jonathan Mackey 2020
*	Class to manage sets of blast gate sensors and the corresponding pressure
*	readings of when the dust collector filter is clean and dirty.
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
#ifndef GateSets_h
#define GateSets_h

#include <inttypes.h>
class Gates;

class DataStream;

/*
*	next and prev below, when multiplied by the size of SGateSetLink, is a
*	physical offset from the start of the data stream.
*	The first location (index 0) is the root.
*
*	Any change in the size of SGateSetLink must be reflected in SGateSetRoot by
*	adjusting the size of SGateSetRoot.unused[].
*	The sizeof(SGateSetRoot) must equal the sizeof(SGateSetLink)
*/
typedef struct
{
	uint8_t		prev;		// Index of the previous set.  0 if head.
	uint8_t		next;		// Index of the next set.  0 if tail.
	uint32_t	gatesMask;	// The set of gates, 1 bit per gate.
	uint32_t	clean;		// Pressure delta for this set of gates when clean
	uint32_t	dirty;		// Pressure delta for this set of gates when dirty
} SGateSetLink;

typedef struct
{
	uint8_t		tail;		// Index of the last set.
	uint8_t		head;		// Index of the first set.  0xFFFF if none.
	/*
	*	freeHead is the set of the first free set.  This will only be
	*	non-zero when a set is removed.
	*	The length of the data stream determines the capacity.  freeHead only
	*	indicates that there is fragmentation within the gate sets.
	*/
	uint8_t		freeHead;
	// The root must be the same size as SGateSetLink
	uint8_t		unused[sizeof(SGateSetLink) - 3];
} SGateSetRoot;

class GateSets
{
public:
							GateSets(void);

	void					begin(void);
	uint8_t					GetCount(void) const
								{return(mCount);}
	const SGateSetLink&		GetCurrent(void) const			// Loaded GateSet
								{return(mCurrent);}
	uint32_t				CurrentDirtyPressure(void) const;
	uint32_t				CurrentCleanPressure(void) const;
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
	void					GoToGateSet(
								uint8_t					inRecIndex);	// Unsorted physical record index
	bool					GoToNthGateSet(
								uint8_t					inLogIndex);	// Sorted logical index
	bool					GoToGateSetWithMask(
								uint32_t				inGateMask);
	bool					GoToNearestGateSet(
								uint32_t				inGateMask);
	uint8_t					Add(
								SGateSetLink&			inGateSet);
	bool					SaveCleanSet(
								uint32_t				inGateMask,
								uint32_t				inCleanDelta);
	bool					SaveDirtySet(
								uint32_t				inGateMask,
								uint32_t				inDirtyDelta);
	bool					RemoveCurrent(void);
							/*
							*	Called when the stream is uninitialized and when
							*	resetting/clearing all gate sets for debugging.
							*/
	void					RemoveAllGateSets(void);
	void					RemoveGateSetsContainingGate(
								uint32_t				inGateMask);
	void					GateStateChanged(
								uint32_t				inOpenGates);
//	void					Dump(void);
	bool					SaveToSD(
								Gates&					inGates);
protected:
	DataStream*		mGateSets;
	SGateSetLink	mCurrent;
	uint32_t		mDefaultCleanDelta;	// Lowest clean pressure of all sets.
	uint32_t		mDefaultDirtyDelta;	// Lowest dirty pressure of all sets.
	uint8_t			mCurrentIndex;
	uint8_t			mCount;
	
	void					ReadGateSet(
								uint8_t					inIndex,	// Physical record index
								void*					inGateSet) const;
	void					WriteGateSet(
								uint8_t					inIndex,	// Physical record index
								const void*				inGateSet) const;
	bool					GoToRelativeGateSet(
								int16_t					inRelLogIndex);	// Relative sorted logical index
	static uint8_t			CountBits(
								uint32_t				inValue);

};

#endif // GateSets_h
