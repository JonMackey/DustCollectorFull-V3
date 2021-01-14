/*
*	DCSensor.ino, Copyright Jonathan Mackey 2020
*=
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
*	This code uses a modified version of Jack Christensen's tinySPI Library.
*	Copyright 2018 Jack Christensen https://github.com/JChristensen/tinySPI
*
*	The tinySPI Library modification allows for 800KHz, 2MHz and 4MHz SPI speeds
*	when used with an 8MHz cpu clock.  The original code allowed for only a
*	single speed of approximately 666KHz with a 8MHz clock.
*/
#include <Arduino.h>
#include <EEPROM.h>
#include <avr/interrupt.h>
#include "tinySPI.h"
#include "DCGateSensor.h"
#include "CompileTime.h"
#include "DCSConfig.h"

// volatile is added to keep the optimizer from turning the variable into
// a constant.  Constants are generally compiled as immediate data wherever the
// constant is referenced.  Where as, as a volitile there will be 4 bytes of
// initialization data in the data initialization section that can be modified
// to change the kTimestamp value without having to recompile the sketch.
volatile uint32_t	kTimestamp = UNIX_TIMESTAMP;

DCGateSensor	gateSensor;

#ifdef HAS_SERIAL
SendOnlySoftwareSerial swSerial(DCSConfig::kRedRGBTxPin); // Tx
#define BAUD_RATE	19200
#endif

/********************************* setup **************************************/
void setup(void)
{    
#ifdef HAS_SERIAL
	swSerial.begin(BAUD_RATE);
	delay(10);
	swSerial.println(F("Starting..."));
#endif

	SPI.begin();
	gateSensor.begin();
}

/********************************** loop **************************************/
void loop(void)
{
	gateSensor.Update();
}
