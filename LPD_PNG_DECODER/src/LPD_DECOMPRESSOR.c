#include "LPD_DECOMPRESSOR.h"
#include "LPD_PNG.h"
#include <stdio.h>

static BOOL      first_fixed_huffman_call = TRUE;
static LPD_FIXED fix_data;
static UINT_8    fixed_literal_length_buffer_bitlen[288];
static UINT_8    fixed_distance_buffer_bitlen      [30];

//***********************************************************************************************************

void lpd_initialize_bit_stream(LPD_BIT_STREAM* bit_stream, UINT_8* compressed_data)
{
	bit_stream->data32            = (UINT_32*)compressed_data;
	bit_stream->dword_in_use      = *bit_stream->data32;
	bit_stream->bits_remained     = 32;
	bit_stream->debug_byte_border = 0;
}

//***********************************************************************************************************

UINT_32 lpd_get_bits(LPD_BIT_STREAM* bit_stream, UINT_8 bits_required)
{
	UINT_32 dword_in_use      = bit_stream->dword_in_use;
	UINT_8  bits_to_get       = (bits_required > bit_stream->bits_remained) ? bit_stream->bits_remained : bits_required;
	UINT_8  new_bits_to_order = (bits_required > bit_stream->bits_remained) ? bits_required - bit_stream->bits_remained : 0;
	UINT_32 result            = 0;
	UINT_32 pos               = 32 - bit_stream->bits_remained;
	UINT_32 index             = 0;
	UINT_32 bit;

	while (bits_to_get)
	{
		bit = (dword_in_use >> pos & 1);
		if (BITS_DEBUG)
		{
			printf("%i", bit);
			bit_stream->debug_byte_border++;
			if (bit_stream->debug_byte_border == 8)
			{
				bit_stream->debug_byte_border = 0;
				printf(" - ");
			}
		}
		result |= (bit << index);
		bit_stream->bits_remained--;
		bits_to_get--;
		pos++;
		index++;
	}
	if (new_bits_to_order)
	{
		bit_stream->data32++;
		bit_stream->dword_in_use = *bit_stream->data32;
		dword_in_use = bit_stream->dword_in_use;
		bit_stream->bits_remained = 32;
		pos = 0;
		while (new_bits_to_order)
		{
			bit = (dword_in_use >> pos & 1);
			if (BITS_DEBUG)
			{
				printf("%i", bit);
				bit_stream->debug_byte_border++;
				if (bit_stream->debug_byte_border == 8)
				{
					bit_stream->debug_byte_border = 0;
					printf(" - ");
				}
			}
			result |= (bit << index);
			bit_stream->bits_remained--;
			new_bits_to_order--;
			pos++;
			index++;
		}
	}
	if (BITS_DEBUG)
		printf(": %i\n", result);
	return result;
}

//***********************************************************************************************************

void lpd_decompress_zlib_buffer(LPD_PNG* png, UINT_8* compressed_data)
{
	// check out the compression methods of the encoded ZLIB block
	lpd_compression_info(compressed_data);

	// jump over the CMF and CFLAG bytes
	compressed_data += 2;

	// define and initialize a LPD_BIT_STREAM data structure
	LPD_BIT_STREAM bit_stream;
	lpd_initialize_bit_stream(&bit_stream, compressed_data);

	// define the final_block
	UINT_32 final_block = 0;
	UINT_32 block_type  = 0;

	do {
		// extract the information of the current block
		final_block = lpd_get_bits(&bit_stream, 1);
		block_type  = lpd_get_bits(&bit_stream, 2);

		// print that information
		printf("\nfinal: %i\ntype: %i (%s)\n********\n", final_block, block_type, (!block_type) ? noc : (block_type == 1) ? fix : (block_type == 2 ? dyn : 0));

		// select the appropriate compression method obtained from the information and choose the right function to handle the decompression
		switch (block_type)
		{
		case NO_COMPRESSION:
			lpd_decompress_zlib_buffer_no_compression(png, &bit_stream);
			break;
		case FIXED_HUFFMANN:
			lpd_decompress_zlib_buffer_fixed(png, &bit_stream);
			break;
		case DYNAMIC_HUFFMANN:
			lpd_decompress_zlib_buffer_dynamic(png, &bit_stream);
			break;
		case ERROR_COMPRESSION:
			printf("error compression occured --> fatal error\n");
			return;
		}
	} while (!final_block);
}

