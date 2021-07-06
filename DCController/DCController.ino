/*
*	DCController.ino, Copyright Jonathan Mackey 2020
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
*	This code uses Felix Rusu's RFM69 library.
*	Copyright Felix Rusu 2016, http://www.LowPowerLab.com/contact
*/
#include <avr/sleep.h>
#include <SPI.h>
#include <EEPROM.h>

#include "TFT_ST7789.h"
#include "DCConfig.h"
#include "DustCollectorUI.h"
#include "DustCollector.h"
#include <Wire.h>
#include "DS3231SN.h"
#include "ATmega644RTC.h"

#define BAUD_RATE	19200

TFT_ST7789	display(DCConfig::kDispDCPin, DCConfig::kDispResetPin, DCConfig::kDispCSPin, DCConfig::kBacklightPin, 240, 240);
DustCollectorUI	dustCollectorUI;
// Define "xFont" to satisfy the auto-generated code with the font files
// This implementation uses layout as a subclass of xFont
#define xFont dustCollectorUI
#include "MyriadPro-Regular_36_1b.h"
#include "MyriadPro-Regular_18.h"
#include "DC_Icons.h"

DustCollector		dustCollector;
DS3231SN			externalRTC;


/*********************************** setup ************************************/
void setup(void)
{
	Serial.begin(BAUD_RATE);

	SPI.begin();
	Wire.begin();
	
	externalRTC.begin();
	ATmega644RTC::RTCInit(0, &externalRTC);
	UnixTime::SetTimeFromExternalRTC();
	{
		uint8_t	flags;
		EEPROM.get(DCConfig::kFlagsAddr, flags);
		UnixTime::SetFormat24Hour((flags & 1) == 0);	// Default is 12 hour.
	}
	UnixTime::ResetSleepTime();	// Display sleep time


	// Pull-up all unused pins
	pinMode(DCConfig::kUnusedPinB0, INPUT_PULLUP);
	pinMode(DCConfig::kUnusedPinB1, INPUT_PULLUP);
	pinMode(DCConfig::kUnusedPinB4, INPUT_PULLUP);
	
	display.begin(2, LOW); // Init TFT
	dustCollector.begin();
	display.Fill();
	dustCollectorUI.begin(&dustCollector, &display,
							&MyriadPro_Regular_36_1b::font,
								&MyriadPro_Regular_18::font,
									&DC_Icons::font);	
}

/************************************ loop ************************************/
void loop(void)
{
	/*
	*	If the dust collector isn't in a time critical state THEN
	*	allow the UI time to update.
	*/
	if (dustCollector.Update())
	{
		dustCollectorUI.Update();	
	}

#if 0
	if (Serial.available())
	{
		dustCollector.DoSerial();
	}
#endif
	
}


