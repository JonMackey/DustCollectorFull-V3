// Subset font representing icons used in the project.
// The icons correspond to characters starting with the glyph 'A'.
// A = first icon, B = second, etc.

#ifndef DC_Icons_h
#define DC_Icons_h

#include "XFontGlyph.h"
#include "XFont16BitDataStream.h"

namespace DC_Icons
{
	const FontHeader	fontHeader PROGMEM =
	{
		1,		// version, currently version = 1
		0,		// oneBit, 1 = 1 bit per pixel, 0 = 8 bit (antialiased)
		0,		// rotated, glyph data is rotated (applies to 1 bit only)
		0,		// wideOffsets, 1 = 32 bit, 0 = 16 bit glyph data offsets
		0,		// monospaced, fixed width font (for this subset)
		32,		// ascent, font in pixels
		0,		// descent, font in pixels
		32,		// height, font height (ascent+descent+leading) in pixels
		32,		// width, widest glyph advanceX within subset in pixels
		2,		// numCharcodeRuns
		2		// numCharCodes
	};
	
	// 'A' = Fan 90
	// 'B' = Fan 45
	const CharcodeRun	charcodeRun[] PROGMEM = // {start, entryIndex}, ...
	{
		{0x0041, 0}, {0xFFFF, 2}
	};
	
	const uint16_t	glyphDataOffset[] PROGMEM =
	{				//+0x02EA =
		0x0000, 0x02E3, 0x5CD
	};

