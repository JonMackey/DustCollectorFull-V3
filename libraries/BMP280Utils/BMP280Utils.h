/*
*	BMP280Utils.h, Copyright Jonathan Mackey 2019
*	Utility class for the Bosch BMP280 Digital Pressure Sensor.
*
*	This class is not intended to be a general interface.  It is specifically
*	for tinySPI and the FORCED mode of the BMP280 operation used by the data logger.
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
#ifndef BMP280Utils_h
#define BMP280Utils_h

#include <inttypes.h>

struct SBMP280Packet
{
	uint32_t	message;
	int32_t		temp;
	uint32_t	pres;
};

class BMP280Utils
{
public:
							// in tempC is C * 100. e.g. 2762 = 27.62C
	static int16_t			CToF(
								int16_t					inTempC)
								{return(((inTempC * 9) / 5) + 3200);}
	static int16_t			FToC(
								int16_t					inTempF)
								{return(((inTempF - 3200) * 5) / 9);}
	static float			CalcAltitude(
								float					inSeaLevelhPa,
								float					inPressurePa);
	static float			CalcSeaLevelForAltitude(
								float					inAltitude,
								float					inPressurePa);
	static float			CalcPressureForAltitude(
								float					inAltitude,
								float					inSeaLevelhPa);
	static uint8_t			Int32ToIntStr(
								int32_t					inNum,
								char*					inBuffer);
	static uint8_t			Int32ToDec21Str(
								int32_t					inNum,
								char*					inBuffer);
	static uint8_t			Int32ToDec22Str(
								int32_t					inNum,
								char*					inBuffer);
};
#endif // BMP280Utils_h
