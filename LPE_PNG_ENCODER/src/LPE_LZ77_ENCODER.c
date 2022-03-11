//***********************************************************************************************************

#include "LPE_LZ77_ENCODER.h"
#include "LPE_COMPRESSOR.h"
#include "LPE_STRING_UNIT.h"
#include "LPE_PNG.h"

//***********************************************************************************************************

lz77_local LPE_LZ77_DICTIONARY_WORD dictionary_words[KB(32)];
lz77_local UINT_32                  position_of_longest_match           = LPE_LZ77_NO_MATCH;
lz77_local UINT_32                  reference_position_of_longest_match = ~0;
lz77_local UINT_32                  dictionary_words_counter            = 0;
lz77_local UINT_32*                 lz77_literal_length_data            = LPE_LZ77_NULL;
lz77_local UINT_32*                 lz77_distance_data                  = LPE_LZ77_NULL;
lz77_local UINT_32                  lz77_literal_length_data_size       = 0;
lz77_local UINT_32                  lz77_distance_data_size             = 0;

//***********************************************************************************************************

lz77_local UINT_32 lpe_lz77_string_length(UINT_8* str)
{
	return lpe_string_call lpe_string_size(str);
}

//***********************************************************************************************************

lz77_local void lpe_lz77_zero_dictionary_metrics(void)
{
	position_of_longest_match           = LPE_LZ77_NO_MATCH;
	reference_position_of_longest_match = ~0;
	dictionary_words_counter            = 0;
	dictionary_words_counter            = 0;
	memset(dictionary_words, 0, KB(32) * sizeof(LPE_LZ77_DICTIONARY_WORD));
}

//***********************************************************************************************************

lz77_local bool lpe_lz77_search_buffer_contains_this_word(UINT_8* search_buffer, UINT_8* word, UINT_32 search_buffer_size, UINT_32 word_size, UINT_32* ref)
{
	// define aliases for the SB and the current word pointers, while source is the word and target would be the SB
	UINT_8* src = word;
	UINT_8* trg = search_buffer;

	// we made previousely sure that the SB size is greater than the word size. So find the difference in order not to go too far to the end of the SB
	UINT_32 possible_combinations = search_buffer_size - word_size;

	// define a position for the start of the current combination
	UINT_32 start_of_combination = 0;

	// define an alias for the word size
	UINT_32 alias_word_size = word_size;

	// start the construction of all possible words combinations
	while (possible_combinations--)
	{
		// slide the trg pointer to the start of the current word combination
		trg = &search_buffer[start_of_combination];

		// loop through the current word combination up until the end of the word size
		while (alias_word_size--)
		{
			// If we could find any mismatch, we would set skip to the next combination
			if (*src++ != *trg++)
				goto _lpe_lz77_search_buffer_contains_this_word_skip_to_the_next_combination;
		}

		// we have already found the first matched combination, so, set the reference point
		*ref = start_of_combination;
		
		// return
		return true;

// set a label
_lpe_lz77_search_buffer_contains_this_word_skip_to_the_next_combination:

		// in case of failure, shift the start_of_combination one ahead, set back src pointer and alias_word_size
		start_of_combination++;
		src             = word;
		alias_word_size = word_size;
	}

	// if we made up until here, there was no matched combination in the SB for the provided LAB's word
	return false;
}

//***********************************************************************************************************

