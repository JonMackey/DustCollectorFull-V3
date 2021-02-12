/*
*	DustCollector.cpp, Copyright Jonathan Mackey 2020
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
*
*/
#include <Arduino.h>
#include "DustCollector.h"
#include "BMP280SPI.h"
#include "UnixTimeEditor.h"
#include <EEPROM.h>
#include "DCConfig.h"
#include "DCMessages.h"
/*
	There were issues with the 16 MHz MCU consuming the CAN messages too slowly.
	This resulted in receive overflow errors.  This happened when a request for
	the gate state was broadcast to all of the gate sensors.  The sensors
	responded all at once and the MCU couldn't process the responses fast
	enough. Lowering the baud rate to something really slow didn't help. The
	current implementation uses a fifo message queue that instead of
	broadcasting the request to all sensors all at once, each sensor is now
	queried individually, with a delay between each message sent to limit the
	traffic on the bus.  The message queue is now the default mechanism for
	sending messages when one or more sensors are expected to respond.
*/
//					The timing config is written CNF3, CNF2, CNF1
const uint8_t	DustCollector::kTimingConfig[] = {0x07, 0xAC, 0x04}; // 40kHz CAN baud rate
volatile bool	sMCP2515IntTriggered;
const uint32_t	kControllerID = 0x20000;// b 0010 0000 0000 0000 0000
const uint32_t	kBroadcastID = 0x20001;
const uint32_t	kBaseIDMask = 0x3FFE0;	// b 0011 1111 1111 1110 0000
const uint32_t	kGateIndexMask = 0x1F;	// b				   1 1111
const uint32_t	kPressureUpdatePeriod = 1500;	// in milliseconds
const uint32_t	kMotorSenseePeriod = 500;		// in milliseconds
const uint32_t	kCANBusyPeriod = 200;			// in milliseconds

/******************************* DustCollector ********************************/
DustCollector::DustCollector(void)
  : MCP2515(DCConfig::kCANCSPin, DCConfig::kCANResetPin),
	mBMP280Ambient(DCConfig::kBMP1CSPin), mBMP280Duct(DCConfig::kBMP0CSPin),
	mRadio(DCConfig::kRadioNSSPin, DCConfig::kRadioIRQPin),
	mPressureUpdatePeriod(kPressureUpdatePeriod), mMotorSensePeriod(kMotorSenseePeriod),
	mFlashingGates(0), mCANBusyPeriod(kCANBusyPeriod), mDeltaAveragesLoaded(false),
	mDeltaAverageIndex(0)
{
}

/*********************************** begin ************************************/
void DustCollector::begin(void)
{
	/*
	*	MCP2515 setup
	*/
	{
		MCP2515::begin(kTimingConfig);
		/*
		*	mGateBaseID is the base value of every valid gate ID.  Any gate ID that
		*	doesn't have this ID as the most significant 13 bits is considered
		*	invalid and will automatically be assigned a new ID containing the 13
		*	bit base ID.
		*
		*	gate ID = mGateBaseID + the gate's SGateLink physical index.
		*
		*	The base ID makes it possible to reset all of the gate IDs by changing
		*	the base ID.
		*/
		EEPROM.get(DCConfig::kGateBaseIDAddr, mGateBaseID);
		/*
		*	A CAN extended ID is 18 bits.  Mask the 13 bit base ID, reserving the 5
		*	least significant bits as the gate index.
		*/
		mGateBaseID &= kBaseIDMask;	
		Serial.print(F("Gate base ID = 0x"));
		Serial.println(mGateBaseID, HEX);
		
		attachInterrupt(digitalPinToInterrupt(DCConfig::kCANIntPin), ExtIntReq2, FALLING);
		// The above does the following, in addition to saving the function address.
		// EICRA = (EICRA & ~(_BV(ISC20))) | _BV(ISC21);	// Bits 5:4, ISC2n 0b10 = FALLING
		// EIMSK |= _BV(INT2); // Enable INT2
	}

	/*
	*	BMP280 pressure sensor setup
	*/
	{
		/*
		*	Even though it's not used, turning the temperature oversampling off
		*	causes a larger delta between the pressure readings.  I don't know why.
		*/
		mBMP280Ambient.SetOversampling(1, 3);
		mBMP280Duct.SetOversampling(1, 3);

		/*
		*	The filter coefficient should be left off (default).  DustCollector provides
		*	its own pressure averaging.
		*/
		//mBMP280Ambient.SetFilterCoefficient(3);
		//mBMP280Duct.SetFilterCoefficient(3);

		int8_t status = mBMP280Ambient.begin();
		Serial.print(F("BMP280Ambient status = "));
		Serial.println(status);
		status = mBMP280Duct.begin();
		Serial.print(F("BMP280Duct status = "));
		Serial.println(status);
		mPressureUpdatePeriod.Start();	
	}
	
	//mGates.RemoveAllGates();
	mGates.begin();
	//mGateSets.RemoveAllGateSets();
	mGateSets.begin();
	
	mRadio.initialize(RF69_433MHZ, DCConfig::kNodeID, DCConfig::kNetworkID);
	
	StopFlasher();
	mFaultAcknowledged = true;
	StopDustBinMotor();	// Stop before setting pinMode
	pinMode(DCConfig::kFlasherControlPin, OUTPUT);
	pinMode(DCConfig::kMotorControlPin, OUTPUT);
	
	/*
	*	Motor trigger threshold setup
	*/
	{
		EEPROM.get(DCConfig::kMotorTriggerThresholdAddr, mTriggerThreshold);
		/*
		*	Sanity check the threshold value.
		*	This value will be out of range when the EEPROM is erased and/or never
		*	been accessed.
		*/
		if (mTriggerThreshold > DCConfig::kThresholdUpperLimit ||
			mTriggerThreshold < DCConfig::kThresholdLowerLimit)
		{
			mTriggerThreshold = DCConfig::kDefaultTriggerThreshold;
		}
	}
	mStatus = eNotRunning;

#ifdef DEBUG_FRAMES
	mFrameIndex = 0;
#endif
	mCANMessageQueueHead = 0;
	mCANMessageQueueTail = 0;
	RequestAllGateStates();
	mCANBusyPeriod.Start();
}

