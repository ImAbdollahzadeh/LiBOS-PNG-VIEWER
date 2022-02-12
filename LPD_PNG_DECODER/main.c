#include <iostream>
#include "LPD_ALIASES.h"
#include "LPD_STRING_UNIT.h"
#include "LPD_PNG.h"
#include "LPD_DECOMPRESSOR.h"
#include "LPD_DEFILTERIZER.h"
#include "LPD_RENDERER.h"

//***********************************************************************************************************

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#pragma warning(disable:4715)

//***********************************************************************************************************

int main(void)
{
	// build an LPD_PNG data structure
	LPD_PNG png;
	memset(&png, 0, sizeof(LPD_PNG));

	// read the png file info into it
	lpd_read_png_file(&png, "flower.png");

	// report the png file info 
	lpd_chunk_table_report(&png);
	lpd_idat_text_ztxt_chunks_counter_report(&png);

	// allocate space for the decompressed_data buffer of the png
	png.decompressed_data_buffer = (UINT_8*)lpd_zero_allocation(png.total_size);

	// decompress the concatanetad IDAT's ZLIB block 
	lpd_decompress_zlib_buffer(&png, png.idat_buffer->data);
	
	// find the IHDR chunk
	Chunk*  ch = png.chunks_table;
	UINT_32 number_of_chunks = png.number_of_chunks;
	UINT_32 i = 0;
	while (i < number_of_chunks)
	{
		if (ch->chunk_type == IHDR_ID)
			break;
		ch++;
		i++;
	}
	IHDR* ihdr = (IHDR*)ch->chunk_data;

	// define an output buffer for the actual RGB value
	UINT_8* rgb_buffer = LPD_NULL;

	// define total byte in rgb buffer
	UINT_32 total_bytes = 0;

	// define a byte per pixel value to be used in defilterizer and renderer
	UINT_8 bpp = 0;

	// do the actual defilterization
	lpd_defilter(png.decompressed_data_buffer, (void**)&rgb_buffer, png.decompressed_data_counter, &total_bytes, &bpp, ihdr);

	// render the filtered buffer
	lpd_render_buffer(rgb_buffer, bpp, ihdr);

	// clean up the png data structure
	lpd_clean_up_the_png(&png);

	// clean up the RGB buffer if any
	lpd_free_allocated_mem(rgb_buffer);

	// safely return
	return 0;
}

//***********************************************************************************************************

