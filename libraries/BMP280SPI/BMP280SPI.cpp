/*
*	BMP280SPI.cpp, Copyright Jonathan Mackey 2019
*	Interface class for the Bosch BMP280 Digital Pressure Sensor.
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
#include "BMP280SPI.h"

//#define HAS_SERIAL
#ifdef HAS_SERIAL
#include <SendOnlySoftwareSerial.h>
extern SendOnlySoftwareSerial swSerial;
#endif

/*
*					 765		 432		   0
*	config 0xF5 t_sb[2:0] filter[2:0] spi3w_en[0] 0x00
*						  765		  432		 10
*	ctrl_meas 0xF4 osrs_t[2:0] osrs_p[2:0] mode[1:0] 0x00	
*/
const uint8_t	kConfig = (BMP280_ODR_0_5_MS << 5) | (BMP280_FILTER_OFF << 2) | BMP280_SPI3_WIRE_DISABLE;
const uint8_t	kCtrlMeas = (BMP280_OS_1X << 5) | (BMP280_OS_1X << 2) | BMP280_SLEEP_MODE;


/********************************* BMP280SPI *********************************/
BMP280SPI::BMP280SPI(
	uint8_t		inCSPin)
	: mCSPin(inCSPin), mCtrlMeas(kCtrlMeas), mConfig(kConfig)
#ifdef SPI_HAS_TRANSACTION
	  , mSPISettings(10000000, MSBFIRST, SPI_MODE0)
#endif
{
	// Setting the CS pin mode and state was moved from begin to avoid
	// interference with other SPI devices on the bus.
	digitalWrite(mCSPin, HIGH);
	pinMode(mCSPin, OUTPUT);
}

/****************************** SetOversampling *******************************/
/*
*	Both sampling values should be in the range of 0 to 5.
*	No sanity checking is performed on the passed values.
*/
void BMP280SPI::SetOversampling(
	uint8_t	inTempOversampling,
	uint8_t	inPressureOversampling)
{
	mCtrlMeas = (mCtrlMeas & 3) | (inTempOversampling << 5) | (inPressureOversampling << 2);
}

/**************************** SetFilterCoefficient ****************************/
/*
*	The setting should be in the range of 0 to 4.
*	No sanity checking is performed on the passed value.
*/
void BMP280SPI::SetFilterCoefficient(
	uint8_t	inFilterSetting)
{
	mConfig = (mConfig & 0x1C) | (inFilterSetting << 2);
}

/*********************************** begin ************************************/
int8_t BMP280SPI::begin(void)
{
	uint8_t	retries = 5;
	int8_t	result = BMP280_E_DEV_NOT_FOUND;
	do
	{
		/*
		*	Make sure we have a BMP280 by checking the ID.
		*	The ID of the production version of the BMP280 is BMP280_CHIP_ID3.
		*/
		if (ReadReg8(BMP280_CHIP_ID_ADDR) == BMP280_CHIP_ID3)
		{
			// Reset the BMP280
			WriteReg8(BMP280_SOFT_RESET_ADDR, BMP280_SOFT_RESET_CMD);
			delay(2);
			// Get the compensation params
			ReadReg8(BMP280_DIG_T1_LSB_ADDR, BMP280_CALIB_DATA_SIZE, (uint8_t*)&mCParams);
			// Write the configuration
			WriteReg8(BMP280_CTRL_MEAS_ADDR, mCtrlMeas);
			WriteReg8(BMP280_CONFIG_ADDR, mConfig);
			//DumpCParams();
			result = BMP280_OK;
			break;
		}
		delay(10);
		retries--;
	} while (retries);
	return(result);
}

/********************************** ReadReg8 **********************************/
uint8_t BMP280SPI::ReadReg8(
	uint8_t	inRegAddr)
{
	BeginTransaction();
	SPI.transfer(inRegAddr);
	uint8_t	value = SPI.transfer(0);
	EndTransaction();
	return(value);
}

/********************************* WriteReg8 **********************************/
void BMP280SPI::WriteReg8(
	uint8_t	inRegAddr,
	uint8_t	inRegData)
{
	BeginTransaction();
	SPI.transfer(inRegAddr & 0x7F);
	SPI.transfer(inRegData);
	EndTransaction();
}

/********************************** ReadReg8 **********************************/
void BMP280SPI::ReadReg8(
	uint8_t		inRegAddr,
	uint8_t		inDataLength,
	uint8_t*	outRegData)
{
	BeginTransaction();
	SPI.transfer(inRegAddr);
	uint8_t*	endRegData = &outRegData[inDataLength];
	while (outRegData != endRegData)
	{
		*(outRegData++) = SPI.transfer(0);
	}
	EndTransaction();
}

/******************************* ReadUncompData *******************************/
void BMP280SPI::ReadUncompData(
	int32_t&	outUncompPres,
	int32_t&	outUncompTemp)
{
	BeginTransaction();
	SPI.transfer(BMP280_PRES_MSB_ADDR);
	outUncompPres = ReadUncompData();
	outUncompTemp = ReadUncompData();
	EndTransaction();
}

/******************************* ReadUncompData *******************************/
int32_t BMP280SPI::ReadUncompData(void)
{
	union
	{
		int32_t	data32;
		uint8_t	data8[4];
	} uncomp;
	uncomp.data8[3] = 0;
	uncomp.data8[2] = SPI.transfer(0);
	uncomp.data8[1] = SPI.transfer(0);
	uncomp.data8[0] = SPI.transfer(0);
	return(uncomp.data32 >> 4);
}


