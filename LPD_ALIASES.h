#ifndef LPD_ALIASES__H__
#define LPD_ALIASES__H__

#define KB(X) ((X)   << 10)
#define MB(X) (KB(X) << 10)
#define GB(X) (MB(X) << 10)

#define UNDEFINED_MARKER_FOUND 0xFF
#define ERROR_ID               0xFE
#define LPD_NULL               0x00
#define FILTER_TYPE_BYTE       0x01

#define ALIGN_4(M)             ((M+3)&(~3))

typedef unsigned long long UINT_64;
typedef unsigned int       UINT_32;
typedef unsigned short     UINT_16;
typedef unsigned char      UINT_8;
typedef long long          INT_64;
typedef int                INT_32;
typedef short              INT_16;
typedef char               INT_8;

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

enum {
	GREYSCALE            = 0,
	TRUECOLOR            = 2,
	INDEXEDCOLOR         = 3,
	GREYSCALE_WITH_ALPHA = 4,
	TRUECOLOR_WITH_ALPHA = 6
};

#endif