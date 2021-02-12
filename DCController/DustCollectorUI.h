/*
*	DustCollectorUI.h, Copyright Jonathan Mackey 2020
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
#ifndef DustCollectorUI_h
#define DustCollectorUI_h

#include "XFont.h"
#include <inttypes.h>
#include <time.h>
#include "MSPeriod.h"
#include "FilterStatusMeter.h"
#include "DisplayController.h"
#include "UnixTimeEditor.h"
#include "DCInfoField.h"
#include "AnimatedFontIcon.h"
#include "DCConfig.h"

class DustCollector;

class DustCollectorUI : public XFont
{
public:
							DustCollectorUI(void);	
		
	void					begin(
								DustCollector*			inDustCollector,
								DisplayController*		inDisplay,
								Font*					inNormalFont,
								Font*					inSmallFont,
								Font*					inIconsFont);
	
	void					Update(void);
#ifdef DEBUG_DELTAS
	bool					SaveDebug(void);
#endif
	// The following 2 routines are used by ISRs
	static void				SetSDInsertedOrRemoved(void)
								{sSDInsertedOrRemoved = true;}
	static void				SetButtonPressed(
								bool					inButtonPressed)
								{sButtonPressed = sButtonPressed || inButtonPressed;}
protected:
	DustCollector*			mDustCollector;
	FilterStatusMeter		mFilterStatusMeter;
	UnixTimeEditor			mUnixTimeEditor;
	DCInfoField				mDCInfoField0;
	DCInfoField				mDCInfoField1;
	AnimatedFontIcon		mMotorIcon;
	MSPeriod				mDebouncePeriod;	// For buttons and SD card
	MSPeriod				mSelectionPeriod;	// Selection frame flash rate
	uint8_t					mMode;
	uint8_t					mCurrentFieldOrItem;
	uint8_t					mStartPinState;
	bool					mSDCardPresent;
	bool					mDisplaySleeping;
	bool					mIgnoreButtonPress;
	bool					mSleepEnabled;
	bool					mPrevSleepEnabled;
	bool					mPrevBinMotorIsRunning;
	uint8_t					mSelectionIndex;
	uint8_t					mSavedMotorThreshold;
	uint8_t					mPrevMotorThreshold;
	uint8_t					mPrevBinMotorReading;
	Font*					mNormalFont;
	Rect8_t					mSelectionRect;
	uint8_t					mMessageLine0;
	uint8_t					mMessageLine1;
	uint8_t					mMessageReturnMode;
	uint8_t					mMessageReturnItem;
	// vars used to determine if the display needs to be updated
	uint16_t				mCurrentUnresponsiveGateIndex;
	uint16_t				mUIGateIndex;
	uint8_t					mPrevMode;
	uint8_t					mPrevStatus;
	uint8_t					mPrevGateState;
	bool					mPrevDCIsRunning;
	uint8_t					mSelectionFieldOrItem;

	static bool				sButtonPressed;
	static bool				sSDInsertedOrRemoved;
	static const uint8_t	kNextInfoField[];
	static const uint8_t	kPrevInfoField[];

	void					UpdateActions(void);
	void					UpdateDisplay(void);
	uint8_t					Mode(void) const
								{return(mMode);}
	uint8_t					CurrentFieldOrItem(void) const
								{return(mCurrentFieldOrItem);}
	void					GoToInfoMode(void);
	void					SetSDCardPresent(
								bool					inSDCardPresent);
	void					EnterPressed(void);
	void					UpDownButtonPressed(
								bool					inIncrement);
	void					LeftRightButtonPressed(
								bool					inIncrement);

	void					WakeUp(void);
	void					GoToSleep(void);

	void					ClearLines(
								uint8_t					inFirstLine,
								uint8_t					inNumLines);
	void					DrawCenteredList(
								uint8_t					inLine,
								uint8_t					inTextEnum, ...);
	void					DrawCenteredDescP(
								uint8_t					inLine,
								uint8_t					inTextEnum);
	void					DrawCenteredItemP(
								uint8_t					inLine,
								const char*				inTextPStr,
								uint16_t				inColor);
	void					DrawCenteredItem(
								uint8_t					inLine,
								const char*				inTextStr,
								uint16_t				inColor);
	void					DrawDescP(
								uint8_t					inLine,
								uint8_t					inTextEnum,
								uint8_t					inColumn = DCConfig::kTextInset,
								bool					inClearTillEOL = false);
	void					DrawItemP(
								uint8_t					inLine,
								const char*				inTextPStr,
								uint16_t				inColor,
								uint8_t					inColumn = DCConfig::kTextInset,
								bool					inClearTillEOL = false);
	void					DrawItemValueP(
								const char*				inTextPStr,
								uint16_t				inColor);
	void					DrawItem(
								uint8_t					inLine,
								const char*				inTextStr,
								uint16_t				inColor,
								uint8_t					inColumn = DCConfig::kTextInset,
								bool					inClearTillEOL = false);
	static char*			UInt8ToDecStr(
								uint8_t					inNum,
								char*					inBuffer);
	void					UpdateSelectionFrame(void);
	void					HideSelectionFrame(void);
	void					InitializeSelectionRect(void);
	void					QueueMessage(
								uint8_t					inMessageLine0,
								uint8_t					inMessageLine1,
								uint8_t					inReturnMode,
								uint8_t					inReturnItem);
	
	enum EMode
	{
		eMainMenuMode,
		eInfoMode,
		eGateSensorsMode,
		eGateSetsMode,
		eSetTimeMode,
		eBinMotorMode,
		eVerifyResetGatesMode,
		eVerifyResetGateSetsMode,
		eMessageMode,
		eWaitingForGateCheckMode,
		eResolveUnregisteredGateMode,
		eVerifyGateRemovalMode
	};
	
	enum EMainMenuItem
	{
		eGateNamesItem,
		eGateSetsItem,
		eSetTimeItem,
		eBinMotorItem,
		eEnableSleepItem
	};
	enum EInfoField
	{
		eGateNameField,
		eGateStateField,
		eCollectorStatusField,
		eInfoField0,
		eInfoField1
	};
	enum EGateSensorsMenuItem
	{
		eResetItem,
		eCheckGatesItem,
		eSaveNamesToSDItem,
		eLoadNamesFromSDItem
	};
	enum EGateSetsMenuItem
	{
		eSaveCleanSetItem,
		eSaveDirtySetItem,
		eResetSetsItem,
		eSaveSetsToSDItem
	};
	enum EVerifyResetItem
	{
		eVerifyItem,
		eVerifyYesItem,
		eVerifyNoItem
	};
	enum EResolveUnregisteredGateItem
	{
		eRegisterItem,
		eGateAsItem,
		eResolutionItem,
		eDoResolutionItem
	};
	enum EBinMotorMenuItem
	{
		eSensitivityTitleItem,
		eMotorSensitivityItem,
		eSaveSensitivityItem,
		eMotorControlItem,
		eMotorValueItem
	};
	enum EMessageItem
	{
		eMessage0Item,
		eMessage1Item,
		eOKItemItem,
	};
	enum ETextDesc
	{
		eTextListEnd,
		// Messages
		eNoMessage,
		eSavedMessage,
		eSaveFailedMessage,
		eLoadedMessage,
		eLoadFailedMessage,
		eGateCheckSuccessMessage,
		eGateCheckFailedMessage,
		eCheckInfoMessage,
		eCleanSetSavedMessage,
		eDirtySetSavedMessage,
		eNoSDCardMessage,
		eCollectorMessage,
		eNotRunningMessage,
		eDustBinFullMessage,
		eFilterLoadedMessage,
		eOKItemDesc,
		// Gate States
		eClosedStateDesc,
		eOpenStateDesc,
		eErrorStateDesc,
		eErrorMessage = eErrorStateDesc, 	// Also reused as a message
		// Main Menu items
		eGateSensorsItemDesc,
		eGateSetsItemDesc,
		eSetTimeItemDesc,
		eSleepItemDesc,
		// Gate Sensors menu items
		eSaveToSDItemDesc,
		eLoadFromSDItemDesc,
		eResetItemDesc,
		eCheckGatestemDesc,
		// Gate Sets menu items
		eSaveCleanItemDesc,
		eSaveDirtyItemDesc,
		// Verify Reset Gates Yes/No
		eRemoveAllItemDesc,
		eYesItemDesc,
		eNoItemDesc,
		// Verify Remove (unresponsive) Gate Yes/No
		eRemoveGateItemDesc,
		// Resolve unregistered gate
		eRegisterItemDesc,
		eGateAsItemDesc,
		eNewItemDesc,
		// Bin Motor items
		eBinMotorDesc,
		eSensitivityDesc,
		eSaveDesc,
		eMotorDesc,
		eCurrentDesc,
		eOffDesc,
		eOnDesc
	};
};

#endif // DustCollectorUI_h
