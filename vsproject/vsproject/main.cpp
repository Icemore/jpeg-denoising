#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>


#include "jpeg_utils.h"
#include "io_png.h"
#include "jpeg_optimize.h"
#include "dct.h"

using namespace std;

#define FAST_FLOAT float

int main(int argc, char **argv)
{
	//freopen("out.txt", "w", stdout);

	size_t h, w, c;
    vector<float> jpegData;
	CoeffBlocks coeffs;
	QuantTables qtables;
	SamplingFactors sfactors;

	read_JPEG_file(argv[1], jpegData, w, h, c);
	read_JPEG_coefficients(argv[1], coeffs, qtables, sfactors);

	vector<block> rho, orig;
	vector<block> lower, upper;

	jpegCoeffBlocksToMyBlocks(coeffs, rho);
	orig = rho;
	getBounds(rho, (w/DCTSIZE)*(h/DCTSIZE), qtables, lower, upper);
	dequant(rho, (w/DCTSIZE)*(h/DCTSIZE), qtables);

	optimize(jpegData, rho, lower, upper, w, h, c);

	write_png_f32(argv[2], &jpegData[0], w, h, c);

	// test coeffs
	int chSize = (w/DCTSIZE)*(h/DCTSIZE);
	for(size_t i = 0; i < orig.size(); ++i)
	{
		for(size_t j = 0; j < DCTSIZE2; ++j)
		{
			if(round(rho[i][j] / qtables[i/chSize][j]) != orig[i][j])
				cout << rho[i][j] <<" " << orig[i][j] << " " << qtables[i/chSize][j] <<endl;
		}
	}
    
	// from coeffs
	vector<block> y(rho.size());
	for(size_t i = 0; i < rho.size(); ++i)
	{
		y[i].resize(DCTSIZE2);
		idct(rho[i], y[i]);
	}
	blocksToImage(y, jpegData, w, h, c);
	color_space_transform(jpegData, w, h, c, false);
	write_png_f32(argv[3], &jpegData[0], w, h, c);
	write_JPEG_file("q.jpg", jpegData, w, h, c, 80, &qtables);

	return 0;
}
