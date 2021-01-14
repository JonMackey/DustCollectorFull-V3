/*
*	DCMessages.h, Copyright Jonathan Mackey 2020
*	Commands used on the Dust Collector CAN
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
#ifndef DCMessages_h
#define DCMessages_h

#include <inttypes.h>

// The 11 standard identifier bits are used as command IDs.  All command enum
// values must be kept below 0x7FF or 2047.

namespace DCSensor
{
	enum ECommands
	{
		// The eGate commands also serve as a login when sent to the controller.
		eGateIsOpen			= 1,	// Extended frame, data is the sensor ID
		eGateIsClosed,				// Extended frame, data is the sensor ID
		eTimestamp					// Extended frame, data is the sensor ID + unix timestamp
	};
}

namespace DCController
{
	enum ECommands
	{
		eRequestGateState	= 0x100,// Extended frame (only sent to registered gates)
		eCheckGateStateResponses,	// Internal command, see below.
		eFlash,						// Extended frame, no data
		eStopFlash,					// Extended frame, no data
		eSetID,						// Extended frame, data is the new ID
		eSetFactoryID,				// Extended frame, no data
		// The timestamp is the date and time of when the software was compiled.
		eRequestTimestamp,			// Extended frame, no data
		eReplaceID					// Internal command, see below.
	};
	
	/*
	*	eCheckGateStateResponses is used internally to know when all of the
	*	gate sensor responses from the eRequestGateState requests sent to all
	*	gates sensors,  has completed.  At this point it should be possible to
	*	determine if there are any sensors that aren't responding.
	*
	*	eReplaceID is used internally to set an unregistered gate sensor to the
	*	ID of an existing gate that isn't responding.  This assumes the
	*	unregistered gate sensor is replacing the existing gate sensor.
	*/
}
#endif // DCMessages_h
