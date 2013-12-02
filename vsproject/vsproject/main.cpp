#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>


#include "jpeg_utils.h"
#include "io_png.h"
#include "jpeg_optimize.h"

using namespace std;

#define FAST_FLOAT float

int main(int argc, char **argv)
{
	size_t h, w, c;
    vector<float> jpegData;
	CoeffBlocks coeffs;
	QuantTables qtables;
	SamplingFactors sfactors;

	read_JPEG_file(argv[1], jpegData, w, h, c);
	read_JPEG_coefficients(argv[1], coeffs, qtables, sfactors);

	vector<block> rho;
	vector<block> lower, upper;

	jpegCoeffBlocksToMyBlocks(coeffs, rho);
	getBounds(rho, (w/DCTSIZE)*(h/DCTSIZE), qtables, lower, upper);
	dequant(rho, (w/DCTSIZE)*(h/DCTSIZE), qtables);

	optimize(jpegData, rho, lower, upper, w, h, c);
	
	write_png_f32(argv[2], &jpegData[0], w, h, c);
    write_JPEG_file("q.jpg", jpegData, w, h, c, 80, &qtables);

	return 0;
}