/********************************** DoConfig **********************************/
/*
*	Configure the MCP2515
*/
void DustCollector::DoConfig(void)
{
	/*
	*	Configure the message acceptance filters and masks.
	*	
	*	The standard ID bits are used to pass commands and the extended ID bits
	*	are the sensor ID. The standard ID of the mask and filters are setup to
	*	allow any command through. The extended ID of the mask and filters are
	*	setup to allow only commands targeting this sensor through.
	*	
	*	Either receive buffer can be targeted by any command.
	*	The interrupt pin is configured to trigger for errors and commands
	*	received.
	*/
	{
		CANFrame	filter(0, kControllerID);
		// Set the filter to match just the controller ID
		const uint8_t*	filterRawFrame = filter.GetRawFrame();
		WriteReg(eRXF0Reg, 4, filterRawFrame);
		// Set the rest of the filters to do nothing
		filter.SetStandardID(0x7FF);
		WriteReg(eRXF1Reg, 4, filterRawFrame);
		WriteReg(eRXF2Reg, 4, filterRawFrame);
		WriteReg(eRXF3Reg, 4, filterRawFrame);
		WriteReg(eRXF4Reg, 4, filterRawFrame);
		WriteReg(eRXF5Reg, 4, filterRawFrame);
	}
	{
		// Set the mask to allow any command and force the entire controller ID
		// to match before a frame is accepted.
		CANFrame	mask(0, (uint32_t)0x3FFFF);
		const uint8_t*	maskRawFrame = mask.GetRawFrame();
		WriteReg(eRXM0Reg, 4, maskRawFrame);
		// Set the other mask to do nothing
		mask.SetStandardID(0x7FF);
		WriteReg(eRXM1Reg, 4, maskRawFrame);
	}
	// Generate an interrupt when the receive buffer is full and if an
	// error occurs.
	WriteReg(eCANINTEReg, _BV(eRX0IE) | _BV(eRX1IE) | _BV(eERRIE));
	// Allow receive buffer 1 to be used if buffer 0 is full
	ModifyReg(eRXB0CTRLReg, _BV(eBUKT), _BV(eBUKT));
}

/******************************* RemoveAllGates *******************************/
void DustCollector::RemoveAllGates(void)
{
	// Increment the bits 15:5 of the 18 bit base ID by adding "1" to bit 5.
	// Mask off any overflow with 0x3FFE0 when 0x3FFE0 increments to 0x40000.
	mGateBaseID = (mGateBaseID + 0x20) & kBaseIDMask;
	if (mGateBaseID == kControllerID)
	{
		mGateBaseID = kControllerID + 0x20;
	}
	EEPROM.put(DCConfig::kGateBaseIDAddr, mGateBaseID);
	Serial.print(F("Setting Gate base ID to = 0x"));
	Serial.println(mGateBaseID, HEX);
	//uint32_t	actualBaseID;
	//EEPROM.get(DCConfig::kGateBaseIDAddr, actualBaseID);
	//Serial.print(F("Actual base ID = 0x"));
	//Serial.println(actualBaseID, HEX);
	
	mGates.RemoveAllGates();
	mGateSets.RemoveAllGateSets();
	ResetAllGatesToFactoryID();
	RequestAllGateStates();
}

/***************************** RemoveAllGateSets ******************************/
void DustCollector::RemoveAllGateSets(void)
{
	mGateSets.RemoveAllGateSets();
	// ?? Update ??
}

/******************************** SaveCleanSet ********************************/
bool DustCollector::SaveCleanSet(void)
{
	bool	success = mDCIsRunning;
	if (success)
	{
		success = mGateSets.SaveCleanSet(mOpenGates, DeltaAverage());
	}
	return(success);
}

/******************************** SaveDirtySet ********************************/
bool DustCollector::SaveDirtySet(void)
{
	bool	success = mDCIsRunning;
	if (success)
	{
		success = mGateSets.SaveDirtySet(mOpenGates, DeltaAverage());
		mStatus = eFilterFull;	// No need to report the filter being full.
	}
	return(success);
}

