/*
*	DCConfig.h, Copyright Jonathan Mackey 2020
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
#ifndef DCConfig_h
#define DCConfig_h

#include <inttypes.h>

namespace DCConfig
{
	// Pins ATmega644PA
	const uint8_t	kUnusedPinB0		= 0;	// PB0
	const uint8_t	kUnusedPinB1		= 1;	// PB1
	const uint8_t	kCANIntPin			= 2;	// PB2	CAN_INT INT2
	const uint8_t	kFlasherControlPin	= 3;	// PB3
	const uint8_t	kUnusedPinB4		= 4;	// PB4
	const uint8_t	kMOSI				= 5;	// PB5
	const uint8_t	kMISO				= 6;	// PB6
	const uint8_t	kSCK				= 7;	// PB7

	const uint8_t	kRxPin				= 8;	// PD0
	const uint8_t	kTxPin				= 9;	// PD1
	const uint8_t	kRadioIRQPin		= 10;	// PD2	INT0
	const uint8_t	kSDSelectPin		= 11;	// PD3
	const uint8_t	kCANCSPin			= 12;	// PD4	CAN_CS
	const uint8_t	kSDDetectPin		= 13;	// PD5	PCINT29
	const int8_t	kCANResetPin		= 14;	// PD6	CAN_RESET
	const uint8_t	kMotorControlPin	= 15;	// PD7

	const uint8_t	kSCL				= 16;	// PC0
	const uint8_t	kSDA				= 17;	// PC1
	const uint8_t	kRadioNSSPin		= 18;	// PC2
	const uint8_t	kBMP1CSPin			= 19;	// PC3
	const uint8_t	kBMP0CSPin			= 20;	// PC4
	const uint8_t	kDispDCPin			= 21;	// PC5
	const uint8_t	k32KHzPin			= 22;	// PC6
	const uint8_t	kDispCSPin			= 23;	// PC7

	const uint8_t	kMotorSensePin		= 24;	// PA0
	const uint8_t	kDispResetPin		= 25;	// PA1
	const uint8_t	kBacklightPin		= 26;	// PA2
	const uint8_t	kUpBtnPin			= 27;	// PA3	PCINT3
	const uint8_t	kLeftBtnPin 		= 28;	// PA4	PCINT4
	const uint8_t	kEnterBtnPin		= 29;	// PA5	PCINT5
	const uint8_t	kRightBtnPin		= 30;	// PA6	PCINT6
	const uint8_t	kDownBtnPin			= 31;	// PA7	PCINT7
	const uint8_t	kPINABtnMask = (_BV(PINA3) | _BV(PINA4) | _BV(PINA5) | _BV(PINA6) | _BV(PINA7));
	
	/*
	*	EEPROM usage, 2K bytes
	*
	*	[0]		uint8_t		unused (networkID on other boards)
	*	[1]		uint8_t		unused (nodeID on other boards)
	*	[2]		uint8_t		flags, bit 0 is for 24 hour clock format on all boards.
	*						0 = 24 hour
	*	[3]		uint8_t		Dust bin motor trigger threshold.  The value at which
	*						the motor will be stopped and the bin considered full.
	*	[4]		uint32_t	Gate base ID
	*
	*	Gates Data
	*	[8]		SGateLink		gatesData[33]	// 32 gates + 1 root, 33 * 22 = 726
	*	[734]	SGateSetLink	gateSetsData[33] // 32 gate sets + 1 root, 33 * 14 = 462
	*	[1188]	uint8_t 		infoDataPresets[4]
	*	[1192]	uint8_t			unassigned[]
	*	[1200]	uint32_t		default clean delta
	*	[1204]	uint32_t		default dirty delta
	*/
	const uint16_t	kFlagsAddr	= 2;
	const uint16_t	kMotorTriggerThresholdAddr	= 3;
	const uint16_t	kGateBaseIDAddr = 4;
	const uint16_t	kGatesDataAddr = 8;
	const uint16_t	kInfoDataPresetAddr	= 1188;
	const uint16_t	kDefaultCleanDeltaAddr	= 1200;
	const uint16_t	kkDefaultDirtyDeltaAddr	= 1204;
	
	// Dust filter
	const uint32_t	kPressureUpdatePeriod = 1500;	// in milliseconds
	const uint8_t	kNumDeltas = 4;	// Number of deltas contained in mDeltaSum.
	const uint8_t	kNumDeltaAvgs = 8;	// Number of Delta Averages representing
						// averages over the period kNumDeltaAvgs*kPressureUpdatePeriod
	
	// Dust bin motor
	const uint8_t	kBinMotorSampleSize = 8;
	const uint32_t	kMotorSensePeriod = 500;	// in milliseconds
	const uint8_t	kThresholdLowerLimit = 5;
	const uint8_t	kDefaultTriggerThreshold = 15;
	const uint8_t	kThresholdUpperLimit = 50;
	// Radio
	const uint16_t	kNodeID		= 56;	// Unique for each node on same network
	const uint8_t	kNetworkID	= 100;	// The same on all nodes that talk to each other
	const uint16_t	kAudioAlertNodeID	= 1;	// ID of the audio alert gateway
	const uint32_t	kFullMessage = 0x464344;	// DCF (big endian)
	const uint32_t	kFilterLoadedMessage = 0x4C4344;	// DCL (big endian)
	
	// CAN
	const uint8_t	kCANQueueSize		= 64;
	const uint32_t	kControllerID = 0x20000;// b 0010 0000 0000 0000 0000
	const uint32_t	kBroadcastID = 0x20001;
	const uint32_t	kBaseIDMask = 0x3FFE0;	// b 0011 1111 1111 1110 0000
	const uint32_t	kGateIndexMask = 0x1F;	// b				   1 1111
	const uint32_t	kCANBusyPeriod = 200;			// in milliseconds

	const uint8_t	kTextInset			= 3; // Makes room for drawing the selection frame
	const uint8_t	kTextVOffset		= 6; // Makes room for drawing the selection frame
	// To make room for the selection frame the actual font height in the font
	// file is reduced.  The actual height is kFontHeight.
	const uint8_t	kFontHeight			= 43;
	const uint8_t	kDisplayWidth		= 240;
	// The dust collector information displayed below the filter status meter
	// needs to be shifted down so that the selection frame doesn't draw too
	// close to the meter.
	const uint8_t	DCInfoOffset		= 10;
	// Gates & Gate Sets
	const uint8_t	kMaxGates = 32;
	const uint8_t	kMaxGateSets = 32;
	const uint32_t	kDefaultCleanDelta = 249; // Pa = ~1" water
	/*
	*	The dirty delta is set high so that the collector doesn't go into an
	*	error state when no gate sets are defined.
	*/
	const uint32_t	kDefaultDirtyDelta = 1245; // Pa = ~5" water
	
}

#endif // DCConfig_h

