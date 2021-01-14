/*
*	CANFrame.h, Copyright Jonathan Mackey 2020
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
#ifndef CANFrame_h
#define CANFrame_h

#include <inttypes.h>

class CANFrame
{
public:
							CANFrame(void){}
							CANFrame(
								uint16_t				inStandardID,
								bool					isRemoteTxRequest = false);
							CANFrame(
								uint16_t				inStandardID,
								uint32_t				inExtendedID,
								bool					isRemoteTxRequest = false);
							CANFrame(
								uint16_t				inStandardID,
								uint8_t					isDataLen,
								const uint8_t*			inData);
							CANFrame(
								uint16_t				inStandardID,
								uint32_t				inExtendedID,
								uint8_t					isDataLen,
								const uint8_t*			inData);
							CANFrame(
								uint16_t				inStandardID,
								uint32_t				inExtendedID,
								uint8_t					inData);
							CANFrame(
								uint16_t				inStandardID,
								uint32_t				inExtendedID,
								uint32_t				inData);
							CANFrame(
								const uint8_t*			inRaw);
	void					Init(
								bool					isRemoteTxRequest = false);
							// On Rx IsRemoteRequest is for an extended frames only
							// For standard frames use IsStandardRemoteRequest
	inline bool				IsRemoteRequest(void) const
								{return(mRaw[4] & 0x40);}
	inline bool				IsStandardRemoteRequest(void) const
								{return(mRaw[1] & 0x10);}
	void					SetStandardID(
								uint16_t				inStandardID);
	uint16_t				GetStandardID(void) const;
	void					SetExtendedID(
								uint32_t				inExtendedID);
	inline bool				IsExtended(void) const
								{return(mRaw[1] & 8);}
	uint32_t				GetExtendedID(void) const;
	void					SetData(
								uint8_t					inDataLen,
								const uint8_t*			inData);
	void					SetData(
								uint8_t					inData);
	void					SetData(
								uint16_t				inData);
	void					SetData(
								uint32_t				inData);
	inline uint8_t			GetDataLen(void) const
								{return(mRaw[4] & 0xF);}
	inline const uint8_t*	GetData(void) const
								{return(&mRaw[5]);}
	inline uint8_t*			GetRawFrame(void)
								{return(mRaw);}
	void					CopyFromRaw(
								uint8_t*				outRaw);
	void					CopyToRaw(
								const uint8_t*			inRaw);
	inline uint8_t			RawLength(void) const
								{return(GetDataLen() + 5);}
protected:
	uint8_t	mRaw[13];
};

#endif // CANFrame_h
