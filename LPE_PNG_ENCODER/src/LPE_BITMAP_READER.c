//***********************************************************************************************************

#include "LPE_BITMAP_READER.h"
#include "LPE_PNG.h"

//***********************************************************************************************************

#pragma warning(disable:4996)
#pragma warning(disable:4244)
#define _CRT_SECURE_NO_WARNINGS

//***********************************************************************************************************

static UINT_32 __bmp_width          = 0;
static UINT_32 __bmp_height         = 0;
static UINT_8  __bmp_colortype      = 0;
static UINT_8  __bmp_bitdepth       = 0;
static UINT_8  __bmp_byte_per_pixel = 0;
static UINT_8* __bmp_data           = LPE_NULL;

//***********************************************************************************************************

void bitmap_reader_parse_bmp_file(const char* address_of_bmp)
{
	FILE* f = fopen(address_of_bmp, "rb");
	if (!f)
	{
		printf("provided bitmap image address is wrong --> fatal error\n");
		return;
	}

	unsigned char info[54];
	fread(info, sizeof(UINT_8), 54, f);
	__bmp_width          = *(UINT_32*)&info[18];
	__bmp_height         = *(UINT_32*)&info[22];
	__bmp_byte_per_pixel = __bmp_bitdepth = (*(UINT_8 *)&info[28]) >> 3;
	__bmp_colortype      = 2;
	UINT_32 size         = LPE_WINDOW_ALIGN((__bmp_width * __bmp_height * __bmp_byte_per_pixel));
	__bmp_data           = (UINT_8*)lpe_zero_allocation(size);
	fread(__bmp_data, sizeof(UINT_8), size, f);
	fclose(f);
}

//***********************************************************************************************************

UINT_32 bitmap_reader_tell_me(BITMAP_READER_ID bmpid)
{
	UINT_32 ret_val = 0;
	switch (bmpid)
	{
	case BMP_WIDTH:        ret_val = (UINT_32)__bmp_width;          break;
	case BMP_HEIGHT:       ret_val = (UINT_32)__bmp_height;         break;
	case BMP_BITDEPTH:     ret_val = (UINT_32)__bmp_bitdepth;       break;
	case BMP_COLORTYPE:    ret_val = (UINT_32)__bmp_colortype;      break;
	case BMP_BYTEPERPIXEL: ret_val = (UINT_32)__bmp_byte_per_pixel; break;
	case BMP_DATA:         ret_val = (UINT_32)__bmp_data;           break;
	}

	return ret_val;
}

//***********************************************************************************************************