lz77_local UINT_32 lpe_lz77_find_longest_match(LPE_SLIDING_WINDOW* sw)
{
	// initial sanity check to discard the still small SB 
	if (sw->search_buffer_size <= LPE_LZ77_SEARCH_BUFFER_IS_STILL_NOT_READY)
		return LPE_LZ77_SEARCH_BUFFER_IS_STILL_NOT_READY;

	// define a default valued place holder for *longest match*
	UINT_32 longest_match = LPE_LZ77_DEFAULT_LONGEST_MATCH;

	// define several aliases for LPE_SLIDING_WINDOW data
	UINT_8* look_ahead_buffer      = sw->look_ahead_buffer;
	UINT_32 look_ahead_buffer_size = lpe_lz77_string_length(look_ahead_buffer);
	UINT_8* search_buffer_end      = sw->search_buffer_end;
	UINT_32 search_buffer_size     = sw->search_buffer_size;

	// allocate memory for longest possible LAB word
	UINT_8* look_ahead_buffer_word = (UINT_8*)lpe_zero_allocation(look_ahead_buffer_size + 1);

	// define a loop index
	UINT_32 i = 0;

	// loop through the LAB and construct different words
	while (i < look_ahead_buffer_size + 1)
	{
		// put the LPE_NULL_TERMINATING_CHARACTER at the end of current word
		look_ahead_buffer_word[(i >= LPE_LZ77_MAXIMUM_ACCEPTABLE_LENGTH) ? LPE_LZ77_MAXIMUM_ACCEPTABLE_LENGTH : i] = LPE_LZ77_NULL_TERMINATING_CHARACTER;

		// make an alias for look_ahead_buffer
		UINT_8* look_ahead_buffer_pointer = look_ahead_buffer;

		// make sure that the constructed word did not pass the limit of 285 characters
		UINT_32 maximum_end = (i >= LPE_LZ77_MAXIMUM_ACCEPTABLE_LENGTH) ? LPE_LZ77_MAXIMUM_ACCEPTABLE_LENGTH : i;

		// define a loop counter
		UINT_32 j = 0;

		// start constructing a word
		while (j < maximum_end)
		{
			look_ahead_buffer_word[j] = *look_ahead_buffer_pointer++;
			j++;
		}

		// which size has the current word
		UINT_32 look_ahead_buffer_word_size = lpe_lz77_string_length(look_ahead_buffer_word);

		// define a boolean dafaulted to false 
		bool contain = false;

		// define a place holder for the reference of the matched word in the SB, if any
		UINT_32 reference = ~0;

		// only if the length of the constructed word would be greater than SB size, proceed to see if the SB has this constructed word
		if (look_ahead_buffer_word_size <= search_buffer_size)
			contain = lpe_lz77_search_buffer_contains_this_word(search_buffer_end, look_ahead_buffer_word, search_buffer_size, look_ahead_buffer_word_size, &reference);

		// in case the length of the constructed word is greater than 3 and the SB showed that it contains such a word, proceed
		if ((look_ahead_buffer_word_size > LPE_LZ77_LOOK_AHEAD_BUFFER_MIN_ACCEPTED_SIZE) && (contain))
		{
			// is this word still growing or stops here
			if (look_ahead_buffer_word_size > longest_match)
			{
				// set the longest_match to the new value
				longest_match = look_ahead_buffer_word_size;

				// keep track of the position of the longest_match
				position_of_longest_match = sw->search_buffer_size;

				// keep track of the reference position of the longest_match
				reference_position_of_longest_match = reference;
			}
		}

		// increment the loop index
		i++;
	}

	// free the allocated memory for the current LAB
	lpe_free_allocated_mem(look_ahead_buffer_word);

	// return the longest found match
	return longest_match;
}

//***********************************************************************************************************

lz77_local void lpe_lz77_lz77_algorithm(LPE_SLIDING_WINDOW* sw, UINT_32 buffer_size)
{
	// define the place holder for the longest found match 
	UINT_32 longest_match = 0;

	// make an alias for the buffer size
	UINT_32 buff_size = buffer_size;

	// loop up until the SB size reaches the whole assigned buffer size
	while (sw->search_buffer_size <= buff_size)
	{
		// update the longest_match
		longest_match = lpe_lz77_find_longest_match(sw);

		// did we find any match?
		if (position_of_longest_match)
		{
			dictionary_words[dictionary_words_counter].distance_of_word_from_the_origin      = position_of_longest_match;
			dictionary_words[dictionary_words_counter].distance_of_reference_from_the_origin = reference_position_of_longest_match;
			dictionary_words[dictionary_words_counter].length_of_matched_word                = longest_match;
			dictionary_words_counter++;
		}

		// update the sliding window in any case 
		sw->look_ahead_buffer  += longest_match;
		sw->search_buffer_size += longest_match;

		// set back the position_of_longest_match to LPE_NO_MATCH
		position_of_longest_match = LPE_LZ77_NO_MATCH;

		// set back the reference_position_of_longest_match
		reference_position_of_longest_match = ~0;
	}
}

