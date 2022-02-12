#ifndef _LPD_DEFILTERIZER__H__
#define _LPD_DEFILTERIZER__H__

#include "LPD_PNG.h"
#include "LPD_ALIASES.h"

#define LPD_ABS(NUMBER) ((NUMBER >= 0) ? (NUMBER) : (-NUMBER))

typedef enum _FILTER_TYPE {
	FILTER_TYPE_NONE      = 0x00,
	FILTER_TYPE_SUB       = 0x01,
	FILTER_TYPE_UP        = 0x02,
	FILTER_TYPE_AVERAGE   = 0x03,
	FILTER_TYPE_PEATH     = 0x04,
	FILTER_TYPE_UNDEFINED = 0xFF
} FILTER_TYPE;

FILTER_TYPE lpd_defilter_row (UINT_8* row, UINT_8* output, UINT_32 byte_in_row, UINT_32 width, UINT_8 byte_per_pixel);
void        lpd_defilter     (void* in_buffer, void** out_buffer, UINT_32 in_buffer_size, UINT_32* total_output_bytes, UINT_8* byte_per_pixel, IHDR* ihdr);


#endif