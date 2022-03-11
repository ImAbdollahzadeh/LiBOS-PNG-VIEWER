//***********************************************************************************************************

#include "LPE_FILTERIZER.h"
#include <iostream>

//***********************************************************************************************************

static UINT_32 array_min_index(UINT_32* arr, UINT_32 size) 
{
	UINT_32 min = ~0;
	UINT_32 ret = ~0;
	for (UINT_32 i = 0; i < size; i++)
	{
		if (arr[i] < min)
		{
			min = arr[i];
			ret = i;
		}
	}
	return ret;
}

//***********************************************************************************************************

static INT_32 lpe_peath_predictor(INT_32 a, INT_32 b, INT_32 c)
{
	INT_32 p = a + b - c;
	INT_32 pa = LPE_ABS((p - a));
	INT_32 pb = LPE_ABS((p - b));
	INT_32 pc = LPE_ABS((p - c));
	return (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;
}

//***********************************************************************************************************

static UINT_32 lpe_heuristic_filter_determination_of_row(UINT_8* src, UINT_32 bytes, UINT_8 bpp)
{
	// define a 5 value array to store upcoming values
	UINT_32 filters_result[5] = { 0, 0, 0, 0, 0 };

	// make an alias of bytes
	UINT_32 total = bytes;

	// make an alias of src
	UINT_8* ptr = src;

	// define a 32 bit sum of all bytes in this row
	UINT_32 sum_of_all_bytes_in_this_row = 0;

	// define a pixel value holder
	UINT_8 current_value = 0;

	// define an loop index
	UINT_32 i = 0;

	/* -----------------------------------------------------------------------------------*/
	/* one by one test each filter type and choose the one resulting in the minimum value */
	/* -----------------------------------------------------------------------------------*/

	// (1) none
	while (total--)
		sum_of_all_bytes_in_this_row += (UINT_32)*ptr++;
	filters_result[LPE_NONE_FILTER] = sum_of_all_bytes_in_this_row;

	// set back the sum_of_all_bytes_in_this_row
	sum_of_all_bytes_in_this_row = 0;

	// set back the ptr
	ptr = src;

	// set back total bytes
	total = bytes;

	// (2) sub
	total -= bpp;
	for (i = 0; i < bpp; i++)
		sum_of_all_bytes_in_this_row += (UINT_32)*ptr++;

	while (total--)
	{
		current_value = (*ptr - *(ptr - bpp));
		ptr++;
		sum_of_all_bytes_in_this_row += (UINT_32)current_value;
	}
	filters_result[LPE_SUB_FILTER] = sum_of_all_bytes_in_this_row;

	// set back the sum_of_all_bytes_in_this_row and current_value
	sum_of_all_bytes_in_this_row = 0;
	current_value                = 0;

	// set back the ptr
	ptr = src;

	// set back total bytes
	total = bytes;

	// (3) up
	UINT_8* previous_row = src - bytes;
	while (total--)
	{
		current_value = (*ptr++ - *previous_row++);
		sum_of_all_bytes_in_this_row += (UINT_32)current_value;
	}
	filters_result[LPE_UP_FILTER] = sum_of_all_bytes_in_this_row;

	// set back the sum_of_all_bytes_in_this_row and current_value
	sum_of_all_bytes_in_this_row = 0;
	current_value                = 0;

	// set back the ptr
	ptr = src;

	// set back total bytes
	total = bytes;

	// (4) average
	previous_row = src - bytes;
	total       -= bpp;
	for (i = 0; i < bpp; i++)
		sum_of_all_bytes_in_this_row += (UINT_32)((*ptr++ - *previous_row++) >> 1);

	while (total--)
	{
		current_value = (*ptr - ((*(ptr - bpp) + *previous_row++) >> 1));
		ptr++;
		sum_of_all_bytes_in_this_row += (UINT_32)current_value;
	}
	filters_result[LPE_AVERAGE_FILTER] = sum_of_all_bytes_in_this_row;

	// set back the sum_of_all_bytes_in_this_row and current_value
	sum_of_all_bytes_in_this_row = 0;
	current_value                = 0;

	// set back the ptr
	ptr = src;

	// set back total bytes
	total = bytes;
	
	// (5) peath
	previous_row = src - bytes;
	total       -= bpp;
	for (i = 0; i < bpp; i++)
	{
		sum_of_all_bytes_in_this_row += (UINT_32)(*ptr++ - (UINT_8)( lpe_peath_predictor(0, (INT_32)*previous_row, 0) ));
		previous_row++;
	}

	while (total--)
	{
		current_value = (UINT_32)(*ptr - (UINT_8)( lpe_peath_predictor((INT_32)*(ptr - bpp), (INT_32)*previous_row, (INT_32)*(previous_row - bpp)) ));
		ptr++;
		previous_row++;
		sum_of_all_bytes_in_this_row += current_value;
	}
	filters_result[LPE_PEATH_FILTER] = sum_of_all_bytes_in_this_row;

	// find the index of the minimum value of all values
	 UINT_32 index = array_min_index(filters_result, 5);

	// return the found result
	switch (index)
	{
	case 0: return LPE_NONE_FILTER;
	case 1: return LPE_SUB_FILTER;
	case 2: return LPE_UP_FILTER;
	case 3: return LPE_AVERAGE_FILTER;
	case 4: return LPE_PEATH_FILTER;
	case ~0: break;
	}

	// if we made up to this point, function failed to find a filter type, so, send an error message
	return ~0;
}

//***********************************************************************************************************

static void lpe_filterize_row_by_none_filter(UINT_8* src, UINT_8* trg, UINT_32 bytes)
{
	// mark the filter type byte and increment the pointer
	*trg++ = LPE_NONE_FILTER;

	// copy the rest of bytes, untouched
	memcpy(trg, src, bytes);
}

//***********************************************************************************************************

static void lpe_filterize_row_by_sub_filter(UINT_8* src, UINT_8* trg, UINT_32 bytes, UINT_8 bpp)
{
	// mark the filter type byte and increment the pointer
	*trg++ = LPE_SUB_FILTER;

	// from zero to bpp, bytes are not filtered and must be kept untouched
	UINT_32 index = 0;
	while (index < bpp)
	{
		trg[index] = (src[index] - 0);
		index++;
	}

	// start the applying *SUB* filter from bpp on
	while (index < bytes)
	{
		trg[index] = (src[index] - src[index - bpp]);
		index++;
	}
}

//***********************************************************************************************************

static void lpe_filterize_row_by_up_filter(UINT_8* src, UINT_8* trg, UINT_32 bytes)
{
	// mark the filter type byte and increment the pointer
	*trg++ = LPE_UP_FILTER;

	// start the applying *UP* filter from zeroth byte on
	UINT_8* previous_row = src - bytes;
	UINT_32 index        = 0;
	while (index < bytes)
	{
		trg[index] = src[index] - previous_row[index];
		index++;
	}
}

//***********************************************************************************************************

static void lpe_filterize_row_by_avg_filter(UINT_8* src, UINT_8* trg, UINT_32 bytes, UINT_8 bpp)
{
	// mark the filter type byte and increment the pointer
	*trg++ = LPE_AVERAGE_FILTER;

	// from zero to bpp, bytes are not filtered and must be kept untouched
	UINT_8* previous_row = src - bytes;
	UINT_32 index        = 0;
	while (index < bpp)
	{
		trg[index] = src[index] - ((0 + previous_row[index]) >> 1);
		index++;
	}

	// start the applying *AVG* filter from bpp on
	while (index < bytes)
	{
		trg[index] = src[index] - ((src[index - bpp] + previous_row[index]) >> 1);
		index++;
	}
}

//***********************************************************************************************************

static void lpe_filterize_row_by_peath_filter(UINT_8* src, UINT_8* trg, UINT_32 bytes, UINT_8 bpp)
{
	// mark the filter type byte and increment the pointer
	*trg++ = LPE_PEATH_FILTER;

	// start the applying *PEATH* filter
	UINT_8* previous_row = src - bytes;
	UINT_32 index        = 0;
	while (index < bpp)
	{
		trg[index] = src[index] - (UINT_8)(lpe_peath_predictor(0, (INT_32)previous_row[index], 0));
		index++;
	}

	// bppth bytes are not filtered and must be kept untouched
	while (index < bytes)
	{
		trg[index] = src[index] - (UINT_8)(lpe_peath_predictor((INT_32)src[index - bpp], (INT_32)previous_row[index], (INT_32)previous_row[index - bpp]));
		index++;
	}
}

//***********************************************************************************************************

void lpe_filterize_data(UINT_8* raw_data, UINT_8* filtered_data, UINT_32 width, UINT_32 height, UINT_8 byte_per_pixel)
{
	// we go down row by row and for every single row, heuristically, decide the filter type of all bytes of the row.
	// for very first row, we choose *SUB* filter, if bytes of the row are >= 600 and *NONE* filter, in case of bytes < 600.

	// define a filter type value which will be updated for each row
	UINT_32 filter = ~0;

	// keep note of the bytes in a row
	UINT_32 stride = width * byte_per_pixel;

	// choose a right filter type for first row
	filter = (stride < 600) ? LPE_NONE_FILTER : LPE_SUB_FILTER;

	if (filter == LPE_NONE_FILTER)
		lpe_filterize_row_by_none_filter(raw_data, filtered_data, stride);
	else if (filter == LPE_SUB_FILTER)
		lpe_filterize_row_by_sub_filter(raw_data, filtered_data, stride, byte_per_pixel);

	// set back the filter type to ERROR type
	filter = ~0;

	// decrement the height in order to pass the already done first row
	height--;

	// go to the next row pointer in raw_data
	UINT_8* next_raw_row = raw_data + stride;

	// go to the next row pointer in filtered_data buffer
	UINT_8* next_filtered_row = filtered_data + stride + LPE_FILTER_TYPE_BYTE;

	// from second row down, test separately each row
	while (height--)
	{
		// heuristically find the appropriate filter type for this row of data
		filter = lpe_heuristic_filter_determination_of_row(next_raw_row, stride, byte_per_pixel);

		// act accordingly
		switch (filter)
		{
		case LPE_NONE_FILTER:
			printf("non ");
			lpe_filterize_row_by_none_filter(next_raw_row, next_filtered_row, stride);
			break;
		case LPE_SUB_FILTER:
			printf("sub ");
			lpe_filterize_row_by_sub_filter(next_raw_row, next_filtered_row, stride, byte_per_pixel);
			break;
		case LPE_UP_FILTER:
			printf("up ");
			lpe_filterize_row_by_up_filter(next_raw_row, next_filtered_row, stride);
			break;
		case LPE_AVERAGE_FILTER:
			printf("avg ");
			lpe_filterize_row_by_avg_filter(next_raw_row, next_filtered_row, stride, byte_per_pixel);
			break;
		case LPE_PEATH_FILTER:
			printf("peath ");
			lpe_filterize_row_by_peath_filter(next_raw_row, next_filtered_row, stride, byte_per_pixel);
			break;
		case ~0:
			printf("LPE encoder could not find a filter type for this row --> fatal error\n");
			break;
		}

		// set back the filter type to ERROR type
		filter = ~0;

		// update both buffers
		next_raw_row      += stride;
		next_filtered_row += (stride + LPE_FILTER_TYPE_BYTE);
	}
}