/*
*	DCSConfig.h, Copyright Jonathan Mackey 2020
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
#ifndef DCSConfig_h
#define DCSConfig_h

#include <inttypes.h>

//#define HAS_SERIAL
#ifdef HAS_SERIAL
#include "SendOnlySoftwareSerial.h"
extern SendOnlySoftwareSerial swSerial; // Tx
#endif

namespace DCSConfig
{
	// Pins ATtiny84A
	const uint8_t kHallPin			= 0;	// PA0
	const uint8_t kRedRGBTxPin		= 1;	// PA1
	const uint8_t kGreenRGBRxPin	= 2;	// PA2
	const uint8_t kBlueRGBPin		= 3;	// PA3
	const uint8_t kSCKPin			= 4;	// PA4
	const uint8_t kMOSIPin			= 5;	// PA5
	const uint8_t kMISOPin			= 6;	// PA6
	const uint8_t kCSPin			= 7;	// PA7
	const uint8_t kIntPin			= 8;	// PB2
	const int8_t kResetPin			= 9;	// PB1
	const int8_t kGateOpenLEDPin	= 10;	// PB0
	const uint8_t	kPINBMask = _BV(PINB2);	// PB2 CAN_INT
	
	/*
	*	EEPROM usage, 512 bytes
	*
	*	[0]		uint32_t	CAN ID.  Initially this is set to the compile time
	*/
	const uint16_t	kCAN_ID_Addr	= 0;
}

#endif // DCSConfig_h

