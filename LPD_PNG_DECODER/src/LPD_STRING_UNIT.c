//***********************************************************************************************************

#include "LPD_STRING_UNIT.h"

//***********************************************************************************************************

const char* marker_table[14] = { "IHDR", "IEND", "IDAT", "PLTE", "sRGB", "bKGD", "cHRM", "gAMA", "sBIT", "zTXt", "tEXt", "tIME", "pHYs", "hIST" };

//***********************************************************************************************************

static BOOL lpd_png_strcmp(const char* str1, const char* str2)
{
	UINT_8 sz = 0;
	while (sz < 4)
	{
		if (str1[sz] != str2[sz])
			return LPD_FALSE;
		sz++;
	}
	return LPD_TRUE;
}

//***********************************************************************************************************

UINT_8 lpd_which_png_marker(const char* str)
{
	UINT_8 marker_table_index = 0;
	while (marker_table_index < 14)
	{
		const char* trg_str = marker_table[marker_table_index];
		if (lpd_png_strcmp(str, trg_str))
			return marker_table_index;
		marker_table_index++;
	}
	return LPD_UNDEFINED_MARKER;
}

//***********************************************************************************************************