//***********************************************************************************************************

lz77_local UINT_32 lpe_lz77_this_index_is_in_dictionary(UINT_32 index)
{
	// define a loop counter
	UINT_32 i = 0;

	// loop up until all of the dictionary words will be considered
	while (i < dictionary_words_counter)
	{
		// in case we find any match, break and return true
		if (index == dictionary_words[i].distance_of_word_from_the_origin)
			return i;

		// if not, increment to the next entry in dictionary
		i++;
	}

	// we made until this point, then return false
	return ~0;
}

//***********************************************************************************************************

lz77_local void lpe_lz77_put_lz77_data_into_extracted_data_buffer(LPE_LZ77_BUFFER* lz77, UINT_32* lz77_litlen, UINT_32* lz77_dist)
{
	// define the starting pointer in litlen buffer where the extracted data will be gone
	UINT_32* litlen_out_buffer = &(lz77_litlen[lz77_literal_length_data_size]);

	// define the starting pointer in dist buffer where the extracted data will be gone
	UINT_32* dist_out_buffer = &(lz77_dist[lz77_distance_data_size]);

	// where is the input buffer?
	UINT_8* input_buffer = lz77->buffer;

	// what is the byte size of the input buffer?
	UINT_32 byte_size = lz77->buffer_size;

	// make a loop index
	UINT_32 i = 0;

	// define the bit containing dword
	UINT_32 dword = 0;

	// loop to the end of bytes
	while (i < byte_size)
	{		
		// define a dictionary hit holder
		UINT_32 dict_holder = lpe_lz77_this_index_is_in_dictionary(i);

		// we hit a length-distance entry
		if (dict_holder != ~0)
		{
			/* first comes a lenght entry in litlen buffer 
			   and then a distance entry in the distance buffer */

			// extract the length code
			UINT_32 length_code = lengths_table[dictionary_words[dict_holder].length_of_matched_word];

			// update the literal-length buffer
			*litlen_out_buffer++ = length_code;

			// update the literal-length counter
			lz77_literal_length_data_size++;

			// Find out the back pointer distance code (a. k. a. bpdc)		
			UINT_32 bpdc = distance_table[dictionary_words[dict_holder].distance_of_word_from_the_origin - dictionary_words[dict_holder].distance_of_reference_from_the_origin];

			// set the bpdc into the distance buffer
			*dist_out_buffer++ = bpdc;

			// update the distance counter
			lz77_distance_data_size++;

			// go to the next byte for evaluation (keep in mind we have to jump over repeated bytes)
			i += dictionary_words[dict_holder].length_of_matched_word;
		}

		// this is a literal
		else
		{
			// convert the BYTE to the DWORD 
			dword = (UINT_32)(input_buffer[i]);

			// update the literal-length buffer
			*litlen_out_buffer++ = dword;

			// update the literal-length counter
			lz77_literal_length_data_size++;

			// go to the next byte for evaluation
			i++;
		}
	}

	// set back the static dictionary words metrics
	lpe_lz77_zero_dictionary_metrics();
}

//***********************************************************************************************************

