#ifndef _LPD_DECOMPRESSOR__H__
#define _LPD_DECOMPRESSOR__H__

//***********************************************************************************************************

#include "LPD_ALIASES.h"
#include "LPD_PNG.h"

//***********************************************************************************************************

#define local_function static 
#define callable

//***********************************************************************************************************

#define BITS_DEBUG 0
#define zTXt_DECOMPRESSION_DEBUG 0

//***********************************************************************************************************

static const char* fix = "fixed huffman";
static const char* dyn = "dynamic huffman";
static const char* noc = "no compression";

//***********************************************************************************************************

enum {
	NO_COMPRESSION    = 0,
	FIXED_HUFFMANN    = 1,
	DYNAMIC_HUFFMANN  = 2,
	ERROR_COMPRESSION = 3
};

//***********************************************************************************************************

static UINT_8 lengths_table_extra_bits[] = {
	0, 0, 0, 0, 0, 0, 0, 0, //257 - 264
	1, 1, 1, 1,             //265 - 268
	2, 2, 2, 2,             //269 - 273
	3, 3, 3, 3,             //274 - 276
	4, 4, 4, 4,             //278 - 280
	5, 5, 5, 5,             //281 - 284
	0                       //285
};

//***********************************************************************************************************

static UINT_32 lengths_table[] = {
	3, 4, 5, 6, 7, 8, 9, 10, //257 - 264
	11, 13, 15, 17,          //265 - 268
	19, 23, 27, 31,          //269 - 273
	35, 43, 51, 59,          //274 - 276
	67, 83, 99, 115,         //278 - 280
	131, 163, 195, 227,      //281 - 284
	258                      //285
};

//***********************************************************************************************************

static UINT_32 distance_table[] = {
	/*00*/ 1, 2, 3, 4,   //0-3
	/*01*/ 5, 7,         //4-5
	/*02*/ 9, 13,        //6-7
	/*03*/ 17, 25,       //8-9
	/*04*/ 33, 49,       //10-11
	/*05*/ 65, 97,       //12-13
	/*06*/ 129, 193,     //14-15
	/*07*/ 257, 385,     //16-17
	/*08*/ 513, 769,     //18-19
	/*09*/ 1025, 1537,   //20-21
	/*10*/ 2049, 3073,   //22-23
	/*11*/ 4097, 6145,   //24-25
	/*12*/ 8193, 12289,  //26-27
	/*13*/ 16385, 24577, //28-29
	0 , 0                //30-31 error, they can not be used
};

//***********************************************************************************************************

static UINT_32 distance_table_extra_bits[] = {
	/*00*/ 0, 0, 0, 0, //0-3
	/*01*/ 1, 1,       //4-5
	/*02*/ 2, 2,       //6-7
	/*03*/ 3, 3,       //8-9
	/*04*/ 4, 4,       //10-11
	/*05*/ 5, 5,       //12-13
	/*06*/ 6, 6,       //14-15
	/*07*/ 7, 7,       //16-17
	/*08*/ 8, 8,       //18-19
	/*09*/ 9, 9,       //20-21
	/*10*/ 10, 10,     //22-23
	/*11*/ 11, 11,     //24-25
	/*12*/ 12, 12,     //26-27
	/*13*/ 13, 13,     //28-29
	0 , 0              //30-31 error, they can not be used
};

//*********************************************************************************************************** 

typedef struct _LPD_BIT_STREAM {
	UINT_32* data32;
	UINT_32  dword_in_use;
	UINT_32  bits_remained;
	UINT_8   debug_byte_border;
} LPD_BIT_STREAM;

//***********************************************************************************************************

typedef struct _LPD_FIXED {
	UINT_32* literal_length_tree; //[288];
	UINT_32* distance_tree      ; //[30];
} LPD_FIXED;

//*********************************************************************************************************** 

local_function UINT_32  lpd_get_bits                              (LPD_BIT_STREAM*, UINT_8);
local_function UINT_8   lpd_maximum_of_bitlen_array               (UINT_8*, UINT_32);
local_function void     lpd_get_bit_length_count                  (UINT_32*, UINT_8*, UINT_32);
local_function void     lpd_first_code_for_bitlen                 (UINT_32*, UINT_32*, UINT_32);
local_function void     lpd_assign_huffman_code                   (UINT_32*, UINT_32*, UINT_8*, UINT_32);
local_function UINT_32* lpd_construct_huffman_tree                (UINT_8*, UINT_32);
local_function UINT_32  lpd_get_bits_reversed_without_consuming   (LPD_BIT_STREAM*, UINT_32);
local_function void     lpd_update_bit_stream                     (LPD_BIT_STREAM*, UINT_32);
local_function UINT_32  lpd_decode_huffman_tree                   (LPD_BIT_STREAM*, UINT_32*, UINT_8*, UINT_32);
local_function void     lpd_deflate_block                         (LPD_PNG*, LPD_BIT_STREAM*, UINT_32*, UINT_8*, UINT_32, UINT_32*, UINT_8*, UINT_32);
local_function void     lpd_decompress_zlib_buffer_dynamic        (LPD_PNG*, LPD_BIT_STREAM*);
local_function void     lpd_decompress_zlib_buffer_fixed          (LPD_PNG*, LPD_BIT_STREAM*);
local_function void     lpd_decompress_zlib_buffer_no_compression (LPD_PNG*, LPD_BIT_STREAM*);

callable       void     lpd_decompress_zlib_buffer                (LPD_PNG* png, UINT_8* compressed_data);
callable       void     lpd_compression_info                      (UINT_8* buffer);
callable       BOOL     lpd_is_adler32                            (UINT_8* buffer);

//***********************************************************************************************************

#endif // !_LPD_DECOMPRESSOR__H__