/********************************* RemoveGate *********************************/
/*
*	Removes a gate and any gate sets that refer to it.  This will fail if the
*	gate is not in an error/unresponsive state.
*/
void DustCollector::RemoveGate(
	uint16_t	inRecIndex)
{
	if (GetGateState(inRecIndex) == eErrorState)
	{
		uint32_t	gateMask = ((uint32_t)1 << (inRecIndex -1));
		mOpenGates &= ~gateMask;	// Should already be cleared.
		mGateSets.RemoveGateSetsContainingGate(gateMask);
		mGates.GoToGate(inRecIndex);
		mGates.RemoveCurrent();
		mGateSets.GateStateChanged(mOpenGates);
	}
}

/*************************** UserAcknowledgedFault ****************************/
void DustCollector::UserAcknowledgedFault(void)
{
	if (!mFaultAcknowledged)
	{
		mFaultAcknowledged = true;
		StopFlasher();
	}
}

/******************************** StartFlasher ********************************/
void DustCollector::StartFlasher(void)
{
	digitalWrite(DCConfig::kFlasherControlPin, HIGH);
}

/******************************** StopFlasher *********************************/
void DustCollector::StopFlasher(void)
{
	digitalWrite(DCConfig::kFlasherControlPin, LOW);
}

/***************************** StartDustBinMotor ******************************/
void DustCollector::StartDustBinMotor(void)
{
	digitalWrite(DCConfig::kMotorControlPin, HIGH);

	mSampleCount = 0;
	mSampleAccumulator = 0;
	mRingBufIndex = 0;
	mBinMotorAverage = 0;
	// Give the motor 2 seconds to start before taking any readings.
	mMotorSensePeriod.Set(2000);
	mMotorSensePeriod.Start();
}

/****************************** StopDustBinMotor ******************************/
void DustCollector::StopDustBinMotor(void)
{
	// Set the sense period to 0.
	// This will stop sensing the motor.
	mMotorSensePeriod.Set(0);
	mBinMotorAverage = 0;
	digitalWrite(DCConfig::kMotorControlPin, LOW);
}

/******************************* ToggleBinMotor *******************************/
void DustCollector::ToggleBinMotor(void)
{
	if (mMotorSensePeriod.Get())
	{
		StopDustBinMotor();
		SendAudioAlertMessage(DCConfig::kFullMessage);
	} else
	{
		StartDustBinMotor();
	}
}

/**************************** SetTriggerThreshold *****************************/
void DustCollector::SetTriggerThreshold(
	uint8_t	inTriggerThreshold)
{
	mTriggerThreshold = inTriggerThreshold;
}

/**************************** SaveTriggerThreshold ****************************/
void DustCollector::SaveTriggerThreshold(void)
{
	EEPROM.put(DCConfig::kMotorTriggerThresholdAddr, mTriggerThreshold);
}

/*************************** SendAudioAlertMessage ****************************/
void DustCollector::SendAudioAlertMessage(
	uint32_t	inMessage)
{
	mRadio.sendWithRetry(DCConfig::kAudioAlertNodeID, &inMessage, 4);
	//mRadio.sendWithRetry(DCConfig::kAudioAlertNodeID, "DCF", 4);
	mRadio.sleep();
}

/*********************************** Update ***********************************/
/*
*	Called from loop() just after the layout has updated.
*/
bool DustCollector::Update(void)
{
	bool notBusy = CheckGates();
	// Any activity on the CAN bus takes precidence over checking the filter
	// and dust bin motor.
	if (notBusy)
	{
		CheckFilter();
		CheckDustBinMotor();
	}
	return(notBusy);
}

