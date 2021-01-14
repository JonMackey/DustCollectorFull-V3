/*
*	DCGateSensor.cpp, Copyright Jonathan Mackey 2020
*	MCP2515 subclass for the Dust Collector blast gate Sensor (DCS)
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
#include "DCGateSensor.h"
#include "Arduino.h"
#include <EEPROM.h>
#include "DCMessages.h"
#include "DCSConfig.h"
//#include "CompileTime.h"
#ifdef HAS_SERIAL
uint32_t	lastHallValue;
#endif

//					The timing config is written CNF3, CNF2, CNF1
const uint8_t	DCGateSensor::kTimingConfig[] = {0x07, 0xAC, 0x04}; // 40kHz CAN baud rate

const uint32_t	kControllerID = 0x20000;
const uint32_t	kBroadcastID = 0x20001;

volatile bool	sMCP2515IntTriggered;

/********************************* DCGateSensor *********************************/
DCGateSensor::DCGateSensor(void)
	: MCP2515(DCSConfig::kCSPin, DCSConfig::kResetPin)
{
}

/*********************************** begin ************************************/
void DCGateSensor::begin(void)
{
#ifndef HAS_SERIAL
	pinMode(DCSConfig::kRedRGBTxPin, OUTPUT);
#endif
	pinMode(DCSConfig::kHallPin, INPUT);
	pinMode(DCSConfig::kGreenRGBRxPin, OUTPUT);
	pinMode(DCSConfig::kBlueRGBPin, OUTPUT);
	SetStatusRGB(DCGateSensor::eBlue);
	
	MCP2515::begin(kTimingConfig);
	sMCP2515IntTriggered = false;
	mPrevGateIsOpen = GateIsOpen();
	pinMode(DCSConfig::kGateOpenLEDPin, OUTPUT);
	digitalWrite(DCSConfig::kGateOpenLEDPin, mPrevGateIsOpen);

	cli();
	/*
	*	As per 9.3.1 in the ATtiny84 doc, clearing ISC00 and setting ISC01 sets
	*	the mode to generate an interrupt when PB2/INT0 is falling.
	*/
	MCUCR = (MCUCR & ~(_BV(ISC00))) | _BV(ISC01);	// Bits 1:0, ISC0n 0b10 = FALLING
	GIMSK |= _BV(INT0); // Enable INT0
	sei();
}

