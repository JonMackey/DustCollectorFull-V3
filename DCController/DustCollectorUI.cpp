/*
*	DustCollectorUI.cpp, Copyright Jonathan Mackey 2020
*	Handles input from the UI.
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
#include "DustCollector.h"
#include "DustCollectorUI.h"
#include "UnixTimeEditor.h"
#include "DCConfig.h"


bool DustCollectorUI::sSDInsertedOrRemoved;
bool DustCollectorUI::sButtonPressed;


const char kInfoStr[] PROGMEM = "INFO";
const char kGateSensorsStr[] PROGMEM = "GATE SENSORS";
const char kGateSetsStr[] PROGMEM = "GATE SETS";
const char kSetTimeStr[] PROGMEM = "SET TIME";
const char kEnableSleepStr[] PROGMEM = "ENABLE SLEEP";
const char kDisableSleepStr[] PROGMEM = "DISABLE SLEEP";

// Gate Sensors menu items
const char kSaveToSDStr[] PROGMEM = "SAVE TO SD";
const char kLoadFromSDStr[] PROGMEM = "LOAD FROM SD";
const char kResetStr[] PROGMEM = "RESET";
const char kCheckGatesStr[] PROGMEM = "CHECK GATES";

// Gate Sets menu items
const char kSaveCleanStr[] PROGMEM = "SAVE CLEAN";
const char kSaveDirtyStr[] PROGMEM = "SAVE DIRTY";
//const char kNoSDCardStr[] PROGMEM = "NO SD CARD";
//const char kSaveToSDStr[] PROGMEM = "SAVE TO SD";

// Info gate status
const char kOpenStr[] PROGMEM = "OPEN";
const char kClosedStr[] PROGMEM = "CLOSED";
const char kErrorStr[] PROGMEM = "ERROR";

// Load/Save Status Messages
const char kSavedStr[] PROGMEM = "SAVED";
const char kSaveFailedStr[] PROGMEM = "SAVE FAILED";
const char kLoadedStr[] PROGMEM = "LOADED";
const char kLoadFailedStr[] PROGMEM = "LOAD FAILED";
const char kAllGatesOKStr[] PROGMEM = "ALL GATES OK";
const char kCheckFailedStr[] PROGMEM = "CHECK FAILED";
const char kCheckInfoStr[] PROGMEM = "CHECK INFO";
const char kCleanSavedStr[] PROGMEM = "CLEAN SAVED";
const char kDirtySavedStr[] PROGMEM = "DIRTY SAVED";
const char kNoSDCardStr[] PROGMEM = "NO SD CARD";
const char kCollectorStr[] PROGMEM = "COLLECTOR";
const char kNotRunningStr[] PROGMEM = "NOT RUNNING!";
const char kNoMessageStr[] PROGMEM = " ";

// Verify Reset Gates Yes/No
const char kRemoveAllStr[] PROGMEM = "REMOVE ALL?";
const char kYesStr[] PROGMEM = "YES";
const char kNoStr[] PROGMEM = "NO";

// Verify Remove (unresponsive) Gate Yes/No
const char kRemoveGateStr[] PROGMEM = "REMOVE GATE?";
//const char kYesStr[] PROGMEM = "YES";
//const char kNoStr[] PROGMEM = "NO";

// Resolve unregistered gate
const char kRegisterStr[] PROGMEM = "REGISTER";
const char kGateAsStr[] PROGMEM = "GATE AS:";
const char kNewStr[] PROGMEM = "NEW";

const char kOKStr[] PROGMEM = "OK";

// Dust Collector Fault Status
const char kDustBinFullStr[] PROGMEM = "DUST BIN FULL";
const char kFilterLoadedStr[] PROGMEM = "FILTER LOADED";


struct SString_PDesc
{
	const char*	descStr;
	uint16_t	color;
};
const SString_PDesc kTextDesc[] PROGMEM =
{
	{kNoMessageStr, XFont::eWhite},
	{kSavedStr, XFont::eGreen},
	{kSaveFailedStr, XFont::eRed},
	{kLoadedStr, XFont::eGreen},
	{kLoadFailedStr, XFont::eRed},
	{kAllGatesOKStr, XFont::eGreen},
	{kCheckFailedStr, XFont::eRed},
	{kCheckInfoStr, XFont::eYellow},
	{kCleanSavedStr, XFont::eGreen},
	{kDirtySavedStr, XFont::eGreen},
	{kNoSDCardStr, XFont::eYellow},
	{kCollectorStr, XFont::eYellow},
	{kNotRunningStr, XFont::eYellow},
	{kDustBinFullStr, XFont::eRed},
	{kFilterLoadedStr, XFont::eRed},
	{kOKStr, XFont::eWhite},
	// Gate States
	{kClosedStr, XFont::eYellow},
	{kOpenStr, XFont::eGreen},
	{kErrorStr, XFont::eRed},
	// Main Menu items
	{kInfoStr, XFont::eWhite},
	{kGateSensorsStr, XFont::eWhite},
	{kGateSetsStr, XFont::eWhite},
	{kSetTimeStr, XFont::eWhite},
	{kEnableSleepStr, XFont::eWhite},
	{kDisableSleepStr, XFont::eWhite},
	// Gate Sensors menu items
	{kSaveToSDStr, XFont::eWhite},
	{kLoadFromSDStr, XFont::eWhite},
	{kResetStr, XFont::eRed},
	{kCheckGatesStr, XFont::eYellow},
	// Gate Sets menu items
	{kSaveCleanStr, XFont::eWhite},
	{kSaveDirtyStr, XFont::eWhite},
	// Verify Reset Gates Yes/No
	{kRemoveAllStr, XFont::eWhite},
	{kYesStr, XFont::eGreen},
	{kNoStr, XFont::eRed},
	// Verify Remove (unresponsive) Gate Yes/No
	{kRemoveGateStr, XFont::eWhite},
	// Resolve unregistered gate
	{kRegisterStr, XFont::eWhite},
	{kGateAsStr, XFont::eWhite},
	{kNewStr, XFont::eGreen}
};

#define DEBOUNCE_DELAY	20	// ms

/****************************** DustCollectorUI *******************************/
DustCollectorUI::DustCollectorUI(void)
: mSDCardPresent(false), mDebouncePeriod(DEBOUNCE_DELAY),
	mPrevDCIsRunning(false), mIgnoreButtonPress(false), mSleepEnabled(true)
{
}