	const uint8_t	glyphData[] PROGMEM =
	{
		0x20, 0x00, 0x00, 0x20, 0x20, 0x0A, 0x00, 0xF4, 0x05, 0x22, 0x4D, 0x69,
		0x74, 0x7E, 0x7E, 0x74, 0x69, 0x4D, 0x22, 0x05, 0x13, 0x00, 0xF0, 0x0B,
		0x41, 0x68, 0x6B, 0x58, 0x40, 0x2E, 0x26, 0x26, 0x2E, 0x41, 0x5B, 0x6F,
		0x6B, 0x41, 0x0B, 0x0E, 0x00, 0xFA, 0x02, 0x38, 0x72, 0x66, 0x2B, 0x06,
		0x08, 0x00, 0xFA, 0x07, 0x2D, 0x67, 0x76, 0x38, 0x02, 0x0B, 0x00, 0xFB,
		0x0A, 0x5A, 0x74, 0x2A, 0x01, 0x0C, 0x00, 0xFB, 0x01, 0x2A, 0x78, 0x5F,
		0x0A, 0x09, 0x00, 0xFC, 0x0F, 0x6B, 0x5F, 0x0A, 0x0B, 0x00, 0xF7, 0x08,
		0x38, 0x68, 0x6F, 0x4F, 0x22, 0x66, 0x6E, 0x0F, 0x07, 0x00, 0xFA, 0x0A,
		0x6B, 0x57, 0x03, 0x06, 0x02, 0x09, 0x00, 0xF5, 0x23, 0x9D, 0xEF, 0xFF,
		0xFF, 0xFB, 0xC0, 0x29, 0x5B, 0x70, 0x0A, 0x05, 0x00, 0xF8, 0x01, 0x5D,
		0x60, 0x35, 0x99, 0xB9, 0x96, 0x31, 0x07, 0x00, 0xFE, 0x27, 0xC5, 0x06,
		0xFF, 0xFB, 0x7D, 0x02, 0x6A, 0x5F, 0x01, 0x04, 0x00, 0xFC, 0x39, 0x76,
		0x26, 0xC9, 0x03, 0xFF, 0xFE, 0xCF, 0x23, 0x05, 0x00, 0xFE, 0x0D, 0xAB,
		0x07, 0xFF, 0xFB, 0x98, 0x00, 0x0F, 0x80, 0x39, 0x03, 0x00, 0xFB, 0x0A,
		0x78, 0x2C, 0x50, 0xFA, 0x04, 0xFF, 0xFF, 0x78, 0x05, 0x00, 0xFE, 0x5A,
		0xF8, 0x07, 0xFF, 0xF4, 0x76, 0x00, 0x00, 0x36, 0x7E, 0x0A, 0x00, 0x00,
		0x45, 0x70, 0x00, 0x6E, 0x05, 0xFF, 0xFE, 0xC4, 0x10, 0x03, 0x00, 0xFE,
		0x0B, 0xB7, 0x06, 0xFF, 0xF1, 0xFD, 0xB9, 0x1E, 0x00, 0x00, 0x03, 0x79,
		0x45, 0x00, 0x04, 0x70, 0x32, 0x00, 0x5E, 0xFE, 0x04, 0xFF, 0xFE, 0xF5,
		0x4B, 0x03, 0x00, 0xFE, 0x32, 0xEC, 0x04, 0xFF, 0xFC, 0xEE, 0xB6, 0x66,
		0x17, 0x04, 0x00, 0xF7, 0x38, 0x75, 0x04, 0x22, 0x79, 0x09, 0x00, 0x2C,
		0xE5, 0x05, 0xFF, 0xF4, 0xB0, 0x0C, 0x00, 0x00, 0x51, 0xFB, 0xFF, 0xFF,
		0xF1, 0x9C, 0x3A, 0x09, 0x06, 0x00, 0xF6, 0x0D, 0x83, 0x22, 0x50, 0x67,
		0x00, 0x00, 0x03, 0x89, 0xFE, 0x04, 0xFF, 0xF6, 0xF9, 0x77, 0x10, 0x00,
		0x45, 0xF6, 0xFF, 0xED, 0x5C, 0x05, 0x09, 0x00, 0xFC, 0x70, 0x50, 0x73,
		0x4E, 0x03, 0x00, 0xFD, 0x16, 0xAD, 0xFE, 0x04, 0xFF, 0xF8, 0xF6, 0xC2,
		0x27, 0x10, 0xA0, 0xF1, 0xA9, 0x07, 0x0A, 0x00, 0xFC, 0x56, 0x75, 0x83,
		0x39, 0x04, 0x00, 0xFD, 0x17, 0x8F, 0xED, 0x04, 0xFF, 0xFA, 0xEC, 0x63,
		0x9C, 0xA2, 0x56, 0x1C, 0x0B, 0x00, 0xFC, 0x41, 0x88, 0x8B, 0x31, 0x05,
		0x00, 0xF0, 0x05, 0x3D, 0x9B, 0xDB, 0xF1, 0xED, 0x8F, 0xA8, 0xFF, 0xFF,
		0x85, 0x17, 0x4E, 0x54, 0x32, 0x0A, 0x07, 0x00, 0xFC, 0x34, 0x8E, 0x8D,
		0x31, 0x07, 0x00, 0xF0, 0x04, 0x1D, 0x36, 0x2E, 0x0A, 0x94, 0xFF, 0xFF,
		0x95, 0xB6, 0xFC, 0xFC, 0xEC, 0xB2, 0x52, 0x0A, 0x05, 0x00, 0xFC, 0x35,
		0x8E, 0x85, 0x3C, 0x0A, 0x00, 0xF9, 0x01, 0x38, 0x70, 0x99, 0x7A, 0x66,
		0xF8, 0x04, 0xFF, 0xFD, 0xF6, 0xA1, 0x1F, 0x04, 0x00, 0xFC, 0x44, 0x8A,
		0x77, 0x54, 0x0A, 0x00, 0xF8, 0x14, 0xCA, 0xFC, 0xA6, 0x0D, 0x29, 0xAF,
		0xF1, 0x05, 0xFF, 0xFE, 0xB6, 0x18, 0x03, 0x00, 0xFC, 0x5D, 0x77, 0x50,
		0x72, 0x09, 0x00, 0xF6, 0x14, 0x8B, 0xFA, 0xFF, 0xF0, 0x37, 0x00, 0x06,
		0x71, 0xF8, 0x05, 0xFF, 0xF7, 0x88, 0x02, 0x00, 0x00, 0x7C, 0x50, 0x22,
		0x87, 0x0F, 0x05, 0x00, 0xF3, 0x02, 0x1D, 0x63, 0xC4, 0xFC, 0xFF, 0xFF,
		0xF2, 0x39, 0x00, 0x00, 0x0D, 0xB2, 0x05, 0xFF, 0xF7, 0xDF, 0x24, 0x00,
		0x12, 0x8F, 0x22, 0x04, 0x7E, 0x3F, 0x03, 0x00, 0xFB, 0x01, 0x32, 0x91,
		0xD8, 0xFB, 0x04, 0xFF, 0xFE, 0xDA, 0x1D, 0x03, 0x00, 0xFE, 0x51, 0xF7,
		0x04, 0xFF, 0xF2, 0xFA, 0x4E, 0x00, 0x46, 0x7F, 0x04, 0x00, 0x45, 0x82,
		0x05, 0x00, 0x00, 0x3A, 0xDC, 0x07, 0xFF, 0xFE, 0x95, 0x01, 0x03, 0x00,
		0xFE, 0x14, 0xCA, 0x04, 0xFF, 0xF3, 0xFD, 0x56, 0x02, 0x8A, 0x45, 0x00,
		0x00, 0x0A, 0x88, 0x42, 0x00, 0x00, 0x96, 0x07, 0xFF, 0xFE, 0xE8, 0x37,
		0x05, 0x00, 0xFF, 0x7F, 0x04, 0xFF, 0xFB, 0xEF, 0x35, 0x46, 0x8D, 0x0A,
		0x03, 0x00, 0xFB, 0x39, 0x94, 0x17, 0x02, 0xAD, 0x06, 0xFF, 0xFD, 0xFD,
		0x84, 0x02, 0x05, 0x00, 0xFE, 0x23, 0xC9, 0x03, 0xFF, 0xFC, 0xA4, 0x26,
		0x9A, 0x39, 0x04, 0x00, 0xFB, 0x01, 0x68, 0x83, 0x09, 0x86, 0x05, 0xFF,
		0xFD, 0xFB, 0x9D, 0x11, 0x07, 0x00, 0xF8, 0x25, 0x7A, 0x98, 0x6E, 0x22,
		0x89, 0x68, 0x01, 0x05, 0x00, 0xF5, 0x0A, 0x7D, 0x77, 0x2C, 0xB0, 0xF1,
		0xFB, 0xF7, 0xD6, 0x71, 0x0E, 0x0B, 0x00, 0xFC, 0x0B, 0x82, 0x7F, 0x0A,
		0x07, 0x00, 0xF7, 0x0F, 0x7F, 0x85, 0x25, 0x34, 0x4E, 0x43, 0x1B, 0x01,
		0x0B, 0x00, 0xFC, 0x1D, 0x8D, 0x7F, 0x0F, 0x09, 0x00, 0xFB, 0x0A, 0x68,
		0x98, 0x47, 0x06, 0x0C, 0x00, 0xFB, 0x08, 0x4D, 0xA0, 0x68, 0x0A, 0x0B,
		0x00, 0xF9, 0x02, 0x38, 0x90, 0x92, 0x4B, 0x15, 0x02, 0x06, 0x00, 0xF9,
		0x02, 0x16, 0x4D, 0x93, 0x92, 0x38, 0x02, 0x0E, 0x00, 0xF0, 0x0B, 0x45,
		0x84, 0x98, 0x87, 0x69, 0x51, 0x42, 0x43, 0x52, 0x6B, 0x89, 0x99, 0x86,
		0x45, 0x0B, 0x12, 0x00, 0xF4, 0x05, 0x22, 0x50, 0x7B, 0x93, 0x9A, 0x9A,
		0x93, 0x7B, 0x50, 0x22, 0x05, 0x0A, 0x00,

		0x20, 0x00, 0x00, 0x20, 0x20, 0x0A, 0x00, 0xF4, 0x05, 0x22, 0x50, 0x73,
		0x83, 0x8C, 0x8C, 0x84, 0x74, 0x50, 0x22, 0x05, 0x13, 0x00, 0xF0, 0x0B,
		0x44, 0x75, 0x7E, 0x6A, 0x4D, 0x41, 0x35, 0x2F, 0x3B, 0x51, 0x6D, 0x82,
		0x78, 0x44, 0x0B, 0x0E, 0x00, 0xEC, 0x02, 0x38, 0x7E, 0x77, 0x36, 0x0A,
		0x0C, 0x64, 0xB1, 0xA7, 0x43, 0x01, 0x00, 0x00, 0x0C, 0x38, 0x79, 0x83,
		0x38, 0x02, 0x0B, 0x00, 0xF3, 0x0A, 0x61, 0x82, 0x35, 0x03, 0x00, 0x0F,
		0x9C, 0xFB, 0xFF, 0xFF, 0xE6, 0x43, 0x04, 0x00, 0xFB, 0x04, 0x3A, 0x89,
		0x63, 0x0A, 0x09, 0x00, 0xFC, 0x0F, 0x73, 0x6D, 0x10, 0x03, 0x00, 0xFE,
		0x70, 0xFC, 0x04, 0xFF, 0xFE, 0xA0, 0x02, 0x05, 0x00, 0xFC, 0x11, 0x74,
		0x76, 0x0F, 0x07, 0x00, 0xFC, 0x0A, 0x73, 0x64, 0x05, 0x03, 0x00, 0xFE,
		0x11, 0xC8, 0x05, 0xFF, 0xFE, 0xAE, 0x04, 0x06, 0x00, 0xFC, 0x07, 0x6A,
		0x76, 0x0A, 0x05, 0x00, 0xFC, 0x01, 0x62, 0x6D, 0x05, 0x04, 0x00, 0xFE,
		0x2E, 0xEB, 0x05, 0xFF, 0xFF, 0x7A, 0x08, 0x00, 0xFC, 0x07, 0x76, 0x65,
		0x01, 0x04, 0x00, 0xFD, 0x39, 0x82, 0x10, 0x05, 0x00, 0xFE, 0x3F, 0xF5,
		0x04, 0xFF, 0xFE, 0xE0, 0x29, 0x09, 0x00, 0xFD, 0x13, 0x89, 0x39, 0x03,
		0x00, 0xFD, 0x0A, 0x7D, 0x36, 0x06, 0x00, 0xFE, 0x35, 0xF0, 0x04, 0xFF,
		0xFE, 0x95, 0x02, 0x0A, 0x00, 0xF8, 0x3B, 0x82, 0x0A, 0x00, 0x00, 0x45,
		0x73, 0x02, 0x06, 0x00, 0xFE, 0x19, 0xD5, 0x03, 0xFF, 0xFE, 0xF5, 0x47,
		0x0B, 0x00, 0xF9, 0x03, 0x7D, 0x45, 0x00, 0x04, 0x73, 0x36, 0x07, 0x00,
		0xFE, 0x02, 0x9A, 0x03, 0xFF, 0xFE, 0xDB, 0x1C, 0x04, 0x00, 0xF2, 0x05,
		0x20, 0x3C, 0x40, 0x33, 0x13, 0x01, 0x00, 0x3D, 0x7B, 0x04, 0x22, 0x7C,
		0x0B, 0x08, 0x00, 0xE9, 0x49, 0xF4, 0xFF, 0xFF, 0xD5, 0x19, 0x00, 0x00,
		0x11, 0x58, 0xA8, 0xDE, 0xF3, 0xF6, 0xEE, 0xCB, 0x6F, 0x0C, 0x0E, 0x87,
		0x22, 0x50, 0x6A, 0x09, 0x00, 0xF6, 0x0B, 0xA5, 0xFF, 0xFF, 0xF9, 0x53,
		0x00, 0x29, 0xB5, 0xFA, 0x06, 0xFF, 0xF4, 0xFB, 0x91, 0x08, 0x73, 0x50,
		0x71, 0x4F, 0x00, 0x00, 0x06, 0x0A, 0x02, 0x05, 0x00, 0xF9, 0x1F, 0xA4,
		0xDE, 0xB5, 0x2A, 0x0D, 0xB6, 0x09, 0xFF, 0xF3, 0xF7, 0x55, 0x54, 0x75,
		0x82, 0x39, 0x00, 0x4B, 0xB0, 0xC1, 0x8E, 0x38, 0x06, 0x04, 0x00, 0xFA,
		0x07, 0x46, 0xA8, 0x94, 0x4B, 0xE6, 0x0A, 0xFF, 0xF9, 0xA1, 0x43, 0x88,
		0x8B, 0x2F, 0x46, 0xE9, 0x03, 0xFF, 0xF0, 0xEC, 0xA8, 0x58, 0x27, 0x21,
		0x63, 0x35, 0x8F, 0xFF, 0xFF, 0x98, 0xBB, 0xF7, 0xCD, 0xD7, 0xF4, 0x06,
		0xFF, 0xFA, 0x94, 0x34, 0x8D, 0x8B, 0x34, 0xA2, 0x06, 0xFF, 0xE6, 0xFA,
		0xE6, 0xDF, 0xFD, 0xBF, 0x97, 0xFF, 0xFF, 0x7C, 0x29, 0x4B, 0x12, 0x19,
		0x44, 0x93, 0xE0, 0xFE, 0xFF, 0xFF, 0xD9, 0x32, 0x33, 0x8F, 0x84, 0x40,
		0xA5, 0x0A, 0xFF, 0xFA, 0xDF, 0x3D, 0x80, 0x96, 0x45, 0x0E, 0x04, 0x00,
		0xF3, 0x02, 0x27, 0x79, 0xAD, 0x96, 0x34, 0x00, 0x44, 0x8A, 0x73, 0x50,
		0x54, 0xF6, 0x09, 0xFF, 0xF9, 0xA0, 0x06, 0x39, 0xC9, 0xEB, 0xB4, 0x27,
		0x06, 0x00, 0xF5, 0x03, 0x01, 0x00, 0x00, 0x5C, 0x77, 0x50, 0x6F, 0x06,
		0x86, 0xF7, 0x06, 0xFF, 0xF6, 0xF0, 0x9A, 0x1A, 0x00, 0x57, 0xF9, 0xFF,
		0xFF, 0xAC, 0x0B, 0x09, 0x00, 0xE9, 0x79, 0x50, 0x22, 0x86, 0x0E, 0x08,
		0x5B, 0xB9, 0xE2, 0xEE, 0xE8, 0xCB, 0x8D, 0x3D, 0x07, 0x00, 0x00, 0x1B,
		0xDA, 0xFF, 0xFF, 0xF5, 0x4C, 0x08, 0x00, 0xF2, 0x11, 0x8D, 0x22, 0x04,
		0x7B, 0x3D, 0x00, 0x00, 0x0A, 0x22, 0x31, 0x28, 0x11, 0x01, 0x04, 0x00,
		0xFE, 0x24, 0xE3, 0x03, 0xFF, 0xFE, 0x9B, 0x02, 0x07, 0x00, 0xF9, 0x45,
		0x80, 0x04, 0x00, 0x45, 0x82, 0x04, 0x0B, 0x00, 0xFE, 0x55, 0xFA, 0x03,
		0xFF, 0xFE, 0xD4, 0x17, 0x06, 0x00, 0xF8, 0x06, 0x8A, 0x45, 0x00, 0x00,
		0x0A, 0x87, 0x40, 0x0A, 0x00, 0xFE, 0x06, 0xA7, 0x04, 0xFF, 0xFE, 0xEC,
		0x2E, 0x06, 0x00, 0xFD, 0x48, 0x8D, 0x0A, 0x03, 0x00, 0xFD, 0x39, 0x91,
		0x16, 0x09, 0x00, 0xFE, 0x38, 0xEB, 0x04, 0xFF, 0xFE, 0xEF, 0x32, 0x05,
		0x00, 0xFD, 0x1A, 0x98, 0x39, 0x04, 0x00, 0xFC, 0x01, 0x67, 0x81, 0x0A,
		0x08, 0x00, 0xFF, 0x8C, 0x05, 0xFF, 0xFE, 0xE5, 0x26, 0x04, 0x00, 0xFC,
		0x0C, 0x86, 0x68, 0x01, 0x05, 0x00, 0xFC, 0x0A, 0x7C, 0x78, 0x0A, 0x06,
		0x00, 0xFE, 0x09, 0xBC, 0x05, 0xFF, 0xFE, 0xB9, 0x0A, 0x03, 0x00, 0xFC,
		0x0C, 0x7E, 0x7E, 0x04, 0x07, 0x00, 0xFC, 0x0F, 0x7C, 0x85, 0x19, 0x05,
		0x00, 0xFE, 0x03, 0xA4, 0x04, 0xFF, 0xFE, 0xF5, 0x5A, 0x03, 0x00, 0xFC,
		0x1B, 0x89, 0x7F, 0x0A, 0x09, 0x00, 0xFB, 0x0A, 0x68, 0x9A, 0x49, 0x07,
		0x04, 0x00, 0xF3, 0x3D, 0xDF, 0xFF, 0xFF, 0xF3, 0x81, 0x08, 0x00, 0x08,
		0x4D, 0x9D, 0x69, 0x02, 0x0B, 0x00, 0xEC, 0x02, 0x38, 0x8E, 0x90, 0x4B,
		0x15, 0x02, 0x00, 0x00, 0x36, 0x90, 0x96, 0x47, 0x06, 0x15, 0x4D, 0x93,
		0x91, 0x38, 0x02, 0x0E, 0x00, 0xF0, 0x08, 0x45, 0x85, 0x97, 0x85, 0x68,
		0x51, 0x3F, 0x42, 0x52, 0x68, 0x88, 0x98, 0x86, 0x45, 0x08, 0x12, 0x00,
		0xF5, 0x04, 0x22, 0x50, 0x7B, 0x93, 0x9A, 0x9A, 0x93, 0x7B, 0x50, 0x22,
		0x0B, 0x00
};
	
	// Usage: Copy the 3 commented lines to your code, removing the comments.
	//#include "XFont.h"
	//XFont xFont;
	//#include "DC_Icons.h"
	
	// Leave the next 3 lines here, as is.
	DataStream_P	dataStream(glyphData, sizeof(glyphData));
	XFont16BitDataStream xFontDataStream(&xFont, &dataStream);
	XFont::Font font(&fontHeader, charcodeRun, glyphDataOffset, &xFontDataStream);
	
	// The display needs to be set before using xFont.  This only needs
	// to be done once at the beginning of the program.
	// Use xFont.SetDisplay(&display, &DC_Icons::font); to do this.
	// To change to this font anywhere after setting the display,
	// use: xFont.SetFont(&DC_Icons::font);
}

#endif // DC_Icons_h

