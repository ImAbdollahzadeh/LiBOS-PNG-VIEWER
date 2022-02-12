#ifndef _LPD_PNG__H__
#define _LPD_PNG__H__

#include "LPD_ALIASES.h"

#pragma pack(push, 1)
typedef struct _IHDR {
	UINT_32 width;
	UINT_32 height;
	UINT_8  bit_depth;
	UINT_8  color_type;
	UINT_8  compression_method;
	UINT_8  filter_method;
	UINT_8  interlace_method;
} IHDR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _IDAT {
	UINT_8* data;
} IDAT;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _PLTE {
	UINT_8 red;
	UINT_8 green;
	UINT_8 blue;
} PLTE;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _sRGB {
	UINT_8 rendering_intent;
} sRGB;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _bKGD {
	UINT_8  palette_index;
	UINT_16 gray;
	UINT_16 red;
	UINT_16 green;
	UINT_16 blue;
} bKGD;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _cHRM {
	UINT_32 white_Point_x;
	UINT_32 white_Point_y;
	UINT_32 red_x;
	UINT_32 red_y;
	UINT_32 green_x;
	UINT_32 green_y;
	UINT_32 blue_x;
	UINT_32 blue_y;
} cHRM;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _gAMA {
	UINT_32 image_gamma;
} gAMA;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _sBIT {
	UINT_8  color_type;
	UINT_8* significant_bits;
} sBIT;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _zTXt {
	UINT_8* compressed_buffer;
	// 1-79 bytes -> keyword
	// 1 byte -> null seperator
	// 1 byte -> compression method (only 0 (a.k.a. DEFLATE-INFLATE) is valid)
	// N bytes -> compressed text data stream
} zTXt;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _tEXt {
	UINT_8* uncompressed_buffer;
	// 1-79 bytes -> keyword
	// 1 byte -> null seperator
	// Nbytes -> text
} tEXt;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _tIME {
	UINT_16 year;
	UINT_8  month;
	UINT_8  day;
	UINT_8  hour;
	UINT_8  minute;
	UINT_8  second;
} tIME;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _pHYs {
	UINT_32 pixels_per_unit_x;
	UINT_32 pixels_per_unit_y;
	UINT_8  unit_specifier;
} pHYs;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _tRNS {
	UINT_8* alphas;
} tRNS;
#pragma pack(pop)

typedef struct _hIST {
	UINT_16* serie;
} hIST;

#pragma pack(push, 1)
typedef struct _Chunk {
	UINT_32      data_length;
	LPD_CHUNK_ID chunk_type;
	UINT_8*      chunk_data;
	UINT_32      crc;
} Chunk;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _LPD_PNG {
	UINT_8  signature[8];
	UINT_8  number_of_idats;
	UINT_8  number_of_texts;
	UINT_8  number_of_ztxts;
	UINT_8  number_of_chunks;
	Chunk*  chunks_table;
	IDAT*   idat_buffer;
	tEXt*   text_buffer;
	zTXt*   ztxt_buffer;
	UINT_32 idat_buffer_size;
	UINT_32 text_buffer_size;
	UINT_32 ztxt_buffer_size;
	UINT_8* decompressed_data_buffer;
	UINT_32 decompressed_data_counter;
	UINT_32 total_size;
} LPD_PNG;
#pragma pack(pop)

UINT_32 lpd_png_find_size                        (const char* file_address);
bool    lpd_read_png_file                        (LPD_PNG* png, const char* file_address);
void*   lpd_buffer_reallocation                  (void* old_pointer, UINT_32 old_size, UINT_32 new_extra_size);
void*   lpd_zero_allocation                      (UINT_32 byte_size);
void    lpd_free_allocated_mem                   (void* mem);
void    lpd_chunk_table_report                   (LPD_PNG* png);
void    lpd_idat_text_ztxt_chunks_counter_report (LPD_PNG* png);
void    lpd_clean_up_the_png                     (LPD_PNG* png);

#endif // !_LPD_PNG__H__




