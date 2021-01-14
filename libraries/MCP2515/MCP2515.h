/*
*	MCP2515.h, Copyright Jonathan Mackey 2020
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
#ifndef MCP2515_h
#define MCP2515_h

#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
#include "tinySPI.h"
#else
#include <SPI.h>
#endif
#include "CANFrame.h"

class MCP2515
{
public:
							MCP2515(
								uint8_t					inCSPin,
								int8_t					inResetPin = -1);

	uint8_t					GetStatus(void);
	enum EReqToSendMask
	{
		// Any combination of eReqToSendxxx is ORd with eReqToSendInst
		eReqToSendTXBO			= 0x01,
		eReqToSendTXB1			= 0x02,
		eReqToSendTXB2			= 0x04
	};
protected:
	enum EInstruction
	{
		eResetInst				= 0xC0,
		eReadInst				= 0x03,
		eReadRx0IDBuffInst		= 0x90,
		eReadRx0DataBuffInst	= 0x92,
		eReadRx1IDBuffInst		= 0x94,
		eReadRx1DataBuffInst	= 0x96,
		eWriteInst				= 0x02,
		eLoadTx0IDBuffInst		= 0x40,
		eLoadTx0DataBuffInst,
		eLoadTx1IDBuffInst,
		eLoadTx1DataBuffInst,
		eLoadTx2IDBuffInst,
		eLoadTx2DataBuffInst,
		eReqToSendInst			= 0x80,
		eReadStatusInst			= 0xA0,
			eRX0IF_CANINTF		= 0,
			eRX1IF_CANINTF,
			eTXREQ_TXB0CTRL,
			eTX0IF_CANINTF,
			eTXREQ_TXB1CTRL,
			eTX1IF_CANINTF,
			eTXREQ_TXB2CTRL,
			eTX2IF_CANINTF,
		eRxStatusInst			= 0xB0,
		eBitModifyInst			= 0x05
	};
	enum ERegs
	{
		eRXF0Reg				= 0,	// Receive Filter 0 (CM)
		eRXF1Reg				= 0x4,
		eRXF2Reg				= 0x8,
		eRXF3Reg				= 0x10,
		eRXF4Reg				= 0x14,
		eRXF5Reg				= 0x18,
		eRXM0Reg				= 0x20,	// Receive Mask 0 (CM)
		eRXM1Reg				= 0x24,
		eBFPCTRLReg				= 0x0C,	// RXnBF Pin Control & Status (CM)
		eTXRTCTRLReg			= 0x0D,	// TXnRTS Pin Control & Status (CM)
		eCANSTATReg				= 0x0E,	// Status
			eOpModeMask			= 0xE0,
				eNormalMode		= 0,
				eSleepMode		= 0x20,
				eLoopbackMode	= 0x40,
				eListenOnlyMode	= 0x60,
				eConfigMode		= 0x80,
			eICODMask			= 0x0E,
				eNoInterrupt	= 0,
				eErrorInterrupt	= 2,
				eWakupInterrupt	= 4,
				eTXB0Interrupt	= 6,
				eTXB1Interrupt	= 8,
				eTxB2Interrupt	= 0x0A,
				eRxB0Interrupt	= 0x0C,
				eRxB1Interrupt	= 0x0E,
		eCANCTRLReg				= 0x0F,	// Control
		eTECReg					= 0x1C,	// Transmit Error Counter
		eRECReg,						// Receive Error Counter
		eCNF3Reg				= 0x28,	// (CM)
		eCNF2Reg,						// (CM)
		eCNF1Reg,						// (CM)
		eCANINTEReg				= 0x2B, // Interrupt Enable
			eRX0IE				= 0,
			eRX1IE,
			eTX0IE,
			eTX1IE,
			eTX2IE,
			eERRIE,
			eWAKIE,
			eMERRE,
		eCANINTFReg				= 0x2C,	// Interrupt Flag
			eRX0IF				= 0,
			eRX1IF,
			eTX0IF,
			eTX1IF,
			eTX2IF,
			eERRIF,
			eWAKIF,
			eMERRF,
		eEFLGReg				= 0x2D,	// Error Flags
			eEWARN				= 0,
			eRXWAR,
			eTXWAR,
			eRXEP,
			eTXEP,
			eTXBO,
			eRX0OVR,
			eRX1OVR,
		eTXB0CTRLReg			= 0x30,	// Transmit Buffer 0 Control
			eTXREQ				= 3,	// Message Transmit Request bit
			eTXERR,						// Transmission Error Detected bit
		eTXB1CTRLReg			= 0x40,
			//eTXREQ			= 3,	// Message Transmit Request bit
			//eTXERR,					// Transmission Error Detected bit
		eTXB2CTRLReg			= 0x50,
			//eTXREQ			= 3,	// Message Transmit Request bit
			//eTXERR,					// Transmission Error Detected bit
		eRXB0CTRLReg			= 0x60,	// Receive Buffer 0 Control
			eFILHIT0			= 0,	// Acceptance filter 0/1 RXF0/RXF1
			eBUKT				= 2,	// Roll over to RXB1 if RXB0 is full
			eRXRTR,						// Remote Transfer request received
			eRXM				= 0x60,	// Filters on/off 0/1 >> This is a Mask
		eRXB1CTRLReg			= 0x70,
	};

	uint8_t		mCSPin;
	uint8_t		mResetPin;
#ifdef SPI_HAS_TRANSACTION
	SPISettings	mSPISettings;
#endif
	uint8_t		mChipSelBitMask;
	volatile uint8_t*	mChipSelPortReg;


	void					begin(	
								const uint8_t*			inTimingConfig);
	//virtual void			Init(void);
	inline void				BeginTransaction(void)
							{
							#ifdef SPI_HAS_TRANSACTION
								SPI.beginTransaction(mSPISettings);
							#else
								SPI.setDataMode(SPI_MODE0);
							#endif
								*mChipSelPortReg &= ~mChipSelBitMask;
							}

	inline void				EndTransaction(void)
							{
								*mChipSelPortReg |= mChipSelBitMask;
							#ifdef SPI_HAS_TRANSACTION
								SPI.endTransaction();
							#endif
							}
	virtual void			DoConfig(void) = 0;

	uint8_t					ReadReg(
								uint8_t					inReg);
	void					WriteReg(
								uint8_t					inReg,
								uint8_t					inData);
	void					WriteReg(
								uint8_t					inReg,
								uint8_t					inDataLen,
								const uint8_t*			inData);
	void					ModifyReg(
								uint8_t					inReg,
								uint8_t					inMask,
								uint8_t					inData);
	void					SetMode(
								uint8_t					inMode);
	bool					SendFrame(
								CANFrame&				inCANFrame);
	bool					ReceiveFrame(
								CANFrame&				outCANFrame);
};
#endif // MCP2515_h