/******************************** CheckFilter *********************************/
void DustCollector::CheckFilter(void)
{
	if (mPressureUpdatePeriod.Passed())
	{
		int32_t	temp;
		mBMP280Ambient.DoForcedRead(temp, mAmbientPressure);
		mBMP280Duct.DoForcedRead(temp, mDuctPressure);
		mPressureUpdatePeriod.Start();
	
		/*
		*	The mDeltaSum is the sum of the last 4 deltas.
		*
		*	The delta average is updated every 1.5 seconds.  This is the average
		*	delta between the ambient and duct pressure readings when the dust
		*	collector is off.  Four averages are maintained.  Each is an average
		*	of the delta sum measured 1.5 seconds apart.
		*
		*	The storing of averages stop once the dust collector starts. To
		*	detect when the dust collector starts the current mDeltaSum average
		*	must increase by 25Pa over the oldest stored average.
		*
		*	Note that the adjusted delta average is the delta average minus the
		*	baseline (off state) average between the two pressure sensors. The
		*	adjusted value is used when displaying the value to the user and
		*	when determining when the collector is running.  For comparisons,
		*	like determining when the filter is loaded, the simple delta average
		*	is used because there is no need to subtract the baseline provided
		*	the both readings being compared are based on the simple delta
		*	average.  If both readings are +10Pa, who cares?  It's only when you
		*	need to display the value that the baseline needs to be subtracted.
		*
		*	Whether to use the adjusted average may become an issue if the
		*	baseline changes dramatically over time.
		*/
		// The expected delta is in the range of an signed 16 bit integer.
		int32_t	thisDelta = abs(mDuctPressure - mAmbientPressure);
		/*
		*	When the pressure sensors start up, the first few deltas can be very
		*	large.  At about the 4th reading the delta value becomes rational
		*	for the expected dust collector off state. (a delta less than 200Pa)
		*/
		if (thisDelta < 1500)
		{
			/*
			*	Member variables:
			*	- mDelta[4] is an array containing the last 4 delta values.
			*	- mDeltaSum is the sum of the 4 delta values in mDelta[].  The
			*	mDeltaSum is only valid after mDelta contains all 4 values.
			*	- mDeltaSumLoaded is a flag indicating that the mDelta array
			*	contains 4 values.
			*	- mDeltaAverage[4] is an array containing the last 4 delta
			*	averages over a timespan of 6 seconds.
			*	- mDeltaAveragesLoaded is a flag indicating that the
			*	mDeltaAverage array contains 4 values.  The mDeltaAverage array
			*	values aren't used to determine if the dust collector is running
			*	till this is true.
			*/
			mDeltaSum = mDeltaSum - mDelta[mDeltaIndex & 3] + thisDelta;
			mDelta[mDeltaIndex & 3] = thisDelta;
			mDeltaIndex++;
			// deltaAverage is the average of the 4 values in mDelta.  This is
			// calculated as mDeltaSum/4
			int32_t	deltaAverage = DeltaAverage();
	
		#ifdef DEBUG_DELTAS
			if (mDebugAverageIndex < 511)
			{
				mDeltaAverageDebug[mDebugAverageIndex] = deltaAverage;
				mDebugAverageIndex++;
				mDeltaAverageDebug[mDebugAverageIndex] = 0;
			}
		#endif
	
			if (mDeltaAveragesLoaded)
			{
				/*
				*	Calculate the adjusted average delta using the oldest delta
				*	average.
				*	(mDeltaAverageIndex & 3) = oldest average
				*/
				int32_t	adjustedDeltaAverage = deltaAverage  -
										mDeltaAverage[mDeltaAverageIndex & 3]; //AdjustedDeltaAverage();
				bool isRunning = adjustedDeltaAverage > 25;	// 100 = 1hPa
				if (isRunning != mDCIsRunning)
				{
					mDCIsRunning = isRunning;
					/*
					*	If the dust collector just started THEN
					*	start the dust bin motor.
					*/
					if (isRunning)
					{
						mStatus = eRunning;
						StartDustBinMotor();
					/*
					*	Else the dust collector just stopped.
					*	Reset the delta averages.
					*	It takes about 15 seconds for everything to reload.
					*/
					} else
					{
						mStatus = eNotRunning;
						mDeltaAveragesLoaded = false;
						mDeltaSumLoaded = false;
						mDeltaIndex = 0;
						mDeltaAverageIndex = 0;
						mFaultAcknowledged = true;
						StopDustBinMotor();
						StopFlasher();
					}
				}
			} else if (mDeltaAverageIndex > 4)
			{
				mDeltaAveragesLoaded = true;
			}
			/*
			*	If mDeltaSum contains 4 deltas... (it generally does)
			*/ 
			if (mDeltaSumLoaded)
			{
				/*
				*	If the dust collector is running THEN
				*	see if the filter is loaded.
				*/
				if (mStatus == eRunning)
				{
					/*
					*	If the average is greater than or equal to the dirty pressure THEN
					*	set the status to filter full, start flasher, send message.
					*/
					if (deltaAverage >= mGateSets.CurrentDirtyPressure())
					{
						mStatus = eFilterFull;
						mFaultAcknowledged = false;
						StartFlasher();
						SendAudioAlertMessage(DCConfig::kFilterLoadedMessage);
					}
				} else if (!mDCIsRunning)
				{
					// Set the oldest average to the newest.
					mDeltaAverage[mDeltaAverageIndex & 3] = (int16_t)deltaAverage;
					mDeltaAverageIndex++;	// The next average is now the oldest
				}
			} else if (mDeltaIndex > 4)
			{
				mDeltaSumLoaded = true;
			}
		}
	}
}

/***************************** CheckDustBinMotor ******************************/
void DustCollector::CheckDustBinMotor(void)
{
	if (mMotorSensePeriod.Passed())
	{
		uint16_t	reading = analogRead(DCConfig::kMotorSensePin);
		mSampleAccumulator += reading;				// Add the newest reading
		uint16_t	oldestReading = mRingBuf[mRingBufIndex];
		mRingBuf[mRingBufIndex] = reading;			// Save the newest reading
		mRingBufIndex++;
		if (mRingBufIndex >= SAMPLE_SIZE)
		{
			mRingBufIndex = 0;
		}
		/*
		*	If the ring buffer is full THEN
		*	use the paddle motor average voltage drop to determine if the motor
		*	is overloaded due to shavings blocking the paddle path.
		*/
		if (mSampleCount >= SAMPLE_SIZE)
		{
			mSampleAccumulator -= oldestReading;	// Remove the oldest reading
			uint16_t average = mSampleAccumulator/SAMPLE_SIZE;
			mBinMotorAverage = average;
			/*
			*	If the paddle motor doesn't have significant resistance THEN
			*	setup the sense period to check the motor in a few ms.
			*/
			if (average < mTriggerThreshold)
			{
				mMotorSensePeriod.Start();
			/*
			*	Else the bin is considered full.
			*/
			} else
			{
				mStatus = eBinFull;
				mFaultAcknowledged = false;
				StopDustBinMotor();
				StartFlasher();
				SendAudioAlertMessage(DCConfig::kFullMessage);
			}
		} else
		{
			if (!mSampleCount)
			{
				mMotorSensePeriod.Set(250);
			}
			mSampleCount++;
			mMotorSensePeriod.Start();
		}
	#ifdef DEBUG_MOTOR
		{
			Serial.print('A');
			Serial.println(average);
		}
	#endif
	}
}

