#ifndef LPE_STRING_UNIT__H__
#define LPE_STRING_UNIT__H__

#include "LPE_ALIASES.h"

//***********************************************************************************************************

#define string_local static 
#define lpe_string_call

//*********************************************************************************************************** global functions

BOOL lpe_png_strcmp(const char* str1, const char* str2);
UINT_32 lpe_string_size(UINT_8* str);

//***********************************************************************************************************

#endif
