/*
*	BMP280Utils.cpp, Copyright Jonathan Mackey 2019
*	Utility class for the Bosch BMP280 Digital Pressure Sensor.
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
#ifndef __MACH__
#include <Arduino.h>
#else
#include <math.h>
#endif
#include "BMP280Utils.h"

/******************************** CalcAltitude ********************************/
/*
*	Copyright (C) 2019 Bosch Sensortec GmbH
*
*	Calculates the approximate altitude using barometric pressure and the
*	supplied sea level hPa as a reference.
*	inSeaLevelhPa = current hPa at sea level.
*	inPressurePa = current hPa.
*	Returns the approximate altitude above sea level in meters.
*/
float BMP280Utils::CalcAltitude(
	float	inSeaLevelhPa,
	float	inPressurePa)
{
/*
  float pressure = readPressure(); // in Si units for Pascal
  pressure /= 100;
*/
	return (44330 * (1.0 - pow(inPressurePa / inSeaLevelhPa, 1.0/5.255 /*0.1903*/)));
}

/************************** CalcSeaLevelForAltitude ***************************/
/*
*	Copyright (C) 2019 Bosch Sensortec GmbH
*
*	Calculates the pressure at sea level (in hPa) from the specified altitude
*	(in meters), and atmospheric pressure (in hPa).
*	inAltitude = Altitude in meters
*	inPressurePa = Atmospheric pressure in hPa
*	Returns the approximate pressure at sea level.
*/
float BMP280Utils::CalcSeaLevelForAltitude(
	float	inAltitude,
	float	inPressurePa)
{
	// Equation taken from BMP180 datasheet (page 17):
	// http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

	// Note that using the equation from wikipedia can give bad results
	// at high altitude.  See this thread for more information:
	// http://forums.adafruit.com/viewtopic.php?f=22&t=58064
	return (inPressurePa / pow(1.0 - (inAltitude / 44330.0), 5.255));
}

/************************** CalcPressureForAltitude ***************************/
/*
*	Copyright (C) 2019 Bosch Sensortec GmbH
*
*	Calculates the pressure for a given altitude using the sea level (in hPa.)
*	inAltitude = Altitude in meters
*	inSeaLevelhPa = current hPa at sea level.
*	Returns the approximate pressure for inAltitude.
*/
float BMP280Utils::CalcPressureForAltitude(
	float	inAltitude,
	float	inSeaLevelhPa)
{
	// Equation taken from BMP180 datasheet (page 17):
	// http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

	// Note that using the equation from wikipedia can give bad results
	// at high altitude.  See this thread for more information:
	// http://forums.adafruit.com/viewtopic.php?f=22&t=58064
	return (inSeaLevelhPa * pow(1.0 - (inAltitude / 44330.0), 5.255));
}

/******************************* Int32ToIntStr ********************************/
/*
*	inNum is of the form NNdd as returned by the BMP280, where dd are the two
*	places after the decimal point. This routine rounds the number up, returning
*	only an integer string.
*	Ex: 3195 = 32, -3195 = -31, -3192 = -31
*	Returns the number of characters. -12 returns 3
*/
uint8_t BMP280Utils::Int32ToIntStr(
	int32_t	inNum,
	char*	inBuffer)
{
	uint8_t	charsInStr = 0;
	inNum += 55;
	if (inNum < 0)
	{
		*(inBuffer++) = '-';
		inNum = -inNum;
		charsInStr++;
	}
	int32_t	decNum = inNum/100;
	for (int32_t num = decNum; num/=10; inBuffer++);
	char*	bufPtr = inBuffer;
	do
	{
		*(bufPtr--) = (decNum % 10) + '0';
		decNum /= 10;
	} while (decNum);
	charsInStr += (inBuffer - bufPtr);
	inBuffer[1] = 0;
	return(charsInStr);
}

/****************************** Int32ToDec21Str *******************************/
/*
*	inNum is of the form NNdd as returned by the BMP280, where dd are the two
*	places after the decimal point. This routine rounds the number up, returning
*	only 1 place after the decimal point.
*	Ex: 3195 = 32.0, -3195 = -31.9, -3192 = -31.8
*	Returns the number of characters before the decimal point. -12.3 returns 3
*	The resulting string always has 1 place after the decimal point. 0 = 0.0
*/
uint8_t BMP280Utils::Int32ToDec21Str(
	int32_t	inNum,
	char*	inBuffer)
{
	uint8_t	charsBeforeDec = 0;
	inNum += 5;
	if (inNum < 0)
	{
		*(inBuffer++) = '-';
		inNum = -inNum;
		charsBeforeDec++;
	}
	int32_t	decNum = inNum/100;
	for (int32_t num = decNum; num/=10; inBuffer++);
	char*	bufPtr = inBuffer;
	do
	{
		*(bufPtr--) = (decNum % 10) + '0';
		decNum /= 10;
	} while (decNum);
	charsBeforeDec += (inBuffer - bufPtr);
	inBuffer[1] = '.';
	inBuffer[2] = ((inNum/10) % 10) + '0';
	inBuffer[3] = 0;
	return(charsBeforeDec);
}

/****************************** Int32ToDec22Str *******************************/
/*
*	inNum is of the form -1234 = -12.34
*	Returns the number of characters before the decimal point. -12.34 returns 3
*	The resulting string always has 2 places after the decimal point. 0 = 0.00
*/
uint8_t BMP280Utils::Int32ToDec22Str(
	int32_t	inNum,
	char*	inBuffer)
{
	uint8_t	charsBeforeDec = 0;
	if (inNum < 0)
	{
		*(inBuffer++) = '-';
		inNum = -inNum;
		charsBeforeDec++;
	}
	int32_t	num;
	int32_t	decNum = num = inNum/100;
	for (; num/=10; inBuffer++);
	char*	bufPtr = inBuffer;
	do
	{
		*(bufPtr--) = (decNum % 10) + '0';
		decNum /= 10;
	} while (decNum);
	charsBeforeDec += (inBuffer - bufPtr);
	inBuffer[1] = '.';
	inBuffer[4] = 0;
	bufPtr = &inBuffer[3];
	decNum = inNum%100;
	num = 2;
	do
	{
		*(bufPtr--) = (decNum % 10) + '0';
		decNum /= 10;
	} while (--num);
	return(charsBeforeDec);
}