/******************************** SetGateState ********************************/
void DustCollector::SetGateState(
	uint16_t	inRecIndex,
	bool		inGateIsOpen)
{
	bool stateChanged = false;
	if (inRecIndex)
	{
		uint32_t	gateMask = ((uint32_t)1 << (inRecIndex -1));
		uint32_t	openGates = mOpenGates;
		if (inGateIsOpen)
		{
			openGates |= gateMask;
		} else
		{
			openGates &= ~gateMask;
		}
		mUnresponsiveGates &= ~gateMask;
		if (openGates != mOpenGates)
		{
			mOpenGates = openGates;
			mGateSets.GateStateChanged(openGates);
		}
	}
}

/******************************** GetGateState ********************************/
uint8_t DustCollector::GetGateState(
	uint16_t	inRecIndex) const
{
	uint8_t	gateState = eErrorState;
	if (inRecIndex)
	{
		uint32_t	gateMask = ((uint32_t)1 << (inRecIndex -1));
		if ((mUnresponsiveGates & gateMask) == 0)
		{
			gateState = (mOpenGates & gateMask) ? 1:0;
		}
	}
	return(gateState);
}

/********************************* CheckGates *********************************/
/*
*	This is called every time through the main sketch's loop.
*/
bool DustCollector::CheckGates(void)
{
	/*
	*	If the INT pin on the MCP2515 is low THEN
	*	see why.
	*/
	if (sMCP2515IntTriggered)
	{
		uint8_t	canICODStat = ReadReg(eCANSTATReg) & eICODMask;
		uint32_t	timeout = millis() + 200;
		CANFrame	canFrame;
		while (canICODStat && timeout > millis())
		{
			switch (canICODStat)
			{
				case eRxB0Interrupt:
				case eRxB1Interrupt:
				{
					if (ReceiveFrame(canFrame))
					{
					#ifdef DEBUG_FRAMES
						AppendFrame(canFrame);
					#endif
						HandleReceivedFrame(canFrame);
					}
					break;
				}
				case eErrorInterrupt:
				{
				#ifdef DEBUG_FRAMES
				    canFrame.SetStandardID(0x200);	// Fake error frame
				    canFrame.SetExtendedID(0);
				    canFrame.GetRawFrame()[5] = ReadReg(eEFLGReg);
				    canFrame.GetRawFrame()[6] = ReadReg(eCANINTEReg);
				    canFrame.GetRawFrame()[7] = ReadReg(eCANINTFReg);
				    AppendFrame(canFrame);
				#endif
				    ModifyReg(eCANINTFReg, _BV(eERRIF), 0);	// punt
					break;
				}
			}
			// The controller returns the status related to an available frame
			// or error in order of priority so keep reading till the status
			// goes to zero.
			canICODStat = ReadReg(eCANSTATReg) & eICODMask;
		}
		if (canICODStat)
		{
			Serial.print(F("Timeout reading frame, canICODStat = 0x"));
			Serial.println(canICODStat, HEX);
		}
		mCANBusyPeriod.Start();
		sMCP2515IntTriggered = false;
	} else if (mCANBusyPeriod.Passed())
	{
#ifdef DEBUG_FRAMES
		DumpFrames();
#endif
		if (mCANMessageQueueHead != mCANMessageQueueTail)
		{
			SendNextQueuedMessage();
			mCANBusyPeriod.Start();
		}
	}

	return(mCANBusyPeriod.Passed());
}

