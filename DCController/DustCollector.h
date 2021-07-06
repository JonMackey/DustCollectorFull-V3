/*
*	DustCollector.h, Copyright Jonathan Mackey 2020
*	- MCP2515 subclass that monitors and communicates with gate sensors.
*	- Monitors the dust collector to determine when the drum or filter is full.
*	- Controls the drum motor and warning flasher.
*	- Sends radio messages for two major events: dust bin full, and filter full.
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
#ifndef DustCollector_h
#define DustCollector_h

#include <inttypes.h>
#include "MSPeriod.h"
#include "Gates.h"
#include "GateSets.h"
#include "BMP280SPI.h"
#include "RFM69.h"    // https://github.com/LowPowerLab/RFM69
#include "MCP2515.h"
#include "DCConfig.h"

//#define DEBUG_MOTOR	1
//#define DEBUG_DELTAS	1

//#define DEBUG_FRAMES		50
#define CAN_QUEUE_SIZE		64

class DustCollector : public MCP2515
{
public:
	enum EStatus
	{
		eNotRunning,
		eRunning,
		eBinFull,
		eFilterFull
	};
	
	enum EGateState
	{
		eClosedState,
		eOpenState,
		eErrorState	// Gate sensor is not responding
	};
							DustCollector(void);
		
	void					begin(void);

	uint8_t					Status(void) const
								{return(mStatus);}
	void					UserAcknowledgedFault(void);
	uint32_t				AmbientPressure(void) const
								{return(mAmbientPressure);}
	uint32_t				DuctPressure(void) const
								{return(mDuctPressure);}
	inline int32_t			DeltaAverage(void) const
								{return(mDeltaSum/DCConfig::kNumDeltas);}
	bool					DeltaAveragesLoaded(void) const
								{return(mDeltaAveragesLoaded);}
	bool					DCIsRunning(void) const
								{return(mDCIsRunning);}
	bool					BinMotorIsRunning(void) const
								{return(mMotorSensePeriod.Get() != 0);}
							// Start/Stop motor from UI
	void					ToggleBinMotor(void);
	uint8_t					GetBinMotorReading(void) const
								{return(mBinMotorAverage);}
	int32_t					Baseline(void) const // Returns the oldest average
								{return(mDeltaAverage[mDeltaAverageIndex % DCConfig::kNumDeltaAvgs]);}
							// The adjusted delta average
	inline int32_t			AdjustedDeltaAverage(void) const
								{return(DeltaAverage() - Baseline());}
	bool					Update(void);
	void					SendAudioAlertMessage(
								uint32_t				inMessage);
	Gates&					GetGates(void)
								{return(mGates);}
	GateSets&				GetGateSets(void)
								{return(mGateSets);}
	void					SetGateState(
								uint16_t				inRecIndex,		// Unsorted physical record index
								bool					inGateIsOpen);
							/*
							*	Removes all gates and gate sets.  This should
							*	obviously not be performed unless the gate
							*	configuration has changed significantly.
							*/
	void					RemoveAllGates(void);
	void					RemoveAllGateSets(void);
	bool					SaveCleanSet(void);
	bool					SaveDirtySet(void);
							/*
							*	Removes a gate and any gate sets that refer to
							*	it.  This will fail if the gate is not in an
							*	error/unresponsive state.
							*/
	void					RemoveGate(
								uint16_t				inRecIndex);
							/*
							*	Sends a message to the gate sensor to either
							*	start or stop flashing the sensor's RGB LED
							*/
	void					ToggleGateFlasher(
								uint16_t				inGateIndex);
	void					ToggleCurrentGateFlasher(void)
								{ToggleGateFlasher(mGates.GetCurrentIndex());}
	void					StopAllFlashingGateLEDs(void);
	void					ResetAllGatesToFactoryID(void);
	void					RequestAllGateStates(void);
							/*
							*	Sends a message to the sensor to get the current
							*	gate state.  The response from the sensor will
							*	be handled in CheckGates() as if the user
							*	manually opened or closed this gate.
							*/
	void					RequestGateState(
								uint16_t				inGateIndex);
	uint32_t				UnresponsiveGates(void) const
								{return(mUnresponsiveGates);}
	uint8_t					NextUnresponsiveGate(
								uint8_t					inGateIndex,
								bool					inForward) const;
	uint32_t				UnregisteredGateID(void) const
								{return(mUnregisteredGateID);}
							/*
							*	This routine will either replace or create a new
							*	gate entry for mUnregisteredGateID.  In either
							*	case mUnregisteredGateID is reset to 0.
							*/
	void					RegisterUnregisteredGate(
								uint16_t				inReplaceExistingIndex = 0);
	uint8_t					GetGateState(
								uint16_t				inRecIndex) const;
	uint32_t				OpenGates(void) const
								{return(mOpenGates);}
	bool					GateCheckDone(void) const
								{return(mGateCheckDone);}
	uint8_t					GetTriggerThreshold(void) const
								{return(mTriggerThreshold);}
	void					SetTriggerThreshold(
								uint8_t					inTriggerThreshold);
	void					SaveTriggerThreshold(void);	// Save to EEPROM
	void					StartFlasher(void);
	void					StopFlasher(void);
								
