/*
*	AnimatedFontIcon.h, Copyright Jonathan Mackey 2020
*
*	Sequences between sequential icons stored as XFont glyphs.
*	The icons font passed in the initialization routine must contain all of the
*	char codes between 'from' and 'to'.  The character range does not support
*	UTF-8 therefore all of the char codes must be less than 256.
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
#ifndef AnimatedFontIcon_h
#define AnimatedFontIcon_h

#include <inttypes.h>
#include "MSPeriod.h"
#include "XFont.h"

class AnimatedFontIcon
{
public:
							AnimatedFontIcon(void);
	void					Initialize(
								XFont*					inXFont,
								XFont::Font*			inIconsFont,
								uint8_t					inTop,
								uint8_t					inLeft,
								uint8_t					inFromChar,
								uint8_t					inToChar,
								uint16_t				inRunningColor,
								uint16_t				inStoppedColor);
	void					Update(
								bool					inUpdateAll);
							// Start/Stop.  Stop = 0
	void					SetAnimationPeriod(
								uint32_t				inPeriod);
protected:
	MSPeriod			mAnimationPeriod;
	XFont::Font*		mIconsFont;
	XFont*				mXFont;
	uint16_t			mRunningColor;
	uint16_t			mStoppedColor;
	uint8_t				mTop;
	uint8_t				mLeft;
	char				mFromChar;
	char				mToChar;
	char				mFontStr[2];
	bool				mDirty;
};

#endif // AnimatedFontIcon_h
