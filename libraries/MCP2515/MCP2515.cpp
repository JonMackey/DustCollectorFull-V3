/*
*	MCP2515.cpp, Copyright Jonathan Mackey 2020
*	Interface class for the Microchip MCP2515 CAN Controller
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
#include "MCP2515.h"
#include "Arduino.h"


/********************************** MCP2515 ***********************************/
MCP2515::MCP2515(
	uint8_t inCSPin,
	int8_t	inResetPin)
	: mCSPin(inCSPin), mResetPin(inResetPin)
#ifdef SPI_HAS_TRANSACTION
	, mSPISettings(10000000, MSBFIRST, SPI_MODE0)
#endif
{
	mChipSelBitMask = digitalPinToBitMask(mCSPin);
	mChipSelPortReg = portOutputRegister(digitalPinToPort(mCSPin));
	
	// Setting the CS pin mode and state is done here rather than in begin to
	// avoid interference with other SPI devices on the bus.
#ifdef SPI_HAS_TRANSACTION
	if (mCSPin != SS)	// If it's SS, SPI.begin() takes care of initializing it.
#endif
	{
		digitalWrite(mCSPin, HIGH);
		pinMode(mCSPin, OUTPUT);
	}
}

/*********************************** begin ************************************/
void MCP2515::begin(	
	const uint8_t*	inTimingConfig)
{
	/*
	*	Re the two delayMicroseconds() calls below:
	*	Per MCP2515 doc, wait 128 OSC1 clock cycles after the occurrence of a
	*	Power-on Reset, SPI Reset, after the assertion of the RESET pin, and
	*	after a wake-up from Sleep mode before accessing the MCP2515 via SPI.
	*
	*	Assuming 4MHz as the slowest crystal used on the MCP2515, delay 32us.
	*/

	// SPI.begin();	should be called early in ino setup() before this routine
	// is called.
	if (mResetPin >= 0)
	{
		digitalWrite(mResetPin, HIGH);
		pinMode(mResetPin, OUTPUT);
		digitalWrite(mResetPin, LOW);
		delay(1);
		digitalWrite(mResetPin, HIGH);
	} else
	{
		delayMicroseconds(32);	// Wait 128 OSC1 clock cycles
		BeginTransaction();
		SPI.transfer(eResetInst);
		EndTransaction();
	}

	delayMicroseconds(32);	// Wait 128 OSC1 clock cycles
	// After reset the MCP2515 is in configuration mode.
	// Set the configuration
	WriteReg(eCNF3Reg, 3, inTimingConfig);
	
	DoConfig();
	
	// Change to normal mode, disable OSM, disable CLKOUT pin
	SetMode(eNormalMode);
}

/********************************* GetStatus **********************************/
uint8_t MCP2515::GetStatus(void)
{
	uint8_t	status;
	BeginTransaction();
	SPI.transfer(eReadStatusInst);
	status = SPI.transfer(0);
	EndTransaction();
	return(status);
}

/********************************** ReadReg ***********************************/
uint8_t MCP2515::ReadReg(
	uint8_t	inReg)
{
	uint8_t	regValue;
	BeginTransaction();
	SPI.transfer(eReadInst);
	SPI.transfer(inReg);
	regValue = SPI.transfer(0);
	EndTransaction();
	return(regValue);
}

/********************************** WriteReg **********************************/
/*
*	Write to multiple sequential registers.
*/
void MCP2515::WriteReg(
	uint8_t			inReg,
	uint8_t			inDataLen,
	const uint8_t*	inData)
{
	BeginTransaction();
	SPI.transfer(eWriteInst);
	SPI.transfer(inReg);
	for (uint8_t i = 0; i < inDataLen; i++)
	{
		SPI.transfer(inData[i]);
	}
	EndTransaction();
}

/********************************** WriteReg **********************************/
/*
*	Write to a single register.
*/
void MCP2515::WriteReg(
	uint8_t	inReg,
	uint8_t	inData)
{
	BeginTransaction();
	SPI.transfer(eWriteInst);
	SPI.transfer(inReg);
	SPI.transfer(inData);
	EndTransaction();
}

