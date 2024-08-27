//***********************************************************************************************************

#include "LPE_HUFFMAN_ENCODER.h"
#include "LPE_PNG.h"
#include "LPE_COMPRESSOR.h"
#include "LPE_BITMAP_READER.h"
#include "LPE_STRING_UNIT.h"

//***********************************************************************************************************

huffman_local LPE_HUFFMAN_VFLAB* litlen_vflab                = LPE_HUFFMAN_NULL;
huffman_local LPE_HUFFMAN_VFLAB* dist_vflab                  = LPE_HUFFMAN_NULL;
huffman_local LPE_HUFFMAN_VFLAB* tree_of_trees_vflab         = LPE_HUFFMAN_NULL;
huffman_local UINT_32            litlen_dword_size           = 0;
huffman_local UINT_32            dist_dword_size             = 0;
huffman_local UINT_32            two_trees_dword_size        = 0;
huffman_local UINT_32            lpe_huffman_two_tree_freq[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
huffman_local UINT_32*           code_bit_lengths            = LPE_HUFFMAN_NULL;

//***********************************************************************************************************

huffman_local void lpe_huffman_nodes_number_two_borders(UINT_32 nodes_number, UINT_32* down, UINT_32* up)
{
	// sanity check for especial case of N = 1
	if (nodes_number <= 1)
	{
		*down = 0;
		*up   = 1;
		return;
	}
	UINT_32 d = 2;
	UINT_32 i = 0;
	while (d < nodes_number)
	{
		d <<= 1;
		i++;
	}
	*up   = i--;
	*down = i;
}

//***********************************************************************************************************

huffman_local UINT_32 lpe_huffman_string_length(UINT_8* str)
{
	return lpe_string_call lpe_string_size(str);
}

//***********************************************************************************************************

huffman_local UINT_32 lpe_huffman_find_non_empty_vflab_entries(LPE_HUFFMAN_VFLAB* vflab, UINT_32 entries)
{
	UINT_32 i = 0;
	while (i < entries)
	{
		if ( !(vflab[i].frequency) )
			break;
		i++;
	}
	return i;
}

//***********************************************************************************************************

LPE_HUFFMAN_VFLAB* lpe_huffman_create_litlen_histogram(LPE_LZ77_LZ77_OUTPUT_PACKAGE* lz77_output)
{
	// allocate memory for litlen vflab
	litlen_vflab = (LPE_HUFFMAN_VFLAB*)lpe_zero_allocation(LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES * sizeof(LPE_HUFFMAN_VFLAB));

	// fill up the value of the vflab by hand
	for (UINT_32 j = 0; j < LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES; j++)
		litlen_vflab[j].value = j;

	// get the DWORDs of litlen buffer in lz77_output buffer
	UINT_32* litlen_buffer_dword = (UINT_32*)lz77_output->litlen_buffer;

	// get the size of the litlen buffer
	UINT_32 litlen_buffer_dword_size = lz77_output->litlen_buffer_size;
	
	// update the statically defined litlen_dword_size
	litlen_dword_size = litlen_buffer_dword_size;

	// define a loop index
	UINT_32 i = 0;

	// fill up the vflab
	while (i < litlen_buffer_dword_size)
	{
		UINT_32 dword = litlen_buffer_dword[i];
		litlen_vflab[dword].frequency++;
		i++;
	}

	/* Up to here, we have filled up VFLAB's values and frequencies. 
	   To find their level, we have to sort them based on frequency from max to min, and then, we would assign bits to them */

	lpe_huffman_lexicographically_sort_vflab_by_frequency(litlen_vflab, LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES);

	// acquire the number of non empty entries in vflab
	UINT_32 non_empty_entries_count = lpe_huffman_find_non_empty_vflab_entries(litlen_vflab, LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES);

	// check the non empty count
	if (non_empty_entries_count == 0)
	{
		printf("literal length vflab shows all entries zero --> fatal error\n");
		lpe_free_allocated_mem(litlen_vflab);
		return LPE_HUFFMAN_NULL;
	}

	// get the L_(n-1) and L_(n) borders
	UINT_32 down = 0;
	UINT_32 up   = 0;	
	lpe_huffman_nodes_number_two_borders(non_empty_entries_count, &down, &up);

	// set back loop index to zero
	i = 0;

	// set levels (L_(n)-N goes into down level while 2N-L_(n) goes into up level)
	UINT_32 L_down = up - non_empty_entries_count;
	UINT_32 L_up   = (non_empty_entries_count << 1) - up;

	while (i < non_empty_entries_count)
	{
		litlen_vflab[i].level = (i < L_down) ? down : up;
		i++;
	}

	// set back loop index to zero
	i = 0;

	// define a bit holder
	UINT_32 bit = 0;

	// assign bits to upper level in tree
	while (i < L_down)
	{
		litlen_vflab[i].assigned_reversed_bits = (UINT_16)(lpe_bit_reverse(bit, (litlen_vflab[i].level)));
		bit++;
		i++;
	}

	// shift one level down
	bit++;
	bit <<= 1;

	// assign bits to lower level in tree
	while ((i >= L_down) && (i < non_empty_entries_count))
	{
		litlen_vflab[i].assigned_reversed_bits = (UINT_16)(lpe_bit_reverse(bit, (litlen_vflab[i].level)));
		bit++;
		i++;
	}

	// return the constructed litlen vflab
	return litlen_vflab;
}

//***********************************************************************************************************

LPE_HUFFMAN_VFLAB* lpe_huffman_create_dist_histogram(LPE_LZ77_LZ77_OUTPUT_PACKAGE* lz77_output)
{
	// allocate memory for dist vflab
	dist_vflab = (LPE_HUFFMAN_VFLAB*)lpe_zero_allocation(LPE_HUFFMAN_DISTANCE_MAX_ENTRIES * sizeof(LPE_HUFFMAN_VFLAB));

	// fill up the value of the vflab by hand
	for (UINT_32 j = 0; j < LPE_HUFFMAN_DISTANCE_MAX_ENTRIES; j++)
		dist_vflab[j].value = j;

	// get the DWORDs of dist buffer in lz77_output buffer
	UINT_32* dist_buffer_dword = (UINT_32*)lz77_output->dist_buffer;

	// get the size of the dist buffer
	UINT_32 dist_buffer_dword_size = lz77_output->dist_buffer_size;

	// update the statically defined dist_dword_size
	dist_dword_size = dist_buffer_dword_size;

	// define a loop index
	UINT_32 i = 0;

	// fill up the vflab
	while (i < dist_buffer_dword_size)
	{
		UINT_32 dword = dist_buffer_dword[i];
		dist_vflab[dword].frequency++;
		i++;
	}

	/* Up to here, we have filled up VFLAB's values and frequencies.
	To find their level, we have to sort them from max to min, and then, we would assign bits to them */

	lpe_huffman_lexicographically_sort_vflab_by_frequency(dist_vflab, LPE_HUFFMAN_DISTANCE_MAX_ENTRIES);

	// acquire the number of non empty entries in vflab
	UINT_32 non_empty_entries_count = lpe_huffman_find_non_empty_vflab_entries(dist_vflab, LPE_HUFFMAN_DISTANCE_MAX_ENTRIES);

	// check the non empty count
	if (non_empty_entries_count == 0)
	{
		printf("distance vflab shows all entries zero --> fatal error\n ");
		lpe_free_allocated_mem(dist_vflab);
		return LPE_HUFFMAN_NULL;
	}

	// get the L_(n-1) and L_(n) borders
	UINT_32 down = 0;
	UINT_32 up   = 0;
	lpe_huffman_nodes_number_two_borders(non_empty_entries_count, &down, &up);

	// set back loop index to zero
	i = 0;

	// set levels (L_(n)-N goes into down level while 2N-L_(n) goes into up level)
	UINT_32 L_down = up - non_empty_entries_count;
	UINT_32 L_up   = (non_empty_entries_count << 1) - up;

	while (i < non_empty_entries_count)
	{
		dist_vflab[i].level = (i < L_down) ? down : up;
		i++;
	}

	// set back loop index to zero
	i = 0;

	// define a bit holder
	UINT_32 bit = 0;

	// assign bits to upper level in tree
	while (i < L_down)
	{
		dist_vflab[i].assigned_reversed_bits = lpe_bit_reverse(bit, (dist_vflab[i].level));
		bit++;
		i++;
	}

	// shift one level down
	bit++;
	bit <<= 1;

	// assign bits to lower level in tree
	while ((i >= L_down) && (i < non_empty_entries_count))
	{
		dist_vflab[i].assigned_reversed_bits = lpe_bit_reverse(bit, (dist_vflab[i].level));
		bit++;
		i++;
	}

	// return the constructed dist vflab
	return dist_vflab;
}

//***********************************************************************************************************

huffman_local void lpe_huffman_lexicographically_sort_vflab_by_frequency(LPE_HUFFMAN_VFLAB* vflab, UINT_32 entry_size)
{
	// make a loop counter
	UINT_32 loop_counter = 0;

	// define the end of search border
	UINT_32 search_border = entry_size - 1;

	// do several passes of searchs (some sort of BUBBLE sort)
	while (search_border > 1)
	{
		while (loop_counter < search_border)
		{
			// in case the frequency of lower index entry is larger, swap it with the next index entry
			if ((vflab[loop_counter].frequency) < vflab[loop_counter + 1].frequency)
				LPE_HUFFMAN_SWAP_VFLAB((&vflab[loop_counter]), (&vflab[loop_counter + 1]));

			// lexicographically_sort: if frequency of lower index entry is equal to that of the higher index entry, check out their value. Lower value goes ahead
			else if ((vflab[loop_counter].frequency) == vflab[loop_counter + 1].frequency)
			{
				if ((vflab[loop_counter].value) > vflab[loop_counter + 1].value)
					LPE_HUFFMAN_SWAP_VFLAB((&vflab[loop_counter]), (&vflab[loop_counter + 1]));
			}

			// go the next entry
			loop_counter++;
		}

		// set the loop counter again to zero
		loop_counter = 0;

		// decrement the search border
		search_border--;
	}
}

//***********************************************************************************************************

huffman_local void lpe_huffman_sort_vflab_by_value(LPE_HUFFMAN_VFLAB* vflab, UINT_32 entry_size)
{
	// make a loop counter
	UINT_32 loop_counter = 0;

	// define the end of search border
	UINT_32 search_border = entry_size - 1;

	// do several passes of searchs (some sort of BUBBLE sort)
	while (search_border > 1)
	{
		while (loop_counter < search_border)
		{
			if ((vflab[loop_counter].value) > vflab[loop_counter + 1].value)
				LPE_HUFFMAN_SWAP_VFLAB((&vflab[loop_counter]), (&vflab[loop_counter + 1]));
			loop_counter++;
		}

		// set the loop counter again to zero
		loop_counter = 0;

		// decrement the search border
		search_border--;
	}
}

//***********************************************************************************************************

void lpe_huffman_handle_vflab_litlen(UINT_32** output_buffer, UINT_32* bits_number, UINT_32 dword_size)
{
	// allocate memory for the output buffer where the actual tree bits are saved
	*output_buffer = (UINT_32*)lpe_zero_allocation(sizeof(UINT_32) * dword_size);

	// sort litlen vflab by its value
	lpe_huffman_sort_vflab_by_value(litlen_vflab, LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES);

	// define an index
	UINT_32 i = 0;

	// go through all assigned bits
	while (i < LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES)
	{	
		// only if the entry has non-zero level, proceed to get its actual bits
		if (litlen_vflab[i].level)
		{
			UINT_32 actual_data = (UINT_32)(litlen_vflab[i].assigned_reversed_bits);
			UINT_32 bit_size    = (UINT_32)(litlen_vflab[i].level);
			lpe_put_bits_in_buffer(*output_buffer, actual_data, bit_size);
			*bits_number = *bits_number + bit_size;
		}
		
		// go the next vflab entry
		i++;
	}

	// in the really end of this tree, we have to make sure that there is no bits un-flushed to the bit stream 
	lpe_final_flush_bit_dword(*output_buffer);
}

//***********************************************************************************************************

void lpe_huffman_handle_vflab_dist(UINT_32** output_buffer, UINT_32* bits_number, UINT_32 dword_size)
{
	// allocate memory for the output buffer where the actual tree bits are saved
	*output_buffer = (UINT_32*)lpe_zero_allocation(sizeof(UINT_32) * dword_size);

	// sort dist vflab by its value
	lpe_huffman_sort_vflab_by_value(dist_vflab, LPE_HUFFMAN_DISTANCE_MAX_ENTRIES);

	// define an index
	UINT_32 i = 0;

	// go through all assigned bits
	while (i < LPE_HUFFMAN_DISTANCE_MAX_ENTRIES)
	{
		// only if the entry has non-zero level, procedd to get its actual bits
		if (dist_vflab[i].level)
		{
			UINT_32 actual_data = (UINT_32)(dist_vflab[i].assigned_reversed_bits);
			UINT_32 bit_size    = (UINT_32)(dist_vflab[i].level);
			lpe_put_bits_in_buffer(*output_buffer, actual_data, bit_size);
			*bits_number = *bits_number + bit_size;
		}

		// go the next vflab entry
		i++;
	}

	// in the really end of this tree, we have to make sure that there is no bits un-flushed to the bit stream 
	lpe_final_flush_bit_dword(*output_buffer);
}

//***********************************************************************************************************

void lpe_huffman_handle_vflab_two_trees(UINT_32** output_buffer, UINT_32* bits_number)
{
	// update the two_trees_dword_size
	two_trees_dword_size = LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES + LPE_HUFFMAN_DISTANCE_MAX_ENTRIES;

	// allocate memory for a tree_of_trees_vflab
	tree_of_trees_vflab = (LPE_HUFFMAN_VFLAB*)lpe_zero_allocation(two_trees_dword_size * sizeof(LPE_HUFFMAN_VFLAB));

	// define a loop index
	UINT_32 i = 0;

	// bring all litlen and dist levels into this vflab's values
	while (i < two_trees_dword_size)
	{
		tree_of_trees_vflab[i].value = (i < LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES) ? (UINT_32)litlen_vflab[i].level : (UINT_32)dist_vflab[i].level;
		i++;
	}

	// make an alias for the output buffer
	UINT_32* two_trees_binary_bits_buffer = *output_buffer;

	// post process the *two_trees_vflab*
	lpe_huffman_post_process_two_trees_vflab(tree_of_trees_vflab, two_trees_dword_size, &two_trees_binary_bits_buffer, bits_number);

	// Safely return to the caller
	return;
}

//***********************************************************************************************************

void lpe_huffman_construct_huffman_binary_tree(UINT_32** binary_bits_buffer, UINT_32* bits_number, TREE_ID id)
{
	switch (id)
	{
	case LITLEN:   
		lpe_huffman_handle_vflab_litlen(binary_bits_buffer, bits_number, litlen_dword_size);
		break;
	case DIST:
		lpe_huffman_handle_vflab_dist(binary_bits_buffer, bits_number, dist_dword_size);
		break;
	case TWO_TREES:
		lpe_huffman_handle_vflab_two_trees(binary_bits_buffer, bits_number);
		break;
	}
}

//***********************************************************************************************************

void lpe_huffman_concatenate_two_bitstreams(UINT_32* target, UINT_32* bit_stream_1, UINT_32* bit_stream_2, UINT_32 bits_number_1, UINT_32 bits_number_2)
{
	// make aliases
	UINT_32* trg        = target;
	UINT_32* bitstream1 = bit_stream_1;
	UINT_32* bitstream2 = bit_stream_2;
	UINT_32  nbits1     = bits_number_1;
	UINT_32  nbits2     = bits_number_2;

	// how many DWORDS do we have in bit stream 1 and 2?
	UINT_32 dwords_in_1 = nbits1 >> 5;
	UINT_32 dwords_in_2 = nbits2 >> 5;

	// how many remaining bits do we get in both bit streams?
	UINT_32 extra_bits_1 = nbits1 & 31;
	UINT_32 extra_bits_2 = nbits2 & 31;

	// define a dword counter
	UINT_32 i = 0;
	while (dwords_in_1)
	{
		// copy all dwords in bit stream 1 into the target stream
		trg[i] = bitstream1[i];

		// decrement the dwords in bit stream 1
		dwords_in_1--;

		// increment the dwords counter
		i++;
	}

	// in case we have any extra bits remained in bit stream 1
	if (extra_bits_1)
	{
		// simply copy the whole dword since it contains the remaining bits plus extra zeros
		trg[i] = bitstream1[i];

		// now make a copy of the target dwords counter
		UINT_32 j = i;

		// set back the dword counter to zero
		i = 0;

		// we need to know how many bits in the last dword in target have been already consumed
		UINT_32 remained = extra_bits_1;

		// define a place holder for the position of the current bit in taget stream
		UINT_32 trg_bit_position = remained;

		// define a counter for the number of bits in bit stream 2
		UINT_32 src_bit_counter = 0;

		// loop until all bits in bit stream 2 will be getting consumed
		while (nbits2)
		{
			// do the actual extraction of single bits and copying until we reach to the end of dword in the target stream
			while (trg_bit_position < 32)
			{
				// important sanity check: in case nbits2 reaches zero, we are done with anything
				if (!nbits2)
					break;

				// extract the current bit
				UINT_32 src_bit = ((bitstream2[i] & (1 << src_bit_counter)) >> src_bit_counter);

				// copy it into the target
				trg[j] |= (src_bit << trg_bit_position);

				// increment the position of the target bit
				trg_bit_position++;

				// increment the counter for the number of bits in bit stream 2
				src_bit_counter++;

				// decrement the total bits in bit stream 2 by one
				nbits2--;

				// did we consume a whole dword from bit stream 2
				if (src_bit_counter == 32)
				{
					// if so, set back the counter to zero
					src_bit_counter = 0;

					// and go to the next dword 
					i++;
				}
			}

			// if we reached to the end of the 32nd bit position in target, we have to jump to the next bit position in target simply by incrementing the target dwords counter
			j++;

			// and simply set back the target bit position to zero
			trg_bit_position = 0;
		}
	}

	// but in case we do NOT have any extra bits remained in bit stream 1
	else
	{
		// now make a copy of the target dwords counter
		UINT_32 j = i;

		// set back the dwords counter to zero
		i = 0;

		// we will simply copy the whole bit stream 2 as well since they are aligned
		while (dwords_in_2)
		{
			// copy all dwords in bit stream 2 into the target stream
			trg[j] = bitstream2[i];

			// decrement the dwords in bit stream 1
			dwords_in_2--;

			// increment the dwords counter
			i++;

			// increment the target dword counter
			j++;
		}

		// in case we have any extra bits remained in bit stream 2, copy the whole dword since it contains the remaining bits plus extra zeros
		if (extra_bits_2)

			// do the last bits copying
			trg[j] = bitstream2[i];
	}
}

//***********************************************************************************************************

huffman_local void lpe_huffman_shift_buffer_entries(LPE_HUFFMAN_VFLAB* vflab, UINT_32 vflab_counter, UINT_32 start, UINT_32 end)
{
	// define a loop index
	UINT_32 i = 0;

	// loop to the end of the buffer
	while (i < vflab_counter - 1)
	{
		// do the actual shift
		vflab[start + i].value = vflab[end + 1 + i].value;

		// increment the loop index
		i++;
	}
}

//***********************************************************************************************************

huffman_local void lpe_huffman_eliminate_tagged_vflab_entries(LPE_HUFFMAN_VFLAB* vflab, UINT_32 vflab_counter, UINT_32* updated_vflab_counter)
{
	// define a loop index
	UINT_32 i = 0;

	// define a holder for index where elimination starts
	UINT_32 start = LPE_HUFFMAN_ELIMINATION_START_END;

	// define a holder for index where elimination ends
	UINT_32 end = LPE_HUFFMAN_ELIMINATION_START_END;

	// loop to the end of the buffer
	while (i < vflab_counter)
	{
		// keep track of eliminations until the end of *ELIMINATION_TAG*
		while (vflab[i].value == LPE_HUFFMAN_ELIMINATION_TAG)
		{
			// determine if we have to keep starting point or it has been already kept
			if (start == LPE_HUFFMAN_ELIMINATION_START_END)
				start = i;

			// keep track of ending point
			end = i;

			// go ahead with elimination tracking
			i++;
		}

		// only if we set a starting point for elimination, give the buffer for elimination
		if (start != LPE_HUFFMAN_ELIMINATION_START_END)
		{
			// shift left all other values (the actual elimination)
			lpe_huffman_shift_buffer_entries(vflab, vflab_counter, start, end);

			// update the left vflab counter
			vflab_counter -= (end - start + 1);

			// set back the start point to LPE_HUFFMAN_ELIMINATION_START_END
			start = LPE_HUFFMAN_ELIMINATION_START_END;

			// set back the end point to LPE_HUFFMAN_ELIMINATION_START_END
			end = LPE_HUFFMAN_ELIMINATION_START_END;

			// for sanity, we keep checking eliminations, all from the beginning. So, set back i to zero
			i = 0;
		}

		// if no starting point was set, go ahead in buffer
		else
			i++;
	}

	// in the end, copy the new vflab counter to its holder
	*updated_vflab_counter = vflab_counter;
}

//***********************************************************************************************************

huffman_local void lpe_huffman_give_repetition_tag(LPE_HUFFMAN_VFLAB* vflab, UINT_32 updated_vflab_counter_after_elimination)
{
	// define a loop index
	UINT_32 i = 0;

	// loop to the end of the buffer
	while (i < updated_vflab_counter_after_elimination)
	{
		// give repetition byte a tag, if any
		if ((vflab[i].value == 16) || (vflab[i].value == 17) || (vflab[i].value == 18))
		{
			// jump over the especial entry
			i++;

			// give a tag to the repetition entry
			vflab[i].value = LPE_HUFFMAN_NEGLECTED_DWORD;
		}

		// increment the loop index
		i++;
	}
}

//***********************************************************************************************************

huffman_local void lpe_huffman_which_index_is_zero_with_repetition(LPE_HUFFMAN_VFLAB* vflab, 
                                                                   UINT_32            start_index, 
                                                                   UINT_32            end_index,
                                                                   UINT_32*           zero_contained_index, 
                                                                   UINT_32*           repetition)
{
	// define a loop index
	UINT_32 i = start_index;

	// define a repetition counter
	UINT_32 counter = 0;

	// loop until facing the end of the first occurance of the repetition
	while (i < end_index)
	{
		// if we face a non-zero value, we are done with our search
		if (vflab[i].value != 0)
		{
			// keep note of the index at which the repetition started and also the number of the repetition
			*repetition = counter;
			*zero_contained_index = i;

			// safely return
			return;
		}

		// if the value is zero, we are still recording zero occurances
		else
			counter++;

		// go ahead in buffer
		i++;
	}
}

//***********************************************************************************************************

huffman_local void lpe_huffman_which_index_is_duplicated_with_repetition(LPE_HUFFMAN_VFLAB* vflab,
                                                                         UINT_32            start_index,
                                                                         UINT_32            end_index,
                                                                         UINT_32*           repetition_contained_index,
                                                                         UINT_32*           repetition)
{
	// define a loop index
	UINT_32 i = start_index + 1;

	// define a repetition counter
	UINT_32 counter = 0;

	// loop until facing the end of the first occurance of the repetition
	while (i < end_index)
	{
		// if we face a non-equal value, we are done with our search
		if (vflab[i].value != vflab[i - 1].value)
		{
			// keep note of the index at which the repetition started and also the number of the repetition
			*repetition = counter + 1;
			*repetition_contained_index = i;

			// safely return
			return;
		}

		// if values are still equal, we would record the equal occurances
		else
		{
			// only if the equal repetition is *non-zero* value
			if (vflab[i].value != 0)
				counter++;
		}

		// go ahead in buffer
		i++;
	}
}

//***********************************************************************************************************

void lpe_huffman_assign_frequency(LPE_HUFFMAN_VFLAB* vflab, UINT_32 eliminated_count)
{
	// define a loop counter
	UINT_32 i = 0;

	// loop through all of the entries
	while (i < eliminated_count)
	{
		// a small sanity check
		if (vflab[i].value == LPE_HUFFMAN_NEGLECTED_DWORD)
			goto lpe_huffman_assign_frequency_end;

		// increment the frequency of this entry
		lpe_huffman_two_tree_freq[vflab[i].value]++;

		// in case of especial values, take care of the repetition entry which was tagged before
		if ((vflab[i].value == 16) || (vflab[i].value == 17) || (vflab[i].value == 18))
		{
			// jump over the especial entry to be on top of the repetition entry which was tagged before
			i++;

			// make sure that it was eliminated before, otherwise there must be an error
			LPE_HUFFMAN_ASSERT((vflab[i].value), LPE_HUFFMAN_NEGLECTED_DWORD);
		}

		// go ahead in buffer
lpe_huffman_assign_frequency_end:
		i++;			
	}
}

//***********************************************************************************************************

void lpe_huffman_set_assigned_bits(LPE_HUFFMAN_VFLAB* in_buffer, UINT_32 eliminated_count)
{
	lpe_huffman_lexicographically_sort_vflab_by_frequency(in_buffer, eliminated_count);

	// get rid of "LPE_HUFFMAN_NEGLECTED_DWORD" vflab entries which are in the beginning of the sorted list
	UINT_32 i = 0;

	//set a pointer to the start of non-LPE_HUFFMAN_NEGLECTED_DWORD entry
	LPE_HUFFMAN_VFLAB* v = in_buffer;

	while (i < eliminated_count)
	{
		if (in_buffer[i].level != LPE_HUFFMAN_NEGLECTED_DWORD)
			break;
		i++;
		v++;
	}

	// now v is set to the start of the non-LPE_HUFFMAN_NEGLECTED_DWORD entry. Get the number of non empty entries in vflab
	UINT_32 non_empty_entries_count = lpe_huffman_find_non_empty_vflab_entries(v, eliminated_count);

	// get the L_(n-1) and L_(n) borders
	UINT_32 down = 0;
	UINT_32 up = 0;
	lpe_huffman_nodes_number_two_borders(non_empty_entries_count, &down, &up);

	// set back loop index to zero
	i = 0;

	// set levels (L_(n)-N goes into down level while 2N-L_(n) goes into up level)
	UINT_32 L_down = up - non_empty_entries_count;
	UINT_32 L_up   = (non_empty_entries_count << 1) - up;

	while (i < non_empty_entries_count)
	{
		v[i].level = (i < L_down) ? down : up;
		i++;
	}

	// set back loop index to zero
	i = 0;

	// define a bit holder
	UINT_32 bit = 0;

	// assign bits to upper level in tree
	while (i < L_down)
	{
		dist_vflab[i].assigned_reversed_bits = (UINT_16)(lpe_bit_reverse(bit, (dist_vflab[i].level)));
		bit++;
		i++;
	}

	// shift one level down
	bit++;
	bit <<= 1;

	// assign bits to lower level in tree
	while ((i >= L_down) && (i < non_empty_entries_count))
	{
		dist_vflab[i].assigned_reversed_bits = (UINT_16)(lpe_bit_reverse(bit, (dist_vflab[i].level)));
		bit++;
		i++;
	}
}

//***********************************************************************************************************

void lpe_huffman_post_process_two_trees_vflab(LPE_HUFFMAN_VFLAB* vflab, UINT_32 vflab_counter, UINT_32** two_trees_binary_bits_buffer, UINT_32* bits_number_holder)
{
	// define a loop index
	UINT_32 i = 0;

	// make a holder to keep the zero repetition occurance
	UINT_32 zero_contained_index = 0;

	// make a holder to keep the non-zero repetition occurance
	UINT_32 duplication_contained_index = 0;

	// make a holder for the number of repetition
	UINT_32 repetition = 0;

	// loop until the end of the buffer
	while (i < vflab_counter)
	{
		// find the index at which the zero repetition occured plus its repetition value
		lpe_huffman_which_index_is_zero_with_repetition(vflab, i, vflab_counter, &zero_contained_index, &repetition);

		// decide how many times it occured and give a right *VALUE* to it
		if (repetition >= 3 && repetition <= 10)
		{
			// level 17
			vflab[i    ].value = 17;
			vflab[i + 1].value = repetition;

			// define an elimination tag count
			UINT_32 elimination_count = repetition - 2;

			// define a loop counter
			UINT_32 j = 0;

			// tag the rest of the repetition values with elimination tag
			while (j < elimination_count)
			{
				vflab[i + j + 2].value = LPE_HUFFMAN_ELIMINATION_TAG;
				j++;
			}

			// jump over all repeated values in buffer
			i = zero_contained_index;
		}

		else if (repetition >= 11 && repetition <= 138)
		{
			// level 18
			vflab[i    ].value = 18;
			vflab[i + 1].value = repetition;

			// define an elimination tag count
			UINT_32 elimination_count = repetition - 2;

			// define a loop counter
			UINT_32 j = 0;

			// tag the rest of the repetition values with elimination tag
			while (j < elimination_count)
			{
				vflab[i + j + 2].value = LPE_HUFFMAN_ELIMINATION_TAG;
				j++;
			}

			// jump over all repeated values in buffer
			i = zero_contained_index;
		}

		// if there was no zero repetition, go ahead in buffer
		else
			i++;
	}

	// set back the loop index to zero
	i = 0;

	// loop until the end of the buffer
	while (i < vflab_counter)
	{
		// find the index at which the non-zero repetition occured plus its repetition value
		lpe_huffman_which_index_is_duplicated_with_repetition(vflab, i, vflab_counter, &duplication_contained_index, &repetition);

		// decide how many times it occured and give a right *VALUE* to it
		if (repetition >= 3 && repetition <= 6)
		{
			// level 16
			vflab[i    ].value = 16;
			vflab[i + 1].value = repetition;

			// define an elimination tag count
			UINT_32 elimination_count = repetition - 2;

			// define a loop counter
			UINT_32 j = 0;

			// tag the rest of the repetition values with elimination tag
			while (j < elimination_count)
			{
				vflab[i + j + 2].value = LPE_HUFFMAN_ELIMINATION_TAG;
				j++;
			}

			// jump over all repeated values in buffer
			i = duplication_contained_index;
		}

		// if there was no non-zero repetition, go ahead in buffer
		else
			i++;
	}

	// define a holder to keep track of the updated vflab counter after elimination
	UINT_32 updated_vflab_counter_after_elimination = vflab_counter;

	// eliminate the tagged values (a.k.a. bring non-tagged vflab entries shifted next to each other and update the vflab counter)
	lpe_huffman_eliminate_tagged_vflab_entries(vflab, vflab_counter, &updated_vflab_counter_after_elimination);

	// give a tag value to the actual repetition entries (entries after the especial entries)
	lpe_huffman_give_repetition_tag(vflab, updated_vflab_counter_after_elimination);

	// give frequencies to the entries
	lpe_huffman_assign_frequency(vflab, updated_vflab_counter_after_elimination);

	// make a new vflab data structure
	LPE_HUFFMAN_VFLAB cpy_vflab[19];

	// extract cpy_vflab entries
	lpe_huffman_assign_copy_vflab(cpy_vflab);

	// sort vflab entries by frequency
	lpe_huffman_lexicographically_sort_vflab_by_frequency(cpy_vflab, 19);

	// define a non_empty count holder
	UINT_32 non_empty_entries_count = 0;

	// get the non_empty_entries_count
	lpe_huffman_find_non_empty_vflab_entries(cpy_vflab, 19);

	// get the L_(n-1) and L_(n) borders
	UINT_32 down = 0;
	UINT_32 up   = 0;
	lpe_huffman_nodes_number_two_borders(non_empty_entries_count, &down, &up);

	// set back loop index to zero
	i = 0;

	// set levels (L_(n)-N goes into down level while 2N-L_(n) goes into up level)
	UINT_32 L_down = up - non_empty_entries_count;
	UINT_32 L_up = (non_empty_entries_count << 1) - up;

	while (i < non_empty_entries_count)
	{
		cpy_vflab[i].level = (i < L_down) ? down : up;
		i++;
	}

	// set back loop index to zero
	i = 0;

	// define a bit holder
	UINT_32 bit = 0;

	// assign bits to upper level in tree
	while (i < L_down)
	{
		cpy_vflab[i].assigned_reversed_bits = (UINT_16)(lpe_bit_reverse(bit, (cpy_vflab[i].level)));
		bit++;
		i++;
	}

	// shift one level down
	bit++;
	bit <<= 1;

	// assign bits to lower level in tree
	while ((i >= L_down) && (i < non_empty_entries_count))
	{
		cpy_vflab[i].assigned_reversed_bits = (UINT_16)(lpe_bit_reverse(bit, (cpy_vflab[i].level)));
		bit++;
		i++;
	}

	/* \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
	\\\\    after elimination of tagged entries:                                                                                                               \\\\
	\\\\            +----------------------------------------------------------------------------+                                                             \\\\
	\\\\    value:  | 0x07 | 0x07 | 0x07 | 0x11 | 0x09 | 0x02 | 0x10 | 0x05 | 0x00 | 0x01 | 0x03 |                                                             \\\\
	\\\\            +----------------------------------------------------------------------------+                                                             \\\\
	\\\\                                                                                                                                                       \\\\
	\\\\    give repetition values a new tag (bytes after 16, 17 and 18 must be tagged)                                                                        \\\\
	\\\\            +----------------------------------------------------------------------------+                                                             \\\\
	\\\\    value:  | 0x07 | 0x07 | 0x07 | 0x11 | 0x09 | 0x02 | 0x10 | 0x05 | 0x00 | 0x01 | 0x03 |                                                             \\\\
	\\\\            +----------------------------------------------------------------------------+                                                             \\\\
	\\\\    tagged: | 0x00 | 0x00 | 0x00 | 0x00 | 0x01 | 0x00 | 0x00 | 0x01 | 0x00 | 0x00 | 0x00 |                                                             \\\\
	\\\\            +----------------------------------------------------------------------------+                                                             \\\\
	\\\\                                                                                                                                                       \\\\
	\\\\    Then, it must give such a frequency (tagged values have been neglected)                                                                            \\\\
	\\\\    freq       -> 1  -  1  -  1  -  1  -  0  -  0  -  0  -  3  -  0  -  0  -  0  -  0  -  0  -  0  -  0  -  0  -  1  -  1  -  0                        \\\\
	\\\\    At this stage, make a copy of VFLAB above. Then sort max-to-min ordered by frequency                                                               \\\\
	\\\\    max_to_min -> 7  -  0  -  1  -  2  -  3  -  16 -  17 -  4  -  5  -  6  -  8  -  9  -  10 -  11 -  12 -  13 -  14 -  15 - 18                        \\\\
	\\\\                                                                                                                                                       \\\\
	\\\\    In the end, it should give assigned reversed bits to each value in vflab                                                                           \\\\
	\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */

	// allocate memory for the provided buffer
	*two_trees_binary_bits_buffer = (UINT_32*)lpe_zero_allocation(two_trees_dword_size * sizeof(UINT_32));

	// now, we have a ready vflab at hand. We are able to put assigned bits in the provided buffer
	lpe_huffman_generate_binary_output_stream(*two_trees_binary_bits_buffer, vflab, updated_vflab_counter_after_elimination, cpy_vflab, 19, bits_number_holder);
}

//***********************************************************************************************************

huffman_local void lpe_huffman_assign_copy_vflab(LPE_HUFFMAN_VFLAB* new_vflab)
{
	// make a loop index
	UINT_32 i = 0;

	// loop through the new vflab entries and set values and frequencies
	while (i < 19)
	{
		new_vflab[i].level     = i;
		new_vflab[i].frequency = lpe_huffman_two_tree_freq[i];

		// increment the loop index
		i++;
	}
}

//***********************************************************************************************************

huffman_local void lpe_huffman_generate_binary_output_stream(UINT_32*           bit_buffer, 
                                                             LPE_HUFFMAN_VFLAB* vflab, 
                                                             UINT_32            vflab_counter, 
                                                             LPE_HUFFMAN_VFLAB* freq_vflab, 
                                                             UINT_32            freq_vflab_counter,
                                                             UINT_32*           bits_number_holder)
{
	// define a loop index
	UINT_32 i = 0;

	// loop through all of the vflab entries
	while (i < vflab_counter)
	{
		// handle values between 0-15
		if ((vflab[i].value >= 0) && (vflab[i].value < 16))
		{
			// acquire the actual bits
			UINT_32 bits = (UINT_32)(freq_vflab[vflab[i].value].assigned_reversed_bits);

			// find the bits number
			UINT_32 bit_size = freq_vflab[vflab[i].value].level;

			// put them in the buffer
			lpe_put_bits_in_buffer(bit_buffer, bits, bit_size);

			// update the bits_number
			*bits_number_holder = *bits_number_holder + bit_size;
		}

		// handle values 16, 17, and 18 plus their repetition bits
		else if ((vflab[i].value == 16) || (vflab[i].value == 17) || (vflab[i].value == 18))
		{
			// acquire the actual bits
			UINT_32 bits = (UINT_32)(freq_vflab[vflab[i].value].assigned_reversed_bits);

			// find the bits number
			UINT_32 bit_size = freq_vflab[vflab[i].value].level;

			// put them in the buffer
			lpe_put_bits_in_buffer(bit_buffer, bits, bit_size);

			// update the bits_number
			*bits_number_holder = *bits_number_holder + bit_size;

			// define an extra bits number holder
			UINT_32 extra_bits_number = 0;

			// is the especial value, 16?
			if (vflab[i].value == 16)
			{
				// jump over the especial entry which was handeled above
				i++;

				// find the actual bits related to this extra bits from tables
				bits = (vflab[i].value) - 3;

				// put them in the buffer
				lpe_put_bits_in_buffer(bit_buffer, bits, 2);

				// update the bits_number
				*bits_number_holder = *bits_number_holder + 2;
			}
			
			// or, it is value 17?
			else if (vflab[i].value == 17)
			{
				// jump over the especial entry which was handeled above
				i++;

				// find the actual bits related to this extra bits from tables
				bits = (vflab[i].value) - 3;

				// put them in the buffer
				lpe_put_bits_in_buffer(bit_buffer, bits, 3);

				// update the bits_number
				*bits_number_holder = *bits_number_holder + 3;
			}

			// or, it is value 18?
			else if (vflab[i].value == 18)
			{
				// jump over the especial entry which was handeled above
				i++;

				// find the actual bits related to this extra bits from tables
				bits = (vflab[i].value) - 11;

				// put them in the buffer
				lpe_put_bits_in_buffer(bit_buffer, bits, 7);

				// update the bits_number
				*bits_number_holder = *bits_number_holder + 7;
			}
		}

		// increment the loop index
		i++;
	}

	// in the really end, we have to make sure that there is no bits un-flushed to the bit buffer 
	lpe_final_flush_bit_dword(bit_buffer);

	// define the order of the code bit length
	UINT_32 order[]  = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

	// prepare the actual code bit length tree
	code_bit_lengths = (UINT_32*)lpe_zero_allocation(19 * sizeof(UINT_32));

	// set the loop counter to zero
	i = 0;

	// loop through all 19 entries
	while (i < 19)
	{
		code_bit_lengths[order[i]] = lpe_bit_reverse((UINT_32)(freq_vflab[order[i]].assigned_reversed_bits), freq_vflab[i].level); // << ------- OR REVERSED OF REVERSED ------

		// go ahead
		i++;
	}

	// safely return
	return;
}

//***********************************************************************************************************

huffman_local LPE_HUFFMAN_VFLAB* lpe_huffman_find_literal_length_in_vflab_buffer(UINT_32 current_litlen)
{
	// define a loop index
	UINT_32 i = 0;

	// loop until the end of the vflab buffer
	while (i < LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES)
	{
		// did we hit the demanded vflab entry?
		if (litlen_vflab[i].value == current_litlen)

			// return the address of the entry
			return &litlen_vflab[i];

		// go ahead in litlen vflab buffer
		i++;
	}

	// we made it up until here, there is an error
	return LPE_HUFFMAN_NULL;
}

//***********************************************************************************************************


huffman_local LPE_HUFFMAN_VFLAB* lpe_huffman_find_dist_in_vflab_buffer(UINT_32 distance_code)
{
	// define a loop index
	UINT_32 i = 0;

	// loop until the end of the vflab buffer
	while (i < LPE_HUFFMAN_DISTANCE_MAX_ENTRIES)
	{
		// did we hit the demanded vflab entry?
		if (dist_vflab[i].value == distance_code)

			// return the address of the entry
			return &dist_vflab[i];

		// go ahead in dist vflab buffer
		i++;
	}

	// we made it up until here, there is an error
	return LPE_HUFFMAN_NULL;
}

//***********************************************************************************************************

void lpe_huffman_encode_actual_data(LPE_LZ77_LZ77_OUTPUT_PACKAGE* lz77_output, UINT_32** encoded_data, UINT_32* encoded_data_size)
{
	// as a rule of thumb, allocate necessary memory for the output buffer, by 4x size of the initial dwords
	*encoded_data = (UINT_32*)lpe_zero_allocation(((lz77_output->litlen_buffer_size + lz77_output->dist_buffer_size) * sizeof(UINT_32)) << 2);

	//make an alias for the provided buffer
	UINT_32* out_buffer = *encoded_data;

	// make an alias for litlen and dist buffers
	UINT_32* litlen = (UINT_32*)lz77_output->litlen_buffer;
	UINT_32* dist   = (UINT_32*)lz77_output->dist_buffer;

	// define a litlen buffer index
	UINT_32 litlen_index = 0;

	// define a dist buffer index
	UINT_32 dist_index = 0;

	// define a vflab entry holder
	LPE_HUFFMAN_VFLAB* vflab = LPE_HUFFMAN_NULL;

	// loop until the end of the litlen data
	while (litlen_index < lz77_output->litlen_buffer_size)
	{
		// get the next litlen value as a DWORD
		UINT_32 current_litlen = litlen[litlen_index];

		// if the gotten DWORD is literal
		if (LPE_LZ77_IS_LITERAL(current_litlen))
		{
			// find the corresponding vflab entry
			vflab = lpe_huffman_find_literal_length_in_vflab_buffer(current_litlen);

			// Is it a real entry that we found?
			if (vflab == LPE_HUFFMAN_NULL)
			{
				printf("In encoding the actual data from LITLEN vflab buffer, there is a fatal error\n");
				lpe_free_allocated_mem(out_buffer);
				return;
			}

			// get the bits and the bits count
			UINT_32 bits_size = vflab->level;
			UINT_32 bits      = lpe_bit_reverse(vflab->assigned_reversed_bits, bits_size); // or reversed? << ---------- TODO ---------- <<

			// put it in output buffer
			lpe_put_bits_in_buffer(out_buffer, bits, bits_size);

			// update the output buffer counter
			*encoded_data_size = *encoded_data_size + bits_size;
		}

		// otherwise, if the gotten DWORD is length
		else if (LPE_LZ77_IS_LENGTH(current_litlen))
		{
			// find the corresponding vflab entry
			vflab = lpe_huffman_find_literal_length_in_vflab_buffer(current_litlen);

			// Is it a real entry that we found?
			if (vflab == LPE_HUFFMAN_NULL)
			{
				printf("In encoding the actual data from LITLEN vflab buffer, there is a fatal error\n");
				lpe_free_allocated_mem(out_buffer);
				return;
			}

			// get the bits and the bits count
			UINT_32 bits_size = vflab->level;
			UINT_32 bits      = lpe_bit_reverse(vflab->assigned_reversed_bits, bits_size); // or reversed? << ---------- TODO ---------- <<

			// put it in output buffer
			lpe_put_bits_in_buffer(out_buffer, bits, bits_size);

			// determine how many litlen extra bits needed to be imposed
			UINT_32 length_extra_bits_number = lengths_table_extra_bits[current_litlen];

			// get the length value. It is one DWORD after length code in litlen buffer
			litlen_index++;
			UINT_32 length = litlen[litlen_index];

			// the *length* minus the first occurance of the *current_litlen* in lengths_table determines the right bits
			UINT_32 first_index = lpe_huffman_first_occurance_of_this_number(current_litlen, lengths_table, 259);

			// the right bits
			bits = length - first_index;

			// sanity check
			if (bits < 0)
			{
				printf("There is a fatal error in LITLEN extra bits determination decifering\n");
				lpe_free_allocated_mem(out_buffer);
				return;
			}

			// put the length extra bits in output buffer
			lpe_put_bits_in_buffer(out_buffer, bits, length_extra_bits_number);

			// update the output buffer counter
			*encoded_data_size = *encoded_data_size + (bits_size + length_extra_bits_number);

			// find the distance value
			UINT_32 current_dist = dist[dist_index];

			// find the corresponding distance code
			UINT_32 distance_code = distance_table[current_dist];

			// find the corresponding vflab entry
			vflab = lpe_huffman_find_dist_in_vflab_buffer(distance_code);

			// Is it a real entry that we found?
			if (vflab == LPE_HUFFMAN_NULL)
			{
				printf("In encoding the actual data from DIST vflab buffer, there is a fatal error\n");
				lpe_free_allocated_mem(out_buffer);
				return;
			}

			// get the bits and the bits count
			bits_size = vflab->level;
			bits      = lpe_bit_reverse(vflab->assigned_reversed_bits, bits_size); // or reversed? << ---------- TODO ---------- <<

			// put it in output buffer
			lpe_put_bits_in_buffer(out_buffer, bits, bits_size);

			// determine how many distance extra bits needed to be imposed
			UINT_32 dist_extra_bits_number = distance_table_extra_bits[current_dist];

			// the *length* minus the first occurance of the *current_litlen* in lengths_table determines the right bits
			first_index = lpe_huffman_first_occurance_of_this_number(distance_code, distance_table, KB(32));

			// the *current_dist* minus the first occurance of the *first index* in distance_table determines the right bits
			bits = current_dist - first_index;

			// sanity check
			if (bits < 0)
			{
				printf("There is a fatal error in DIST extra bits determination decifering\n");
				lpe_free_allocated_mem(out_buffer);
				return;
			}

			// put the length extra bits in output buffer
			lpe_put_bits_in_buffer(out_buffer, bits, dist_extra_bits_number);

			// update the output buffer counter
			*encoded_data_size = *encoded_data_size + (bits_size + dist_extra_bits_number);

			// go ahead in distance buffer
			dist_index++;
		}

		// go ahead in litlen buffer
		litlen_index++;
	}

	// check out if there is any un-flushed bits
	lpe_final_flush_bit_dword(out_buffer);

	// safely return to the caller
	return;
}

//***********************************************************************************************************

huffman_local UINT_32 lpe_huffman_first_occurance_of_this_number(UINT_32 num, UINT_32* table, UINT_32 entries)
{
	// define a loop index
	UINT_32 i = 0;

	// loop to the end of table
	while (i < entries)
	{
		// in case we found the hit
		if (table[i] == num)

			// return the index that we found
			return i;
	
		// go ahead
		i++;
	}

	// here means an error
	return ~0;
}

//***********************************************************************************************************

huffman_local UINT_32 lpe_huffman_find_non_empty_code_bit_lengths_entries(void)
{
	//define the result holder
	UINT_32 result = 0;

	// define a loop index
	UINT_32 i = 0;

	// loop to the end of the *code_bit_lengths* buffer
	while (i < 19)
	{
		// check if this entry shows non_zero value
		if (code_bit_lengths[i])

			// add one to the output result
			result++;
			
		// increment the index
		i++;
	}

	// return the obtained result
	return result;
}

//***********************************************************************************************************

UINT_32 lpe_huffman_get_header_values(UINT_32* header, UINT_32* hlit, UINT_32* hdist, UINT_32* hclen)
{
	// define the result holder
	UINT_32 result = 0;

	// tag the header with *DYNAMIC HUFFMAN* method
	*header |= LPE_HUFFMAN_DYNAMIC_HUFFMAN_METHOD;

	// update the hlit value (for this, we need the number of non-zero entries in the "litlen_vflab")
	lpe_huffman_sort_vflab_by_value                  (litlen_vflab, LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES);
	*hlit = (lpe_huffman_find_non_empty_vflab_entries(litlen_vflab, LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES) - 257);
	 
	// update the hdist value (for this, we need the number of non-zero entries in the "dist_vflab")
	lpe_huffman_sort_vflab_by_value                   (dist_vflab, LPE_HUFFMAN_DISTANCE_MAX_ENTRIES);
	*hdist = (lpe_huffman_find_non_empty_vflab_entries(dist_vflab, LPE_HUFFMAN_DISTANCE_MAX_ENTRIES) - 1);

	// update the hclen value (for this, we need the number of non-zero entries in the "code_bit_lengths")
	*hdist = (lpe_huffman_find_non_empty_code_bit_lengths_entries() - 4);

	// set these values into the result
	result |= (((*header) << 0) | ((*hlit) << 3) | ((*hdist) << 8) | ((*hclen) << 13));

	// safely return the obtained result to the caller
	return result;
}

//***********************************************************************************************************

UINT_32* lpe_huffman_get_code_bit_lengths(void)
{
	return huffman_local_call code_bit_lengths;
}

//***********************************************************************************************************

void lpe_huffman_free_vflabs(void)
{
	lpe_free_allocated_mem(litlen_vflab);
	lpe_free_allocated_mem(dist_vflab);
	lpe_free_allocated_mem(tree_of_trees_vflab);
	litlen_vflab         = LPE_HUFFMAN_NULL;
	dist_vflab           = LPE_HUFFMAN_NULL;
	tree_of_trees_vflab  = LPE_HUFFMAN_NULL;
	litlen_dword_size    = 0;
	dist_dword_size      = 0;
	two_trees_dword_size = 0;
	memset(lpe_huffman_two_tree_freq, 0, 19);
}

//***********************************************************************************************************

huffman_local void lpe_huffman_bit_dumper(LPE_HUFFMAN_VFLAB* vflab_entry, char* out)
{
	// get the number of bits to be printed
	UINT_32 size      = vflab_entry->level;

	// get the bit stream of this entry
	UINT_16 bitstream = vflab_entry->assigned_reversed_bits;

	// loop through all bits
	for (size_t i = 0; i < size; i++)
	{
		// retrieve the bit
		UINT_8 val = (UINT_8)(bitstream & LPE_BIT(i)) >> i;

		// convert to character
		char ch = (val == 0) ? '0' : '1';

		// put it in its right place at output buffer
		out[size - 1 - i] = ch;
	}

	// NULL-terminate the buffer
	out[size] = 0;
}

//***********************************************************************************************************

void lpe_huffman_dump_vflab(LPE_HUFFMAN_VFLAB* vflab, UINT_32 entries)
{
	// sanity check to see if we have a valid vflab
	if ((!vflab) || (!entries))
	{
		printf("invalid VFLAB at address 0x%x and size %u. Fatal error!\n", vflab, entries);
		return;
	}

	// define an indexer
	UINT_32 i = 0;

	// define a string holder buffer
	char tmp_buffer[0x20];

	// zero it out
	memset(tmp_buffer, 0, 0x20);

	// loop through all vflab's entry
	while(i < entries)
	{
		// retrieve the assigned reverse bits as string
		lpe_huffman_bit_dumper(&vflab[i], tmp_buffer);

		// print all fields out
		printf("INDEX: %i -> VALUE: %i | FREQ: %i | LEVEL: %i | ASSIGNED REVERSED BITS: %s\n", i, vflab[i].value, vflab[i].frequency, vflab[i].level, tmp_buffer);
		
		// go to the next entry
		i++;
	}
}

//***********************************************************************************************************