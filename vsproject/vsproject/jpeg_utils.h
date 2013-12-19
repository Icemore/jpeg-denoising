#pragma once

#include <stdio.h>
#include "jpeglib.h"
#include <vector>

typedef std::vector< std::vector< std::vector < std::vector<float> > > > CoeffBlocks;
typedef std::vector< std::vector<unsigned int> > QuantTables;
typedef std::vector< std::pair< int, int> > SamplingFactors;

typedef std::vector<float> block;
typedef std::vector< std::vector< std::vector< std::vector<float> > > > imageBlocks;

inline int round(float r)
{
    float res =  (r > 0.0f) ? (r + 0.5f) : (r - 0.5f); 
	return (int)res;
}

void coeffsToImage(std::vector<block> &rho, std::vector<float> &image, int w, int h, int c);
void blocksToImage(std::vector<block> const & blocks, std::vector<float> & image, int w, int h, int c);
void jpegCoeffBlocksToMyBlocks(CoeffBlocks & from, std::vector<block> & to);
void dequant(std::vector<block> & coeffs, size_t chSize, QuantTables const & qtables);
void getBounds(std::vector<block> const & coeffs, size_t chSize, QuantTables const & qtables, std::vector<block> & lower, std::vector<block> & upper);
void imageToBlocks(std::vector<float> const & image, std::vector<block> & blocks, int w, int h, int c);

void color_space_transform(
    std::vector<float> &img
,   const unsigned width
,   const unsigned height
,   const unsigned chnls
,   const bool rgb2ycbcr
);

void write_JPEG_file(
	char const * filename
,	std::vector<float>& image
,	size_t image_width
,	size_t image_height
,	size_t channels
,	int quality
,	QuantTables * qtables = 0
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
