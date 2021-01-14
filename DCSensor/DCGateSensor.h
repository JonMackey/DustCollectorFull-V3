/*
*	DCGateSensor.h, Copyright Jonathan Mackey 2020
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
#ifndef DCGateSensor_h
#define DCGateSensor_h

#include "MCP2515.h"
#include "MSPeriod.h"

extern volatile uint32_t	kTimestamp;

class DCGateSensor : public MCP2515
{
public:
							DCGateSensor();

	void					begin(void);

	void					Update(void);
	enum ERGBMask
	{
		eRGBOff,		
		eRed				= _BV(PORTA1),
		eGreen				= _BV(PORTA2),
		eBlue				= _BV(PORTA3),
		eCyan				= _BV(PORTA3) | _BV(PORTA2),
		eMagenta			= _BV(PORTA3) | _BV(PORTA1),
		eYellow				= _BV(PORTA1) | _BV(PORTA2),
		eWhite				= _BV(PORTA1) | _BV(PORTA2) | _BV(PORTA3),
	};
	void					SetStatusRGB(
								uint8_t					inState);
	void					IncRGB(void);
protected:
	uint32_t	mID;
	MSPeriod	mSendDelay;
	MSPeriod	mFlashDelay;
	uint8_t		mPrevGateIsOpen;
	static const uint8_t	kTimingConfig[];

	virtual void			DoConfig(void);
	void					SetSensorID(
								uint32_t				inSensorID);
	void					SendGateState(void);
	void					SendTimestamp(void);
	void					HandleReceivedFrame(
								CANFrame&				inCANFrame);
	uint8_t					GateIsOpen(void);
	void					ResendIfError(void);
};
#endif // DCGateSensor_h
