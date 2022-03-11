/*|###################################################################################################################|
  |  LiBOS PNG ENCODER (LPE) compressor implementation in C programming language written by Iman Abdollahzadeh.       |
  |  This is a freely distributable package under the name of the LiBOS operating system. LPE_COMPRESSOR.c document   |
  |  contains other two implementations LPE_LZ7_ENCODER.c and LPE_HUFFMAN_ENCODER.c packages as well. The compressor  |
  |  uses the DEFLATE/INFLATE algorithm to compress the input byte stream.                                            |
  |  The current source code was written as the first implementation of the PNG encoder package and was not optimized |
  |  for speed purposes. The author aims at the fully acceleration of the package in a separate project with the use  |
  |  of x86 SSE technology. The optimal goal would be the obtaining of the compression of a 1024 x 768 RGB PNG image  |
  |  in less than 10 msec.                                                                                            |																						  |
  |###################################################################################################################|*/


//***********************************************************************************************************

#include "LPE_COMPRESSOR.h"
#include "LPE_PNG.h"
#include "LPE_LZ77_ENCODER.h"
#include "LPE_HUFFMAN_ENCODER.h"
#include <iostream>

//***********************************************************************************************************

static UINT_32 lpe_bit_dword                  = 0x00000000L;
static UINT_32 lpe_dword_offset_in_buffer     = 0;
static UINT_32 lpe_bit_offset_in_dword_offset = 32;

//***********************************************************************************************************

void lpe_zero_bit_metrics(void)
{
	lpe_bit_dword                  = 0x00000000L;
	lpe_dword_offset_in_buffer     = 0;
	lpe_bit_offset_in_dword_offset = 32;
}

//***********************************************************************************************************

