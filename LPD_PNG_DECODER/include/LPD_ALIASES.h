#ifndef _LPD_ALIASES__H__
#define _LPD_ALIASES__H__

//***********************************************************************************************************

#include <stdio.h>
#include <stdlib.h>

//***********************************************************************************************************

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#pragma warning(disable:4244)

//***********************************************************************************************************

#define LPD_TRUE  1
#define LPD_FALSE 0

//***********************************************************************************************************

#define KB(X) ((X)   << 10)
#define MB(X) (KB(X) << 10)
#define GB(X) (MB(X) << 10)

//***********************************************************************************************************

#define LPD_UNDEFINED_MARKER   0xFF
#define LPD_ERROR_ID           0xFE
#define LPD_NULL               0x00
#define LPD_FILTER_TYPE_BYTE   0x01
#define LPD_ABS(NUMBER)        ((NUMBER >= 0) ? (NUMBER) : (-NUMBER))

//***********************************************************************************************************

#define LPD_ALIGN(ON, NUM) ((NUM + (ON - 1)) & (~(ON - 1)))
#define LPD_ALIGN_0x04(NUM) LPD_ALIGN(0x04, NUM)
#define LPD_ALIGN_0x08(NUM) LPD_ALIGN(0x08, NUM)
#define LPD_ALIGN_0x10(NUM) LPD_ALIGN(0x10, NUM)
#define LPD_ALIGN_0x20(NUM) LPD_ALIGN(0x20, NUM)
#define LPD_WINDOW_ALIGN(NUM) LPD_ALIGN_0x20(NUM)

//***********************************************************************************************************

typedef unsigned long long UINT_64;
typedef unsigned int       UINT_32;
typedef unsigned short     UINT_16;
typedef unsigned char      UINT_8;
typedef long long          INT_64;
typedef int                INT_32;
typedef short              INT_16;
typedef char               INT_8;
typedef unsigned int       BOOL;

//***********************************************************************************************************

typedef enum _LPD_CHUNK_ID {
	IHDR_ID = 0x00,
	IEND_ID = 0x01,
	IDAT_ID = 0x02,
	PLTE_ID = 0x03,
	sRGB_ID = 0x04,
	bKGD_ID = 0x05,
	cHRM_ID = 0x06,
	gAMA_ID = 0x07,
	sBIT_ID = 0x08,
	zTXt_ID = 0x09,
	tEXt_ID = 0x0A,
	tIME_ID = 0x0B,
	pHYs_ID = 0x0C,
	hIST_ID = 0x0D,
	UNDF_ID = 0x0E
} LPD_CHUNK_ID;

//***********************************************************************************************************

enum {
	GREYSCALE            = 0,
	TRUECOLOR            = 2,
	INDEXEDCOLOR         = 3,
	GREYSCALE_WITH_ALPHA = 4,
	TRUECOLOR_WITH_ALPHA = 6
};

//***********************************************************************************************************

#endif // !_LPD_ALIASES__H__
