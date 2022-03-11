#ifndef _LPE_STREAM_WRITER__H__
#define _LPE_STREAM_WRITER__H__

#include "LPE_ALIASES.h"
#include <iostream>

//***********************************************************************************************************

typedef struct _LPE_STREAM_WRITER {
	FILE*       file;
	const char* path_to_write;
} LPE_STREAM_WRITER;

//***********************************************************************************************************

#endif // !_LPE_STREAM_WRITER__H__
