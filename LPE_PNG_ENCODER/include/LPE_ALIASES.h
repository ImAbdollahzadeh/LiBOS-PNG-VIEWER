#ifndef _LPE_ALIASES__H__
#define _LPE_ALIASES__H__

//***********************************************************************************************************

#include <stdio.h>

//***********************************************************************************************************

#define KB(X) ((X)   << 10)
#define MB(X) (KB(X) << 10)
#define GB(X) (MB(X) << 10)

//***********************************************************************************************************

#define UNDEFINED_MARKER_FOUND 0xFF
#define ERROR_ID               0xFE
#define LPE_NULL               0x00
#define FILTER_TYPE_BYTE       0x01

//***********************************************************************************************************

#define ALIGN_4(M) ((M+3)&(~3))

//***********************************************************************************************************

#define PHYSICAL_ADDRESS_32(PTR) (UINT_32)((void*)PTR)

//***********************************************************************************************************

#define LPE_OUTPUT_ADDRESS "_LPE_OUTPUT_FILE_.png"

//***********************************************************************************************************

#define LPE_SWAP_TWO_OBJECTS(LPE_TYPE, OBJ_PTR_1, OBJ_PTR_2) do {  \
	LPE_TYPE __LPE_TYPE_OBJECT__;                              \
	memcpy(&__LPE_TYPE_OBJECT__, OBJ_PTR_1, sizeof(LPE_TYPE)); \
	memcpy(OBJ_PTR_1, OBJ_PTR_2, sizeof(LPE_TYPE));            \
	memcpy(OBJ_PTR_2, &__LPE_TYPE_OBJECT__, sizeof(LPE_TYPE)); \
} while(0)

//***********************************************************************************************************

typedef unsigned long long UINT_64;
typedef unsigned int       UINT_32;
typedef unsigned short     UINT_16;
typedef unsigned char      UINT_8;
typedef long long          INT_64;
typedef int                INT_32;
typedef short              INT_16;
typedef char               INT_8;

//***********************************************************************************************************

typedef enum _LPE_CHUNK_ID {
	IHDR_ID = 0x00000000,
	IEND_ID = 0x00000001,
	IDAT_ID = 0x00000002,
	PLTE_ID = 0x00000003,
	sRGB_ID = 0x00000004,
	bKGD_ID = 0x00000005,
	cHRM_ID = 0x00000006,
	gAMA_ID = 0x00000007,
	sBIT_ID = 0x00000008,
	zTXt_ID = 0x00000009,
	tEXt_ID = 0x0000000A,
	tIME_ID = 0x0000000B,
	pHYs_ID = 0x0000000C,
	hIST_ID = 0x0000000D,
	UNDF_ID = 0x0000000E
} LPE_CHUNK_ID;

enum {
	GREYSCALE            = 0,
	TRUECOLOR            = 2,
	INDEXEDCOLOR         = 3,
	GREYSCALE_WITH_ALPHA = 4,
	TRUECOLOR_WITH_ALPHA = 6
};

//***********************************************************************************************************

#endif // !_LPE_ALIASES__H__
