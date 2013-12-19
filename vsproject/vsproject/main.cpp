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

double PSNR_Matrix( vector<float> img1, // changed image
                    vector<float> img2 // original image
					)
{
    long i;
    double error, pix, x, psnr;

    /* calculating distortion */
    error = 0.0;
    pix = (double)img1.size();
    for(i=0;i<img1.size();i++)
    {
        x = (double)img1[i] - (double)img2[i];
        error += ((x * x) / pix);
    }
    psnr = 10.0 * log10((255.0*255.0)/error);

    return (psnr);
}

int main(int argc, char **argv)
{
	//freopen("out.txt", "w", stdout);
	initDCT();

	size_t h, w, c;
    vector<float> jpegData;
	vector<float> origImage;
	CoeffBlocks coeffs;
	QuantTables qtables;
	SamplingFactors sfactors;

	load_image(argv[1], origImage, &w, &h, &c);
	write_JPEG_file("test.jpg", origImage, w, h, c, 20);

	read_JPEG_file("test.jpg", jpegData, w, h, c);
	read_JPEG_coefficients("test.jpg", coeffs, qtables, sfactors);

	vector<block> rho, orig;
	vector<block> lower, upper;
	int chSize = (w/DCTSIZE)*(h/DCTSIZE);
	
	jpegCoeffBlocksToMyBlocks(coeffs, rho);
    orig = rho;
    getBounds(rho, chSize, qtables, lower, upper);
    dequant(rho, chSize, qtables);
	
    optimize(atoi(argv[3]), jpegData, rho, lower, upper, w, h, c);

    // test coeffs
    for(size_t i = 0; i < orig.size(); ++i)
    {
        for(size_t j = 0; j < DCTSIZE2; ++j)
        {
            if(round(rho[i][j] / qtables[i/chSize][j]) != orig[i][j])
                    cout << "1 coef fail " << rho[i][j] <<" " << orig[i][j] << " " << qtables[i/chSize][j] <<endl;
        }
    }

	//printChannels(rho, 1, "rho", chSize);

	//block tmp(DCTSIZE2);
	//idct(rho[1], tmp);
	//for(size_t i = 0; i < DCTSIZE2; ++i)
	//{
	//	tmp[i]=round(tmp[i]);
	//}
	//printBlock(tmp, 0, "rho idct");
	//fdct(tmp, tmp);
	//printBlock(tmp, 0, "rho idct - fdct");

	//printChannels(lower, 1, "lower", chSize);
	//printChannels(upper, 1, "upper", chSize);
    
    // from coeffs
	coeffsToImage(rho, jpegData, w, h, c);

	cout << "!!!PSNR: " << PSNR_Matrix(jpegData, origImage) << endl;

	write_png_f32(argv[2], &jpegData[0], w, h, c);
    write_JPEG_file("q.jpg", jpegData, w, h, c, 80, &qtables);

	vector<block> t;
	imageToBlocks(jpegData, t, w, h, c);
	//printChannels(t, 1, "pixels rgb", chSize);
	

	color_space_transform(jpegData, w, h, c, true);
	for(size_t i = 0; i < jpegData.size(); ++i)
	{
		jpegData[i] = round(jpegData[i]);
		jpegData[i] = std::max(0.0f, jpegData[i]);
		jpegData[i] = std::min(255.0f, jpegData[i]);
		jpegData[i]-=128;
	}

	imageToBlocks(jpegData, t, w, h, c);
	//printChannels(t, 1, "ycbcr", chSize);
	for(size_t i = 0; i < t.size(); ++i)
	{
		fdct(t[i], t[i]);
	}

	//printChannels(t, 1, "coeffs from image", chSize);


	read_JPEG_coefficients("q.jpg", coeffs, qtables, sfactors);
	
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