/*********************************** begin ************************************/
void DustCollectorUI::begin(
	DustCollector*		inDustCollector,
	DisplayController*	inDisplay,
	Font*				inNormalFont,
	Font*				inSmallFont,
	Font*				inIconsFont)
{
	pinMode(DCConfig::kUpBtnPin, INPUT_PULLUP);
	pinMode(DCConfig::kLeftBtnPin, INPUT_PULLUP);
	pinMode(DCConfig::kEnterBtnPin, INPUT_PULLUP);
	pinMode(DCConfig::kRightBtnPin, INPUT_PULLUP);
	pinMode(DCConfig::kDownBtnPin, INPUT_PULLUP);

	pinMode(DCConfig::kSDDetectPin, INPUT_PULLUP);
	pinMode(DCConfig::kSDSelectPin, OUTPUT);
	digitalWrite(DCConfig::kSDSelectPin, HIGH);	// Deselect the SD card.

	cli();
	/*
	*	To respond to button presses and the interrupt line from an SD Card
	*	being inserted, setup pin change interrupts for the associated pins. All
	*	of the button pins are on the same port.
	*/
	PCMSK0 = _BV(PCINT3) | _BV(PCINT4) | _BV(PCINT5) | _BV(PCINT6) | _BV(PCINT7);
	PCMSK3 = _BV(PCINT29);	// PD5 SD Card Inserted
	PCICR = _BV(PCIE0) | _BV(PCIE3);
	/*
	*	As per 13.2.1 in the ATmega644PA doc, clearing ISC00 and ISC01 of EIMSK
	*	sets the mode to generate an interrupt when PB2/INT2 goes low. This is
	*	the POR value so there's no reason to explicitly clear these bits here.
	*/
	// EICRA = EICRA & ~(_BV(ISC00) | _BV(ISC01));
	EIMSK |= _BV(INT2); // Enable INT2
	sei();

	// In case booting with the SD card in
	sSDInsertedOrRemoved = digitalRead(DCConfig::kSDDetectPin) == LOW;

	mDustCollector = inDustCollector;
	
	mPrevMode = 99;
	mNormalFont = inNormalFont;
	SetDisplay(inDisplay, inNormalFont);
	mFilterStatusMeter.Initialize(mDisplay, 43*2, 50);
	mFilterStatusMeter.SetMinMax(
		mDustCollector->GetGateSets().CurrentCleanPressure(),
			mDustCollector->GetGateSets().CurrentDirtyPressure());
	mUnixTimeEditor.Initialize(this);
	{
		mDCInfoField0.Initialize(this, inSmallFont, inDustCollector, 3,
					DCConfig::k1stInfoDataPresetAddr, DCInfoField::eTimeInfo);
		mDCInfoField1.Initialize(this, inSmallFont, inDustCollector, 4,
					DCConfig::k2ndInfoDataPresetAddr, DCInfoField::eDateInfo);
	}
	mMotorIcon.Initialize(this, inIconsFont, (2*43)+8, 3, 'A', 'B', eWhite, eGray);
	
	GoToInfoMode();
	GoToSleep();
}