void lpe_compress_data(UINT_8** compressed_data_buffer, UINT_8* filtered_data, UINT_32 filtered_data_bytes)
{
	// define an LPE_LZ77_LZ77_OUTPUT_PACKAGE to receive the output results after doing LZ77 on filtered data
	LPE_LZ77_LZ77_OUTPUT_PACKAGE lz77_output = lpe_lz77_start(filtered_data, filtered_data_bytes);

	/* Having performed the above routine, we would have two internal buffers 
	   containing both LITLEN and DIST buffers, separately, at hand.
	   They are separate, because we would need to construct two different Huffman trees for each. */

	// set back all static metrics for bit putting in their default values
	lpe_zero_bit_metrics();

	// make two *maximum-to-minimum sorted* VFALBs, one for literal/length buffer and one for distance buffer
	LPE_HUFFMAN_VFLAB* litlen_vflab = lpe_huffman_create_litlen_histogram(&lz77_output);
	LPE_HUFFMAN_VFLAB* dist_vflab   = lpe_huffman_create_dist_histogram  (&lz77_output);

	/* Now we have the actual data in the lz77_output and the frequency, level, and assigned reversed bits in vflabs.
	   Whenever we hit a value more than 256, we know that this would be a length-distance pair, 
	   and subsequently we are able to encode it properly by looking at the buffers.
	   Secondly, we would need the encoded bit-stream for the actual data. For this reason, we have to make 
	   a buffer to hold the concatenated huffman trees for the liten and dist. */

	// define arrays and size holders to keep the binary tree for litlen and dist huffman trees
	UINT_32* litlen_tree            = LPE_HUFFMAN_NULL;
	UINT_32* dist_tree              = LPE_HUFFMAN_NULL;
	UINT_32  litlen_tree_bit_number = 0;
	UINT_32  dist_tree_bit_number   = 0;

	// construct huffman tree for litlen and dist
	lpe_huffman_construct_huffman_binary_tree(&litlen_tree, &litlen_tree_bit_number, LITLEN);
	lpe_huffman_construct_huffman_binary_tree(&dist_tree,   &dist_tree_bit_number,   DIST);

	/* now the *litlen_tree* contains the binary data for literal length huffman tree and the *dist_tree* contains the binary data for distance huffman tree.
	   the total number of bits for each tree is also inside *litlen_tree_bit_number* and *dist_tree_bit_number*, respectively. */

	// define a UINT_32 variable keeping the total number of dwords for litlen_dist binary data
	UINT_32 litlen_dist_data_dwords = ((litlen_tree_bit_number + dist_tree_bit_number) >> 5) + 1;

	// define a holder to hold the concatenated litlen_dist binary data
	UINT_32* litlen_dist_binary_data = (UINT_32*)lpe_zero_allocation(litlen_dist_data_dwords * sizeof(UINT_32));

	// concatenate litlen binary tree and dist binary tree together
	lpe_huffman_concatenate_two_bitstreams(litlen_dist_binary_data, litlen_tree, dist_tree, litlen_tree_bit_number, dist_tree_bit_number);

	// define arrays and size holders to keep the binary tree for two trees
	UINT_32* two_trees_binary_array = LPE_HUFFMAN_NULL;
	UINT_32  two_trees_bit_number   = 0;

	// construct huffman tree for two_trees
	lpe_huffman_construct_huffman_binary_tree(&two_trees_binary_array, &two_trees_bit_number, TWO_TREES);

	/* Now we have the two_trees and the litlen_dist_binary data sets at hand. 
	   We will concatenate their bits in a big data set and name it dyn_huffman_binary_data. */

	// define a UINT_32 variable keeping the total number of dwords for dyn_huffman_binary data
	UINT_32 dyn_huffman_binary_data_dwords = litlen_dist_data_dwords + (two_trees_bit_number >> 5) + 1;

	// define a holder to hold the concatenated litlen_dist data
	UINT_32* dyn_huffman_binary_data = (UINT_32*)lpe_zero_allocation(dyn_huffman_binary_data_dwords * sizeof(UINT_32));

	// concatenate two_trees binary tree and litlen_dist binary tree together
	lpe_huffman_concatenate_two_bitstreams(dyn_huffman_binary_data, 
                                               two_trees_binary_array,
                                               litlen_dist_binary_data,
                                               two_trees_bit_number,
                                               (litlen_tree_bit_number + dist_tree_bit_number));

	// get the code bit length array
	UINT_32* code_bit_len_array_tree = lpe_huffman_get_code_bit_lengths();

	// define the array and the size holder to keep the binary data
	UINT_32* encoded_bistream      = LPE_HUFFMAN_NULL;
	UINT_32  encoded_bistream_size = 0;

	// encode the actual data embedded in the lz77_output data structure
	lpe_huffman_encode_actual_data(&lz77_output, &encoded_bistream, &encoded_bistream_size);

	// define a UINT_32 variable keeping the total number of dwords for block_data section
	UINT_32 block_data_dwords = dyn_huffman_binary_data_dwords + (encoded_bistream_size >> 5) + 1;

	// define a holder to hold the concatenated dyn_huffman_binary_data and encoded_bistream data
	UINT_32* block_data = (UINT_32*)lpe_zero_allocation(block_data_dwords * sizeof(UINT_32));

	// concatenate *dyn_huffman_binary_data* and *encoded_bistream* together into *block_data* buffer
	lpe_huffman_concatenate_two_bitstreams(block_data, 
                                               dyn_huffman_binary_data,
                                               encoded_bistream, 
                                               (two_trees_bit_number + litlen_tree_bit_number + dist_tree_bit_number), 
                                               encoded_bistream_size);

	// define 3-bits value called header
	UINT_32 header = ~0;

	// define 5-bits value called hlit (the number of literals/length codes - 257)
	UINT_32 hlit = ~0;

	// define 5-bits value called hdist (the number of distance codes - 1)
	UINT_32 hdist = ~0;

	// define 4-bits value called hclen (the number of length codes that are used to encode the Huffman tree that will encode the other 2 trees - 4)
	UINT_32 hclen = ~0;

	// obtain the necessary values by calling the *lpe_huffman_get_header_values* function
	UINT_32 result = lpe_huffman_get_header_values(&header, &hlit, &hdist, &hclen);

	// reallocate the block_data buffer, since now we have an extra DWORD containing these new 17 bits
	block_data = (UINT_32*)lpe_buffer_reallocation(block_data, (block_data_dwords * sizeof(UINT_32)), sizeof(UINT_32));

	// concatenate *result* and old *block_data* together into the new *block_data* buffer
	lpe_huffman_concatenate_two_bitstreams(block_data, 
                                               &result, 
                                               block_data, 
                                               17, 
                                               (two_trees_bit_number + litlen_tree_bit_number + dist_tree_bit_number + encoded_bistream_size));

	
	// ...


	// free the code bit length array
	lpe_free_allocated_mem(code_bit_len_array_tree);

	// free the allocated memory for the *block_data*
	lpe_free_allocated_mem(block_data);
	block_data_dwords = 0;

	// free the allocated memory for the *encoded_bistream*
	lpe_free_allocated_mem(encoded_bistream);
	encoded_bistream_size = 0;

	// after doing huffman encoding, we don't need the allocated output buffers, anymore
	lpe_free_allocated_mem(litlen_dist_binary_data);
	lpe_free_allocated_mem(litlen_tree);
	lpe_free_allocated_mem(dist_tree);
	lpe_free_allocated_mem(two_trees_binary_array);
	dyn_huffman_binary_data_dwords = 0;
	litlen_tree_bit_number         = 0;
	dist_tree_bit_number           = 0;
	two_trees_bit_number           = 0;

	// after doing huffman encoding, we don't need the litlen, dist, and two_trees buffers from lz77 step, anymore
	lpe_lz77_free_litlen_dist_buffers();

	// we don't need vflabs anymore
	lpe_huffman_free_vflabs();


	/* ********************** STRUCTURE OF THE DEFLATE BLOCK **************************************************************
	   |                                                                                                                  |
	   |    << ordered bits >>                << ordered bits >>                     << reversed bits >>                  |
	   |  +------------------------+------------------------------------------+-------------------------------------+ ... |
	   |  | BLOCK_HEADER (17 bits) | CODE BITLEN FOR 2TREES (NHCLEN * 3 bits) | 2TREES BINARY ((HLIT + HDIST) bits) |     |
	   |  +------------------------+------------------------------------------+-------------------------------------+ ... |
	   |                                                                                                                  |
	   |        << reversed bits >>      << reversed bits >>             << ordered bits >>                               |
	   |  ... +-------------------------+------------------------+------------------------------------+                   |
	   |	  | LITLEN TREE (HLIT bits) | DIST TREE (HDIST bits) | ENCODED BIT STREAM (variable bits) |                   |
	   |  ... +-------------------------+------------------------+------------------------------------+                   |
	   |                                                                                                                  |
	   ******************************************************************************************************************** */
}

