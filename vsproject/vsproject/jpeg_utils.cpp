#include <stdio.h>
#include <stdlib.h>
#include "jpeg_utils.h"
#include "io_png.h"
#include <memory.h>

#include "dct.h"

#include <iostream>

void write_JPEG_file(
    char const * filename
,   std::vector<float>& image
,   size_t image_width
,   size_t image_height
,   size_t channels
,   int quality
,   QuantTables * qtables
){
	jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr;
  
	FILE * outfile;		/* target file */
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	int row_stride;		/* physical row width in image buffer */

	// Create buffer aligned as libjpeg wants
	JSAMPLE* image_buffer = new JSAMPLE[image.size()];
	convertToJpegData(image_buffer, image, image_width, image_height, channels);

	// Create JPEG compression object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	// Specify data destination (eg, a file)
	if ((outfile = fopen(filename, "wb")) == NULL) 
	{
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}
	jpeg_stdio_dest(&cinfo, outfile);

	// Set parameters for compression
	cinfo.image_width = image_width;
	cinfo.image_height = image_height;
	cinfo.input_components = channels;
	
	cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */

	jpeg_set_defaults(&cinfo);
	if(qtables==0)
	{
		jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
	}
	else
	{
		for(size_t i = 0; i < qtables->size(); ++i)
		{
			jpeg_add_quant_table(&cinfo, i, &((*qtables)[i][0]), 100, TRUE);
		}
	}

	// Temporary solution
	for(size_t i = 0; i < channels; ++i)
	{
		cinfo.comp_info[i].h_samp_factor=1;
		cinfo.comp_info[i].v_samp_factor=1;
	}

	cinfo.do_fancy_downsampling = FALSE;


	// Start compressor
	jpeg_start_compress(&cinfo, TRUE);

	// Write to file
	row_stride = image_width * channels;	/* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	// Finish compression
	jpeg_finish_compress(&cinfo);
	fclose(outfile);

	// Release JPEG compression object and buffer
	jpeg_destroy_compress(&cinfo);
	delete[] image_buffer;
}


void read_JPEG_file(
    char const * filename
,   std::vector<float>& image
,   size_t &image_width
,   size_t &image_height
,   size_t &channels
){

	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
  
	FILE * infile;		/* source file */
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */


	if ((infile = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}

	// Allocate and initialize JPEG decompression object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	// Specify data source (eg, a file)
	jpeg_stdio_src(&cinfo, infile);

	// Read file parameters
	jpeg_read_header(&cinfo, TRUE);

	cinfo.do_fancy_upsampling = FALSE;
	cinfo.dither_mode = JDITHER_NONE;
	cinfo.dct_method = JDCT_FLOAT;

	// Start decompressor
	jpeg_start_decompress(&cinfo);

	// Fill in information and allocate buffer
	image_width = cinfo.output_width;
	image_height = cinfo.output_height;
	channels = cinfo.output_components;
	JSAMPLE* image_buffer = new JSAMPLE[image_width * image_height * channels];

	// JSAMPLEs per row in output buffer
	row_stride = cinfo.output_width * cinfo.output_components;

	// Make a one-row-high sample array that will go away when done with image
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
	
	// Read data
	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, buffer, 1);
		memcpy(image_buffer+row_stride*(cinfo.output_scanline-1), buffer[0], sizeof(JSAMPLE)*row_stride);
	}
  
	// Finish decompression
	jpeg_finish_decompress(&cinfo);
	fclose(infile);

	// Convert buffer to vector
	convertToPngData(image, image_buffer, image_width, image_height, channels);

	// Release JPEG decompression object and buffer
	jpeg_destroy_decompress(&cinfo);
	delete[] image_buffer;
}