const uint8_t	DustCollectorUI::kNextInfoField[] = {eInfoField0, 0, 0, eInfoField1, eGateNameField};
const uint8_t	DustCollectorUI::kPrevInfoField[] = {eInfoItem, 0, 0, eGateNameField, eInfoField0};
/**************************** UpDownButtonPressed *****************************/
void DustCollectorUI::UpDownButtonPressed(
	bool	inIncrement)
{
/*
	Main Menu 0 1 2 3
	Info 0 3 4
	Gate Names 0 1, 2 3 if SD card inserted
	Gate Sets 0 1, 2 if SD card inserted
	Set Time handled by UnixTimeEditor
*/
	uint8_t	mode = mMode;
	switch (mode)
	{
		case eMainMenuMode:
			if (inIncrement)
			{
				if (mCurrentFieldOrItem < eEnableSleepItem)
				{
					mCurrentFieldOrItem++;
				} else
				{
					mCurrentFieldOrItem = eInfoItem;
				}
			} else if (mCurrentFieldOrItem > eInfoItem)
			{
				mCurrentFieldOrItem--;
			} else
			{
				mCurrentFieldOrItem = eEnableSleepItem;
			}
			break;
		case eInfoMode:
			if (mCurrentFieldOrItem == eGateNameField &&
				inIncrement == false)
			{
				//mCurrentFieldOrItem = eInfoItem;	same as eGateNameField
				mode = eMainMenuMode;
			} else
			{
				mCurrentFieldOrItem = inIncrement ? kNextInfoField[mCurrentFieldOrItem] :
												kPrevInfoField[mCurrentFieldOrItem];
			}
			break;
		case eGateSensorsMode:
			if (inIncrement)
			{
				if (mCurrentFieldOrItem < eLoadNamesFromSDItem)
				{
					mCurrentFieldOrItem++;
				} else
				{
					mCurrentFieldOrItem = eResetItem;
				}
			} else if (mCurrentFieldOrItem > eResetItem)
			{
				mCurrentFieldOrItem--;
			} else
			{
				mCurrentFieldOrItem = eGateNamesItem;
				mode = eMainMenuMode;
			}
			break;
		case eGateSetsMode:
			if (inIncrement)
			{
				if (mCurrentFieldOrItem < eSaveSetsToSDItem)
				{
					mCurrentFieldOrItem++;
				} else
				{
					mCurrentFieldOrItem = eSaveCleanSetItem;
				}
			} else if (mCurrentFieldOrItem > eSaveCleanSetItem)
			{
				mCurrentFieldOrItem--;
			} else
			{
				mCurrentFieldOrItem = eGateSetsItem;
				mode = eMainMenuMode;
			}
			break;
		case eSetTimeMode:
			// The only way to get out of eSetTimeMode is to press enter.
			mUnixTimeEditor.UpDownButtonPressed(inIncrement);
			break;
		case eVerifyResetGatesMode:
		case eVerifyGateRemovalMode:
		case eVerifyResetGateSetsMode:
			mCurrentFieldOrItem = mCurrentFieldOrItem == eVerifyNoItem ? eVerifyYesItem : eVerifyNoItem;
			break;
	}
	mMode = mode;
}

