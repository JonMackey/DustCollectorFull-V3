/*
*	FilterStatusMeter.h, Copyright Jonathan Mackey 2020
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
#ifndef FilterStatusMeter_h
#define FilterStatusMeter_h

#include "MSPeriod.h"

class DisplayController;

class FilterStatusMeter
{
public:
							FilterStatusMeter(void);
	void					Initialize(
								DisplayController*		inDisplay,
								uint8_t					inTop,
								uint8_t					inLeft);
	void					Update(
								bool					inUpdateAll);
	void					SetMinMax(
								uint32_t				inMin,
								uint32_t				inMax);
	void					SetValue(
								uint32_t				inValue);
	enum ESetup
	{
		eInset			= 13,		// Also defines the arrow dimensions (should be an odd number)
		eHeight			= 48,
		eWidth			= 190,
		eTransHeight	= eHeight - (eInset*2),	// 22
		eTransWidth		= eWidth - eInset,	// 181
		eBackColor		= 0,		// Black
		eIndicatorColor	= 0xFFFF,	// White
		eStartColor		= 0x4665,	// Green
		eCenterColor	= 0x07DF,	// Yellow
		eEndColor		= 0x31DF	// Red
	};
protected:
	MSPeriod			mAnimationPeriod;
	DisplayController*	mDisplay;
	uint8_t				mTop;
	uint8_t				mLeft;
	uint8_t				mIndicatorPos;	// Managed by DrawIndicator
	uint8_t				mPos;			// Desired indicator position
	uint32_t			mMin;
	uint32_t			mMax;

	void					DrawIndicator(
								bool					inRefresh);
	void					DrawTransitions(void);
	static uint16_t			TransColorAtPos(
								uint8_t					inPosition);
	static void				GenerateTransitionLine(
								uint16_t				inFromColor,
								uint16_t				inToColor,
								uint8_t					inNumSteps,
								uint16_t*				outLine);
	static uint16_t			CalcTransitionColor(
								uint16_t				inFromColor,
								uint16_t				inToColor,
								uint8_t					inNumSteps,
								uint8_t					inStep);
};

#endif // FilterStatusMeter_h
