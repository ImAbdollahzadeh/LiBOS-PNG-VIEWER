#include "../include/LPE_ALIASES.h"
#include "../include/LPE_BITMAP_READER.h"
#include "../include/LPE_PNG.h"
#include "../include/LPE_STRING_UNIT.h"
#include "../include/LPE_LZ77_ENCODER.h"
#include "../include/LPE_HUFFMAN_ENCODER.h"
#include "../include/LPE_COMPRESSOR.h"


int main(void)
{
    char filtered_data[] = {'0','1','1','1','1','1','1','1','1','1','2','1','6','6','6','6','6','6','6','6','6','6','6','6','6','6','6','6','6','6'};
    unsigned int filtered_data_bytes = 30;

    LPE_LZ77_LZ77_OUTPUT_PACKAGE lz77_output = lpe_lz77_start(filtered_data, filtered_data_bytes);
	lpe_zero_bit_metrics();
	LPE_HUFFMAN_VFLAB* litlen_vflab = lpe_huffman_create_litlen_histogram(&lz77_output);

    lpe_huffman_dump_vflab(litlen_vflab, LPE_HUFFMAN_LITERAL_LENGTH_MAX_ENTRIES);
}
