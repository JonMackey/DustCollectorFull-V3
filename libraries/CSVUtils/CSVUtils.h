/*
*	CSVUtils.h, Copyright Jonathan Mackey 2020
*	Basic CSV reading/writing class.
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
#ifndef CSVUtils_h
#define CSVUtils_h

#include <inttypes.h>

#ifdef __MACH__
#include <stdio.h>
#define SdFile	FILE
#else
class SdFile;
#endif

class CSVUtils
{
public:
							CSVUtils(
								SdFile*					inFile,
								char					inDelimiter = ',');
	char					ReadUint8(
								uint8_t*				outValue);
	char					ReadUint16(
								uint16_t*				outValue);
	char					ReadStr(
								uint8_t					inMaxStrLen,
								char*					outStr);
	char					SkipLine(void);
	const char*				QuoteForCSV(
								const char*				inStr,
								char*					outStr);
	char					GetChar(void);
protected:
	SdFile*	mFile;
	char	mDelimiter;
};

#endif // CSVUtils_h
