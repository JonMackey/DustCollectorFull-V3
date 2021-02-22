/*
*	DCInfoField.cpp, Copyright Jonathan Mackey 2020
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
#include <Arduino.h>
#include <EEPROM.h>
#include "DCInfoField.h"
#include "DisplayController.h"
#include "DustCollector.h"
#include "BMP280Utils.h"
#include "UnixTime.h"
#include "FilterStatusMeter.h"

const char kTitleChars[] PROGMEM = " DABSS";
const char khPaSuffixStr[] PROGMEM = "hPa";
const char kInchesSuffixStr[] PROGMEM = "\"";
const char kUsingStr[] PROGMEM = "USING";
const char kExactStr[] PROGMEM = "EXACT";
const char kDefaultStr[] PROGMEM = "DEFAULT";
const char kGateSetPrefixStr[] PROGMEM = "GS:";
const char kMotorPrefixStr[] PROGMEM = "M:";
const char kWarnPrefixStr[] PROGMEM = "WARN:";


/******************************** DCInfoField *********************************/
DCInfoField::DCInfoField(void)
{
}

/********************************* Initialize *********************************/
void DCInfoField::Initialize(
	XFont*			inXFont,
	XFont::Font*	inSmallFont,
	DustCollector*	inDustCollector,
	uint8_t			inTextLine,
	uint8_t			inPresetIndex,
	uint8_t			inPresetDefault)
{
	mXFont = inXFont;
	mSmallFont = inSmallFont;
	mDustCollector = inDustCollector;
	mTextLine = inTextLine;
	mPresetIndex = inPresetIndex;
	{
		uint8_t	preset;
		EEPROM.get(DCConfig::kInfoDataPresetAddr + inPresetIndex, preset);
		if (preset >= eInfoCount)
		{
			preset = inPresetDefault;
		}
		mDCInfo = preset;
	}
	mPrevDCInfo = 99;
}

/******************************* IncrementField *******************************/
void DCInfoField::IncrementField(
	bool	inIncrement)
{
	uint8_t	dcInfo = mDCInfo;
	if (inIncrement)
	{
		if (dcInfo < (eInfoCount-1))
		{
			dcInfo++;
		} else
		{
			dcInfo = 0;
		}
	} else
	{
		if (dcInfo == 0)
		{
			dcInfo = eInfoCount;
		}
		dcInfo--;
	}
	mDCInfo = dcInfo;
}

/********************************* SetPreset **********************************/
void DCInfoField::SetPreset(void)
{
	EEPROM.put(DCConfig::kInfoDataPresetAddr + mPresetIndex, mDCInfo);
}

// Prefix width = 31 ("D:" is the widest at 31.)
// Prefix width + space = 39
// Value width 0000.00 = 115
// Value left offset = Prefix width = 39
// Value right offset = Prefix width + value width = 39 + 115 = 154
// Suffix left offset = Value right offset + space = 154 + 8 = 162
// All of the final offsets need to be shifted by kTextInset or kTextVOffset
/******************************** DrawPressure ********************************/
void DCInfoField::DrawPressure(
	int32_t		inPressure,
	uint16_t	inColor)
{
	char	presStr[15];
	BMP280Utils::Int32ToDec22Str(inPressure, presStr);
	mXFont->SetTextColor(inColor);
	MoveToTextTopLeft();
	uint16_t	textWidth = mXFont->DrawRightJustified(presStr, 154+DCConfig::kTextInset);
	mXFont->GetDisplay()->MoveToColumn(39+DCConfig::kTextInset);
	// Erase any artifacts when moving to a shorter string.
	mXFont->EraseTillColumn((154+DCConfig::kTextInset)-textWidth);
}

/******************************** DrawWaiting *********************************/
void DCInfoField::DrawWaiting(void)
{
	mXFont->SetTextColor(XFont::eGray);
	MoveToTextTopLeft();
	mXFont->DrawRightJustified("---", 154+DCConfig::kTextInset);
	mXFont->EraseTillColumn(154+DCConfig::kTextInset);
}

/***************************** MoveToTextTopLeft ******************************/
void DCInfoField::MoveToTextTopLeft(
	uint8_t	inColumn)
{
	mXFont->GetDisplay()->MoveTo((mTextLine*DCConfig::kFontHeight) +
								DCConfig::DCInfoOffset + DCConfig::kTextVOffset,
								inColumn);
}

/********************************* DrawItemP **********************************/
void DCInfoField::DrawItemP(
	const char*	inTextStrP)
{
	char	textStr[20];	// Assumed all strings are less than 20 bytes
	strcpy_P(textStr, inTextStrP);
	mXFont->DrawStr(textStr);
}

