/*
*	CANFrame.cpp, Copyright Jonathan Mackey 2020
*	Class to manage Microchip MCP2515 CAN Controller's CAN Frame
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
#include "CANFrame.h"
#include "Arduino.h"

/********************************** CANFrame **********************************/
CANFrame::CANFrame(
	uint16_t	inStandardID,
	bool		isRemoteTxRequest)
{
	Init(isRemoteTxRequest);
	SetStandardID(inStandardID);
}

/********************************** CANFrame **********************************/
CANFrame::CANFrame(
	uint16_t	inStandardID,
	uint32_t	inExtendedID,
	bool		isRemoteTxRequest)
{
	Init(isRemoteTxRequest);
	SetStandardID(inStandardID);
	SetExtendedID(inExtendedID);
}

/********************************** CANFrame **********************************/
CANFrame::CANFrame(
	uint16_t				inStandardID,
	uint8_t					isDataLen,
	const uint8_t*			inData)
{
	Init();
	SetStandardID(inStandardID);
	SetData(isDataLen, inData);
}

/********************************** CANFrame **********************************/
CANFrame::CANFrame(
	uint16_t		inStandardID,
	uint32_t		inExtendedID,
	uint8_t			isDataLen,
	const uint8_t*	inData)
{
	Init();
	SetStandardID(inStandardID);
	SetExtendedID(inExtendedID);
	SetData(isDataLen, inData);
}

/********************************** CANFrame **********************************/
CANFrame::CANFrame(
	uint16_t		inStandardID,
	uint32_t		inExtendedID,
	uint8_t			inData)
{
	Init();
	SetStandardID(inStandardID);
	SetExtendedID(inExtendedID);
	SetData(inData);
}

/********************************** CANFrame **********************************/
CANFrame::CANFrame(
	uint16_t		inStandardID,
	uint32_t		inExtendedID,
	uint32_t		inData)
{
	Init();
	SetStandardID(inStandardID);
	SetExtendedID(inExtendedID);
	SetData(inData);
}

/********************************** CANFrame **********************************/
CANFrame::CANFrame(
	const uint8_t*	inRaw)
{
	memcpy(mRaw, inRaw, sizeof(CANFrame));
}

/********************************* CopyToRaw **********************************/
void CANFrame::CopyToRaw(
	const uint8_t*	inRaw)
{
	memcpy(mRaw, inRaw, sizeof(CANFrame));
}

/******************************** CopyFromRaw *********************************/
void CANFrame::CopyFromRaw(
	uint8_t*	outRaw)
{
	memcpy(outRaw, mRaw, sizeof(CANFrame));
}

/************************************ Init ************************************/
void CANFrame::Init(
	bool	isRemoteTxRequest)
{
	memset(mRaw, 0, sizeof(mRaw));
	if (isRemoteTxRequest)
	{
		mRaw[4] = 0x40;
	}
}

/******************************* SetStandardID ********************************/
void CANFrame::SetStandardID(
	uint16_t	inStandardID)
{
	mRaw[0] = inStandardID >> 3;
	mRaw[1] &= 0b00011111;
	mRaw[1] |= (inStandardID << 5);
}
/******************************* GetStandardID ********************************/
uint16_t CANFrame::GetStandardID(void) const
{
	return((((uint16_t)mRaw[0]) << 3) + (mRaw[1] >> 5));
}

/******************************* SetExtendedID ********************************/
void CANFrame::SetExtendedID(
	uint32_t	inExtendedID)
{
	// Set the extended bit and the high 2 bits of the extended ID
	mRaw[1] &= 0b11110000;
	mRaw[1] |= ((inExtendedID >> 16) + 8);
	// Set extended ID bits 8:15
	mRaw[2] = inExtendedID >> 8;
	// Set extended ID bits 0:7
	mRaw[3] = inExtendedID;
}

/******************************* GetExtendedID ********************************/
uint32_t CANFrame::GetExtendedID(void) const
{
	return((((uint32_t)(mRaw[1] & 3)) << 16) + (((uint16_t)mRaw[2]) << 8) + mRaw[3]);
}

/********************************** SetData ***********************************/
/*
*	No check is made to see if inDataLen is valid.
*/
void CANFrame::SetData(
	uint8_t			inDataLen,
	const uint8_t*	inData)
{
	mRaw[4] = inDataLen;	// Overwrites RTR.  RTR doesn't have data.
	memcpy(&mRaw[5], inData, inDataLen);
}

/********************************** SetData ***********************************/
void CANFrame::SetData(
	uint8_t	inData)
{
	mRaw[4] = 1;	// Overwrites RTR.  RTR doesn't have data.
	mRaw[5] = inData;
}

/********************************** SetData ***********************************/
void CANFrame::SetData(
	uint16_t	inData)
{
	mRaw[4] = 2;	// Overwrites RTR.  RTR doesn't have data.
	*((uint16_t*)&mRaw[5]) = inData;
}

/********************************** SetData ***********************************/
void CANFrame::SetData(
	uint32_t	inData)
{
	mRaw[4] = 4;	// Overwrites RTR.  RTR doesn't have data.
	*((uint32_t*)&mRaw[5]) = inData;
}