/******************************** EnterPressed ********************************/
void DustCollectorUI::EnterPressed(void)
{
	switch (mMode)
	{
		case eMainMenuMode:
			if (mCurrentFieldOrItem < eEnableSleepItem)
			{
				mMode = mCurrentFieldOrItem+1;
				mCurrentFieldOrItem = 0;
				if (mMode == eSetTimeMode)
				{
					mUnixTimeEditor.SetTime(UnixTime::Time());
				}
			} else
			{
				mSleepEnabled = !mSleepEnabled;
				mPrevMode = eInfoMode;	// To force a redraw
			}
			break;
		case eInfoMode:
			switch (mCurrentFieldOrItem)
			{
				case eGateNameField:
					if (mDustCollector->GetGateState(mUIGateIndex) != DustCollector::eErrorState)
					{
						mDustCollector->ToggleCurrentGateFlasher();
					/*
					*	Else present the option to remove this gate.
					*	Note that this will also remove any gate sets that refer to this gate.
					*/
					} else
					{
						mMode = eVerifyGateRemovalMode;
						mCurrentFieldOrItem = eVerifyNoItem;
					}
					break;
				case eInfoField0:
					mDCInfoField0.SetPreset();
					break;
				case eInfoField1:
					mDCInfoField1.SetPreset();
					break;
			}
			break;
		case eGateSensorsMode:
		{
			bool	success = false;
			switch(mCurrentFieldOrItem)
			{
				case eResetItem:
					mMode = eVerifyResetGatesMode;
					mCurrentFieldOrItem = eVerifyNoItem;
					break;
				case eCheckGatesItem:
					mDustCollector->RequestAllGateStates();
					mMode = eWaitingForGateCheckMode;
					break;
				case eSaveNamesToSDItem:
					if (mSDCardPresent)
					{
						success = mDustCollector->GetGates().SaveToSD();
						QueueMessage(success ? eSavedMessage : eSaveFailedMessage,
							eNoMessage, eGateSensorsMode, eSaveNamesToSDItem);
					} else
					{
						QueueMessage(eNoSDCardMessage,
								eNoMessage, eGateSensorsMode, eSaveNamesToSDItem);
					}
					break;
				case eLoadNamesFromSDItem:
					if (mSDCardPresent)
					{
						success = mDustCollector->GetGates().LoadFromSD();
						QueueMessage(success ? eLoadedMessage : eLoadFailedMessage,
							eNoMessage, eGateSensorsMode, eLoadNamesFromSDItem);
					} else
					{
						QueueMessage(eNoSDCardMessage,
								eNoMessage, eGateSensorsMode, eLoadNamesFromSDItem);
					}
					break;
			}
			break;
		}
		case eGateSetsMode:
		{
			bool	success = false;
			switch(mCurrentFieldOrItem)
			{
				case eSaveCleanSetItem:
					if (mDustCollector->DCIsRunning())
					{
						mDustCollector->SaveCleanSet();
						QueueMessage(eCleanSetSavedMessage, eNoMessage,
									eGateSetsMode, eSaveCleanSetItem);
					} else
					{
						QueueMessage(eCollectorMessage,
								eNotRunningMessage, eGateSetsMode, eSaveCleanSetItem);
					}
					break;
				case eSaveDirtySetItem:
					if (mDustCollector->DCIsRunning())
					{
						mDustCollector->SaveDirtySet();
						QueueMessage(eDirtySetSavedMessage, eNoMessage,
									eGateSetsMode, eSaveDirtySetItem);
					} else
					{
						QueueMessage(eCollectorMessage,
								eNotRunningMessage, eGateSetsMode, eSaveDirtySetItem);
					}
					break;
				case eResetSetsItem:
					mMode = eVerifyResetGateSetsMode;
					mCurrentFieldOrItem = eVerifyNoItem;
					break;
				case eSaveSetsToSDItem:
					if (mSDCardPresent)
					{
						success = mDustCollector->GetGateSets().SaveToSD(mDustCollector->GetGates());
						QueueMessage(success ? eSavedMessage : eSaveFailedMessage,
										eNoMessage, eGateSetsMode, eSaveSetsToSDItem);
					} else
					{
						QueueMessage(eNoSDCardMessage,
								eNoMessage, eGateSetsMode, eSaveSetsToSDItem);
					}
					break;
			}
			break;
		}
		case eSetTimeMode:
			// If enter was pressed on SET or CANCEL
			if (mUnixTimeEditor.EnterPressed())
			{
				if (!mUnixTimeEditor.CancelIsSelected())
				{
					bool	isFormat24Hour;
					time32_t time = mUnixTimeEditor.GetTime(isFormat24Hour);
					UnixTime::SetTime(time);
					if (UnixTime::Format24Hour() != isFormat24Hour)
					{
						UnixTime::SetFormat24Hour(isFormat24Hour);
						uint8_t	flags;
						EEPROM.get(DCConfig::kFlagsAddr, flags);
						if (isFormat24Hour)
						{
							flags &= ~1;	// 0 = 24 hour
						} else
						{
							flags |= 1;		// 1 = 12 hour (default for new/erased EEPROMs)
						}
						EEPROM.put(DCConfig::kFlagsAddr, flags);
					}
				}
				mMode = eMainMenuMode;
				mCurrentFieldOrItem = eSetTimeItem;
			}
			break;
		case eVerifyResetGatesMode:
			if (mCurrentFieldOrItem == eVerifyYesItem)
			{
				mDustCollector->RemoveAllGates();
			}
			mMode = eGateSensorsMode;
			mCurrentFieldOrItem = eResetItem;
			break;
		case eVerifyResetGateSetsMode:
			if (mCurrentFieldOrItem == eVerifyYesItem)
			{
				mDustCollector->RemoveAllGateSets();
			}
			mMode = eGateSetsMode;
			mCurrentFieldOrItem = eResetSetsItem;
			break;
		case eVerifyGateRemovalMode:
			if (mCurrentFieldOrItem == eVerifyYesItem)
			{
				mDustCollector->RemoveGate(mUIGateIndex);
			}
			GoToInfoMode();
			break;
		case eMessageMode:
			if (mMessageReturnMode == eInfoMode)
			{
				GoToInfoMode();
			} else
			{
				mMode = mMessageReturnMode;
				mCurrentFieldOrItem = mMessageReturnItem;
			}
			break;
		case eResolveUnregisteredGateMode:
			mDustCollector->RegisterUnregisteredGate(mCurrentUnresponsiveGateIndex);
			GoToInfoMode();
			break;
	}
	UnixTime::ResetSleepTime();
}

