/*
*	CSVUtils.cpp, Copyright Jonathan Mackey 2020
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
#include "CSVUtils.h"
#ifndef __MACH__
#include <Arduino.h>
#include "SdFat.h"
#include "sdios.h"
#include "CompileTime.h"
#else
#include <stdio.h>
#endif

/********************************* CSVUtils *********************************/
CSVUtils::CSVUtils(
	SdFile*	inFile,
	char	inDelimiter)
: mFile(inFile), mDelimiter(inDelimiter)
{
}


/********************************** GetChar ***********************************/
char CSVUtils::GetChar(void)
{
	char		thisChar;
#ifdef __MACH__
	return((thisChar = getc(mFile)) != -1 ? thisChar : 0);
#else
	return(mFile->read(&thisChar,1) == 1 ? thisChar : 0);
#endif
}

/********************************* ReadUint8 **********************************/
/*
*	Consumes integer numbers till a delimiter, end of file, end of line or 
*	a non-number is hit.
*	Possible return values:
*		0	= end of file, which may or may not be an error
*		-1	= invalid character
*		delimiter character, which may or may not be an error
*		\n	 = end of line, which may or may not be an error
*
*	No overrun check is performed.
*/
char CSVUtils::ReadUint8(
	uint8_t*	outValue)
{
	uint8_t	value = 0;
	char		thisChar;

	while((thisChar = GetChar()) != 0)
	{
		if (thisChar != mDelimiter)
		{
			if (thisChar != '\r')
			{
				if (thisChar != '\n')
				{
					if (thisChar >= '0' &&
						thisChar <= '9')
					{
						value = (value*10) + (thisChar - '0');
						continue;
					}
					// Else thisChar is invalid
					thisChar = -1;
				}
			/*
			*	If the next character isn't a newline THEN
			*	this is an invalid character
			*/
			} else if ((thisChar = GetChar()) != 0 &&
				thisChar != '\n')
			{
				thisChar = -1;	// Invalid character, expected a newline
			}
		}
		break;
	}
	*outValue = value;

	return(thisChar);
}

/********************************* ReadUint16 *********************************/
/*
*	Consumes integer numbers till a delimiter, end of file, end of line or 
*	a non-number is hit.
*	Possible return values:
*		0	= end of file, which may or may not be an error
*		-1	= invalid character
*		delimiter character, which may or may not be an error
*		\n	 = end of line, which may or may not be an error
*
*	No overrun check is performed.
*/
char CSVUtils::ReadUint16(
	uint16_t*	outValue)
{
	uint16_t	value = 0;
	char		thisChar;

	while((thisChar = GetChar()) != 0)
	{
		if (thisChar != mDelimiter)
		{
			if (thisChar != '\r')
			{
				if (thisChar != '\n')
				{
					if (thisChar >= '0' &&
						thisChar <= '9')
					{
						value = (value*10) + (thisChar - '0');
						continue;
					}
					// Else thisChar is invalid
					thisChar = -1;
				}
			/*
			*	If the next character isn't a newline THEN
			*	this is an invalid character
			*/
			} else if ((thisChar = GetChar()) != 0 &&
				thisChar != '\n')
			{
				thisChar = -1;	// Invalid character, expected a newline
			}
		}
		break;
	}
	*outValue = value;

	return(thisChar);
}

/********************************** ReadStr ***********************************/
/*
*	Consumes string characters till a delimiter, end of file, end of line.
*	Characters are simply ignored once the max is reached.
*	Quotes are handled per CSV convention.
*
*	Possible return values:
*		0	= end of file, which may or may not be an error
*		-1	= invalid character
*		delimiter character, which may or may not be an error
*		\n	 = end of line, which may or may not be an error
*
*	Note that an unmatched quote can cause the entire file to be consumed.
*/
char CSVUtils::ReadStr(
	uint8_t	inMaxStrLen,
	char*	outStr)
{
	char		thisChar;
	char*		strPtr = outStr;
	char*		endOfStrPtr = &outStr[inMaxStrLen -1];

	while((thisChar = GetChar()) != 0)
	{
		if (thisChar != mDelimiter)
		{
			/*
			*	If the thisChar is not a quote...
			*/
			if (thisChar != '\"')
			{
				if (thisChar != '\r')
				{
					if (thisChar != '\n')
					{
						/*
						*	If outStr isn't full THEN
						*	append thisChar to it.
						*/
						if (strPtr < endOfStrPtr)
						{
							*(strPtr++) = thisChar;
						} // else discard the character, the outStr is full
						continue;
					}
				} else if ((thisChar = GetChar()) != 0 &&
					thisChar != '\n')
				{
					thisChar = -1;	// Invalid character, expected a newline
				}
			} else
			{
				/*
				*	Quoted text examples:
				*	""L"" T"xxx, = L"" T"xxx << quote loop stops after the first pair of quotes
				*	".""L"" T"xxx, = ."L" Txxx  << quote loop stops after the last quote
				*	."""L"" T"xxx, = ."""L"" T"xxx << never gets here because the first char is not a quote
				*/
				while((thisChar = GetChar()) != 0)
				{
					/*
					*	If thisChar is not a quote...
					*/
					if (thisChar != '\"')
					{
						/*
						*	If outStr isn't full THEN
						*	append thisChar to it.
						*/
						if (strPtr < endOfStrPtr)
						{
							*(strPtr++) = thisChar;
						} // else discard the character, the outStr is full
						continue;
					}
					if ((thisChar = GetChar()) != 0)
					{
						/*
						*	If the next character is also a quote...
						*/
						if (thisChar == '\"')
						{
							/*
							*	If outStr isn't full THEN
							*	append thisChar to it.
							*/
							if (strPtr < endOfStrPtr)
							{
								*(strPtr++) = thisChar;
							} // else discard the character, the outStr is full
							continue;
						}
					}
					// Else exit quote loop
					break;
				}
				continue;
			}
		}
		break;
	}
	*strPtr = 0;	// Terminate the string
	return(thisChar);
}

/********************************** SkipLine **********************************/
char CSVUtils::SkipLine(void)
{
	char	thisChar;
	while ((thisChar = GetChar()) != 0 && thisChar != '\n'){}
	return(thisChar);
}

/******************************** QuoteForCSV *********************************/
/*
*	To support CSV files, any string containing a delimiter or a quote is
*	wrapped in quotes.  Any quotes within the string are doubled.
*/
const char* CSVUtils::QuoteForCSV(
	const char*	inStr,
	char*		outStr)
{
	char	thisChar = *(inStr++);
	char*	outStrPtr = &outStr[1];	// start after the quote placeholder
	bool	needsToBeWrapped = false;
	for (; thisChar; thisChar = *(inStr++))
	{
		*(outStrPtr++) = thisChar;
		if (thisChar == '\"')
		{
			*(outStrPtr++) = thisChar;	// double the quote
			needsToBeWrapped = true;
		} else if (thisChar == mDelimiter)
		{
			needsToBeWrapped = true;
		}
	}
	if (needsToBeWrapped)
	{
		*outStr = '\"';			// fill in the start quote placeholder
		*(outStrPtr++) = '\"';	// close the quote
	} else
	{
		outStr++;				// Not wrapped, skip the quote placeholder
	}
	*(outStrPtr++) = 0;
	return(outStr);
}