void read_JPEG_coefficients(char const * filename, 	CoeffBlocks& image_coeffs, QuantTables& qtables, SamplingFactors& sfact)
{
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	jvirt_barray_ptr * coef_arrays;

	FILE * infile;		/* source file */

	if ((infile = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}

	// Allocate and initialize JPEG decompression object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	// Specify data source (eg, a file)
	jpeg_stdio_src(&cinfo, infile);

	// Read file parameters
	jpeg_read_header(&cinfo, TRUE);
	cinfo.do_fancy_upsampling = FALSE;
	cinfo.dither_mode = JDITHER_NONE;
	cinfo.dct_method = JDCT_FLOAT;

	// Read data
	image_coeffs.resize(cinfo.num_components);
	qtables.resize(cinfo.num_components);
	sfact.resize(cinfo.num_components);

	coef_arrays=jpeg_read_coefficients(&cinfo);
	for(int c = 0 ; c<cinfo.num_components; ++c)
	{
		JBLOCKARRAY blocks = (*cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coef_arrays[c], 0, 1, FALSE);
		jpeg_component_info &comp_info = cinfo.comp_info[c];

		image_coeffs[c].resize(comp_info.height_in_blocks);
		for(size_t i=0; i<comp_info.height_in_blocks; ++i)
		{
			image_coeffs[c][i].resize(comp_info.width_in_blocks);
			for(size_t j=0; j<comp_info.width_in_blocks; ++j)
			{
				image_coeffs[c][i][j].resize(DCTSIZE2);
				for (size_t k = 0; k < DCTSIZE2; k++)
				{
					image_coeffs[c][i][j][k]=blocks[i][j][k];
				}
			}
		}

		qtables[c].resize(DCTSIZE2);
		for(int i=0; i<DCTSIZE2; ++i)
			qtables[c][i] = comp_info.quant_table->quantval[i];

		sfact[c] = std::make_pair(comp_info.h_samp_factor, comp_info.v_samp_factor);
	}

	jpeg_finish_decompress(&cinfo);
	fclose(infile);
	jpeg_destroy((j_common_ptr)&cinfo);
}

void convertToJpegData(
    JSAMPLE* jpegData
,   std::vector<float>& pngData
,   size_t image_width
,   size_t image_height
,   size_t channels
){
	for(size_t i=0; i<image_height; ++i)
		for(size_t j=0; j<image_width; ++j)
			for(size_t k=0; k<channels; ++k)
				jpegData[i*image_width*channels + j*channels + k] = 
					//std::max(0, std::min((int)round(pngData[k*image_height*image_width + i*image_width + j]), 255));
					(int)round(pngData[k*image_height*image_width + i*image_width + j]);
}

void convertToPngData(
    std::vector<float>& pngData
,   JSAMPLE* jpegData
,   size_t image_width
,   size_t image_height
,   size_t channels
){
	pngData.resize(image_width*image_height*channels);

	for(size_t i=0; i<image_height; ++i)
		for(size_t j=0; j<image_width; ++j)
			for(size_t k=0; k<channels; ++k)
				pngData[k*image_height*image_width + i*image_width + j] = 
					(float) jpegData[i*image_width*channels + j*channels + k]; 
}

void blocksToImage(std::vector<block> const & blocks, std::vector<float> & image, int w, int h, int c)
{
	int heightInBlocks = h / DCTSIZE;
	int widthInBlocks = w / DCTSIZE;

	image.resize(c*w*h);
	for(size_t i = 0; i < blocks.size(); ++i)
	{
		int channel = i / (widthInBlocks * heightInBlocks);
		int height = (i % (widthInBlocks * heightInBlocks)) / widthInBlocks;
		int width = (i % (widthInBlocks * heightInBlocks)) % widthInBlocks; 
		for(size_t j = 0; j  < DCTSIZE2; ++j )
		{
			image[channel * w * h + w * (height * DCTSIZE + j / DCTSIZE) + width * DCTSIZE + j % DCTSIZE] = 
				blocks[i][j];
		}
	}
}

void jpegCoeffBlocksToMyBlocks(CoeffBlocks & from, std::vector<block> & to)
{
	to.clear();
	for(size_t c = 0; c < from.size(); ++c)
	{
		for(size_t w = 0; w < from[0].size(); ++w)
		{
			for(size_t h = 0; h < from[0][0].size(); ++h)
			{
				to.push_back(from[c][w][h]);
			}
		}
	}
}

void dequant(std::vector<block> & coeffs, size_t chSize, QuantTables const & qtables)
{
	for(size_t i = 0; i < coeffs.size(); ++i)
	{
		for(size_t k = 0; k < DCTSIZE2; ++k)
		{
			coeffs[i][k] *= qtables[i/chSize][k];
		}
	}
}

void getBounds(std::vector<block> const & coeffs, size_t chSize, QuantTables const & qtables, std::vector<block> & lower, std::vector<block> & upper)
{
	lower.resize(coeffs.size());
	upper.resize(coeffs.size());

	for(size_t i = 0; i < coeffs.size(); ++i)
	{
		lower[i].resize(DCTSIZE2);
		upper[i].resize(DCTSIZE2);

		for(size_t k = 0; k < DCTSIZE2; ++k)
		{
			lower[i][k]= (coeffs[i][k] - 0.3999f) * qtables[i/chSize][k];
			upper[i][k] = (coeffs[i][k] + 0.3999f) * qtables[i/chSize][k];
		}
	}
}

void imageToBlocks(std::vector<float> const & image, std::vector<block> & blocks, int w, int h, int c)
{
	int heightInBlocks = h / DCTSIZE;
	int widthInBlocks = w / DCTSIZE;

	
	blocks.resize(c * widthInBlocks * heightInBlocks);
	for(size_t i = 0; i < blocks.size(); ++i)
	{
		blocks[i].resize(DCTSIZE2);
		int channel = i / (widthInBlocks * heightInBlocks);
		int height = (i % (widthInBlocks * heightInBlocks)) / widthInBlocks;
		int width = (i % (widthInBlocks * heightInBlocks)) % widthInBlocks; 

		for(size_t j = 0; j < DCTSIZE2; ++j)
		{
			blocks[i][j] = 
				image[channel * w * h + w * (height * DCTSIZE + j / DCTSIZE) + width * DCTSIZE + j % DCTSIZE];
		}
	}
}

void color_space_transform(
    std::vector<float> &img
,   const unsigned width
,   const unsigned height
,   const unsigned chnls
,   const bool rgb2ycbcr
){
	//! Declarations
	std::vector<float> tmp;
	tmp.resize(chnls * width * height);
	const unsigned red   = 0;
	const unsigned green = width * height;
	const unsigned blue  = width * height * 2;


	if (rgb2ycbcr)
	{
	//#pragma omp parallel for
		for (int k = 0; k < int(width * height); k++)
		{
			//! Y
			tmp[k + red  ] =  0.299f * img[k + red] + 0.587f * img[k + green] + 0.114f * img[k + blue];
			//! U
			tmp[k + green] = (float)128 -0.1687f * img[k + red] - 0.3313f * img[k + green] + 0.500f * img[k + blue];
			//! V
			tmp[k + blue ] = (float)128 + 0.500f * img[k + red] - 0.4187f * img[k + green] - 0.0813f * img[k + blue];
		}
	}
	else
	{
	//#pragma omp parallel for
		for (int k = 0; k < int(width * height); k++)
		{
			//! Red   channel
			tmp[k + red] = 1.000f * (img[k + red]) + 0.000f * img[k + green] + 1.402f * (img[k + blue]-128.0f);
			//! Green channel
			tmp[k + green] = 1.000f * (img[k + red]) - 0.34414f * (img[k + green]-128) - 0.71414f * (img[k + blue]-128.0f);
			//! Blue  channel
			tmp[k + blue] = 1.000f * (img[k + red]) + 1.772f * (img[k + green]-128) + 0.000f * img[k + blue];
		}
	}
   
	//#pragma omp parallel for
	for (int k = 0; k < int(width * height * chnls); k++)
	{


		img[k] = tmp[k];
	}
}


void coeffsToImage(std::vector<block> &rho, std::vector<float> &image, int w, int h, int c)
{
	std::vector<block> y;
	y.resize(rho.size());

	for(size_t i = 0; i < rho.size(); ++i)
	{
		for(size_t j = 0; j < DCTSIZE2; ++j)
		{
			//rho[i][j] = (int)(round(rho[i][j]));
		}
		y[i].resize(DCTSIZE2);
		idct(rho[i], y[i]);
	}

	blocksToImage(y, image, w, h, c);

	for(size_t i = 0; i < image.size(); ++i)
	{
		float &cur = image[i];
		
		cur+=(float)128;
		//cur = (float)(int)(cur);
		
		//cur = std::min(255.0f, cur);
		//cur = std::max(0.0f, cur);
	}

	color_space_transform(image, w, h, c, false);
	
	for(size_t k = 0; k < image.size(); ++k)
	{
		image[k] = (float)(round)(image[k]);


		image[k] = std::min(255.0f,  image[k]);
		image[k] = std::max(0.0f,  image[k]);
	}
}