LPE_LZ77_LZ77_OUTPUT_PACKAGE lpe_lz77_start(UINT_8* filtered_data, UINT_32 filtered_data_bytes)
{
	// prepare the lz77 litlen and dist buffers by a default size of roughly 2 times filtered_data_bytes
	lz77_literal_length_data = (UINT_32*)lpe_zero_allocation((filtered_data_bytes * sizeof(UINT_32)) << 1);
	lz77_distance_data       = (UINT_32*)lpe_zero_allocation((filtered_data_bytes * sizeof(UINT_32)) << 1);

	// define an LPE_LZ77_BUFFER and allocate 32 KB memory for its sliding window
	LPE_LZ77_BUFFER lz77;
	lz77.buffer_size = KB(32);
	lz77.buffer      = (UINT_8*)lpe_zero_allocation(lz77.buffer_size);

	// define the sliding window and set its LAB and SB pointers
	LPE_SLIDING_WINDOW sw;
	sw.look_ahead_buffer  = (UINT_8*)lz77.buffer;
	sw.search_buffer_end  = (UINT_8*)lz77.buffer;
	sw.search_buffer_size = 0;

	// start the static dictionary words metrics
	lpe_lz77_zero_dictionary_metrics();

	// how many 32KB blocks we have
	UINT_32 blocks_of_32kb = filtered_data_bytes / KB(32);

	// what is the size of the remained bytes
	UINT_32 last_block_size = filtered_data_bytes - (blocks_of_32kb * KB(32));

	// define a loop index
	UINT_32 index = 0;

	// loop until the very last block
	while (index < blocks_of_32kb)
	{
		// load next 32KB data into sliding window's lab
		memcpy(sw.look_ahead_buffer, (filtered_data + (KB(32) * index)), lz77.buffer_size);

		// set the SB pointer
		sw.search_buffer_end = sw.look_ahead_buffer;

		// set back the SB size to zero
		sw.search_buffer_size = 0;

		// extract necessary data by application of the lz77 algorithm
		lpe_lz77_lz77_algorithm(&sw, lz77.buffer_size);

		// encode the lz77ed data into buffers and update the already set counters
		lpe_lz77_put_lz77_data_into_extracted_data_buffer(&lz77, lz77_literal_length_data, lz77_distance_data);

		// set back all static metrics for bit putting in their default values
		lpe_zero_bit_metrics();

		// zero the lz77 buffer
		memset(lz77.buffer, 0, lz77.buffer_size);

		// set back the sliding window to the lz77 buffer
		sw.look_ahead_buffer  = (UINT_8*)lz77.buffer;
		sw.search_buffer_end  = (UINT_8*)lz77.buffer;
		sw.search_buffer_size = 0;

		// increment loop index
		index++;
	}

	// set the lz77 buffer size
	lz77.buffer_size = last_block_size;

	// work on last block which is not necessarily 32KB in size
	memcpy(sw.look_ahead_buffer, (filtered_data + (KB(32) * blocks_of_32kb)), lz77.buffer_size);

	// extract the lz77 algorithm
	lpe_lz77_lz77_algorithm(&sw, lz77.buffer_size);

	// encode the lz77ed data into buffers and update the already set counters
	lpe_lz77_put_lz77_data_into_extracted_data_buffer(&lz77, lz77_literal_length_data, lz77_distance_data);

	// finally we are done with LZ77 data structure, so that destroy it
	lpe_free_allocated_mem(lz77.buffer);
	lz77.buffer_size = 0;

	// pack the output results
	LPE_LZ77_LZ77_OUTPUT_PACKAGE ret_pack;
	ret_pack.litlen_buffer      = lz77_literal_length_data;
	ret_pack.litlen_buffer_size = lz77_literal_length_data_size;
	ret_pack.dist_buffer        = lz77_distance_data;
	ret_pack.dist_buffer_size   = lz77_distance_data_size;

	// return the packed output
	return ret_pack;
}

//***********************************************************************************************************

void lpe_lz77_free_litlen_dist_buffers(void)
{
	lpe_free_allocated_mem(lz77_literal_length_data);
	lpe_free_allocated_mem(lz77_distance_data);
	lz77_literal_length_data_size = 0;
	lz77_distance_data_size       = 0;
}

//***********************************************************************************************************
