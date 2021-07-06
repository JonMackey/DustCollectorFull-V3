/*
*	DCInfoField.h, Copyright Jonathan Mackey 2020
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
#ifndef DCInfoField_h
#define DCInfoField_h

#include <inttypes.h>
#include "XFont.h"
#include "DCConfig.h"

typedef uint32_t time32_t;
class DustCollector;

class DCInfoField
{
public:
							DCInfoField(void);
	void					Initialize(
								XFont*					inXFont,
								XFont::Font*			inSmallFont,
								DustCollector*			inDustCollector,
								uint8_t					inTextLine,
								uint8_t					inPresetIndex,
								uint8_t					inPresetDefault);
	void					Update(
								bool					inUpdateAll);
	void					IncrementField(
								bool					inIncrement);
	void					SetPreset(void);
	static char*			UInt8ToDecStr(
								uint8_t					inNum,
								char*					inBuffer);
	enum EDCInfo
	{
		eNothingInfo,
		eDuctPaInfo,
		eAmbientPaInfo,
		eBaselinePaInfo,
		eStaticPaInfo,
		eStaticInchesInfo,
		eTimeInfo,
		eDateInfo,
		eGateSetInfo,
		eMotorInfo,
		eSoftwareInfo,
		eInfoCount
	};
	
protected:
	DustCollector*		mDustCollector;
	XFont*				mXFont;
	XFont::Font*		mSmallFont;
	uint8_t				mTextLine;
	uint8_t				mDCInfo;
	uint8_t				mPresetIndex;
	uint8_t				mPrevDCInfo;
	uint8_t				mPrevGateSetIndex;
	bool				mPrevGateSetMatch;
	bool				mPrevDCIsRunning;
	uint8_t				mPrevBinMotorReading;
	uint32_t			mPrevAmbientPressure;
	uint32_t			mPrevDuctPressure;
	time32_t			mPrevDate;
	time32_t			mPrevTime;

	void					DrawPressure(
								int32_t					inPressure,
								uint16_t				inColor);
	void					MoveToTextTopLeft(
							uint8_t						inColumn = DCConfig::kTextInset);
	void					DrawWaiting(void);
	void					DrawItemP(
								const char*				inTextStrP);
};

#endif // DCInfoField_h