/******************************** QueueMessage ********************************/
void DustCollectorUI::QueueMessage(
	uint8_t	inMessageLine0,
	uint8_t	inMessageLine1,
	uint8_t	inReturnMode,
	uint8_t	inReturnItem)
{
	mMessageLine0 = inMessageLine0;
	mMessageLine1 = inMessageLine1;
	mMode = eMessageMode;
	mMessageReturnMode = inReturnMode;
	mMessageReturnItem = inReturnItem;
}

/*************************** LeftRightButtonPressed ***************************/
void DustCollectorUI::LeftRightButtonPressed(
	bool	inIncrement)
{
	switch (mMode)
	{
		case eInfoMode:
			switch(mCurrentFieldOrItem)
			{
				case eGateNameField:
					inIncrement ? mDustCollector->GetGates().Next() :
									mDustCollector->GetGates().Previous();
					break;
				case eInfoField0:
					mDCInfoField0.IncrementField(inIncrement);
					break;
				case eInfoField1:
					mDCInfoField1.IncrementField(inIncrement);
					break;
			}
			break;
		case eSetTimeMode:
			mUnixTimeEditor.LeftRightButtonPressed(inIncrement);
			break;
		case eResolveUnregisteredGateMode:
			mCurrentUnresponsiveGateIndex =
				mDustCollector->NextUnresponsiveGate(mCurrentUnresponsiveGateIndex,
					inIncrement);
			break;
	}
}

/*********************************** Update ***********************************/
/*
*	Called from loop()
*/
void DustCollectorUI::Update(void)
{
	if (mMode == eWaitingForGateCheckMode)
	{
		if (mDustCollector->GateCheckDone())
		{
			bool	success = mDustCollector->UnresponsiveGates() == 0;
			QueueMessage(success ? eGateCheckSuccessMessage : eGateCheckFailedMessage,
				success ? eNoMessage : eCheckInfoMessage,
				eInfoMode, eGateNameField);
		} else
		{
			return;
		}
	} else if (mDustCollector->UnregisteredGateID() &&
		mMode != eResolveUnregisteredGateMode)
	{
		mMode = eResolveUnregisteredGateMode;
		mCurrentUnresponsiveGateIndex = mDustCollector->NextUnresponsiveGate(0, true);
		mCurrentFieldOrItem = eDoResolutionItem;
		mSelectionFieldOrItem = 0;	// Force the selection frame to update
		mUIGateIndex = 33;
	/*
	*	If the current mode isn't modal...
	*/
	} else if (mMode < eSetTimeMode) 
	{
		uint8_t	status = mDustCollector->Status();
		if (mPrevStatus != status)
		{
			mPrevStatus = status;
			if (status > DustCollector::eRunning)
			{
				QueueMessage(status == DustCollector::eBinFull ?
					eDustBinFullMessage : eFilterLoadedMessage, eNoMessage,
						eInfoMode, eGateNameField);
			}
		}
	}

	UpdateDisplay();
	UpdateActions();
}