/********************************** DoConfig **********************************/
/*
*	The 18 bit (3 byte) ID:
*	The first 18 bits of the UNIX build timestamp is used as the ID.  This
*	creates an ID that's unique provided that more than one sketch upload
*	doesn't take place at the same time (same second), and all of the sensors
*	used with the same controller should have the sketch uploaded the same day
*	otherwise there's an extremely slight chance that the IDs will not be
*	unique.
*
*	When the MCU is new, all of the EEPROM bits are set to 1 (all bytes 0xFF.) 
*	When the sensor is powered on the first time, the ID stored in Flash is
*	used as a temporary sensor ID.  When the DCC sees a sensor with an unknown
*	ID it will send the sensor a new ID that the sensor will write to EEPROM.
*/
void DCGateSensor::DoConfig(void)
{
	EEPROM.get(DCSConfig::kCAN_ID_Addr, mID);
	/*
	*	Valid sensor ID ranges are 0x1 to 0x3FFFF, excluding the controller and
	*	broadcast IDs.
	*	When the EEPROM is uninitialized it contains all bits set.
	*	When this is the case, the id is set to the UNIX timestamp as a
	*	temporary ID.
	*/
	if (mID == 0xFFFFFFFF)
	{
		mID = kTimestamp & 0x3FFFF;
		/*
		*	On the highly remote chance that this will ever occur, make sure
		*	the ID derived from the timestamp isn't the controller or broadcast
		*	ID.
		*	If it is THEN
		*	make take a stab at making it unique.
		*/
		if (mID == kControllerID ||
			mID == kBroadcastID)
		{
			mID += 0x1FE;
		}
	}
	/*
	*	The largest valid extended CAN ID is 0x3FFFF (18 bits),  Clip the read
	*	ID value to make it valid in terms of the CAN protocol.  When the DCC
	*	receives a frame with a return address it doesn't recognize it will
	*	register the sensor by assigning and sending the sensor a new ID.
	*/
	mID &= 0x3FFFF;

	/*
	*	Setup the message acceptance filters and masks.
	*	
	*	The standard ID bits are used to pass commands and the extended ID bits
	*	are the sensor ID. The standard ID of the mask and filters are setup to
	*	allow any command through. The extended ID of the mask and filters are
	*	setup to allow only commands targeting this sensor or all sensors via
	*	broadcast.
	*	
	*	Either receive buffer can be targeted by any command.
	*	The interrupt pin is configured to trigger for errors and commands
	*	received.
	*/
	{
		CANFrame	filter(0, mID);
		// Set the filter to match just the sensor ID
		const uint8_t*	filterRawFrame = filter.GetRawFrame();
		WriteReg(eRXF0Reg, 4, filterRawFrame);
		// Set another filter to match the broadcast ID
		filter.SetExtendedID(kBroadcastID);
		WriteReg(eRXF1Reg, 4, filterRawFrame);
		// Set the rest of the filters to do nothing
		filter.SetStandardID(0x7FF);
		WriteReg(eRXF2Reg, 4, filterRawFrame);
		WriteReg(eRXF3Reg, 4, filterRawFrame);
		WriteReg(eRXF4Reg, 4, filterRawFrame);
		WriteReg(eRXF5Reg, 4, filterRawFrame);
	}
	{
		/*
		*	Set the mask to allow any command and force the entire sensor ID to
		*	match before a frame is accepted.
		*/
		CANFrame	mask(0, (uint32_t)0x3FFFF);
		const uint8_t*	maskRawFrame = mask.GetRawFrame();
		WriteReg(eRXM0Reg, 4, maskRawFrame);
		/*
		*	Set the other mask to do nothing.
		*/
		mask.SetStandardID(0x7FF);
		WriteReg(eRXM1Reg, 4, maskRawFrame);
	}
	/*
	*	Generate an interrupt when the receive buffer is full and if an
	*	error occurs.
	*/
	WriteReg(eCANINTEReg, _BV(eRX0IE) | _BV(eRX1IE) | _BV(eERRIE));
	/*
	*	Allow receive buffer 1 to be used if buffer 0 is full
	*/
	ModifyReg(eRXB0CTRLReg, _BV(eBUKT), _BV(eBUKT));
}

/******************************** SetSensorID *********************************/
void DCGateSensor::SetSensorID(
	uint32_t	inSensorID)
{
	if (mID != inSensorID)
	{
		mID = inSensorID;
		EEPROM.put(DCSConfig::kCAN_ID_Addr, mID);
	
		SetMode(eConfigMode);
		DoConfig();
		SetMode(eNormalMode);
	}
}

/*********************************** Update ***********************************/
/*
*	This is called every time the main sketch's loop is called.
*/
void DCGateSensor::Update(void)
{
	/*
	*	If the INT pin on the MCP2515 is low THEN
	*	see why.
	*/
	if (sMCP2515IntTriggered)
	{
		uint8_t	canICODStat = ReadReg(eCANSTATReg) & eICODMask;
		uint32_t	timeout = millis() + 100;
		//SetStatusRGB(eBlue);
		while (canICODStat && timeout > millis())
		{
			switch (canICODStat)
			{
				case eRxB0Interrupt:
				case eRxB1Interrupt:
				{
					CANFrame	canFrame;
					if (ReceiveFrame(canFrame))
					{
						HandleReceivedFrame(canFrame);
						// Clearing the interrupt is done in RecieveFrame
						//ModifyReg(eCANINTFReg, canICODStat == eRxB0Interrupt ? _BV(eRX0IF) : _BV(eRX1IF), 0);
					}
					break;
				}
				case eErrorInterrupt:
				{
					SetStatusRGB(eRed);
					// Clear the error interrupt
				    ModifyReg(eCANINTFReg, _BV(eERRIF), 0);
					/*
					*	See what triggered the error.
					*	If it was a transmit error, then try to resend it.
					*/
					ResendIfError();
					break;
				}
			}
			canICODStat = ReadReg(eCANSTATReg) & eICODMask;
		}
		/*if (!canICODStat)
		{
			SetStatusRGB(eBlue);
		}*/
		sMCP2515IntTriggered = false;
	}
	if (!mSendDelay.Get() ||
		mSendDelay.Passed())
	{
		uint8_t	gateIsOpen = GateIsOpen();
		if (mPrevGateIsOpen != gateIsOpen)
		{
			mPrevGateIsOpen = gateIsOpen;
			digitalWrite(DCSConfig::kGateOpenLEDPin, gateIsOpen);
			// Tell the dust collector controller the gate's open state changed.
			SendGateState();
			mSendDelay.Set(200);
			mSendDelay.Start();
		} else
		{
			mSendDelay.Set(0);
		}
	}
	if (mFlashDelay.Passed())
	{
		IncRGB();
		mFlashDelay.Start();
	}
}

