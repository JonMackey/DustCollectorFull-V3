/*
*	FilterStatusMeter.cpp, Copyright Jonathan Mackey 2020
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
#include "FilterStatusMeter.h"
#include "DisplayController.h"

/***************************** FilterStatusMeter ******************************/
FilterStatusMeter::FilterStatusMeter(void)
: mAnimationPeriod(30)
{
}

/********************************* Initialize *********************************/
void FilterStatusMeter::Initialize(
	DisplayController*	inDisplay,
	uint8_t				inTop,
	uint8_t				inLeft)
{
	mDisplay = inDisplay;
	mTop = inTop;
	mLeft = inLeft;
	SetMinMax(0, eTransWidth);
	mPos = 0;
	mIndicatorPos = 0;
}


/*
	The height should be twice the inset plus the height of the transition.
	The width should be the inset rounded up to the nearest even value plus
	the width of the transition.
	
			  inset
				\/
	1/2 inset -> two color transistions <- 1/2 inset
				/\
			  inset

	432101234
	432101234
	 3210123
	 3210123
	  21012
	  21012
	   101
	   101
	    0
		012345678901234567890	
*/

/********************************* SetMinMax **********************************/
void FilterStatusMeter::SetMinMax(
	uint32_t	inMin,
	uint32_t	inMax)
{
	mMin = inMin;
	if (inMax > inMin)
	{
		mMax = inMax;
	} else
	{
		mMax = inMin +1;
	}
}

/********************************** SetValue **********************************/
void FilterStatusMeter::SetValue(
	uint32_t	inValue)
{
	if (inValue <= mMin)
	{
		mPos = 0;
	} else if (inValue >= mMax)
	{
		mPos = eTransWidth - 1;
	} else
	{
		mPos = ((eTransWidth - 1)*128)/(((mMax - mMin)*128)/(inValue-mMin));
	}
}

/*************************** GetColorForMinMaxValue ***************************/
/*
*	Function to generate a color ranging from green to yellow to red.
*/
uint16_t FilterStatusMeter::GetColorForMinMaxValue(
	uint32_t	inMin,
	uint32_t	inMax,
	uint32_t	inValue)
{
	uint8_t	pos;
	if (inValue <= inMin)
	{
		pos = 0;
	} else if (inValue >= inMax)
	{
		pos = eTransWidth - 1;
	} else
	{
		pos = ((eTransWidth - 1)*128)/(((inMax - inMin)*128)/(inValue-inMin));
	}
	return(TransColorAtPos(pos));
}

/****************************** TransColorAtPos *******************************/
uint16_t FilterStatusMeter::TransColorAtPos(
	uint8_t	inPosition)
{
	uint16_t	transColor;
	if (inPosition < (eTransWidth/2))
	{
		transColor = CalcTransitionColor(eStartColor, eCenterColor, eTransWidth/2, inPosition);
	} else
	{
		transColor = CalcTransitionColor(eCenterColor, eEndColor, eTransWidth/2, inPosition - (eTransWidth/2));
	}
	return(transColor);
}

/******************************* DrawIndicator ********************************/
/*
*	The indicator can only move by one pixel per call.  This allows for a smooth
*	animation of the indicator movement and also simplifies the drawing.  The
*	indicator will always try to match the position mPos but it may take several
*	calls to accomplish this.
*/
void FilterStatusMeter::DrawIndicator(
	bool	inRefresh)
{
	/*
	*	If the indicator needs to move THEN
	*	restore the transition color at the current indicator position and then
	*	move the indicator position closer to the desired position by one pixel.
	*	
	*/
	if (mAnimationPeriod.Passed() &&
		mIndicatorPos != mPos)
	{
		mAnimationPeriod.Start();
		inRefresh = true;

		mDisplay->MoveTo(mTop + eInset, mLeft + (eInset/2) + mIndicatorPos);
		mDisplay->SetColumnRange(1);
		mDisplay->FillPixels(eTransHeight, TransColorAtPos(mIndicatorPos));

		if (mIndicatorPos < mPos)
		{
			mIndicatorPos++;
		} else
		{
			mIndicatorPos--;
		}
	}
	if (inRefresh)
	{
/*
	01234567890
	.432101234.
	.432101234.
	..3210123..
	..3210123..
	...21012...
	...21012...
	....101....
	....101....
	.....0.....
		 012345678901234567890	
*/
		uint16_t	indicatorLine[eInset + 2];
		for (uint8_t k = 1; k < (eInset+1); k++)
		{
			indicatorLine[k] = eIndicatorColor;
		}
		indicatorLine[0] = eBackColor;
		indicatorLine[eInset+1] = eBackColor;
		uint8_t	leftIndPos = mLeft + mIndicatorPos - 1;
								// 9
		for (uint8_t k = 0; k < eInset; k++)
		{
			mDisplay->MoveToRow(mTop+k);
			mDisplay->SetColumnRange(leftIndPos, eInset + 2);
			mDisplay->CopyPixels(indicatorLine, eInset + 2);
			mDisplay->MoveToRow(mTop+(eInset*2)+eTransHeight-k);
			mDisplay->SetColumnRange(leftIndPos, eInset + 2);
			mDisplay->CopyPixels(indicatorLine, eInset + 2);
			if (k&1)
			{
				indicatorLine[(k/2)+1] = eBackColor;
				indicatorLine[eInset - (k/2)] = eBackColor;
			}
		}
		mDisplay->MoveTo(mTop + eInset, mLeft + (eInset/2) + mIndicatorPos);
		mDisplay->SetColumnRange(1);
		mDisplay->FillPixels(eTransHeight, eBackColor);
		//mDisplay->FillPixels(eTransHeight, ~TransColorAtPos(mIndicatorPos));
	}
}