/******************************* UpdateActions ********************************/
/*
*	Called after the display has updated.  Any states that need time are handled
*	here.
*/
void DustCollectorUI::UpdateActions(void)
{
	/*
	*	Some action states need to be reflected in the display before
	*	performing an action.
	*/
	if (sButtonPressed)
	{
		/*
		*	Wakeup the display when any key is pressed.
		*/
		WakeUp();
		/*
		*	If a debounce period has passed
		*/
		{
			uint8_t		pinsState = ((~PINA) & DCConfig::kPINABtnMask);

			/*
			*	If debounced
			*/
			if (mStartPinState == pinsState)
			{
				if (mDebouncePeriod.Passed())
				{
					sButtonPressed = false;
					mStartPinState = 0xFF;
					if (!mIgnoreButtonPress)
					{
						switch (pinsState)
						{
							case _BV(PINA3):	// Up button pressed
								UpDownButtonPressed(false);
								break;
							case _BV(PINA5):	// Enter button pressed
								EnterPressed();
								break;
							case _BV(PINA4):	// Left button pressed
								LeftRightButtonPressed(false);
								break;
							case _BV(PINA7):	// Down button pressed
								UpDownButtonPressed(true);
								break;
							case _BV(PINA6):	// Right button pressed
								LeftRightButtonPressed(true);
								break;
							default:
								mDebouncePeriod.Start();
								break;
						}
					} else
					{
						mIgnoreButtonPress = false;
					}
				}
			} else
			{
				mStartPinState = pinsState;
				mDebouncePeriod.Start();
			}
		}
	} else if (UnixTime::TimeToSleep() &&
		mMode < eSetTimeMode)	// Don't allow sleep or mode change when the mode is modal.
	{
		/*
		*	If sleep is enabled AND
		*	the dust collector isn't running THEN
		*	put the display to sleep.
		*/
		if (mSleepEnabled &&
			!mDustCollector->DCIsRunning())
		{
			GoToSleep();
		}
		GoToInfoMode();
	}
	
	if (sSDInsertedOrRemoved)
	{
		/*
		*	Wakeup the display when an SD card is inserted or removed.
		*/
		WakeUp();
		
		uint8_t		pinsState = (~PIND) & _BV(PIND5);
		/*
		*	If debounced
		*/
		if (mStartPinState == pinsState)
		{
			if (mDebouncePeriod.Passed())
			{
				sSDInsertedOrRemoved = false;
				mStartPinState = 0xFF;
				SetSDCardPresent(pinsState != 0);
			}
		} else
		{
			mStartPinState = pinsState;
			mDebouncePeriod.Start();
		}
	}
	
	/*
	*	Wakeup the display when the dust collector is running OR
	*	a gate was just opened or closed.
	*/
	if (mDisplaySleeping &&
		(mDustCollector->DCIsRunning() ||
			mDustCollector->GetGates().GetCurrentIndex() != mUIGateIndex ||
			mDustCollector->GetGateState(mUIGateIndex) != mPrevGateState))
	{
		WakeUp();
	}
}

/*********************************** WakeUp ***********************************/
/*
*	Wakup the display from sleep and/or keep it awake if not sleeping.
*/
void DustCollectorUI::WakeUp(void)
{
	if (mDisplaySleeping)
	{
		mDisplaySleeping = false;
		// If a button press that caused the display to wake then ignore
		// the current button press (after it debounces)
		// If the wake was because of an SD card being inserted/removed, then
		// don't ignore the next button press.
		mIgnoreButtonPress = sButtonPressed;
		mDisplay->WakeUp();
		mPrevMode = mMode+1;	// Force a full update
	}
	if (mSelectionPeriod.Get() == 0)
	{
		// Display the flashing selection frame.
		mSelectionPeriod.Set(500);
		mSelectionPeriod.Start();
	}
	UnixTime::ResetSleepTime();
}

/********************************* GoToSleep **********************************/
/*
*	Puts the display to sleep.
*/
void DustCollectorUI::GoToSleep(void)
{
	if (!mDisplaySleeping)
	{
		mDisplay->Fill();
		mDisplay->Sleep();
		mDisplaySleeping = true;
		mDustCollector->StopAllFlashingGateLEDs();
	}
}

/************************* Pin change interrupt PCI0 **************************/
/*
*
*	Sets a flag to show that buttons have been pressed.
*	This will also wakeup the mcu if it's sleeping.
*/
ISR(PCINT0_vect)
{
	// We only care when there is a button pressed (or still down), not when it's released.
	// When it's released it will equal the mask value.
	DustCollectorUI::SetButtonPressed((PINA & DCConfig::kPINABtnMask) != DCConfig::kPINABtnMask);
}

/************************* Pin change interrupt PCI3 **************************/
/*
*
*	Sets a flag to show that an SD card has been inserted or removed.
*/
ISR(PCINT3_vect)
{
	DustCollectorUI::SetSDInsertedOrRemoved();
}