/**************************** HandleReceivedFrame *****************************/
void DCGateSensor::HandleReceivedFrame(
	CANFrame&	inCANFrame)
{
	switch (inCANFrame.GetStandardID())
	{
		case DCController::eRequestGateState:
			SetStatusRGB(eGreen);
			SendGateState();
			break;
		case DCController::eFlash:
			mFlashDelay.Set(300);
			mFlashDelay.Start();
			break;
		case DCController::eStopFlash:
			SetStatusRGB(eRGBOff);
			mFlashDelay.Set(0);
			break;
		case DCController::eSetID:
			SetSensorID(*((const uint32_t*)inCANFrame.GetData()));
			break;
		case DCController::eSetFactoryID:
			SetSensorID(0xFFFFFFFF);
			break;
		case DCController::eRequestTimestamp:
			SendTimestamp();
			break;
	}
}

/******************************* ResendIfError ********************************/
void DCGateSensor::ResendIfError(void)
{
	uint8_t	reqToSendBits = 1;
	for (uint8_t txCtrlRegAddr = eTXB0CTRLReg;
			txCtrlRegAddr <= eTXB2CTRLReg; txCtrlRegAddr += 0x10)
	{
		if (ReadReg(txCtrlRegAddr) & eTXERR)
		{
			SetStatusRGB(eYellow);
			// Tell the controller the Tx buffer is ready to be resent.
			BeginTransaction();
			SPI.transfer(eReqToSendInst + reqToSendBits);
			EndTransaction();
			break;
		}
		reqToSendBits <<= 1;
	}
}

/******************************* SendGateState ********************************/
void DCGateSensor::SendGateState(void)
{
	CANFrame	gateStateFrame(mPrevGateIsOpen ? DCSensor::eGateIsOpen :
												DCSensor::eGateIsClosed,
													kControllerID, mID);
	SendFrame(gateStateFrame);
}

/******************************* SendTimestamp ********************************/
/*
*	Sends the sensor ID and the unix timestamp of when the software was compiled.
*	This timestamp is unique enough to use as an initial sensor ID.
*	The controller will change the initial sensor ID the first time the sensor
*	sends anything to the controller.
*/
void DCGateSensor::SendTimestamp(void)
{
	uint32_t	idAndTimestamp[] = {mID, kTimestamp};
	CANFrame	timestampFrame(DCSensor::eTimestamp, kControllerID, 8, (const uint8_t*)&idAndTimestamp);
	SendFrame(timestampFrame);
}

/*********************************** SetStatusRGB ***********************************/
void DCGateSensor::SetStatusRGB(
	uint8_t	inState)
{
	PORTA = (PORTA & ~eWhite) + (~inState & eWhite);
}

/*********************************** IncRGB ***********************************/
void DCGateSensor::IncRGB(void)
{
	uint8_t	rgbState = ~PORTA & eWhite;
	if (rgbState & eBlue)
	{
		rgbState = eRed;
	} else
	{
		rgbState <<= 1;
	}
	SetStatusRGB(rgbState);
}

/********************************* GateIsOpen *********************************/
uint8_t DCGateSensor::GateIsOpen(void)
{
	uint32_t	hallValue = analogRead(DCSConfig::kHallPin);
#ifdef HAS_SERIAL
	if (hallValue != lastHallValue)
	{
		lastHallValue = hallValue;
		swSerial.print(hallValue);
		swSerial.print(F(", "));
	}
#endif
	return(hallValue < 700);
}

/***************************** External INT0 ISR ******************************/
ISR(EXT_INT0_vect)
{
	// sMCP2515IntTriggered is only set here.
	// It's cleared when handled in Update().
	sMCP2515IntTriggered = sMCP2515IntTriggered || (PINB & DCSConfig::kPINBMask) != DCSConfig::kPINBMask;
}