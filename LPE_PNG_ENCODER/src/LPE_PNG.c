//***********************************************************************************************************

#include "LPE_PNG.h"
#include "LPE_BITMAP_READER.h"
#include "LPE_STRING_UNIT.h"
#include "LPE_COMPRESSOR.h"
#include <iostream>

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#pragma warning(disable:4715)

//***********************************************************************************************************
/* table of CRCs of all 8-bit integers */
static UINT_32 crc_table[256];

//***********************************************************************************************************

void lpe_construct_crc_table(void)
{
	UINT_32 index = 0;
	UINT_8 i;
	while (index < 256) 
	{
		UINT_8 ind = (UINT_8)index;
		crc_table[ind] = ind;
		for (i = 8; i > 0; i--)
			crc_table[ind] = (crc_table[ind] & 1) ? (crc_table[ind] >> 1) ^ LPE_CRC32_POLYNOMIAL_GENERATOR : (crc_table[ind] >> 1);
		index++;
	}

	/*UINT_32 c;
	UINT_32 n;
	for (n = 0; n < 256; n++)
	{
		c = n;
		for (size_t i = 0; i < 8; i++)
			c = (c & 1) ? (LPE_CRC32_POLYNOMIAL_GENERATOR ^ (c >> 1)) : (c >> 1);
		crc_table[n] = c;
	}*/
}

//***********************************************************************************************************

void lpe_construct_distance_and_distance_extra_bits_table(void)
{
	// define a loop index
	UINT_32 i = 1;

	// handle both first entries
	distance_table[0] = 0;
	distance_table_extra_bits[0] = 0;

	// loop through the tables
	while (i < KB(32))
	{
		if (i <= 4)
		{
			distance_table[i] = i - 1;
			distance_table_extra_bits[i] = 0;
		}
		else if ((i >= 5) && (i <= 6))
		{
			distance_table[i] = 4;
			distance_table_extra_bits[i] = 1;
		}
		else if ((i >= 7) && (i <= 8))
		{
			distance_table[i] = 5;
			distance_table_extra_bits[i] = 1;
		}
		else if ((i >= 9) && (i <= 12))
		{
			distance_table[i] = 6;
			distance_table_extra_bits[i] = 2;
		}
		else if ((i >= 13) && (i <= 16))
		{
			distance_table[i] = 7;
			distance_table_extra_bits[i] = 2;
		}
		else if ((i >= 17) && (i <= 24))
		{
			distance_table[i] = 8;
			distance_table_extra_bits[i] = 3;
		}
		else if ((i >= 25) && (i <= 32))
		{
			distance_table[i] = 9;
			distance_table_extra_bits[i] = 3;
		}
		else if ((i >= 33) && (i <= 48))
		{
			distance_table[i] = 10;
			distance_table_extra_bits[i] = 4;
		}
		else if ((i >= 49) && (i <= 64))
		{
			distance_table[i] = 11;
			distance_table_extra_bits[i] = 4;
		}
		else if ((i >= 65) && (i <= 96))
		{
			distance_table[i] = 12;
			distance_table_extra_bits[i] = 5;
		}
		else if ((i >= 97) && (i <= 128))
		{
			distance_table[i] = 13;
			distance_table_extra_bits[i] = 5;
		}
		else if ((i >= 129) && (i <= 192))
		{
			distance_table[i] = 14;
			distance_table_extra_bits[i] = 6;
		}
		else if ((i >= 193) && (i <= 256))
		{
			distance_table[i] = 15;
			distance_table_extra_bits[i] = 6;
		}
		else if ((i >= 257) && (i <= 384))
		{
			distance_table[i] = 16;
			distance_table_extra_bits[i] = 7;
		}
		else if ((i >= 385) && (i <= 512))
		{
			distance_table[i] = 17;
			distance_table_extra_bits[i] = 7;
		}
		else if ((i >= 513) && (i <= 768))
		{
			distance_table[i] = 18;
			distance_table_extra_bits[i] = 8;
		}
		else if ((i >= 769) && (i <= 1024))
		{
			distance_table[i] = 19;
			distance_table_extra_bits[i] = 8;
		}
		else if ((i >= 1025) && (i <= 1536))
		{
			distance_table[i] = 20;
			distance_table_extra_bits[i] = 9;
		}
		else if ((i >= 1537) && (i <= 2048))
		{
			distance_table[i] = 21;
			distance_table_extra_bits[i] = 9;
		}
		else if ((i >= 2049) && (i <= 3072))
		{
			distance_table[i] = 22;
			distance_table_extra_bits[i] = 10;
		}
		else if ((i >= 3073) && (i <= 4096))
		{
			distance_table[i] = 23;
			distance_table_extra_bits[i] = 10;
		}
		else if ((i >= 4097) && (i <= 6144))
		{
			distance_table[i] = 24;
			distance_table_extra_bits[i] = 11;
		}
		else if ((i >= 6145) && (i <= 8192))
		{
			distance_table[i] = 25;
			distance_table_extra_bits[i] = 11;
		}
		else if ((i >= 8193) && (i <= 12288))
		{
			distance_table[i] = 26;
			distance_table_extra_bits[i] = 12;
		}
		else if ((i >= 12289) && (i <= 16384))
		{
			distance_table[i] = 27;
			distance_table_extra_bits[i] = 12;
		}
		else if ((i >= 16385) && (i <= 24576))
		{
			distance_table[i] = 28;
			distance_table_extra_bits[i] = 13;
		}
		else if ((i >= 24577) && (i <= 32768))
		{
			distance_table[i] = 29;
			distance_table_extra_bits[i] = 13;
		}

		// go to the next index
		i++;
	}
}