/****************************** SetSDCardPresent ******************************/
void DustCollectorUI::SetSDCardPresent(
	bool	inSDCardPresent)
{
	mSDCardPresent = inSDCardPresent;
}

/********************************* ClearLines *********************************/
void DustCollectorUI::ClearLines(
	uint8_t	inFirstLine,
	uint8_t	inNumLines)
{
	mDisplay->MoveTo(inFirstLine*DCConfig::kFontHeight, 0);
	mDisplay->FillBlock(inNumLines*DCConfig::kFontHeight, DCConfig::kDisplayWidth, eBlack);
}

/******************************* UpdateDisplay ********************************/
void DustCollectorUI::UpdateDisplay(void)
{
	if (!mDisplaySleeping)
	{
		bool updateAll = mMode != mPrevMode;
	
		if (updateAll)
		{
			mPrevMode = mMode;
			if (mMode != eSetTimeMode)
			{
				mDisplay->Fill();
				InitializeSelectionRect();
			}
		}

		switch (mMode)
		{
			case eMainMenuMode:
			{
				if (updateAll)
				{
					DrawCenteredList(0,
						eInfoItemDesc,
						eGateSensorsItemDesc,
						eGateSetsItemDesc,
						eSetTimeItemDesc,
						mSleepEnabled ? eDisableSleepItemDesc : eEnableSleepItemDesc,
						eTextListEnd);
				}
				break;
			}
			case eInfoMode:
			{
				Gates&		gates = mDustCollector->GetGates();
				uint16_t	gateIndex = gates.GetCurrentIndex();
				uint8_t		gateState = mDustCollector->GetGateState(gateIndex);
				if (updateAll ||
					mUIGateIndex != gateIndex)
				{
					mUIGateIndex = gateIndex;
					ClearLines(eGateNameField, 1);
					if (gateIndex)
					{
						SetTextColor(eWhite);
						mDisplay->MoveToRow(DCConfig::kTextVOffset);
						DrawCentered(gates.GetCurrent().name);
					}
				}
				if (updateAll ||
					mPrevGateState != gateState ||
					gateIndex == 0)
				{
					mPrevGateState = gateState;
					mFilterStatusMeter.SetMinMax(
						mDustCollector->GetGateSets().CurrentCleanPressure(),
							mDustCollector->GetGateSets().CurrentDirtyPressure());
					ClearLines(eGateStateField, 1);
					if (gateIndex)
					{
						DrawCenteredList(eGateStateField, eClosedStateDesc+gateState, eTextListEnd);
					}
				}

				mDCInfoField0.Update(updateAll);
				mDCInfoField1.Update(updateAll);
				
				{
					bool dcIsRunning = mDustCollector->DCIsRunning();
					if (dcIsRunning != mPrevDCIsRunning)
					{
						mPrevDCIsRunning = dcIsRunning;
						mMotorIcon.SetAnimationPeriod(dcIsRunning ? 750 : 0);
					}
					mMotorIcon.Update(updateAll);
					mFilterStatusMeter.SetValue(dcIsRunning ? mDustCollector->DeltaAverage() : 0);
					mFilterStatusMeter.Update(updateAll);
				}
				
				break;
			}
			case eGateSensorsMode:
				if (updateAll)
				{
					DrawCenteredList(0,
						eResetItemDesc,
						eCheckGatestemDesc,
						eSaveToSDItemDesc,
						eLoadFromSDItemDesc,
						eTextListEnd);
				}
				break;
			case eGateSetsMode:
				if (updateAll)
				{
					DrawCenteredList(0,
						eSaveCleanItemDesc,
						eSaveDirtyItemDesc,
						eResetItemDesc,
						eSaveToSDItemDesc,
						eTextListEnd);
				}
				break;
			case eSetTimeMode:
				mUnixTimeEditor.Update();
				break;
			case eVerifyResetGatesMode:
			case eVerifyGateRemovalMode:
			case eVerifyResetGateSetsMode:
				if (updateAll)
				{
					DrawCenteredList(0,
						mMode == eVerifyGateRemovalMode ? eRemoveGateItemDesc : eRemoveAllItemDesc,
						eYesItemDesc,
						eNoItemDesc,
						eTextListEnd);
				}
				break;
			case eMessageMode:
				if (updateAll)
				{
					DrawCenteredList(0,
						mMessageLine0,
						mMessageLine1,
						eOKItemDesc,
						eTextListEnd);
					mCurrentFieldOrItem = eOKItemItem;
					mSelectionFieldOrItem = 0;	// Force the selection frame to update
				}
				break;
			case eResolveUnregisteredGateMode:
				if (updateAll)
				{
					DrawCenteredList(0,
						eRegisterItemDesc,
						eGateAsItemDesc,
						eNoMessage,
						eOKItemDesc,
						eTextListEnd);
				}
				if (mUIGateIndex != mCurrentUnresponsiveGateIndex)
				{
					mUIGateIndex = mCurrentUnresponsiveGateIndex;
					ClearLines(eResolutionItem, 1);
					mDisplay->MoveRowBy(DCConfig::kTextVOffset);
					if (mCurrentUnresponsiveGateIndex)
					{
						SetTextColor(eWhite);
						Gates&		gates = mDustCollector->GetGates();
						gates.GoToGate(mCurrentUnresponsiveGateIndex);
						DrawCentered(gates.GetCurrent().name);
					} else
					{
						DrawCenteredList(2, eNewItemDesc, eTextListEnd);
					}
				}
				break;
		}
		
		/*
		*	Set time mode has its own selection frame...
		*/
		if (mMode != eSetTimeMode)
		{
			UpdateSelectionFrame();
		}
	}
}

