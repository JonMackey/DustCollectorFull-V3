/*
*	AnimatedFontIcon.cpp, Copyright Jonathan Mackey 2020
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
#include "AnimatedFontIcon.h"
#include "DisplayController.h"

/****************************** AnimatedFontIcon ******************************/
AnimatedFontIcon::AnimatedFontIcon(void)
{
}

/********************************* Initialize *********************************/
void AnimatedFontIcon::Initialize(
	XFont*			inXFont,
	XFont::Font*	inIconsFont,
	uint8_t			inTop,
	uint8_t			inLeft,
	uint8_t			inFromChar,
	uint8_t			inToChar,
	uint16_t		inRunningColor,
	uint16_t		inStoppedColor)
{
	mXFont = inXFont;
	mIconsFont = inIconsFont;
	mTop = inTop;
	mLeft = inLeft;
	mFromChar = inFromChar;
	mToChar = inToChar;
	mRunningColor = inRunningColor;
	mStoppedColor = inStoppedColor;
	mFontStr[1] = 0;
	SetAnimationPeriod(0);
}

/***************************** SetAnimationPeriod *****************************/
void AnimatedFontIcon::SetAnimationPeriod(
	uint32_t	inPeriod)
{
	mAnimationPeriod.Set(inPeriod);
	if (inPeriod)
	{
		mAnimationPeriod.Start();
	}
	mFontStr[0] = mFromChar;
	mDirty = true;
}

/*********************************** Update ***********************************/
void AnimatedFontIcon::Update(
	bool	inForceDraw)
{
	if (inForceDraw ||
		mDirty ||
		mAnimationPeriod.Passed())
	{
		mDirty = false;
		if (mAnimationPeriod.Get())
		{
			if (mFontStr[0] != mToChar)
			{
				mFontStr[0]++;
			} else
			{
				mFontStr[0] = mFromChar;
			}
			mXFont->SetTextColor(mRunningColor);
			mAnimationPeriod.Start();
		} else
		{
			mXFont->SetTextColor(mStoppedColor);
		}
		mXFont->GetDisplay()->MoveTo(mTop, mLeft);
		XFont::Font*	savedFont = mXFont->GetFont();
		mXFont->SetFont(mIconsFont);
		mXFont->DrawStr(mFontStr);
		mXFont->SetFont(savedFont);
	}
}
