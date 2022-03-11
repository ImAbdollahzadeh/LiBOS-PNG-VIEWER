//***********************************************************************************************************

#include <iostream>
#include "LPE_ALIASES.h"
#include "LPE_STRING_UNIT.h"
#include "LPE_PNG.h"
#include "LPE_COMPRESSOR.h"
#include "LPE_FILTERIZER.h"
#include "LPE_STREAM_WRITER.h"
#include "LPE_BITMAP_READER.h"

//***********************************************************************************************************

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#pragma warning(disable:4715)

//***********************************************************************************************************

int main(void)
{
	// construct the distance table
	lpe_construct_distance_and_distance_extra_bits_table();

	// construct the CRC table
	lpe_construct_crc_table();

	// define a LPE_STREAM_WRITER
	LPE_STREAM_WRITER stream_writer = { LPE_NULL, LPE_OUTPUT_ADDRESS };
	stream_writer.file = fopen(stream_writer.path_to_write, "wb+" );
	
	// write the png signature
	lpe_signature_writer(&stream_writer);

	// define an IHDR and fill up the required contents acquired from bmp parsing
	IHDR ihdr;
	memset(&ihdr, 0, sizeof(IHDR));

	// flush this chunk to the file stream
	lpe_ihdr_writer(&stream_writer, &ihdr);

	// define a tEXt and fill up the required contents
	tEXt text;
	memset(&text, 0, sizeof(tEXt));

	// flush this chunk to the file stream
	lpe_text_writer(&stream_writer, &text);

	// query the raw RGB or RGBA data
	UINT_8* raw_data = (UINT_8*)((void*)bitmap_reader_tell_me(BMP_DATA));

	// query the width
	UINT_32 width = lpe_reverse_a_dword(ihdr.width);

	// query the height
	UINT_32 height = lpe_reverse_a_dword(ihdr.height);

	// query the byte per pixel
	UINT_8 bpp = (UINT_8)bitmap_reader_tell_me(BMP_BYTEPERPIXEL);

	// query the total byte size of the image
	UINT_32 total_bytes = width * height * bpp;

	// allocate memory for filtered data
	UINT_8* filtered_data = (UINT_8*)lpe_zero_allocation(total_bytes + height);

	// filter the actual RGB or RGBA data
	lpe_filterize_data(raw_data, filtered_data, width, height, bpp);

	// define the compressed data buffer holder
	UINT_8* compressed_data = LPE_NULL;

	// compress the filtered RGB or RGBA data
	lpe_compress_data(&compressed_data, filtered_data, total_bytes + height);

	// we don't need the filtered data anymore
	lpe_free_allocated_mem(filtered_data);

	// divide the compressed data into multiple IDAT sections

	// we don't need anymore the allocated memory for the compressed data which was assigned internally in lpe_compress_data
	lpe_free_allocated_mem(compressed_data);

	// define an IEND chunk
	IEND iend;
	memset(&iend, 0, sizeof(IEND));

	// flush this chunk to the file stream
	lpe_iend_writer(&stream_writer, &iend);

	// close the stream_writer
	fclose(stream_writer.file);

	// safely return
	return 0;
}

//***********************************************************************************************************