//***********************************************************************************************************

void lpd_decompress_zlib_buffer_dynamic(LPD_PNG* png, LPD_BIT_STREAM* bits)
{
	// get number of literal/length codes (hlit), number of distance codes (hdist), and number of length codes used to encode the two huffman tree (hclen)
	UINT_32 hlit  = lpd_get_bits(bits, 5) + 257;
	UINT_32 hdist = lpd_get_bits(bits, 5) + 1;
	UINT_32 hclen = lpd_get_bits(bits, 4) + 4;
	printf("hlit: %i\nhdist: %i\nhclen: %i\n********\n", hlit, hdist, hclen);

	// prepare the bit length array based on the provided bit length order by PNG specification
	UINT_8 bitlen_order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
	UINT_8 bit_length  [19] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	// create the bit length buffer from the bit stream
	for (UINT_8 i = 0; i < hclen; i++)
		bit_length[bitlen_order[i]] = lpd_get_bits(bits, 3);

	if (BITS_DEBUG)
	{
		for (UINT_8 i = 0; i <= hclen; i++)
			printf("code bitlen %i: %i\n", i, bit_length[i]);
	}

	// construct the huffman tree of compressed data for literal/length and distance huffman bit length
	UINT_32* huffman_of_two_trees = lpd_construct_huffman_tree(bit_length, 19);

	// decode the codes and build the two huffman trees for the literal/length and distance
	UINT_32 literal_distance_sum  = hdist + hlit;

	// creat a buffer for bit length array of the two huffman trees
	UINT_8* two_trees_code_bitlen = (UINT_8*)lpd_zero_allocation(literal_distance_sum);

	// create a code indexer
	UINT_32 code_count            = 0;

	// do the actual deciphering
	while (code_count < literal_distance_sum)
	{
		// find the decoded value and later on decide based on it
		UINT_32 decoded_value = lpd_decode_huffman_tree(bits, huffman_of_two_trees, bit_length, 19);

		// if decoded value would be less than 16, it represents a bit length of 0 to 15 for literal/length and distance trees
		if (decoded_value < 16)
		{
			two_trees_code_bitlen[code_count++] = decoded_value;
			continue;
		}

		// otherwise, it would be a repeatition code
		UINT_32 repeat_count          = 0;
		UINT_8  code_length_to_repeat = 0;

		// decide based on the value being 16, 17, or 18
		switch (decoded_value)
		{
		case 16:
			// repeat 3 to 6 times last obtained code based on the extra 2 bits plus 3
			repeat_count = lpd_get_bits(bits, 2) + 3; 
			code_length_to_repeat = two_trees_code_bitlen[code_count - 1];
			break;
		case 17:
			// repeat 3 to 10 times code length of 0 based on the extra 3 bits plus 3
			repeat_count = lpd_get_bits(bits, 3) + 3; 
			break;
		case 18:
			// repeat 11 to 138 times code that has been obtained before based on the extra 7 bits plus 11
			repeat_count = lpd_get_bits(bits, 7) + 11; 
			break;
		}

		// set the obtained code and repeating values
		memset(two_trees_code_bitlen + code_count, code_length_to_repeat, repeat_count);

		// update the code indexer
		code_count += repeat_count;
	}

	// construct literal/length huffman tree
	UINT_32* literal_length_huffman_tree = lpd_construct_huffman_tree(two_trees_code_bitlen, hlit);

	// construct distance huffman tree
	UINT_32* distance_huffman_tree       = lpd_construct_huffman_tree(two_trees_code_bitlen + hlit, hdist);

	// decompress the current block
	lpd_deflate_block(png, bits, literal_length_huffman_tree, two_trees_code_bitlen, hlit, distance_huffman_tree, two_trees_code_bitlen + hlit, hdist);
}

//***********************************************************************************************************