/*************************** SendNextQueuedMessage ****************************/
void DustCollector::SendNextQueuedMessage(void)
{
	SCANMessageQueueElement&	queueElement = mCANMessageQueue[mCANMessageQueueHead];
	switch(queueElement.command)
	{
		case DCController::eSetID:
		{
			uint16_t	gateIndex = mGates.AddNew();
			if (gateIndex)
			{
				uint32_t	newID = mGateBaseID + gateIndex - 1;
				CANFrame	canFrame((uint16_t)DCController::eSetID, (uint32_t)queueElement.targetID, newID);
				if (SendFrame(canFrame))
				{
				#ifdef DEBUG_FRAMES
					AppendFrame(canFrame);
				#endif
					// Reuse this element to verify the registered gate sensor.
					queueElement.command = DCController::eRequestGateState;
					queueElement.targetID = newID;
				} else
				{
				#ifdef DEBUG_FRAMES
					canFrame.SetStandardID(0x201);	// Send set ID frame failed
					AppendFrame(canFrame);
				#endif
					mGates.RemoveCurrent();
					mCANMessageQueueHead = (mCANMessageQueueHead + 1) % CAN_QUEUE_SIZE;
				}
			}
			break;
		}
		// Internal command used to replace an unresponsive gate sensor.  The
		// actual command sent it eSetID.  mUnregisteredGateID is the gate
		// sensor to replace the unresponsive gate.
		case DCController::eReplaceID:
		{
			CANFrame	canFrame((uint16_t)DCController::eSetID, mUnregisteredGateID, (uint32_t)queueElement.targetID);
			if (SendFrame(canFrame))
			{
				mUnregisteredGateID = 0;
			#ifdef DEBUG_FRAMES
				AppendFrame(canFrame);
			#endif
				// Reuse this element to verify the replaced gate sensor.
				queueElement.command = DCController::eRequestGateState;
				// queueElement.targetID is already setup
			} else
			{
			#ifdef DEBUG_FRAMES
				canFrame.SetStandardID(0x201);	// Send set ID frame failed
				AppendFrame(canFrame);
			#endif
				mCANMessageQueueHead = (mCANMessageQueueHead + 1) % CAN_QUEUE_SIZE;
			}
			break;
		}
		/*
		*	This is an internal command used to mark the end of the 
		*	RequestAllGateStates set of queued commands to see if any gates
		*	aren't responding.
		*/
		case DCController::eCheckGateStateResponses:
			mCANMessageQueueHead = (mCANMessageQueueHead + 1) % CAN_QUEUE_SIZE;
			mGateCheckDone = true;
			break;
		default:
		{
			CANFrame	canFrame((uint16_t)queueElement.command, (uint32_t)queueElement.targetID);
		#ifdef DEBUG_FRAMES
			AppendFrame(canFrame);
		#endif
			SendFrame(canFrame);
			mCANMessageQueueHead = (mCANMessageQueueHead + 1) % CAN_QUEUE_SIZE;
			break;
		}
	}
}


/************************** RegisterUnregisteredGate **************************/
/*
*	This routine either replaces an existing gate sensor that isn't responding
*	or adds the unregistered gate sensor as a new sensor.
*/
void DustCollector::RegisterUnregisteredGate(
	uint16_t	inReplaceExistingIndex)
{
	if (inReplaceExistingIndex)
	{
		uint32_t	targetID = mGateBaseID + inReplaceExistingIndex - 1;
		QueueCANMessage(targetID, DCController::eReplaceID);
	} else
	{
		QueueCANMessage(mUnregisteredGateID, DCController::eSetID);
		mUnregisteredGateID = 0;
	}
}

/**************************** NextUnresponsiveGate ****************************/
uint8_t DustCollector::NextUnresponsiveGate(
	uint8_t	inGateIndex,
	bool	inForward) const
{
	if (mUnresponsiveGates)
	{
		uint8_t	gateIndex = inGateIndex;
		
		if (inForward)
		{
			for (uint32_t unresponsiveGates = mUnresponsiveGates >> gateIndex;
					 unresponsiveGates; unresponsiveGates >>= 1)
			{
				gateIndex++;
				if ((unresponsiveGates & 1) == 0)
				{
					continue;
				}
				return(gateIndex);
			}
		} else if (gateIndex < 32)
		{
			for (uint32_t unresponsiveGates = mUnresponsiveGates << (33 - gateIndex);
					 unresponsiveGates; unresponsiveGates <<= 1)
			{
				gateIndex--;
				if ((unresponsiveGates & 1) == 0)
				{
					continue;
				}
				return(gateIndex);
			}
		}
	}
	return(0);
}

/***************************** ToggleGateFlasher ******************************/
void DustCollector::ToggleGateFlasher(
	uint16_t	inGateIndex)
{
	if (inGateIndex)
	{
		uint32_t	gateMask = ((uint32_t)1 << (inGateIndex -1));
		bool	flashing = mFlashingGates & gateMask;
		uint32_t	gateID = mGateBaseID + inGateIndex - 1;
		CANFrame	canFrame(flashing ? DCController::eStopFlash :
										DCController::eFlash, gateID);
		if (flashing)
		{
			mFlashingGates &= ~gateMask;
		} else
		{
			mFlashingGates |= gateMask;
		}
	#ifdef DEBUG_FRAMES
		AppendFrame(canFrame);
	#endif
		SendFrame(canFrame);
	}
}

/************************** StopAllFlashingGateLEDs ***************************/
/*
*	Turns off all flashing gate LEDs turned on by ToggleGateFlasher()
*/
void DustCollector::StopAllFlashingGateLEDs(void)
{
	return;
	CANFrame	canFrame(DCController::eStopFlash, kBroadcastID);
#ifdef DEBUG_FRAMES
	AppendFrame(canFrame);
#endif
	SendFrame(canFrame);
	mFlashingGates = 0;
}

