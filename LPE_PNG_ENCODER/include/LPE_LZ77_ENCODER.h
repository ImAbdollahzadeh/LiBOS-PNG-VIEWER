#ifndef _LPE_LZ77_ENCODER__H__
#define _LPE_LZ77_ENCODER__H__

//***********************************************************************************************************

#include "LPE_ALIASES.h"

//***********************************************************************************************************

#define lz77_local                                   static
#define LPE_LZ77_NULL                                0
#define LPE_LZ77_SEARCH_BUFFER_IS_STILL_NOT_READY    3
#define LPE_LZ77_LOOK_AHEAD_BUFFER_MIN_ACCEPTED_SIZE 3
#define LPE_LZ77_DEFAULT_LONGEST_MATCH               1
#define LPE_LZ77_NULL_TERMINATING_CHARACTER          0
#define LPE_LZ77_NO_MATCH                            0
#define LPE_LZ77_MAX_LITERAL_SIZE                    255
#define LPE_LZ77_END_OF_LITERALS                     256
#define LPE_LZ77_BEGINING_OF_LENGTH                  257
#define LPE_LZ77_END_OF_LENGTH                       285
#define LPE_LZ77_MINIMUM_ACCEPTABLE_LENGTH           3
#define LPE_LZ77_MAXIMUM_ACCEPTABLE_LENGTH           285
#define LPE_LZ77_IS_LITERAL(VAL)                     (((VAL) >= 0) && ((VAL) < LPE_LZ77_END_OF_LITERALS))
#define LPE_LZ77_IS_LENGTH(VAL)                      (((VAL) >= LPE_LZ77_BEGINING_OF_LENGTH) && ((VAL) <= LPE_LZ77_END_OF_LENGTH))
#define LPE_LZ77_TERMINATE_ENTRY                     256
#define LPE_LZ77_INDEX_NOT_IN_DICTIONARY             ~0

//***********************************************************************************************************

#define LPE_LZ77_SWAP_DICTIONARY_ENTRY(LPE_LZ77_DICTIONARY_WORD_1, LPE_LZ77_DICTIONARY_WORD_2) do {         \
    LPE_SWAP_TWO_OBJECTS(LPE_LZ77_DICTIONARY_WORD, LPE_LZ77_DICTIONARY_WORD_1, LPE_LZ77_DICTIONARY_WORD_2); \
} while(LPE_FALSE)

//***********************************************************************************************************

typedef struct _LPE_LZ77_BUFFER {
	UINT_8* buffer;
	UINT_32 buffer_size;
} LPE_LZ77_BUFFER;

//***********************************************************************************************************

typedef struct _LPE_SLIDING_WINDOW {
	UINT_8* look_ahead_buffer;
	UINT_8* search_buffer_end;
	UINT_32 search_buffer_size;
} LPE_SLIDING_WINDOW;

//***********************************************************************************************************

typedef struct _LPE_LZ77_DICTIONARY_WORD {
	UINT_32 distance_of_reference_from_the_origin;
	UINT_32 distance_of_word_from_the_origin;
	UINT_32 length_of_matched_word;
} LPE_LZ77_DICTIONARY_WORD;

//***********************************************************************************************************

typedef struct _LPE_LZ77_LZ77_OUTPUT_PACKAGE {
	void*   litlen_buffer;
	UINT_32 litlen_buffer_size;
	void*   dist_buffer;
	UINT_32 dist_buffer_size;
} LPE_LZ77_LZ77_OUTPUT_PACKAGE;

//*********************************************************************************************************** local functions

lz77_local UINT_32 lpe_lz77_string_length                            (UINT_8* str);
lz77_local BOOL    lpe_lz77_search_buffer_contains_this_word         (UINT_8* search_buffer, UINT_8* word, UINT_32 search_buffer_size, UINT_32 word_size, UINT_32* ref);
lz77_local UINT_32 lpe_lz77_find_longest_match                       (LPE_SLIDING_WINDOW* sw, UINT_32 search_window_end);
lz77_local void    lpe_lz77_lz77_algorithm                           (LPE_SLIDING_WINDOW* sw, UINT_32 buffer_size);
lz77_local void    lpe_lz77_put_lz77_data_into_extracted_data_buffer (LPE_LZ77_BUFFER* lz77, UINT_32* lz77_litlen, UINT_32* lz77_dist);
lz77_local UINT_32 lpe_lz77_this_index_is_in_dictionary              (UINT_32 index);

//*********************************************************************************************************** global functions

void                         lpe_lz77_zero_dictionary_metrics  (void);
LPE_LZ77_LZ77_OUTPUT_PACKAGE lpe_lz77_start                    (UINT_8* filtered_data, UINT_32 filtered_data_bytes);
void                         lpe_lz77_free_litlen_dist_buffers (void);

//***********************************************************************************************************

#endif // !_LPE_LZ77_ENCODER__H__

