#include <stdio.h>
#include <stdlib.h>
#include "jpeg_utils.h"
#include "io_png.h"
#include <memory.h>

void write_JPEG_file(
	char const * filename
,	std::vector<float>& image
,	size_t image_width
,	size_t image_height
,	size_t channels
,	int quality
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
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

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
,	std::vector<float>& image
,	size_t &image_width
,	size_t &image_height
,	size_t &channels
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
		for(int i=0; i<comp_info.height_in_blocks; ++i)
		{
			image_coeffs[c][i].resize(comp_info.width_in_blocks);
			for(int j=0; j<comp_info.width_in_blocks; ++j)
			{
				image_coeffs[c][i][j].resize(DCTSIZE2);
				for (int k = 0; k < DCTSIZE2; k++)
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
,	std::vector<float>& pngData
,	size_t image_width
,	size_t image_height
,	size_t channels
){
	for(size_t i=0; i<image_height; ++i)
		for(size_t j=0; j<image_width; ++j)
			for(size_t k=0; k<channels; ++k)
				jpegData[i*image_width*channels + j*channels + k] = 
					(JSAMPLE) (pngData[k*image_height*image_width + i*image_width + j] + 0.5f);
}

void convertToPngData(
	std::vector<float>& pngData
,	JSAMPLE* jpegData
,	size_t image_width
,	size_t image_height
,	size_t channels
){
	pngData.resize(image_width*image_height*channels);

	for(size_t i=0; i<image_height; ++i)
		for(size_t j=0; j<image_width; ++j)
			for(size_t k=0; k<channels; ++k)
				pngData[k*image_height*image_width + i*image_width + j] = 
					(float) jpegData[i*image_width*channels + j*channels + k]; 
}