/**************************** RequestAllGateStates ****************************/
/*
*	Requests all registered gate states.
*	Open and close a gate to register a new gate (or replace an existing gate
*	sensor that isn't responding.)
*/
void DustCollector::RequestAllGateStates(void)
{
	uint32_t	gatesMask = mGates.GatesMask();
	mUnresponsiveGates = gatesMask;
	mUnregisteredGateID = 0;
	mGateCheckDone = false;
	uint16_t	savedCurrent = mGates.GetCurrentIndex();
	if (gatesMask)
	{
		uint8_t	gateIndex = 1;
		for (; gatesMask; gatesMask >>= 1, gateIndex++)
		{
			if (gatesMask & 1)
			{
				RequestGateState(gateIndex);
			}
		}
		// Add an internal command to flag when to check to see if all of the
		// gates have responded.  eCheckGateStateResponses is never sent on the bus.
		QueueCANMessage(0, DCController::eCheckGateStateResponses);
	}
}

/************************** ResetAllGatesToFactoryID **************************/
void DustCollector::ResetAllGatesToFactoryID(void)
{
	CANFrame	canFrame(DCController::eSetFactoryID, kBroadcastID);
#ifdef DEBUG_FRAMES
	AppendFrame(canFrame);
#endif
	SendFrame(canFrame);
}

/****************************** RequestGateState ******************************/
void DustCollector::RequestGateState(
	uint16_t	inGateIndex)
{
	if (inGateIndex)
	{
		uint32_t	gateID = mGateBaseID + inGateIndex - 1;
		QueueCANMessage(gateID, DCController::eRequestGateState);
	}
}

/**************************** HandleReceivedFrame *****************************/
void DustCollector::HandleReceivedFrame(
	CANFrame&	inCANFrame)
{
	uint16_t	command = inCANFrame.GetStandardID();
	switch (command)
	{
		case DCSensor::eGateIsOpen:
		case DCSensor::eGateIsClosed:
			uint32_t	gateID = *(const uint32_t*)inCANFrame.GetData();
			uint16_t	gateIndex = (gateID & kGateIndexMask) + 1;
			
			/*
			*	If gateID is invalid OR unregistered...
			*/
			if ((gateID & kBaseIDMask) != mGateBaseID ||
				 !mGates.IsValidIndex(gateIndex))
			{
				/*
				*	If there are missing gates THEN
				*	A gate sensor may have been replaced.  See if the
				*	unregistered gate should replace one of the missing gates.  
				*/
				if (mUnresponsiveGates)
				{
					// Setting mUnregisteredGateID will cause the UI to change
					// modes to determine how to handle the unregistered gate.
					mUnregisteredGateID = gateID;
				/*
				*	Else, since there are no missing gates, register this gate
				*	as a new gate.
				*/
				} else
				{
					QueueCANMessage(gateID, DCController::eSetID);
				}
			} else
			{
				SetGateState(gateIndex, command == DCSensor::eGateIsOpen);
				mGates.GoToGate(gateIndex);
			}
			break;
	}
}

/****************************** QueueCANMessage *******************************/
void DustCollector::QueueCANMessage(
	uint32_t	inID,
	uint16_t	inCommand)
{
	uint8_t	nextTail = (mCANMessageQueueTail + 1) % CAN_QUEUE_SIZE;
	if (nextTail != mCANMessageQueueHead)
	{
		/*
		*	When the tail == head, the queue is empty.
		*	When the ((tail+1) % CAN_QUEUE_SIZE) == head, the queue is full.
		*/
		mCANMessageQueue[mCANMessageQueueTail].targetID = inID;
		mCANMessageQueue[mCANMessageQueueTail].command = inCommand;
		mCANMessageQueueTail = nextTail;
	} else
	{
		// Message not queued/sent
		Serial.print(F("CAN queue overflow"));
	}
}

/************************ External Interrupt Request 2 ************************/
/*
*
*	Sets a flag to show that the CAN interrupt line is active (low true.)
*/
void DustCollector::ExtIntReq2(void)
{
	sMCP2515IntTriggered = true;
}


#ifdef DEBUG_DELTAS
#include "SdFat.h"
#include "sdios.h"
const char kDebugFilename[] = "Debug.txt";
bool DustCollector::SaveDebug(void)
{
	mDebugAverageIndex = 999;	// Stop recording
	bool	success = false;
	{
		SdFat sd;
		bool	success = sd.begin(DCConfig::kSDSelectPin);
		if (success)
		{
			SdFile::dateTimeCallback(UnixTime::SDFatDateTimeCB);
			SdFile file;
			success = file.open(kDebugFilename, O_WRONLY | O_TRUNC | O_CREAT);
			if (success)
			{
				int16_t	deltaAverage;
				for (uint16_t i = 0; (deltaAverage = mDeltaAverageDebug[i]) != 0; i++)
				{
					file.print(deltaAverage);
					file.write(',');
					if (i & 0xF)continue;
					file.write('\n');
				}
				file.write('\n');
				file.close();
			}
		} else
		{
			sd.initErrorHalt();
		}
	}
	return(success);
}

#endif	// DEBUG_DELTAS

