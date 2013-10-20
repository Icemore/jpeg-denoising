#ifndef jpeg_utils_h_

#include <stdio.h>
#include "jpeglib.h"
#include <vector>

typedef std::vector< std::vector< std::vector < std::vector<short> > > > CoeffBlocks;
typedef std::vector< std::vector<unsigned short> > QuantTables;
typedef std::vector< std::pair< int, int> > SamplingFactors;

void write_JPEG_file(
	char const * filename
,	std::vector<float>& image
,	size_t image_width
,	size_t image_height
,	size_t channels
,	int quality
);

void read_JPEG_file(
	char const * filename
,	std::vector<float>& image
,	size_t &image_width
,	size_t &image_height
,	size_t &channels
);

void convertToJpegData(
	JSAMPLE* jpegData
,	std::vector<float>& pngData
,	size_t image_width
,	size_t image_height
,	size_t channels
);

void convertToPngData(
	std::vector<float>& pngData
,	JSAMPLE* jpegData
,	size_t image_width
,	size_t image_height
,	size_t channels
);

void read_JPEG_coefficients(
	char const * filename
, 	CoeffBlocks& image_coeffs
,	QuantTables& qtables
,	SamplingFactors& sfact
);

#endif // jpeg_utils_h_