/*********************************** Update ***********************************/
void DCInfoField::Update(
	bool	inUpdateAll)
{
	uint16_t	color;
	switch (mDCInfo)
	{
		case eDuctPaInfo:
			color = XFont::eMagenta;
			break;
		case eAmbientPaInfo:
			color = XFont::eCyan;
			break;
		case eStaticPaInfo:
		case eStaticInchesInfo:
			color = XFont::eYellow;
			break;
		default:
			color = XFont::eWhite;
	}
	
	if (inUpdateAll ||
		mDCInfo != mPrevDCInfo)
	{
		mXFont->SetTextColor(color);
		mPrevDCInfo = mDCInfo;
		inUpdateAll = true;
		MoveToTextTopLeft();
		mXFont->EraseTillColumn(240-DCConfig::kTextInset);
		if (mDCInfo && mDCInfo <= eStaticInchesInfo)
		{
			{
				char titleStr[3];
				char titleChars[6];
				strcpy_P(titleChars, kTitleChars);
				titleStr[0] = titleChars[mDCInfo];
				titleStr[1] = ':';
				titleStr[2] = 0;
				mXFont->DrawRightJustified(titleStr, 31+DCConfig::kTextInset);
			}
			{
				mXFont->GetDisplay()->MoveToColumn(162);
				DrawItemP(mDCInfo != eStaticInchesInfo ? khPaSuffixStr : kInchesSuffixStr);
			}
		} else if (mDCInfo == eGateSetInfo)
		{
			mXFont->GetDisplay()->MoveToColumn(DCConfig::kTextInset);
			DrawItemP(kGateSetPrefixStr);
		} else if (mDCInfo == eMotorInfo)
		{
			char	valueStr[15];
			mXFont->GetDisplay()->MoveToColumn(DCConfig::kTextInset);
			DrawItemP(kMotorPrefixStr);
			mXFont->SetTextColor(XFont::eRed);
			mXFont->GetDisplay()->MoveToColumn(DCConfig::kTextInset+88);
			DrawItemP(kWarnPrefixStr);
			UInt8ToDecStr(mDustCollector->GetTriggerThreshold(), valueStr);
			mXFont->GetDisplay()->MoveToColumn(DCConfig::kTextInset + 198);
			mXFont->DrawStr(valueStr, true);
			
		}
	}
	switch (mDCInfo)
	{
		case eDuctPaInfo:
		case eAmbientPaInfo:
		case eBaselinePaInfo:
		case eStaticPaInfo:
		case eStaticInchesInfo:
		{
			bool	presChanged = false;
			uint32_t	ambientPressure = mDustCollector->AmbientPressure();
			if (inUpdateAll ||
				ambientPressure != mPrevAmbientPressure)
			{
				presChanged = true;
				mPrevAmbientPressure = ambientPressure;
				if (mDCInfo == eAmbientPaInfo)
				{
					DrawPressure(ambientPressure, color);
					break;
				}
			}
			uint32_t	ductPressure = mDustCollector->DuctPressure();
			if (inUpdateAll ||
				ductPressure != mPrevDuctPressure)
			{
				presChanged = true;
				mPrevDuctPressure = ductPressure;
				if (mDCInfo == eDuctPaInfo)
				{
					DrawPressure(ductPressure, color);
					break;
				}
			}
			if (inUpdateAll ||
				presChanged)
			{
				if (mDustCollector->DeltaAveragesLoaded())
				{
					switch(mDCInfo)
					{
						case eBaselinePaInfo:
							// The baseline delta is recorded every 1.5 seconds.
							// When the dust collector starts (an adjusted delta above
							// 25Pa), the last 4 baseline readings are averaged.  This averaged
							// baseline value is subtracted from the current delta.
							DrawPressure(mDustCollector->Baseline(), color);
							break;
						case eStaticPaInfo:	// Adjusted static duct pressure
							// A running average is created by subtracting the oldest
							// baseline in the queue, and adding the newest reading.
							DrawPressure(mDustCollector->AdjustedDeltaAverage(), color);
							break;
						case eStaticInchesInfo:	// Adjusted static duct pressure as inches of water.
						{
							// Inches of water is the standard measurement unit for air
							// duct pressure in the USA. 1" of water = 248.84Pa or 2.4884hPa.
							int32_t	inchesWater = (mDustCollector->AdjustedDeltaAverage() * 10000)/24884;
							if (inchesWater < 0)
							{
								inchesWater = 0;
							}
							DrawPressure(inchesWater, color);
							break;
						}
					}
				} else if (inUpdateAll)
				{
					// Waiting for the delta averages to load
					DrawWaiting();
				}
			}
			break;
		}
		case eTimeInfo:
		{
			time32_t	time = UnixTime::Time();
			if (time != mPrevTime)
			{
				mPrevTime = time;
				char timeStr[32];
				bool isPM = UnixTime::CreateTimeStr(timeStr);
				MoveToTextTopLeft();
				mXFont->SetTextColor(XFont::eWhite);
				mXFont->DrawCentered(timeStr);
				/*
				*	If updating everything OR
				*	the AM/PM suffix state changed (to/from AM/PM or hidden) THEN
				*	draw or erase the suffix.
				*/
				if (inUpdateAll)
				{
					if (!UnixTime::Format24Hour())
					{
						XFont::Font*	savedFont = mXFont->GetFont();
						mXFont->SetFont(mSmallFont);
						mXFont->DrawStr(isPM ? " PM":" AM");
						mXFont->SetFont(savedFont);
						// The width of a P is slightly less than an A, so erase any
						// artifacts left over when going from A to P.
						// The width of an 18pt A - the width of a P = 1
						mXFont->GetDisplay()->FillBlock(mXFont->FontRows(), 1, XFont::eBlack);
					}
				}
			}
			break;
		}
		case eDateInfo:
		{
			time32_t	date = UnixTime::Date();
			if (inUpdateAll ||
				date != mPrevDate)
			{
				mPrevDate = date;
				char dateStr[32];
				UnixTime::CreateDateStr(date, dateStr);
				mXFont->SetTextColor(XFont::eWhite);
				MoveToTextTopLeft();
				mXFont->DrawCentered(dateStr);
				mXFont->EraseTillColumn(220+DCConfig::kTextInset);	// Widest date string
			}
			break;
		}
		case eGateSetInfo:
		{
			GateSets&	gateSets = mDustCollector->GetGateSets();
			uint8_t	currentGateSetIndex = gateSets.GetCurrentIndex();
			bool gateSetMatch = currentGateSetIndex > 0 && mDustCollector->OpenGates() == gateSets.GetCurrent().gatesMask;
			if (inUpdateAll ||
				mPrevGateSetIndex != currentGateSetIndex ||
				mPrevGateSetMatch != gateSetMatch)
			{
				char	valueStr[15];
				const char*	setUsedStatusPtr;
				mPrevGateSetIndex = currentGateSetIndex;
				mPrevGateSetMatch = gateSetMatch;
				MoveToTextTopLeft(DCConfig::kTextInset + 56);
				if (currentGateSetIndex)
				{
					char*	valueSuffixPtr = UInt8ToDecStr(currentGateSetIndex, valueStr);
					*(valueSuffixPtr++) = ',';
					*valueSuffixPtr = 0;
					mXFont->SetTextColor(XFont::eWhite);
					mXFont->DrawStr(valueStr, true);
					mXFont->SetTextColor(gateSetMatch ? XFont::eGreen : XFont::eYellow);
					mXFont->GetDisplay()->MoveToColumn(DCConfig::kTextInset + 107);
					setUsedStatusPtr = gateSetMatch ? kExactStr : kUsingStr;
				} else
				{
					mXFont->SetTextColor(XFont::eYellow);
					setUsedStatusPtr = kDefaultStr;
				}
				DrawItemP(setUsedStatusPtr);
			}
			break;
		}
		case eMotorInfo:
		{
			uint8_t	binMotorReading = mDustCollector->GetBinMotorReading();
			if (inUpdateAll ||
				mPrevBinMotorReading != binMotorReading)
			{
				char	valueStr[15];
				mPrevBinMotorReading = binMotorReading;
				MoveToTextTopLeft();
				UInt8ToDecStr(binMotorReading, valueStr);
				mXFont->GetDisplay()->MoveToColumn(DCConfig::kTextInset + 44);
				mXFont->SetTextColor(FilterStatusMeter::GetColorForMinMaxValue(0, mDustCollector->GetTriggerThreshold(), binMotorReading));
				mXFont->DrawStr(valueStr);
				mXFont->EraseTillColumn((88+DCConfig::kTextInset));
			}
			break;
		}
	}
}

/******************************* UInt8ToDecStr ********************************/
/*
*	Returns the pointer to the char after the last char (the null terminator)
*/
char* DCInfoField::UInt8ToDecStr(
	uint8_t	inNum,
	char*	inBuffer)
{
	if (inNum == 0)
	{
		*(inBuffer++) = '0';
	} else
	{
		int8_t num = inNum;
		for (; num/=10; inBuffer++){}
		char*	bufPtr = inBuffer;
		while (inNum)
		{
			*(bufPtr--) = (inNum % 10) + '0';
			inNum /= 10;
		}
		inBuffer++;
	}
	*inBuffer = 0;
	
	return(inBuffer);
}
