#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>

#include "utilities.h"
#include "bm3d.h"
#include "jpeg_utils.h"
#include "io_png.h"
#include "jpeg_optimize.h"
#include "dct.h"

using namespace std;

#define FAST_FLOAT float

void printChannels(vector<block> y, int id, string str, int chSize)
{
	for(size_t i = 0; i < 3; ++i)
	{
		cout << str << ": " << i << endl;
		for(size_t j = 0; j < DCTSIZE2; ++j)
		{
			cout << y[i*chSize+id][j] <<" ";
			if(j%DCTSIZE == 7) cout << endl;
		}
		cout << endl;
	}
	cout << endl;
}

void printBlock(block t, int id, std::string msg)
{
	cout << id << ": " << msg << endl;
	for(size_t j = 0; j < DCTSIZE2; ++j)
	{
		cout << t[j] << " ";
		if(j%DCTSIZE == 7) cout << endl;
	}
	cout << endl;
}

/*
 * parameters:
 * argv[1] - path to original png
 * argv[2] - path to output png
 * argv[3] - sigma for bm3d
 * argv[4] - number of iterations
 * argv[5] - jpeg quality
 */
int main(int argc, char **argv)
{
	initDCT();

	size_t h, w, c;
	vector<float> jpegData;
	vector<float> origImage;
	CoeffBlocks coeffs;
	QuantTables qtables;
	SamplingFactors sfactors;

	// load original png
	load_image(argv[1], origImage, &w, &h, &c);

	//make test jpeg
	write_JPEG_file("test.jpg", origImage, w, h, c, atoi(argv[5]));

	//load and optimize jpeg
	read_JPEG_file("test.jpg", jpegData, w, h, c);
	read_JPEG_coefficients("test.jpg", coeffs, qtables, sfactors);

	vector<block> rho, orig;
	vector<block> lower, upper;
	int chSize = (w/DCTSIZE)*(h/DCTSIZE);
	
	jpegCoeffBlocksToMyBlocks(coeffs, rho);
	orig = rho;

	getBounds(rho, chSize, qtables, lower, upper);
	dequant(rho, chSize, qtables);
	
	optimize(atoi(argv[4]), atoi(argv[3]), origImage, jpegData, rho, lower, upper, w, h, c);

	// test coeffs
	for(size_t i = 0; i < orig.size(); ++i)
	{
		for(size_t j = 0; j < DCTSIZE2; ++j)
		{
			if(round(rho[i][j] / qtables[i/chSize][j]) != orig[i][j])
					cout << "1 coef fail " << rho[i][j] <<" " << orig[i][j] << " " << qtables[i/chSize][j] <<endl;
		}
	}
	
	// restore image from coeffs
	coeffsToImage(rho, jpegData, w, h, c);

	cout << "!!!PSNR: " << PSNR(jpegData, origImage) << endl;

	//write final image to png
	write_png_f32(argv[2], &jpegData[0], w, h, c);
	
	//compress final image with same parameters that was in starting one
	write_JPEG_file("compressed.jpg", jpegData, w, h, c, 80, &qtables);

	//check that we got the same image
	vector<block> t;
	imageToBlocks(jpegData, t, w, h, c);	

	color_space_transform(jpegData, w, h, c, true);
	for(size_t i = 0; i < jpegData.size(); ++i)
	{
		jpegData[i] = (float)round(jpegData[i]);
		jpegData[i] = std::max(0.0f, jpegData[i]);
		jpegData[i] = std::min(255.0f, jpegData[i]);
		jpegData[i] -= 128;
	}

	imageToBlocks(jpegData, t, w, h, c);
	for(size_t i = 0; i < t.size(); ++i)
	{
		fdct(t[i], t[i]);
	}

	read_JPEG_coefficients("compressed.jpg", coeffs, qtables, sfactors);
	
	jpegCoeffBlocksToMyBlocks(coeffs, t);

	int cnt=0;
	for(size_t i = 0; i < t.size(); ++i)
	{
		for(size_t j = 0; j < DCTSIZE2; ++j)
		{
			if(orig[i][j] != t[i][j])
			{
				cout << i << "-" << j << " fail: " << orig[i][j] << " " << t[i][j] << " " << t[i][j]*qtables[i/chSize][j] << " " << qtables[i/chSize][j] << " " << rho[i][j] << endl;
				++cnt;
			}
		}
	}

	cout << "fails cnt: " << cnt << endl;
		
	return 0;
}