#if 0
void DustCollector::DoSerial(void)
{
	bool success = true;
	switch (Serial.read())
	{
		case 'D':
			if (mGates.GetCount())
			{
				mGates.GoToNthGate(0);
				do
				{
					Serial.print('[');
					Serial.print(mGates.GetCurrentIndex());
					Serial.print(F("] \""));
					Serial.print(mGates.GetCurrent().name);
					Serial.println('\"');
				} while (mGates.Next(false));
				Serial.println();
			} else
			{
				Serial.println(F("No Gates"));
			}
			break;
		case 'G':
			RequestGateState(mGates.GetCurrentIndex());
			break;
		case 'L':
			success = mGates.LoadFromSD();
			Serial.println(success ? F("Load OK") : F("Load Failed"));
			break;
		case 'S':
			success = mGates.SaveToSD();
			Serial.println(success ? F("Saved OK") : F("Save Failed"));
			break;
		case 'R':
			RemoveAllGates();
			break;
	}
}
#endif

#ifdef DEBUG_FRAMES
/******************************** AppendFrame *********************************/
void DustCollector::AppendFrame(
	CANFrame&	inCANFrame)
{
	if (mFrameIndex == (DEBUG_FRAMES*sizeof(CANFrame)))
	{
		Serial.println('F');
		DumpFrames();
	}
	inCANFrame.CopyFromRaw(&mFrameBuffer[mFrameIndex]);
	mFrameIndex += sizeof(CANFrame);
}

const char kEWARNStr[] PROGMEM = "EWARN";
const char kRXWARStr[] PROGMEM = "RXWAR";
const char kTXWARStr[] PROGMEM = "TXWAR";
const char kRXEPStr[] PROGMEM = "RXEP";
const char kTXEPStr[] PROGMEM = "TXEP";
const char kTXBOStr[] PROGMEM = "TXBO";
const char kRX0OVRStr[] PROGMEM = "RX0OVR";
const char kRX1OVRStr[] PROGMEM = "RX1OVR";
typedef  const char* ConstCharPtr;
const ConstCharPtr kErrorStrs[] PROGMEM = {
	kEWARNStr, kRXWARStr, kTXWARStr, kRXEPStr,
	kTXEPStr, kTXBOStr, kRX0OVRStr, kRX1OVRStr};

/********************************* DumpFrames *********************************/
void DustCollector::DumpFrames(void)
{
	uint32_t	fromID;
	CANFrame	canFrame;
	for (uint8_t frameIndex = 0; frameIndex < mFrameIndex; frameIndex += sizeof(CANFrame))
	{
		canFrame.CopyToRaw(&mFrameBuffer[frameIndex]);
		fromID = 0;
		Serial.print(F("To: 0x"));
		Serial.print(canFrame.GetExtendedID(), HEX);
		switch (canFrame.GetStandardID())
		{
			case DCController::eRequestGateState:
				Serial.print(F(", eRequestGateState"));
				break;
			case DCController::eRequestTimestamp:
				Serial.print(F(", eRequestTimestamp"));
				break;
			case DCController::eFlash:
				Serial.print(F(", eFlash"));
				break;
			case DCController::eStopFlash:
				Serial.print(F(", eStopFlash"));
				break;
			case DCController::eSetFactoryID:
				Serial.print(F(", eSetFactoryID"));
				break;
			case 0x201:
			case DCController::eSetID:
				Serial.print(F(", eSetID to 0x"));
				Serial.print(*((const uint32_t*)canFrame.GetData()), HEX);
				if (canFrame.GetStandardID() == 0x201)
				{
					Serial.print(F(" (Failed)"));
				}
				break;
			case DCSensor::eGateIsOpen:
				fromID = *(const uint32_t*)canFrame.GetData();
				Serial.print(F(", eGateIsOpen"));
				break;
			case DCSensor::eGateIsClosed:
				fromID = *(const uint32_t*)canFrame.GetData();
				Serial.print(F(", eGateIsClosed"));
				break;
			case DCSensor::eTimestamp:
			{
				fromID = *(const uint32_t*)canFrame.GetData();
				Serial.print(F(", eTimestamp = "));
				Serial.print(((const uint32_t*)canFrame.GetData())[1], HEX);
				break;
			}
			case 0x200:
			{
				Serial.print(F(", Error 0x"));
				uint8_t	error = canFrame.GetRawFrame()[5];
				Serial.print(error, HEX);
				for (uint8_t errIdx = 0; error; error >>= 1)
				{
					if (error & 1)
					{
						char errorStr[10];
						char*	errorPtr;
						memcpy_P(&errorPtr, &kErrorStrs[errIdx], sizeof(char*));
						strcpy_P(errorStr, errorPtr);
						Serial.print(F(", "));
						Serial.print(errorStr);
					}
					errIdx++;
				}
				Serial.print(F(", CANINTE "));
				Serial.print(canFrame.GetRawFrame()[6], HEX);
				Serial.print(F(", CANINTF "));
				Serial.print(canFrame.GetRawFrame()[7], HEX);
				break;
			}
			default:
				Serial.print(F(", Unknown command 0x"));
				Serial.print(canFrame.GetStandardID(), HEX);
				break;
		}
		if (fromID)
		{
			Serial.print(F(", GateID: 0x"));
			Serial.println(fromID, HEX);
		} else
		{
			Serial.println();
		}
	}
	mFrameIndex = 0;
}
#endif