#if 0
	void					DoSerial(void);
#endif
protected:
	Gates		mGates;
	GateSets	mGateSets;
	uint8_t		mStatus;
	MSPeriod	mCANBusyPeriod;
	MSPeriod	mPressureUpdatePeriod;
	int32_t		mDeltaSum;
	int32_t		mDelta[DCConfig::kNumDeltas];
	int16_t		mDeltaAverage[DCConfig::kNumDeltaAvgs];
	uint8_t		mDeltaIndex;
	uint8_t		mDeltaAverageIndex;
	bool		mDeltaSumLoaded;
	bool		mDeltaAveragesLoaded;
	bool		mDCIsRunning;
	bool		mGateCheckDone;
	bool		mFaultAcknowledged;
	BMP280SPI	mBMP280Ambient;
	BMP280SPI	mBMP280Duct;
	RFM69		mRadio;
	uint32_t	mDuctPressure;
	uint32_t	mAmbientPressure;

	uint32_t	mOpenGates;
	uint32_t	mFlashingGates;
	uint32_t	mUnregisteredGateID; // Most recent unregistered gate
	uint32_t	mUnresponsiveGates;	// Gates that didn't respond to gate status request
	uint32_t	mGateBaseID;
	static const uint8_t	kTimingConfig[];

	MSPeriod	mMotorSensePeriod;

	uint16_t	mSampleCount;
	uint16_t	mRingBuf[DCConfig::kBinMotorSampleSize];
	uint8_t		mRingBufIndex;
	uint16_t	mSampleAccumulator;

	uint8_t		mTriggerThreshold;
	uint8_t		mBinMotorAverage;	// used by Bin Motor panel UI
	
	struct SCANMessageQueueElement
	{
		uint32_t	targetID : 18,	// target ID
					command : 14;
	} mCANMessageQueue[DCConfig::kCANQueueSize];
	uint8_t		mCANMessageQueueHead;
	uint8_t		mCANMessageQueueTail;
	uint8_t	mNotUsed[32];

#ifdef DEBUG_FRAMES
	uint8_t				mFrameBuffer[DEBUG_FRAMES*sizeof(CANFrame)];
	uint8_t				mFrameIndex;
#endif
#ifdef DEBUG_DELTAS
	uint16_t	mDebugAverageIndex;
	int16_t		mDeltaAverageDebug[512];
#endif

	static void 			ExtIntReq2(void);
	void					CheckFilter(void);
	void					CheckDustBinMotor(void);
	bool					CheckGates(void);
	virtual void			DoConfig(void);
	void					HandleReceivedFrame(
								CANFrame&				inCANFrame);
	void					StartDustBinMotor(void);
	void					StopDustBinMotor(void);
	void					QueueCANMessage(
								uint32_t				inID,
								uint16_t				inCommand);
	void					SendNextQueuedMessage(void);

#ifdef DEBUG_FRAMES
	void					AppendFrame(
								CANFrame&				inCANFrame);
	void					DumpFrames(void);
#endif
};

#endif // DustCollector_h