//***********************************************************************************************************

UINT_32 lpe_calculate_crc(void* buffer, UINT_32 size)
{
	UINT_8* buf = (UINT_8*)buffer;
	UINT_32 crc = ~0;
	while (size--)
		crc = crc_table[(UINT_8)crc ^ *(buf++)] ^ (crc >> 8);
	return ~crc;
		
	/*UINT_32 n;
	for (n = 0; n < size; n++)
		c = crc_table[(c ^ buf[n]) & 0xFF] ^ (c >> 8);
	return ~crc;*/
}

//***********************************************************************************************************

UINT_32 lpe_reverse_a_dword(UINT_32 dword)
{
	return ((dword & 0xFF) << 24) | ((dword & 0xFF00) << 8) | ((dword & 0xFF0000) >> 8) | ((dword & 0xFF000000) >> 24);
}

//***********************************************************************************************************

UINT_16 lpe_reverse_a_word(UINT_16 word)
{
	return (((word & 0xFF00) >> 8) | ((word & 0x00FF) << 8));
}

//***********************************************************************************************************

void lpe_signature_writer(LPE_STREAM_WRITER* stream_writer)
{
	UINT_8 signature[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	fwrite(signature, 8, 1, stream_writer->file);
}

//***********************************************************************************************************

void lpe_ihdr_writer(LPE_STREAM_WRITER* stream_writer, IHDR* ihdr)
{
	// open up the input bitmap image
	bitmap_reader_parse_bmp_file("test_bitmap.bmp");

	// parse the desired information off the bitmap image
	ihdr->width      = lpe_reverse_a_dword(bitmap_reader_tell_me(BMP_WIDTH));
	ihdr->height     = lpe_reverse_a_dword(bitmap_reader_tell_me(BMP_HEIGHT));
	ihdr->color_type = (UINT_8)bitmap_reader_tell_me(BMP_COLORTYPE);
	ihdr->bit_depth  = (UINT_8)bitmap_reader_tell_me(BMP_BITDEPTH);

	// define a LPE_CHUNK and prepare it for being an IHDR chunk
	LPE_CHUNK ch; 
	memset(&ch, 0, sizeof(LPE_CHUNK));
	ch.chunk_type  = LPE_IHDR;
	ch.data_length = sizeof(IHDR);

	// allocate memory for chunk's chunk_data 
	ch.chunk_data = (UINT_8*)lpe_zero_allocation(sizeof(IHDR));
	
	// copy the ihdr into the chunk_data field
	memcpy(ch.chunk_data, ihdr, sizeof(IHDR));

	// make a temporary buffer to store necessary data for calculating CRC32
	void* buffer = lpe_zero_allocation(sizeof(LPE_MARKER) + sizeof(IHDR));

	// copy IHDR marker into first 4 bytes, while in reversed order
	UINT_32 ihdr_marker = lpe_reverse_a_dword(ch.chunk_type);
	memcpy(buffer, &ihdr_marker, sizeof(LPE_MARKER));

	// copy ch.chunk_data into the rest
	memcpy((void*)(PHYSICAL_ADDRESS_32(buffer) + sizeof(LPE_MARKER)), ch.chunk_data, sizeof(IHDR));

	// now calculate the CRC32 of IHDR chunk
	ch.crc = lpe_calculate_crc(buffer, sizeof(LPE_MARKER) + sizeof(IHDR));

	// write out the data length in big endian
	UINT_32 data_length_in_be = lpe_reverse_a_dword(ch.data_length);
	fwrite(&data_length_in_be, 4, 1, stream_writer->file);

	// write out the marker
	fwrite(&ihdr_marker, 4, 1, stream_writer->file);

	// write out the actual data
	UINT_8* data = ch.chunk_data;
	fwrite(data, ch.data_length, 1, stream_writer->file);

	// write out the crc
	UINT_32 crc = lpe_reverse_a_dword(ch.crc);
	fwrite(&crc, 4, 1, stream_writer->file);

	// free the allocated buffers
	lpe_free_allocated_mem(buffer);
	lpe_free_allocated_mem(ch.chunk_data);
}

//***********************************************************************************************************

void lpe_idat_writer(LPE_STREAM_WRITER* stream_writer, IDAT* idat)
{

}

//***********************************************************************************************************

void lpe_text_writer(LPE_STREAM_WRITER* stream_writer, tEXt* text)
{
	// define a LPE_CHUNK and prepare it for being an tEXt chunk
	LPE_CHUNK ch;
	memset(&ch, 0, sizeof(LPE_CHUNK));
	ch.chunk_type = LPE_tEXt;

	// calculate the size of the whole strings that should be written
	
	UINT_32 keyword_size = lpe_string_size((UINT_8*)lpe_keyword);
	UINT_32 string_size  = lpe_string_size((UINT_8*)lpe_writer_comment);
	ch.data_length       = string_size + keyword_size + 1;

	// fill up the text
	text->keyword        = (UINT_8*)lpe_keyword;
	text->null_seperator = 0;
	text->string         = (UINT_8*)lpe_writer_comment;

	// allocate memory for chunk's chunk_data 
	ch.chunk_data = (UINT_8*)lpe_zero_allocation(ch.data_length);
	
	// copy the whole strings into the chunk_data field
	memcpy(ch.chunk_data, lpe_keyword, keyword_size);
	memset(ch.chunk_data + keyword_size, 0, 1);
	memcpy(ch.chunk_data + keyword_size + 1, lpe_writer_comment, string_size);

	// make a temporary buffer to store necessary data for calculating CRC32
	void* buffer = lpe_zero_allocation(sizeof(LPE_MARKER) + ch.data_length);

	// copy tEXt marker into first 4 bytes, while in reversed order
	UINT_32 text_marker = lpe_reverse_a_dword(ch.chunk_type);
	memcpy(buffer, &text_marker, sizeof(LPE_MARKER));

	// copy ch.chunk_data into the rest
	memcpy((void*)(PHYSICAL_ADDRESS_32(buffer) + sizeof(LPE_MARKER)), ch.chunk_data, ch.data_length);

	// now calculate the CRC32 of tEXt chunk
	ch.crc = lpe_calculate_crc(buffer, sizeof(LPE_MARKER) + ch.data_length);

	// write out the data length in big endian
	UINT_32 data_length_in_be = lpe_reverse_a_dword(ch.data_length);
	fwrite(&data_length_in_be, 4, 1, stream_writer->file);

	// write out the marker
	fwrite(&text_marker, 4, 1, stream_writer->file);

	// write out the actual data
	UINT_8* data = ch.chunk_data;
	fwrite(data, ch.data_length, 1, stream_writer->file);

	// write out the crc
	UINT_32 crc = lpe_reverse_a_dword(ch.crc);
	fwrite(&crc, 4, 1, stream_writer->file);

	// free the allocated buffers
	lpe_free_allocated_mem(buffer);
	lpe_free_allocated_mem(ch.chunk_data);
}

//***********************************************************************************************************

void lpe_iend_writer(LPE_STREAM_WRITER* stream_writer, IEND* iend)
{
	// define a LPE_CHUNK and prepare it for being an IEND chunk
	LPE_CHUNK ch;
	memset(&ch, 0, sizeof(LPE_CHUNK));
	ch.chunk_type  = LPE_IEND;
	ch.data_length = 0;
	ch.chunk_data  = LPE_NULL;
	
	// write out the data length
	fwrite(&ch.data_length, 4, 1, stream_writer->file);

	// write out the marker and CRC32
	UINT_32 marker = lpe_reverse_a_dword(ch.chunk_type);
	fwrite(&marker, 4, 1, stream_writer->file);

	// write out the CRC32
	ch.crc = lpe_calculate_crc(&marker, sizeof(LPE_MARKER));

	// write out the crc
	UINT_32 crc = lpe_reverse_a_dword(ch.crc);
	fwrite(&crc, 4, 1, stream_writer->file);
}

//***********************************************************************************************************

void lpe_ztxt_writer(LPE_STREAM_WRITER* stream_writer, LPE_CHUNK* ch)
{

}

//***********************************************************************************************************

void lpe_phys_writer(LPE_STREAM_WRITER* stream_writer, LPE_CHUNK* ch)
{

}

//***********************************************************************************************************

void lpe_gama_writer(LPE_STREAM_WRITER* stream_writer, LPE_CHUNK* ch)
{

}

//***********************************************************************************************************

void lpe_time_writer(LPE_STREAM_WRITER* stream_writer, LPE_CHUNK* ch)
{

}

//***********************************************************************************************************

void lpe_hist_writer(LPE_STREAM_WRITER* stream_writer, LPE_CHUNK* ch)
{

}

//***********************************************************************************************************

void lpe_plte_writer(LPE_STREAM_WRITER* stream_writer, LPE_CHUNK* ch)
{

}

//***********************************************************************************************************

void lpe_sbit_writer(LPE_STREAM_WRITER* stream_writer, LPE_CHUNK* ch)
{

}

//***********************************************************************************************************

void lpe_bkgd_writer(LPE_STREAM_WRITER* stream_writer, LPE_CHUNK* ch)
{

}

//***********************************************************************************************************

void lpe_srgb_writer(LPE_STREAM_WRITER* stream_writer, LPE_CHUNK* ch)
{

}

//***********************************************************************************************************

void lpe_chrm_writer(LPE_STREAM_WRITER* stream_writer, LPE_CHUNK* ch)
{

}

//***********************************************************************************************************

void* lpe_buffer_reallocation(void* old_pointer, UINT_32 old_size, UINT_32 new_extra_size)
{
	void* new_pointer = lpe_zero_allocation(old_size + new_extra_size);
	memcpy(new_pointer, old_pointer, old_size);
	lpe_free_allocated_mem(old_pointer);
	return new_pointer;
}

//***********************************************************************************************************

void* lpe_zero_allocation(UINT_32 byte_size)
{
	void* pointer = malloc(byte_size);
	memset(pointer, 0, byte_size);
	return pointer;
}

//***********************************************************************************************************

void lpe_free_allocated_mem(void* mem)
{
	if (!mem)
	{
		printf("provided pointer cannot be freed --> fatal error\n");
		return;
	}
	free(mem);
	mem = 0;
}

//***********************************************************************************************************

void lpe_chunk_table_report(LPE_PNG* png)
{
	LPE_CHUNK* ch = png->chunks_table;
	UINT_32 ch_num = png->number_of_chunks;
	UINT_32 i = 0;
	printf("chunk table report\n");
	while (i < ch_num)
	{
		printf("\tchunk entry %i. id = %i\n", i, ch[i].chunk_type);
		i++;
	}
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
}

//***********************************************************************************************************

void lpe_clean_up_the_png(LPE_PNG* png)
{
	if (!png)
	{
		printf("png data structure is empty --> fatal error\n");
		printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
		return;
	}
	lpe_free_allocated_mem(png->chunks_table);
}

//***********************************************************************************************************

