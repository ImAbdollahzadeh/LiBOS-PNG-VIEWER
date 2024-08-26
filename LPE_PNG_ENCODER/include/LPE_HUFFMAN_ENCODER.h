#ifndef _LPE_HUFFMAN_ENCODER__H__
#define _LPE_HUFFMAN_ENCODER__H__

//***********************************************************************************************************

#include "LPE_ALIASES.h"
#include "LPE_LZ77_ENCODER.h"

//***********************************************************************************************************

#define huffman_local_call
#define huffman_local                          static
#define LPE_HUFFMAN_NULL                       0
#define LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES 286
#define LPE_HUFFMAN_DISTANCE_MAX_ENTRIES       32
#define LPE_HUFFMAN_TREE_OF_TREES_MAX_ENTRIES  19
#define LPE_HUFFMAN_ELIMINATION_TAG            0xFF
#define LPE_HUFFMAN_ELIMINATION_START_END      ~0
#define LPE_HUFFMAN_NEGLECTED_DWORD            ~0
#define LPE_HUFFMAN_TERMINATE_ENTRY            256
#define LPE_HUFFMAN_NO_COMPRESSION_METHOD      ((0b00) << 1)
#define LPE_HUFFMAN_FIXED_HUFFMAN_METHOD       ((0b01) << 1)
#define LPE_HUFFMAN_DYNAMIC_HUFFMAN_METHOD     ((0b10) << 1)

//***********************************************************************************************************

#define LPE_HUFFMAN_SWAP_VFLAB(VFLAB_PTR_1, VFLAB_PTR_2) do {          \
	LPE_SWAP_TWO_OBJECTS(LPE_HUFFMAN_VFLAB, VFLAB_PTR_1, VFLAB_PTR_2); \
} while(0)

//***********************************************************************************************************

#define LPE_HUFFMAN_ASSERT(VAL_ONE, VAL_TWO) do { \
	if (VAL_ONE != VAL_TWO)                       \
		printf("ASSERTION FAILED\n");             \
} while(0)

//***********************************************************************************************************

typedef enum _TREE_ID {
	LITLEN,
	DIST,
	TWO_TREES
} TREE_ID;

//***********************************************************************************************************

typedef struct _LPE_HUFFMAN_VFLAB {
	UINT_32 value;
	UINT_32 frequency;
	UINT_8  level;
	UINT_16 assigned_reversed_bits;
} LPE_HUFFMAN_VFLAB;

//***********************************************************************************************************

typedef struct _LPE_HUFFMAN_EXTRA_BITS {
	UINT_32 bit_count;
	UINT_32 bit_stream;
} LPE_HUFFMAN_EXTRA_BITS;

//*********************************************************************************************************** local functions

huffman_local UINT_32            lpe_huffman_string_length                             (UINT_8* str);
huffman_local void               lpe_huffman_lexicographically_sort_vflab_by_frequency (LPE_HUFFMAN_VFLAB* vflab, UINT_32 entry_size);
huffman_local void               lpe_huffman_sort_vflab_by_value                       (LPE_HUFFMAN_VFLAB* vflab, UINT_32 entry_size);
huffman_local void               lpe_huffman_nodes_number_two_borders                  (UINT_32 nodes_number, UINT_32* down, UINT_32* up);
huffman_local UINT_32            lpe_huffman_find_non_empty_vflab_entries              (LPE_HUFFMAN_VFLAB* vflab, UINT_32 entries);
huffman_local void               lpe_huffman_give_repetition_tag                       (LPE_HUFFMAN_VFLAB* vflab, UINT_32 updated_vflab_counter_after_elimination);
huffman_local void               lpe_huffman_assign_copy_vflab                         (LPE_HUFFMAN_VFLAB* new_vflab);
huffman_local void               lpe_huffman_generate_binary_output_stream             (UINT_32* bit_buffer, LPE_HUFFMAN_VFLAB* vflab, UINT_32 vflab_counter, LPE_HUFFMAN_VFLAB* freq_vflab, UINT_32 freq_vflab_counter, UINT_32* bits_number_holder);
huffman_local LPE_HUFFMAN_VFLAB* lpe_huffman_find_literal_length_in_vflab_buffer       (UINT_32 current_litlen);
huffman_local LPE_HUFFMAN_VFLAB* lpe_huffman_find_dist_in_vflab_buffer                 (UINT_32 distance_code);
huffman_local UINT_32            lpe_huffman_first_occurance_of_this_number            (UINT_32 num, UINT_32* table, UINT_32 entries);
huffman_local UINT_32            lpe_huffman_find_non_empty_code_bit_lengths_entries   (void);

//*********************************************************************************************************** global functions

LPE_HUFFMAN_VFLAB* lpe_huffman_create_litlen_histogram       (LPE_LZ77_LZ77_OUTPUT_PACKAGE* lz77_output);
LPE_HUFFMAN_VFLAB* lpe_huffman_create_dist_histogram         (LPE_LZ77_LZ77_OUTPUT_PACKAGE* lz77_output);
void               lpe_huffman_construct_huffman_binary_tree (UINT_32** buffer, UINT_32* bits_number, TREE_ID id);
void               lpe_huffman_concatenate_two_bitstreams    (UINT_32* target, UINT_32* bit_stream_1, UINT_32* bit_stream_2, UINT_32 bits_number_1, UINT_32 bits_number_2);
void               lpe_huffman_free_vflabs                   (void);
void               lpe_huffman_post_process_two_trees_vflab  (LPE_HUFFMAN_VFLAB* vflab, UINT_32 vflab_counter, UINT_32** two_trees_binary_bits_buffer, UINT_32* bits_number_holder);
UINT_32*           lpe_huffman_get_code_bit_lengths          (void);
void               lpe_huffman_encode_actual_data            (LPE_LZ77_LZ77_OUTPUT_PACKAGE* lz77_output, UINT_32** encoded_bitstream, UINT_32* encoded_bitstream_size);
UINT_32            lpe_huffman_get_header_values             (UINT_32* header, UINT_32* hlit, UINT_32* hdist, UINT_32* hclen);

//***********************************************************************************************************

#endif // !_LPE_HUFFMAN_ENCODER__H__