/********************************* ModifyReg **********************************/
void MCP2515::ModifyReg(
	uint8_t	inReg,
	uint8_t	inMask,
	uint8_t	inData)
{
	BeginTransaction();
	SPI.transfer(eBitModifyInst);
	SPI.transfer(inReg);
	SPI.transfer(inMask);
	SPI.transfer(inData);
	EndTransaction();
}

/********************************** SetMode ***********************************/
void MCP2515::SetMode(
	uint8_t	inMode)
{
	// Change to mode, disable OSM, disable CLKOUT pin
	BeginTransaction();
	SPI.transfer(eWriteInst);
	SPI.transfer(eCANCTRLReg);
	SPI.transfer(inMode);
	EndTransaction();
	// Wait for the mode to change (Assumes that it will succeed otherwise
	// something is very wrong with the MCP2515)
	{
		uint32_t	timeout = millis() + 5;
		while ((ReadReg(eCANSTATReg) & eOpModeMask) != inMode && millis() < timeout){}
	}
}

/********************************* SendFrame **********************************/
/*
*	Returns true if one of the 3 Tx buffers was filled and requested to be sent.
*	Else false if all 3 are full and waiting to be sent.
*/
bool MCP2515::SendFrame(
	CANFrame&	inCANFrame)
{
	// Find the first available Tx buffer.
	// The flag that determines if a buffer is available is TXREQ.
	// GetStatus returns the TXREQ flag for all 3 of the Tx buffers.
	uint8_t	status = GetStatus();
	// eTXREQ_TXBnCTRL... = 2, 4, 6. mask = 4 0x10 0x40
	uint8_t mask = _BV(eTXREQ_TXB0CTRL);
	uint8_t	loadTxBuffInst = eLoadTx0IDBuffInst;
	uint8_t	reqToSendBits = 1;
	for (; mask <= 0x40 && (status & mask); mask <<= 2)
	{
		loadTxBuffInst += 2;
		reqToSendBits <<= 1;
	}
	bool success = loadTxBuffInst <= eLoadTx2IDBuffInst;
	if (success)
	{
		// Load the Tx buffer with the CAN frame
		BeginTransaction();
		SPI.transfer(loadTxBuffInst);
		uint8_t	rawLength = inCANFrame.RawLength();
		uint8_t*	rawFrame = inCANFrame.GetRawFrame();
		for (uint8_t i = 0; i < rawLength; i++)
		{
			SPI.transfer(rawFrame[i]);
		}
		EndTransaction();
		
		// Tell the controller the Tx buffer is ready to be sent.
		BeginTransaction();
		SPI.transfer(eReqToSendInst + reqToSendBits);
		EndTransaction();
	}
	return(success);
}

/******************************** ReceiveFrame ********************************/
/*
*	Returns true if one of the 2 Rx buffers was received.
*	Else false if there are no Rx buffers ready to be received/read.
*	This routine takes the first Rx buffer available (0, then 1)
*/
bool MCP2515::ReceiveFrame(
	CANFrame&	outCANFrame)
{
	uint8_t	status = GetStatus();
	uint8_t mask = 1;
	uint8_t	readRxBuffInst = eReadRx0IDBuffInst;
	for (; mask <= 2 && (mask & status) == 0; mask <<= 1)
	{
		readRxBuffInst += 4;
	}
	bool success = readRxBuffInst <= eReadRx1IDBuffInst;
	if (success)
	{
		// Load the CAN frame with the Rx buffer
		BeginTransaction();
		SPI.transfer(readRxBuffInst);
		uint8_t*	rawFrame = outCANFrame.GetRawFrame();
		uint8_t i = 0;
		for (; i < 5; i++)
		{
			rawFrame[i] = SPI.transfer(0);
		}
		// Read the data, if any
		uint8_t	rawLength = outCANFrame.RawLength();
		for (; i < rawLength; i++)
		{
			rawFrame[i] = SPI.transfer(0);
		}
		EndTransaction();
		ModifyReg(eCANINTFReg, readRxBuffInst == eReadRx0IDBuffInst ? _BV(eRX0IF) : _BV(eRX1IF), 0);
	}
	return(success);
}


