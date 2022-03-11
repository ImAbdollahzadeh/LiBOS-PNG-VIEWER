#ifndef _LPE_FILTERIZER__H__
#define _LPE_FILTERIZER__H__

#include "LPE_ALIASES.h"

//***********************************************************************************************************

#define LPE_ABS(NUMBER) ((NUMBER >= 0) ? (NUMBER) : (-NUMBER))
#define LPE_FILTER_TYPE_BYTE 0x01

//***********************************************************************************************************

enum {
	LPE_NONE_FILTER    = 0x00000000,
	LPE_SUB_FILTER     = 0x00000001,
	LPE_UP_FILTER      = 0x00000002,
	LPE_AVERAGE_FILTER = 0x00000003,
	LPE_PEATH_FILTER   = 0x00000004,
};

//***********************************************************************************************************

void lpe_filterize_data(UINT_8* raw_data, UINT_8* filtered_data, UINT_32 width, UINT_32 height, UINT_8 byte_per_pixel);

//***********************************************************************************************************

#endif // !_LPE_FILTERIZER__H__
