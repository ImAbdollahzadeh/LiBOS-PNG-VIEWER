#ifndef _LPE_PNG__H__
#define _LPE_PNG__H__

//***********************************************************************************************************

#include "LPE_ALIASES.h"
#include "LPE_STREAM_WRITER.h"

//***********************************************************************************************************

#define LPE_CRC32_POLYNOMIAL_GENERATOR 0xEDB88320
#define LPE_IDAT_DATA_SIZE             KB(16)

//***********************************************************************************************************

typedef enum _LPE_MARKER {
	LPE_IHDR = 'IHDR',
	LPE_IEND = 'IEND',
	LPE_IDAT = 'IDAT',
	LPE_PLTE = 'PLTE',
	LPE_sRGB = 'sRGB',
	LPE_bKGD = 'bKGD',
	LPE_cHRM = 'cHRM',
	LPE_gAMA = 'gAMA',
	LPE_sBIT = 'sBIT',
	LPE_zTXt = 'zTXt',
	LPE_tEXt = 'tEXt',
	LPE_tIME = 'tIME',
	LPE_pHYs = 'pHYs',
	LPE_hIST = 'hIST',
} LPE_MARKER;

//***********************************************************************************************************

static const char* lpe_writer_comment = "Created by LiBOS_PNG_ENCODER (LPE) written by Iman Abdollahzadeh";
static const char* lpe_keyword        = "Information: ";

//***********************************************************************************************************

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

//***********************************************************************************************************

#pragma pack(push, 1)
typedef struct _IDAT {
	UINT_8* data;
} IDAT;
#pragma pack(pop)

//***********************************************************************************************************

#pragma pack(push, 1)
typedef struct _tEXt {
	UINT_8* keyword;
	UINT_8  null_seperator;
	UINT_8* string;
} tEXt;
#pragma pack(pop)

//***********************************************************************************************************

#pragma pack(push, 1)
typedef struct _IEND { 
	UINT_32 nothing;
} IEND;
#pragma pack(pop)

//***********************************************************************************************************

#pragma pack(push, 1)
typedef struct _LPE_CHUNK {
	UINT_32    data_length;
	LPE_MARKER chunk_type;
	UINT_8*    chunk_data;
	UINT_32    crc;
} LPE_CHUNK;
#pragma pack(pop)

//***********************************************************************************************************

typedef struct _LPE_PNG {
	UINT_8      signature[8];
	UINT_8      number_of_chunks;
	LPE_CHUNK*  chunks_table;
	UINT_8*     compressed_data_buffer;
	UINT_32     compressed_data_counter;
	UINT_32     total_size;
} LPE_PNG;

//*********************************************************************************************************** global functions

UINT_32 lpe_reverse_a_dword                                  (UINT_32 dword);
UINT_16 lpe_reverse_a_word                                   (UINT_16 word);
void    lpe_signature_writer                                 (LPE_STREAM_WRITER* stw);
void    lpe_ihdr_writer                                      (LPE_STREAM_WRITER* stw, IHDR* ihdr);
void    lpe_idat_writer                                      (LPE_STREAM_WRITER* stw, IDAT* idat);
void    lpe_text_writer                                      (LPE_STREAM_WRITER* stw, tEXt* text);
void    lpe_iend_writer                                      (LPE_STREAM_WRITER* stw, IEND* iend);
void    lpe_plte_writer                                      (LPE_STREAM_WRITER* stw, LPE_CHUNK* ch);
void    lpe_srgb_writer                                      (LPE_STREAM_WRITER* stw, LPE_CHUNK* ch);
void    lpe_ztxt_writer                                      (LPE_STREAM_WRITER* stw, LPE_CHUNK* ch);
void    lpe_sbit_writer                                      (LPE_STREAM_WRITER* stw, LPE_CHUNK* ch);
void    lpe_bkgd_writer                                      (LPE_STREAM_WRITER* stw, LPE_CHUNK* ch);
void    lpe_gama_writer                                      (LPE_STREAM_WRITER* stw, LPE_CHUNK* ch);
void    lpe_hist_writer                                      (LPE_STREAM_WRITER* stw, LPE_CHUNK* ch);
void    lpe_chrm_writer                                      (LPE_STREAM_WRITER* stw, LPE_CHUNK* ch);
void    lpe_time_writer                                      (LPE_STREAM_WRITER* stw, LPE_CHUNK* ch);
void    lpe_phys_writer                                      (LPE_STREAM_WRITER* stw, LPE_CHUNK* ch);
void*   lpe_buffer_reallocation                              (void* old_pointer, UINT_32 old_size, UINT_32 new_extra_size);
void*   lpe_zero_allocation                                  (UINT_32 byte_size);
void    lpe_free_allocated_mem                               (void* mem);
void    lpe_chunk_table_report                               (LPE_PNG* png);
void    lpe_clean_up_the_png                                 (LPE_PNG* png);
UINT_32 lpe_calculate_crc                                    (void* buffer, UINT_32 size);
void    lpe_construct_crc_table                              (void);
void    lpe_construct_distance_and_distance_extra_bits_table (void);

//***********************************************************************************************************

#endif // !_LPE_PNG__H__
