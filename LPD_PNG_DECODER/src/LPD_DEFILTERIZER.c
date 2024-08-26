//***********************************************************************************************************

#include "LPD_DEFILTERIZER.h"
#include <stdio.h>

//***********************************************************************************************************

static INT_32 lpd_peath_predictor(INT_32 a, INT_32 b, INT_32 c)
{
	INT_32 p = a + b - c;
	INT_32 pa = LPD_ABS((p - a));
	INT_32 pb = LPD_ABS((p - b));
	INT_32 pc = LPD_ABS((p - c));
	return (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;
}

//***********************************************************************************************************

FILTER_TYPE lpd_defilter_row(UINT_8* row, UINT_8* out, UINT_32 byte_in_row, UINT_32 width, UINT_8 byte_per_pixel)
{
	// get the filter byte and jump over it
	UINT_8 filter_type_byte = *row++;

	switch (filter_type_byte)
	{
		// the row was not filtered, thus, copy the rest of the row to output
		case FILTER_TYPE_NONE:
			while (byte_in_row--)
				*out++ = *row++;
			printf("non ");
			break;

		// the filtered pixel's color (e.g. red) is the difference between the current pixel's color and the previous pixel's color
		case FILTER_TYPE_SUB: {
			UINT_8  bpp = byte_per_pixel;
			UINT_32 w   = 0;
			while (w < bpp)
				*out++ = row[w++];
			while (w < byte_in_row)
			{
				row[w] += row[w - bpp];
				*out++ = row[w++];
			}
			printf("sub ");
			break;
		}

		// the filtered byte is the difference between the current byte and the byte above it
		case FILTER_TYPE_UP: {
			UINT_8* prev_row = row - (byte_in_row + FILTER_TYPE_BYTE);
			UINT_32 w = 0;
			while (w < byte_in_row)
			{
				row[w] += prev_row[w];
				*out++ = row[w++];
			}
			printf("up ");
			break;
		}

		// the filtered pixel's color is the difference between the current pixel's color and the average of the previous and upper pixel's color
		case FILTER_TYPE_AVERAGE: {
			UINT_8* prev_row = row - (byte_in_row + FILTER_TYPE_BYTE);
			UINT_8  bpp      = byte_per_pixel;
			UINT_32 w        = 0;
			UINT_8  prev_color;
			while (w < byte_in_row)
			{
				prev_color = (w < bpp) ? 0 : row[w - bpp];
				*out++ = row[w] = row[w] + ((prev_row[w] + prev_color) >> 1);
				w++;
			}
			printf("avg ");
			break;
		}

		// the current pixel would be the difference between the current pixel and the PaethPredictor pixel
		case FILTER_TYPE_PEATH: {	
			UINT_8* prev_row = row - (byte_in_row + FILTER_TYPE_BYTE);
			UINT_8  bpp = byte_per_pixel;
			UINT_32 w = 0;
			while (w < bpp)
			{
				row[w] += lpd_peath_predictor(0, (INT_32)prev_row[w], 0);
				*out++ = row[w++];
			}
			while (w < byte_in_row)
			{
				row[w] += lpd_peath_predictor((INT_32)row[w - bpp], (INT_32)prev_row[w], (INT_32)prev_row[w - bpp]);
				*out++ = row[w++];
			}
			printf("peath ");
			break;
		}
	}

	// undefined (error) filter type
	return FILTER_TYPE_UNDEFINED;
}

//***********************************************************************************************************

static UINT_32 lpd_defilterizer_reverse_a_dword(UINT_32 num)
{
	return ((num & 0xFF) << 24) | ((num & 0xFF00) << 8) | ((num & 0xFF0000) >> 8) | ((num & 0xFF000000) >> 24);
}

//***********************************************************************************************************

void lpd_defilter(void* in_buffer, void** out_buffer, UINT_32 in_buffer_size, UINT_32* total_output_bytes, UINT_8* bpp, IHDR* ihdr)
{
	UINT_32 width          = lpd_defilterizer_reverse_a_dword(ihdr->width);
	UINT_32 height         = lpd_defilterizer_reverse_a_dword(ihdr->height);
	UINT_8  byte_per_pixel = 0;
	switch (ihdr->color_type)
	{
	case GREYSCALE:
		byte_per_pixel = 1;
		break;
	case GREYSCALE_WITH_ALPHA:
		byte_per_pixel = 2;
		break;
	case TRUECOLOR:
		byte_per_pixel = 3;
		break;
	case TRUECOLOR_WITH_ALPHA:
		byte_per_pixel = 4;
		break;
	case INDEXEDCOLOR:
		byte_per_pixel = 0; //TODO...
		break;
	}

	if(ihdr->bit_depth >= 8) // two cases 8 and 16
		byte_per_pixel *= (ihdr->bit_depth) >> 3;
	else // other values 1, 2, and 4
	{
		switch (ihdr->bit_depth)
		{
		case 1:
			byte_per_pixel >>= 3;
			break;
		case 2:
			byte_per_pixel >>= 2;
			break;
		case 4:
			byte_per_pixel >>= 1;
			break;
		default:
			byte_per_pixel = 0;
			printf("undefined bit depth encountered --> fatal error");
			break;
		}
	}

	// update te bpp for later uses by renderer
	*bpp = byte_per_pixel;

	// allocate memory for output
	*out_buffer = lpd_zero_allocation(byte_per_pixel * width * height);

	// update the provided total_output_bytes for user
	*total_output_bytes = byte_per_pixel * width * height;

	// prepare the first row
	UINT_8* current_row = (UINT_8*)in_buffer;

	// calculate the stride in terms of bytes in each row
	UINT_32 stride = width * byte_per_pixel;

	// provide a height indexer
	UINT_32 h = 0;

	// make a copy of the output buffer
	UINT_8* out = (UINT_8*)*out_buffer;

	// start defilterization of the whole buffer
	while (h < height)
	{
		// defilter the current row
		lpd_defilter_row(current_row, out, stride, width, byte_per_pixel);

		// next row pointer
		current_row += (stride + FILTER_TYPE_BYTE);

		// update output buffer too
		out += stride;

		// go one more row ahead
		h++;
	}

	// safely return
	return;
}

//***********************************************************************************************************