void lpd_decompress_zlib_buffer_fixed(LPD_PNG* png, LPD_BIT_STREAM* bits)
{
	// ********* fixed trees construction method *********
	if (first_fixed_huffman_call == TRUE)
	{
		// prepare the LPD_FIXED data and the two fixed bitlen buffers
		memset(&fix_data, 0, sizeof(LPD_FIXED));
		memset(fixed_literal_length_buffer_bitlen, 0, 288);
		memset(fixed_distance_buffer_bitlen, 0, 30);
		
		// fill up the first fixed buffer
		UINT_32 count = 0;
		for (; count < 144; count++) fixed_literal_length_buffer_bitlen[count] = 8;
		for (; count < 256; count++) fixed_literal_length_buffer_bitlen[count] = 9;
		for (; count < 280; count++) fixed_literal_length_buffer_bitlen[count] = 7;
		for (; count < 288; count++) fixed_literal_length_buffer_bitlen[count] = 8;

		// build the literal_length huffman tree
		fix_data.literal_length_tree = lpd_construct_huffman_tree(fixed_literal_length_buffer_bitlen, 288);

		// fill up the second fixed buffer
		count = 0;
		for (; count < 30; count++) fixed_distance_buffer_bitlen[count] = 5;

		// build the distance huffman tree
		fix_data.distance_tree = lpd_construct_huffman_tree(fixed_distance_buffer_bitlen, 30);

		// the fixed huffman trees have been constructed, thus, never call the fixed trees construction again
		first_fixed_huffman_call = FALSE;
	}

	// decompress the current block like that of dynamic huffman
	lpd_deflate_block(png, bits, fix_data.literal_length_tree, fixed_literal_length_buffer_bitlen, 288, fix_data.distance_tree, fixed_distance_buffer_bitlen, 30);
}

//***********************************************************************************************************

void lpd_decompress_zlib_buffer_no_compression(LPD_PNG* png, LPD_BIT_STREAM* bits)
{
	// round up to the next byte boundary
	while ((bits->bits_remained % 8) != 0)
		bits->bits_remained--;

	// retrieve LEN and NLEN bytes
	UINT_16 len  = (UINT_16)(lpd_get_bits(bits, 16));
	UINT_16 nel  = ~len; // 1's complement of LEN
	UINT_16 nlen = (UINT_16)(lpd_get_bits(bits, 16));
	
	// check the validity of LEN and NLEN
	if (nel != nlen)
	{
		printf("LEN and NLEN *NOT MATCHED* error in no_compression block --> fatal error\n");
		return;
	}

	// prepare input and output buffers
	UINT_8* in  = (UINT_8*)(bits->data32) + ((32 - bits->bits_remained) >> 3);
	UINT_8* out = (png->decompressed_data_buffer + png->decompressed_data_counter);
	
	// make a copy of LEN and do not sacrifice LEN
	UINT_16 len_copy = len;

	// do the actual copy of *stored* data by copying LEN bytes to the decompressed buffer
	while (len_copy--)
		*out++ = *in++;

	// update the bit stream
	UINT_32 bits_in_len       = len << 3;
	UINT_32 blocks_of_32_bits = bits_in_len >> 5;
	UINT_8  remaining         = bits_in_len - (blocks_of_32_bits << 5);

	// update the bit stream with 32 bits limitation
	while (blocks_of_32_bits--)
		lpd_update_bit_stream(bits, 32);

	// update the bit stream with final bit counts
	lpd_update_bit_stream(bits, remaining);

	// update png->decompressed_buffer_counter and return
	png->decompressed_data_counter += len;
}

//***********************************************************************************************************

UINT_8 lpd_maximum_of_bitlen_array(UINT_8* code_bitlen_array, UINT_32 array_length)
{
	UINT_8  maximum_bitlen = 0;
	UINT_32 i              = 0;
	UINT_8  trg            = 0;
	while (i < array_length)
	{
		trg = code_bitlen_array[i];
		if (maximum_bitlen < trg)
			maximum_bitlen = trg;
		i++;
	}
	return maximum_bitlen;
}

//***********************************************************************************************************

void lpd_get_bit_length_count(UINT_32* code_count_array, UINT_8* code_bitlen_array, UINT_32 bitlen_array_length)
{
	for (UINT_32 i = 0; i < bitlen_array_length; i++)
		code_count_array[code_bitlen_array[i]]++;
}

//***********************************************************************************************************

void lpd_first_code_for_bitlen(UINT_32* first_codes_array, UINT_32* code_count_array, UINT_32 max_bitlen)
{
	UINT_32 code = 0;
	UINT_32 i    = 1;
	while (i <= max_bitlen)
	{
		code = ((code + code_count_array[i - 1]) << 1);
		if (code_count_array[i] > 0)
			first_codes_array[i] = code;
		i++;
	}
}