//***********************************************************************************************************

static void lpe_flush_bit_dword(void* buffer, UINT_32 dword)
{
	UINT_32* buffer32 = (UINT_32*)((UINT_8*)buffer + lpe_dword_offset_in_buffer);
	*buffer32 = dword;
}

//***********************************************************************************************************

void lpe_final_flush_bit_dword(void* buffer)
{
	// if there is still some bits remained in the containig dword
	if (lpe_bit_dword)
	{
		// flush the containing dword to the buffer
		lpe_flush_bit_dword(buffer, lpe_bit_dword);

		// set back the lpe_bit_dword 
		lpe_bit_dword = 0;

		// update the dword and bit offsets in buffer
		lpe_dword_offset_in_buffer++;
		lpe_bit_offset_in_dword_offset = 32;
	}
}
//***********************************************************************************************************

void lpe_put_bits_in_buffer(void* buffer, UINT_32 bit_containing_dword, UINT_32 number)
{
	// check out the dword offset in compressed buffer. If we are done with this offset, increment it and reset the bit offset
	if (!lpe_bit_offset_in_dword_offset)
	{
		lpe_dword_offset_in_buffer++;
		lpe_bit_offset_in_dword_offset = 32;
	}

	// how many bits remained in our dword
	UINT_32 bits_remained  = 32 - lpe_bit_offset_in_dword_offset;

	// define number of bits that should be consumed in this dword
	UINT_32 main_bits = (number > bits_remained) ? bits_remained : number;

	// define number of bits that should be consumed in the next dword
	UINT_32 extra_bits = (number > bits_remained) ? (number - bits_remained) : 0;

	// define a bit counter
	UINT_32 bit_count = 0;

	// go ahead with main bits
	if (main_bits)
	{
		// put all necessary bits in the output dword buffer
		while (bit_count < main_bits)
		{
			// safely put bits in lpe_bit_dword
			lpe_bit_dword |= ((bit_containing_dword & LPE_BIT(bit_count)) >> bit_count);

			// go to the next bit
			bit_count++;
		}		

		// prepare lpe_bit_dword for later uses
		lpe_bit_dword <<= main_bits;

		// update lpe_bit_offset_in_dword_offset
		lpe_bit_offset_in_dword_offset -= main_bits;
	}

	// set back the bit counter to zero
	bit_count = 0;

	// go ahead with extra bits if any
	if (extra_bits)
	{
		// flush lpe_bit_dword to the buffer
		lpe_flush_bit_dword(buffer, lpe_bit_dword);

		// set back the lpe_bit_dword 
		lpe_bit_dword = 0;

		// update the dword and bit offsets in buffer
		lpe_dword_offset_in_buffer++;
		lpe_bit_offset_in_dword_offset = 32;

		// put all necessary bits in the output dword buffer
		while (bit_count < extra_bits)
		{
			// safely put bits in lpe_bit_dword
			lpe_bit_dword |= ((bit_containing_dword & LPE_BIT(bit_count)) >> bit_count);

			// go to the next bit
			bit_count++;
		}

		// prepare lpe_bit_dword for later uses
		lpe_bit_dword <<= extra_bits;

		// update lpe_bit_offset_in_dword_offset
		lpe_bit_offset_in_dword_offset -= extra_bits;
	}
}

//***********************************************************************************************************

UINT_32 lpe_bit_reverse(UINT_32 bits, UINT_32 number)
{
	// define a result holder
	UINT_32 ret = 0;

	// define a loop index
	UINT_32 i = 0;

	// loop to the end of the provided bits number
	while (i < number)
	{
		// extract the LSB single bit
		UINT_32 bit = ((bits & LPE_BIT(i)) >> i);

		// put this single bit to the reversed position
		ret |= (bit << (number - i - 1));

		// go to next bit
		i++;
	}

	return ret;
}

//***********************************************************************************************************