/****************************** DrawTransitions *******************************/
void FilterStatusMeter::DrawTransitions(void)
{
	uint16_t	transLine[eTransWidth];	//181
	// 0 -> 90
	GenerateTransitionLine(eStartColor, eCenterColor, eTransWidth/2, transLine);
	// 90 -> 180
	GenerateTransitionLine(eCenterColor, eEndColor, eTransWidth/2, &transLine[eTransWidth/2]);
	for (uint8_t i = 0; i < eTransHeight; i++)
	{
		mDisplay->MoveTo(mTop + eInset + i, mLeft + (eInset/2));
		mDisplay->SetColumnRange(eTransWidth);
		mDisplay->CopyPixels(transLine, eTransWidth);
	}
}

/*************************** GenerateTransitionLine ***************************/
void FilterStatusMeter::GenerateTransitionLine(
	uint16_t	inFromColor,
	uint16_t	inToColor,
	uint8_t		inNumSteps,
	uint16_t*	outLine)
{
	uint8_t	stepValue, value, remInc;
	uint8_t	remAccumulator, remValue;
	uint16_t*	outLinePtr;
	const static uint16_t	mask565[] = {0x1F, 0x7E0, 0xF800};
	const static uint8_t	shiftAmt[] = {0, 5, 11};
	for (uint8_t sep = 0; sep < 3; sep++)
	{
		uint16_t	fromColor = inFromColor & mask565[sep];
		uint16_t	toColor = inToColor & mask565[sep];
		outLinePtr = outLine;
		remAccumulator = 0;
		if (sep)
		{
			*outLinePtr |= fromColor;
			fromColor >>= shiftAmt[sep];
			toColor >>= shiftAmt[sep];
		} else
		{
			*outLinePtr = fromColor;
		}
		outLinePtr++;
		value = fromColor;
		stepValue = toColor - fromColor;
		remValue = (int8_t)stepValue % inNumSteps;
		if (remValue & 0x80)
		{
			remValue = -remValue;
			remInc = -1;
		} else
		{
			remInc = 1;
		}
		stepValue = (int8_t)stepValue/inNumSteps;
		for (uint8_t step = 0; step < inNumSteps; step++)
		{
			value += stepValue;
			remAccumulator += remValue;
			if (remAccumulator >= inNumSteps)
			{
				value += remInc;
				remAccumulator -= inNumSteps;
			}
			if (sep)
			{
				*outLinePtr |= (((uint16_t)value) << shiftAmt[sep]);
			} else
			{
				*outLinePtr = value;
			}
			outLinePtr++;
		}
	}
}

/**************************** CalcTransitionColor *****************************/
uint16_t FilterStatusMeter::CalcTransitionColor(
	uint16_t	inFromColor,
	uint16_t	inToColor,
	uint8_t		inNumSteps,
	uint8_t		inStep)
{
	uint16_t	transColor = 0;
	if (inStep)
	{
		if (inStep <= inNumSteps)
		{
			int8_t	stepValue, remValue, value;
			const static uint16_t	mask565[] = {0x1F, 0x7E0, 0xF800};
			const static uint8_t	shiftAmt[] = {0, 5, 11};
			for (uint8_t sep = 0; sep < 3; sep++)
			{
				uint16_t	fromColor = inFromColor & mask565[sep];
				uint16_t	toColor = inToColor & mask565[sep];
				if (sep)
				{
					fromColor >>= shiftAmt[sep];
					toColor >>= shiftAmt[sep];
				}
				stepValue = toColor - fromColor;
				remValue = stepValue % inNumSteps;
				stepValue /= inNumSteps;
				value = fromColor;
				value += ((stepValue*inStep) + ((remValue*inStep)/inNumSteps));
				if (sep)
				{
					transColor |= (((uint16_t)value) << shiftAmt[sep]);
				} else
				{
					transColor = value;
				}
			}
		} else
		{
			transColor = inToColor;
		}
	} else
	{
		transColor = inFromColor;
	}
	return(transColor);
}

/*********************************** Update ***********************************/
void FilterStatusMeter::Update(
	bool	inUpdateAll)
{
	if (inUpdateAll)
	{
		DrawTransitions();
	}
	DrawIndicator(inUpdateAll);
}