/************************** InitializeSelectionRect ***************************/
void DustCollectorUI::InitializeSelectionRect(void)
{
	mSelectionRect.x = mMode < eVerifyResetGatesMode ? 0 : 89;
	mSelectionRect.y = mCurrentFieldOrItem * DCConfig::kFontHeight;
	if (mMode == eInfoMode && mCurrentFieldOrItem > 0)
	{
		mSelectionRect.y += DCConfig::DCInfoOffset;
	}
	mSelectionRect.width = mMode < eVerifyResetGatesMode ? DCConfig::kDisplayWidth : 62;
	mSelectionRect.height = DCConfig::kFontHeight;
	mSelectionFieldOrItem = mCurrentFieldOrItem;
	mSelectionIndex = 0;
}

/***************************** HideSelectionFrame *****************************/
void DustCollectorUI::HideSelectionFrame(void)
{
	if (mSelectionPeriod.Get())
	{
		/*
		*	If the selection frame was last drawn as white THEN
		*	draw it as black to hide it.
		*/
		if (mSelectionIndex & 1)
		{
			mSelectionIndex = 0;
			mDisplay->DrawFrame8(&mSelectionRect, eBlack, 2);
		}
		mSelectionPeriod.Set(0);
	}
}

/**************************** UpdateSelectionFrame ****************************/
void DustCollectorUI::UpdateSelectionFrame(void)
{
	if (mSelectionPeriod.Get())
	{
		if (mSelectionFieldOrItem != mCurrentFieldOrItem)
		{
			if (mSelectionIndex & 1)
			{
				mDisplay->DrawFrame8(&mSelectionRect, eBlack, 2);
			}
			InitializeSelectionRect();
		}
		if (mSelectionPeriod.Passed())
		{
			mSelectionPeriod.Start();
			mSelectionIndex++;
			uint16_t	selectionColor = (mSelectionIndex & 1) ? eWhite : eBlack;
			mDisplay->DrawFrame8(&mSelectionRect, selectionColor, 2);
		}
	}
}

/******************************** GoToInfoMode ********************************/
void DustCollectorUI::GoToInfoMode(void)
{
	HideSelectionFrame();
	if (mMode != eInfoMode ||
		mCurrentFieldOrItem != eGateNameField)
	{
		mMode = eInfoMode;
		mCurrentFieldOrItem = eGateNameField;
		if (!mDisplaySleeping)
		{
			mUIGateIndex = 0;
		}
		InitializeSelectionRect();
	}
}

/****************************** DrawCenteredList ******************************/
void DustCollectorUI::DrawCenteredList(
	uint8_t		inLine,
	uint8_t		inTextEnum, ...)
{
	uint16_t		row = (inLine*DCConfig::kFontHeight) + DCConfig::kTextVOffset;
	
	SString_PDesc	textDesc;
	char	textStr[15];
	va_list arglist;
	va_start(arglist, inTextEnum);
	for (uint8_t textEnum = inTextEnum; textEnum; textEnum = va_arg(arglist, int))
	{
		mDisplay->MoveToRow(row);
		row += DCConfig::kFontHeight;
		memcpy_P(&textDesc, &kTextDesc[textEnum-1], sizeof(SString_PDesc));
		strcpy_P(textStr, textDesc.descStr);
		SetTextColor(textDesc.color);
		DrawCentered(textStr);
	}
	va_end(arglist);
}