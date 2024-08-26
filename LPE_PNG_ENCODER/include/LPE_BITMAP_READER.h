#ifndef _LPE_BITMAP_READER__H__
#define _LPE_BITMAP_READER__H__

//***********************************************************************************************************

#include "LPE_ALIASES.h"

//***********************************************************************************************************

typedef enum _BITMAP_READER_ID {
	BMP_WIDTH,
	BMP_HEIGHT,
	BMP_COLORTYPE,
	BMP_BITDEPTH,
	BMP_BYTEPERPIXEL,
	BMP_DATA,
} BITMAP_READER_ID;

//***********************************************************************************************************

void    bitmap_reader_parse_bmp_file (const char* address_of_bmp);
UINT_32 bitmap_reader_tell_me        (BITMAP_READER_ID bmpid);

//***********************************************************************************************************

#endif // !_LPE_BITMAP_READER__H__