#if 0
/******************************* DumpCParams **********************************/
void BMP280SPI::DumpCParams(void)
{
	swSerial.print(F("dig_t1 = "));
	swSerial.println(mCParams.dig_t1, DEC);
	swSerial.print(F("dig_t2 = "));
	swSerial.println(mCParams.dig_t2, DEC);
	swSerial.print(F("dig_t3 = "));
	swSerial.println(mCParams.dig_t3, DEC);
	swSerial.print(F("dig_p1 = "));
	swSerial.println(mCParams.dig_p1, DEC);
	swSerial.print(F("dig_p2 = "));
	swSerial.println(mCParams.dig_p2, DEC);
	swSerial.print(F("dig_p3 = "));
	swSerial.println(mCParams.dig_p3, DEC);
	swSerial.print(F("dig_p4 = "));
	swSerial.println(mCParams.dig_p4, DEC);
	swSerial.print(F("dig_p5 = "));
	swSerial.println(mCParams.dig_p5, DEC);
	swSerial.print(F("dig_p6 = "));
	swSerial.println(mCParams.dig_p6, DEC);
	swSerial.print(F("dig_p7 = "));
	swSerial.println(mCParams.dig_p7, DEC);
	swSerial.print(F("dig_p8 = "));
	swSerial.println(mCParams.dig_p8, DEC);
	swSerial.print(F("dig_p9 = "));
	swSerial.println(mCParams.dig_p9, DEC);
}
#endif

/******************************** DoForcedRead ********************************/
void BMP280SPI::DoForcedRead(
	int32_t&	outTemp,
	uint32_t&	outPres)
{
	WriteReg8(BMP280_CTRL_MEAS_ADDR, mCtrlMeas | BMP280_FORCED_MODE);
#if 0
Removed because it may unsync the timing (time between transmits)
	/*
	*	Block/wait for measurement to finish
	*/
	uint32_t	timeout = millis() + 50;	// Max sample measurement time
	while((ReadReg8(BMP280_STATUS_ADDR) & (BMP280_STATUS_MEAS_MASK | BMP280_STATUS_IM_UPDATE_MASK)) != 0 &&
			timeout > millis())
	{
		delay(5);
	}
#endif	
	int32_t	uncompPres;
	int32_t	uncompTemp;
	ReadUncompData(uncompPres, uncompTemp);
	outTemp = UncompToCompTemp32(uncompTemp);
	outPres = UncompToCompPres32(uncompPres);
}

/***************************** UncompToCompTemp32 *****************************/
/*
*	Copyright (C) 2019 Bosch Sensortec GmbH
*
*	This code was copied and modified from the Bosch Sensortec bmp280.c source
*	bmp280_get_comp_temp_32bit().
*
*	This API is used to get the compensated temperature from
*	uncompensated temperature. This API uses 32 bit integers.
*/
int32_t BMP280SPI::UncompToCompTemp32(
	int32_t	inUncompTemp)
{
	int32_t var1 = ((((inUncompTemp / 8) - ((int32_t) mCParams.dig_t1 << 1))) *
				((int32_t) mCParams.dig_t2)) / 2048;
	int32_t var2 = (((((inUncompTemp / 16) - ((int32_t) mCParams.dig_t1)) *
			((inUncompTemp / 16) - ((int32_t) mCParams.dig_t1))) / 4096) *
				((int32_t) mCParams.dig_t3)) / 16384;
	mCParams.t_fine = var1 + var2;

    return((mCParams.t_fine * 5 + 128) / 256);
}

/***************************** UncompToCompPres32 *****************************/
/*
*	Copyright (C) 2019 Bosch Sensortec GmbH
*
*	This code was copied and modified from the Bosch Sensortec bmp280.c source
*	bmp280_get_comp_pres_32bit().
*
*	This API is used to get the compensated pressure from
*	uncompensated pressure. This API uses 32 bit integers.
*/
uint32_t BMP280SPI::UncompToCompPres32(
	int32_t	inUncompPres)
{
	int32_t var1 = (((int32_t) mCParams.t_fine) / 2) - (int32_t) 64000;
	int32_t var2 = (((var1 / 4) * (var1 / 4)) / 2048) * ((int32_t) mCParams.dig_p6);
	var2 = var2 + ((var1 * ((int32_t) mCParams.dig_p5)) * 2);
	var2 = (var2 / 4) + (((int32_t) mCParams.dig_p4) * 65536);
	var1 = (((mCParams.dig_p3 * (((var1 / 4) * (var1 / 4)) / 8192)) / 8) +
				((((int32_t) mCParams.dig_p2) * var1) / 2)) / 262144;
	var1 = ((((32768 + var1)) * ((int32_t) mCParams.dig_p1)) / 32768);
	uint32_t compPres = (((uint32_t) (((int32_t)1048576) - inUncompPres) - (var2 / 4096))) * 3125;

	/* Avoid exception caused by division with zero */
	if (var1 != 0)
	{
		/* Check for overflows against UINT32_MAX/2; if pres is left-shifted by 1 */
		if (compPres < 0x80000000)
		{
			compPres = (compPres << 1) / ((uint32_t) var1);
		} else
		{
			compPres = (compPres / (uint32_t) var1) * 2;
		}
		var1 = (((int32_t) mCParams.dig_p9) * ((int32_t) (((compPres / 8) *
					(compPres / 8)) / 8192))) / 4086;
		var2 = (((int32_t) (compPres / 4)) * ((int32_t) mCParams.dig_p8)) / 8192;
		compPres = (uint32_t) ((int32_t) compPres + ((var1 + var2 + mCParams.dig_p7) / 16));
	} else
	{
		compPres = 0;
	}
    return(compPres);
}