//***********************************************************************************************************

void lpd_assign_huffman_code(UINT_32* assigned_codes_array, UINT_32* first_codes_array, UINT_8* code_bitlen_array, UINT_32 assigned_codes_length)
{
	UINT_32 i = 0;
	while (i < assigned_codes_length)
	{
		if (code_bitlen_array[i])
			assigned_codes_array[i] = first_codes_array[code_bitlen_array[i]]++; // It gives the value to "assigned_code_array[i]", then increments
		i++;
	}
}

//***********************************************************************************************************

UINT_32* lpd_construct_huffman_tree(UINT_8* code_bitlen_array, UINT_32 code_bitlen_array_length)
{
	// first off, find the maximum bit length
	UINT_32  max_bit_length       = lpd_maximum_of_bitlen_array(code_bitlen_array, code_bitlen_array_length);

	// crate a buffer for the code counts for each bit length
	UINT_32* code_counts_array    = (UINT_32*)lpd_zero_allocation(sizeof(UINT_32)*(max_bit_length + 1));

	// create a buffer to store the first code for each bit length
	UINT_32* first_codes_array    = (UINT_32*)lpd_zero_allocation(sizeof(UINT_32)*(max_bit_length + 1));

	// create a buffer with size equal to the provided length of the bitlen array times UINT_32
	UINT_32* huffman_tree         = (UINT_32*)lpd_zero_allocation(sizeof(UINT_32)*(code_bitlen_array_length));

	// calculate the code counts for each bit length
	lpd_get_bit_length_count(code_counts_array, code_bitlen_array, code_bitlen_array_length);

	// important: codes with bit length = 0, should be all neglected
	code_counts_array[0] = 0;

	// for each bit length, find the first code
	lpd_first_code_for_bitlen(first_codes_array, code_counts_array, max_bit_length);

	// construct the huffman tree by looking into bitlen array and first codes array and updating the first code array every time 
	lpd_assign_huffman_code(huffman_tree, first_codes_array, code_bitlen_array, code_bitlen_array_length);

	// return the constructed huffman tree
	return huffman_tree;
}

//***********************************************************************************************************

UINT_32 lpd_get_bits_reversed_without_consuming(LPD_BIT_STREAM* bits, UINT_32 bits_required)
{
	// bits shouldn't be consumed. Make a copy of the structure
	UINT_32  dword_in_use      = bits->dword_in_use;
	UINT_32  bits_remained     = bits->bits_remained;
	UINT_32* data32            = bits->data32;
	UINT_8   debug_byte_border = bits->debug_byte_border;
	UINT_8   bits_to_get       = (bits_required > bits->bits_remained) ? bits->bits_remained : bits_required;
	UINT_8   new_bits_to_order = (bits_required > bits->bits_remained) ? bits_required - bits->bits_remained : 0;
	UINT_32  result            = 0;
	UINT_32  pos               = 32 - bits->bits_remained;
	UINT_32  index             = bits_required - 1;
	UINT_32  bit;

	while (bits_to_get)
	{
		bit = (dword_in_use >> pos & 1);
		if (BITS_DEBUG)
		{
			printf("%i", bit);
			debug_byte_border++;
			if (debug_byte_border == 8)
			{
				debug_byte_border = 0;
				printf(" - ");
			}
		}
		result |= (bit << index);
		bits_remained--;
		bits_to_get--;
		pos++;
		index--;
	}
	if (new_bits_to_order)
	{
		data32++;
		dword_in_use = *data32;
		bits_remained = 32;
		pos = 0;
		while (new_bits_to_order)
		{
			bit = (dword_in_use >> pos & 1);
			if (BITS_DEBUG)
			{
				printf("%i", bit);
				debug_byte_border++;
				if (debug_byte_border == 8)
				{
					debug_byte_border = 0;
					printf(" - ");
				}
			}
			result |= (bit << index);
			bits_remained--;
			new_bits_to_order--;
			pos++;
			index--;
		}
	}
	if (BITS_DEBUG)
		printf(": %i\n", result);
	return result;
}

//***********************************************************************************************************

void lpd_update_bit_stream(LPD_BIT_STREAM* bits, UINT_32 bits_number)
{
	// consume the required bits and always check for bits->bits_remained being as zero
	while (bits_number)
	{
		if (!(bits->bits_remained))
		{
			bits->bits_remained = 32;
			bits->data32++;
			bits->dword_in_use = *bits->data32;
		}
		bits->bits_remained--;		
		bits_number--;
	}
}

