//***********************************************************************************************************

#include "LPD_PNG.h"
#include "LPD_STRING_UNIT.h"
#include "LPD_DECOMPRESSOR.h"

//***********************************************************************************************************

static UINT_32 lpd_png_total_bytes = 0;

//***********************************************************************************************************

static void print_signature(UINT_8* signature_buffer)
{
	UINT_8 index = 0;
	printf("signature is: ");
	while (index < 8)
		printf("%i, ", (INT_32)signature_buffer[index++]);
	printf("\n-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
}

//***********************************************************************************************************

static void print_code(UINT_8* code_buffer)
{
	UINT_8 index = 0;
	printf("code is: ");
	while (index < 4)
		printf("%c", code_buffer[index++]);
	printf("\n");
}

//***********************************************************************************************************

static void extract_bytes_from_a_dword(UINT_32 num, UINT_8* code_buffer)
{
	void* ptr = &num;
	for (size_t i = 0; i < 4; i++) 
		code_buffer[i] = *(INT_8*)((INT_8*)ptr + i);
}

//***********************************************************************************************************

static UINT_32 lpd_reverse_a_dword(UINT_32 num)
{
	return ((num & 0xFF) << 24) | ((num & 0xFF00) << 8) | ((num & 0xFF0000) >> 8) | ((num & 0xFF000000) >> 24);
}

//***********************************************************************************************************

static UINT_32 lpd_reverse_a_word(UINT_16 num)
{
	return (((num & 0xFF00) >> 8) | ((num & 0x00FF) << 8));
}

//***********************************************************************************************************

static BOOL lpd_calculate_crc32(UINT_32 crc)
{
	// TODO .....

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_ihdr(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = IHDR_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// start the png->chunks_table
	png->chunks_table = (Chunk*)lpd_zero_allocation(sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired IHDR data
	IHDR* ihdr = (IHDR*)ch->chunk_data;
	printf("\tihdr width: %i\n",              lpd_reverse_a_dword(ihdr->width));
	printf("\tihdr height: %i\n",             lpd_reverse_a_dword(ihdr->height));
	printf("\tihdr bit_depth: %i\n",          ihdr->bit_depth);
	printf("\tihdr color_type: %i\n",         ihdr->color_type);
	printf("\tihdr compression_method: %i\n", ihdr->compression_method);
	printf("\tihdr filter_method: %i\n",      ihdr->filter_method);
	printf("\tihdr interlace_method: %i\n",   ihdr->interlace_method);
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// ... TODO
	png->total_size = lpd_reverse_a_dword(ihdr->width) * lpd_reverse_a_dword(ihdr->height) * 4;

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_idat(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a 4-byte aligned buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation( LPD_ALIGN_0x04(chunk_data_length) + 6 );
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = IDAT_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired IDAT data
	IDAT* idat = (IDAT*)ch->chunk_data;
	printf("\tidat compressed data size: %i\n", chunk_data_length);
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// save the IDAT data into png->idat_buffer in order to have all IDAT data concatenated
	if (!png->number_of_idats)
	{
		png->idat_buffer       = (IDAT*)  lpd_zero_allocation(sizeof(IDAT));
		png->idat_buffer->data = (UINT_8*)lpd_zero_allocation(ch->data_length);
		memcpy(png->idat_buffer->data, ch->chunk_data, ch->data_length);
		png->idat_buffer_size += ch->data_length;
	}
	else
	{
		png->idat_buffer->data = (UINT_8*)lpd_buffer_reallocation(png->idat_buffer->data, png->idat_buffer_size, ch->data_length);
		memcpy((void*)((INT_8*)(png->idat_buffer->data) + png->idat_buffer_size), ch->chunk_data, ch->data_length);
		png->idat_buffer_size += ch->data_length;
	}

	// increment png->number_of_idats
	png->number_of_idats++;

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_plte(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = PLTE_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired PLTE data
	PLTE* plte = (PLTE*)ch->chunk_data;
	printf("\tplte red: %i\n", plte->red);
	printf("\tplte green: %i\n", plte->green);
	printf("\tplte blue: %i\n", plte->blue);
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_srgb(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = sRGB_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired sRGB data
	sRGB* srgb = (sRGB*)ch->chunk_data;
	printf("\tsrgb rendering_intent: %i (", srgb->rendering_intent);
	switch (srgb->rendering_intent)
	{
	case 0:  printf("Percdptual)\n");            break;
	case 1:  printf("Relative colorimetric)\n"); break;
	case 2:  printf("Saturation9\n");            break;
	case 3:  printf("Absolute colorimetric)\n"); break;
	default: printf("error)\n");                 break;
	}
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_gama(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = gAMA_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired gAMA data
	gAMA* gama = (gAMA*)ch->chunk_data;
	printf("\tgama image_gamma: %i\n", lpd_reverse_a_dword(gama->image_gamma));
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_chrm(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = cHRM_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired cHRM data
	cHRM* chrm = (cHRM*)ch->chunk_data;
	printf("\tchrm white_Point_x: %i\n", lpd_reverse_a_dword(chrm->white_Point_x));
	printf("\tchrm white_Point_y: %i\n", lpd_reverse_a_dword(chrm->white_Point_y));
	printf("\tchrm red_x: %i\n",         lpd_reverse_a_dword(chrm->red_x));
	printf("\tchrm red_y: %i\n",         lpd_reverse_a_dword(chrm->red_y));
	printf("\tchrm green_x: %i\n",       lpd_reverse_a_dword(chrm->green_x));
	printf("\tchrm green_y: %i\n",       lpd_reverse_a_dword(chrm->green_y));
	printf("\tchrm blue_x: %i\n",        lpd_reverse_a_dword(chrm->blue_x));
	printf("\tchrm blue_y: %i\n",        lpd_reverse_a_dword(chrm->blue_y));
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// free the defined chunk
	lpd_free_allocated_mem(ch); 
	
	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_phys(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = pHYs_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired pHYs data
	pHYs* phys = (pHYs*)ch->chunk_data;
	printf("\tphys pixels_per_unit_x: %i\n", lpd_reverse_a_dword(phys->pixels_per_unit_x));
	printf("\tphys pixels_per_unit_y: %i\n", lpd_reverse_a_dword(phys->pixels_per_unit_y));
	printf("\tphys unit_specifier: %i\n", phys->unit_specifier);
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_hist(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = hIST_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired hIST data
	hIST* hist = (hIST*)ch->chunk_data;
	printf("\thist series pointer: %i\n", lpd_reverse_a_dword((UINT_32)hist->serie));
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_sbit(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = sBIT_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired sBIT data
	sBIT* sbit = (sBIT*)ch->chunk_data;
	printf("\tsbit color_type: %i\n",               sbit->color_type);
	printf("\tsbit significant_bits pointer: %i\n", lpd_reverse_a_dword((UINT_32)sbit->significant_bits));
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_text(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = tEXt_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired tEXt data
	tEXt* text = (tEXt*)ch->chunk_data;
	
	// make keyword and text pointers available
	UINT_8* ptr = (UINT_8*)text;
	UINT_32 len = ch->data_length;

	// print out the keyword
	while (*ptr)
	{
		printf("%c", *ptr++);

		// decrement the len
		len--;
	}
	printf(":");

	// print out the actual text
	while (len--)
		printf("%c", *ptr++);
	
	printf("\n");
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// save the text data into png->text_buffer;
	if (!png->number_of_texts)
	{
		png->text_buffer = (tEXt*)lpd_zero_allocation(ch->data_length);
		memcpy(png->text_buffer, ch->chunk_data, ch->data_length);
		png->text_buffer_size += ch->data_length;
	}
	else
	{
		png->text_buffer = (tEXt*)lpd_buffer_reallocation(png->text_buffer, png->text_buffer_size, ch->data_length);
		memcpy((void*)((INT_8*)(png->text_buffer) + png->text_buffer_size), ch->chunk_data, ch->data_length);
		png->text_buffer_size += ch->data_length;
	}

	// increment png->number_of_texts
	png->number_of_texts++;

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_ztxt(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = zTXt_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired zTXt data
	zTXt* ztxt = (zTXt*)ch->chunk_data;

	// make keyword and text pointers available
	UINT_8* ptr = (UINT_8*)ztxt;
	UINT_32 len = ch->data_length;

	// print out the keyword
	while (*ptr)
	{
		printf("%c", *ptr++);

		// decrement the len
		len--;
	}
	printf(":\n");
	
	// jump over null seperator and compression method bytes
	ptr += 2;

	// create a buffer for compressed data stream
	UINT_8* ztxt_compressed_buffer = (UINT_8*)lpd_zero_allocation(len);

	// copy compressed data in the created buffer
	memcpy(ztxt_compressed_buffer, ptr, len);

	// decompress the zTXt's zlib block by creating a fake PNG fool the application to encounter zTXt's zlib block as if that of the IDAT's.
	LPD_PNG fake_png;

	// make a buffer of almost LEN KB as the first guess
	fake_png.decompressed_data_buffer  = (UINT_8*)lpd_zero_allocation(len * KB(1));

	// set the fake png's decompressed_data_counter zero
	fake_png.decompressed_data_counter = 0;

	// do the actual decompression
	lpd_decompress_zlib_buffer(&fake_png, ztxt_compressed_buffer);

	// do we need to print out the actual uncompressed text?
	UINT_32 size_of_text = fake_png.decompressed_data_counter;
	UINT_32 i = 0;
	if (zTXt_DECOMPRESSION_DEBUG)
	{
		while (i < size_of_text)
		{
			printf("%c", fake_png.decompressed_data_buffer[i]);
			i++;
		}
	}

	// clean up the allocated buffers of the fake png
	fake_png.decompressed_data_counter = 0;
	lpd_free_allocated_mem(fake_png.decompressed_data_buffer);

	printf("\n");
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// save the ztxt data into png->ztxt_buffer;
	if (!png->number_of_ztxts)
	{
		png->ztxt_buffer = (zTXt*)lpd_zero_allocation(ch->data_length);
		memcpy(png->ztxt_buffer, ch->chunk_data, ch->data_length);
		png->ztxt_buffer_size += ch->data_length;
	}
	else
	{
		png->ztxt_buffer = (zTXt*)lpd_buffer_reallocation(png->ztxt_buffer, png->ztxt_buffer_size, ch->data_length);
		memcpy((void*)((INT_8*)(png->ztxt_buffer) + png->ztxt_buffer_size), ch->chunk_data, ch->data_length);
		png->ztxt_buffer_size += ch->data_length;
	}

	// increment png->number_of_ztxts
	png->number_of_ztxts++;

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_time(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = tIME_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired tIME data
	tIME* time = (tIME*)ch->chunk_data;
	printf("\ttime year: %i\n",   lpd_reverse_a_word(time->year));
	printf("\ttime month: %i\n",  time->month);
	printf("\ttime day: %i\n",    time->day);
	printf("\ttime hour: %i\n",   time->hour);
	printf("\ttime minute: %i\n", time->minute);
	printf("\ttime second: %i\n", time->second);
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_bkgd(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = bKGD_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired bKGD data
	bKGD* bkgd = (bKGD*)ch->chunk_data;
	printf("\tbkgd palette_index: %i\n", bkgd->palette_index);
	printf("\tbkgd gray: %i\n",          lpd_reverse_a_word(bkgd->gray));
	printf("\tbkgd red: %i\n",           lpd_reverse_a_word(bkgd->red));
	printf("\tbkgd green: %i\n",         lpd_reverse_a_word(bkgd->green));
	printf("\tbkgd blue: %i\n",          lpd_reverse_a_word(bkgd->blue));
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

static BOOL lpd_parse_undefined(LPD_PNG* png, UINT_32 chunk_data_length, FILE* file)
{
	// define a zeroed Chunk structure
	Chunk* ch = (Chunk*)lpd_zero_allocation(sizeof(Chunk));

	// extract the data into a buffer
	UINT_8* data_buffer = (UINT_8*)lpd_zero_allocation(chunk_data_length);
	fread(data_buffer, chunk_data_length, 1, file);

	// fill up the defined Chunk structure
	ch->chunk_type  = UNDF_ID;
	ch->data_length = chunk_data_length;
	ch->chunk_data  = data_buffer;
	fread(&ch->crc, 4, 1, file);

	// check out the crc
	if (!lpd_calculate_crc32(ch->crc))
	{
		lpd_free_allocated_mem(ch);
		lpd_free_allocated_mem(data_buffer);
		printf("calculation of crc32 failed --> fatal error\n");
		return LPD_FALSE;
	}

	// reallocate the png->chunks_table
	Chunk* old_chunks_table_pointer = png->chunks_table;
	png->chunks_table = (Chunk*)lpd_buffer_reallocation(old_chunks_table_pointer, png->number_of_chunks * sizeof(Chunk), sizeof(Chunk));
	png->chunks_table[png->number_of_chunks] = *ch;

	// query the desired UNDF data
	printf("undf\n");
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// free the defined chunk
	lpd_free_allocated_mem(ch);

	return LPD_TRUE;
}

//***********************************************************************************************************

UINT_32 lpd_read_a_chunk(LPD_PNG* png, FILE* file)
{
	UINT_32 chunk_data_length = 0;
	UINT_32 chunk_type        = 0;
	UINT_32 chunk_crc         = 0;
	UINT_8  code[4];
	fread(&chunk_data_length, 4, 1, file);
	fread(&chunk_type,        4, 1, file);
	extract_bytes_from_a_dword(chunk_type, code);
	print_code(code);
	UINT_8  png_marker  = lpd_which_png_marker((const char*)code);
	UINT_32 data_length = lpd_reverse_a_dword(chunk_data_length);

	switch (png_marker)
	{
	case 0: // IHDR
		if (!lpd_parse_ihdr(png, data_length, file))
		{
			printf("parsing the IHDR chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return IHDR_ID;
	case 1: // IEND
		printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
		return IEND_ID;
	case 2: // IDAT
		if (!lpd_parse_idat(png, data_length, file))
		{
			printf("parsing the IDAT chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return IDAT_ID;
	case 3: // PLTE
		if (!lpd_parse_plte(png, data_length, file))
		{
			printf("parsing the PLTE chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return PLTE_ID;
	case 4: // sRGB
		if (!lpd_parse_srgb(png, data_length, file))
		{
			printf("parsing the sRGB chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return sRGB_ID;
	case 5: // bKGD
		if (!lpd_parse_bkgd(png, data_length, file))
		{
			printf("parsing the bKGD chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return bKGD_ID;
	case 6: // cHRM
		if (!lpd_parse_chrm(png, data_length, file))
		{
			printf("parsing the cHRM chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return cHRM_ID;
	case 7: // gAMA
		if (!lpd_parse_gama(png, data_length, file))
		{
			printf("parsing the gAMA chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return gAMA_ID;
	case 8: // sBIT
		if (!lpd_parse_sbit(png, data_length, file))
		{
			printf("parsing the sBIT chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return sBIT_ID;
	case 9: // zTXt
		if (!lpd_parse_ztxt(png, data_length, file))
		{
			printf("parsing the zTXt chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return zTXt_ID;
	case 10: // tEXt
		if (!lpd_parse_text(png, data_length, file))
		{
			printf("parsing the tEXt chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return tEXt_ID;
	case 11: // tIME
		if (!lpd_parse_time(png, data_length, file))
		{
			printf("parsing the tIME chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return tIME_ID;
	case 12: // pHYs
		if (!lpd_parse_phys(png, data_length, file))
		{
			printf("parsing the pHYs chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return pHYs_ID;
	case 13: // hIST
		if (!lpd_parse_hist(png, data_length, file))
		{
			printf("parsing the hIST chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return hIST_ID;
	case LPD_UNDEFINED_MARKER:
		printf("An undefined marker found which cannot be handeled\n");
		if (!lpd_parse_undefined(png, data_length, file))
		{
			printf("parsing the UNDF chunk failed --> fatal error\n");
			return LPD_ERROR_ID;
		}
		png->number_of_chunks++;
		return UNDF_ID;
	}
}

//***********************************************************************************************************

UINT_32 lpd_png_find_size(const char* file_address)
{
	FILE* file = fopen(file_address, "rb");
	if (!file)
		return 0x00;

	fseek(file, 0L, SEEK_END);
	UINT_32 result = ftell(file);
	fclose(file);

	return result;
}

//***********************************************************************************************************

BOOL lpd_read_png_file(LPD_PNG* png, const char* file_address)
{
	// query the whole file size
	lpd_png_total_bytes = lpd_png_find_size(file_address);
	printf("file %s is %i bytes\n", file_address, lpd_png_total_bytes);
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");

	// open the file handle
	FILE* file = fopen(file_address, "rb");
	if (!file) 
		return LPD_FALSE;

	// check out the signature of the file
	fread(png->signature, 1, 8, file);
	print_signature(png->signature);

	// read chunk by chunk into the png buffer until everything has been read
	UINT_32 this_chunk = LPD_ERROR_ID;

	// hang around until hitting the IEND marker
	while (this_chunk != IEND_ID)
	{
		this_chunk = lpd_read_a_chunk(png, file);

		// an error id, clean up the png buffer, close the file handle, and return FALSE
		if (this_chunk == LPD_ERROR_ID)
		{
			fclose(file);
			lpd_clean_up_the_png(png);
			return LPD_FALSE;
		}
	}

	// close the file handle
	fclose(file);

	// return the TRUE result
	return LPD_TRUE;
}

//***********************************************************************************************************

void* lpd_buffer_reallocation(void* old_pointer, UINT_32 old_size, UINT_32 new_extra_size)
{
	void* new_pointer = lpd_zero_allocation(old_size + new_extra_size);
	memcpy(new_pointer, old_pointer, old_size);
	lpd_free_allocated_mem(old_pointer);
	return new_pointer;
}

//***********************************************************************************************************

void* lpd_zero_allocation(UINT_32 byte_size)
{
	void* pointer = malloc(byte_size);
	memset(pointer, 0, byte_size);
	return pointer;
}

//***********************************************************************************************************

void lpd_free_allocated_mem(void* mem)
{
	if (!mem)
	{
		printf("provided pointer cannot be freed --> fatal error\n");
		return;
	}
	free(mem);
	mem = LPD_NULL;
}

//***********************************************************************************************************

void lpd_chunk_table_report(LPD_PNG* png)
{
	Chunk* ch      = png->chunks_table;
	UINT_32 ch_num = png->number_of_chunks;
	UINT_32 i      = 0;
	printf("chunk table report\n");
	while (i < ch_num)
	{
		printf("\tchunk entry %i. id = %i\n", i, ch[i].chunk_type);
		i++;
	}
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
}

//***********************************************************************************************************

void lpd_idat_text_ztxt_chunks_counter_report(LPD_PNG* png)
{
	printf("buffer chunks counter report\n");
	printf("\tpng number of IDATs: %i\n", png->number_of_idats);
	printf("\tpng number of tEXts: %i\n", png->number_of_texts);
	printf("\tpng number of zTXts: %i\n", png->number_of_ztxts);
	printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
}

//***********************************************************************************************************

void lpd_clean_up_the_png(LPD_PNG* png)
{
	if (!png)
	{
		printf("png data structure is empty --> fatal error\n");
		printf("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
		return;
	}
	lpd_free_allocated_mem(png->chunks_table);
}

//***********************************************************************************************************