//***********************************************************************************************************

UINT_32 lpd_decode_huffman_tree(LPD_BIT_STREAM* bits, UINT_32* huffman_tree, UINT_8* bitlen_array, UINT_32 huffman_tree_length)
{
	// loop through the huffman tree
	for (UINT_32 i = 0; i < huffman_tree_length; i++)
	{
		// reject the bit length entry being zero
		UINT_8 this_bitlen_array_entry = bitlen_array[i];
		if (!this_bitlen_array_entry)
			continue;

		// otherwise find the code of this bit length entry size
		UINT_32 code = lpd_get_bits_reversed_without_consuming(bits, this_bitlen_array_entry);

		// in case the obtained code matches the current huffman tree entry, accept it
		if (huffman_tree[i] == code)
		{
			// upon acception, go ahead in the bit stream
			lpd_update_bit_stream(bits, this_bitlen_array_entry);

			// return the index as the result (in canonical huffman coding, alphabet letters have been stored in order)
			return i;
		}
	}

	// return zero
	return 0;
}

//***********************************************************************************************************

void lpd_deflate_block(LPD_PNG*        png, 
                       LPD_BIT_STREAM* bits, 
                       UINT_32*        literal_tree, 
                       UINT_8*         literal_bitlen,
                       UINT_32         literal_array_length,
                       UINT_32*        distance_tree, 
                       UINT_8*         distance_bitlen, 
                       UINT_32         distance_array_length)
{
	// prepare the decompressed buffer
	UINT_8* decompressed_data = png->decompressed_data_buffer + png->decompressed_data_counter;

	// prepare a counter
	UINT_32 count             = 0;

	// loop forever until an EOI (a.k.a. 256) is found
	while (TRUE)
	{
		// obtain the literal/length decoded value
		UINT_32 decoded_value = lpd_decode_huffman_tree(bits, literal_tree, literal_bitlen, literal_array_length);

		if (BITS_DEBUG)
			printf("decoded value: %i\n", decoded_value);

		// decide based on the decoded value
		if (decoded_value == 256)
			break;

		// if a literal is found, keep it
		else if (decoded_value < 256)
		{
			decompressed_data[count++] = decoded_value;
			continue;
		}

		// otherwise it would be a length and distance pair
		else if (decoded_value < 286 && decoded_value > 256)
		{
			// find the length index index
			UINT_32 length_index       = decoded_value - 257;

			// find the duplicate length from reading the extra bits and using LEN and LEXT tables
			UINT_32 duplicate_length   = lengths_table[length_index] + lpd_get_bits(bits, lengths_table_extra_bits[length_index]);

			// obtain the distance index decoded value
			UINT_32 distance_index     = lpd_decode_huffman_tree(bits, distance_tree, distance_bitlen, distance_array_length);

			// find the distance from reading the extra bits and using DIST and DEXT tables
			UINT_32 distance_length    = distance_table[distance_index] + lpd_get_bits(bits, distance_table_extra_bits[distance_index]);

			// how far we have to go back in distance?
			UINT_32 back_pointer_index = count - distance_length;

			// do the actual duplication
			while (duplicate_length--)
				decompressed_data[count++] = decompressed_data[back_pointer_index++];
		}
	}

	// finaly update the png's decompressed_data_counter
	png->decompressed_data_counter += count;
}

//***********************************************************************************************************

void lpd_compression_info(UINT_8* buffer)
{
	// read out first 2 bytes of the ZLIB block
	printf("CMF and FLG of ZLIB block:\n\t0x%x, 0x%x\n ", (int)buffer[0], (int)buffer[1]);
}

//***********************************************************************************************************

BOOL lpd_is_adler32(UINT_8* buffer) // < ======================= UNCOMPLETE ========================
{
	// read out final 4 bytes of the ZLIB block
	UINT_8 adler32_checksum_byte_1 = buffer[0];
	UINT_8 adler32_checksum_byte_2 = buffer[1];
	UINT_8 adler32_checksum_byte_3 = buffer[2];
	UINT_8 adler32_checksum_byte_4 = buffer[3];
	
	if (1)
	{
		// TODO ...
		return TRUE;
	}
	return FALSE;
}

//***********************************************************************************************************
