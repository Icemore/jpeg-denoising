
#include "jpeg_utils.h"
#include "dct.h"
#include "bm3d.h"

#include <iostream>

const float sigma = 0.5f;
const float gamma = 0.4f;
const float sigmaSqr = sigma*sigma;

using std::vector;
using std::cout;
using std::endl;

#define YUV       0
#define YCBCR     1
#define OPP       2
#define RGB       3
#define DCT       4
#define BIOR      5
#define HADAMARD  6
#define NONE      7


void optimizeForBlock(block const & Pomega, block & rho, block & y, block const & l, block const & u)
{
	block lambdaL(DCTSIZE2, 1.0f), muL(DCTSIZE2, 0.5f);
	block lambdaU(DCTSIZE2, 1.0f), muU(DCTSIZE2, 0.5f);
	float const eps = 1e-7f;

	float diff = 1;
	int cnt = 0;
	while(abs(diff) > eps)
	{
		// Step for y
		idct(rho, y);
		for(size_t i = 0; i < DCTSIZE2; ++i)
		{
			y[i] = (y[i]/sigmaSqr + Pomega[i]/gamma)/(1/sigmaSqr + 1/gamma);
		}

		// Step for rho
		fdct(y, rho);
		for(size_t i = 0; i < DCTSIZE2; ++i)
		{
			float left = 1 / sigmaSqr;
			float right = rho[i] / sigmaSqr;

			if(right / left >= u[i] - muU[i] * lambdaU[i])
			{
				left += 1 / muU[i];
				right += u[i] / muU[i] - lambdaU[i];
			}

			if(right / left <= l[i] + muL[i] * lambdaL[i])
			{
				left += 1 / muL[i];
				right += l[i] / muL[i] + lambdaL[i];
			}

			rho[i] = right / left;

			// alternative way to optimize function for rho[i]
			// not sure if it works

			/*
			if(rho[i] < u[i] - muU[i] * lambdaU[i] && rho[i] > l[i] + muL[i] * lambdaL[i])
				continue;

			float cur = (rho[i] / sigmaSqr +  u[i]/muU[i] - lambdaU[i]) / (1/sigmaSqr + 1/muU[i]);
			if(cur >= u[i] - muU[i] * lambdaU[i])
			{
				rho[i] = cur;
				continue;
			}

			cur = (rho[i]/sigmaSqr + l[i]/muL[i] + lambdaL[i]) / (1/sigmaSqr + 1/muL[i]);
			if(cur <= l[i] + muL[i]*lambdaL[i])
			{
				rho[i]=cur;
				continue;
			}

			cout << "Can't optimize! "<<rho[i] << lambdaL[i] << " " << lambdaU[i] <<" " << l[i] <<" " <<u[i]<<endl;
			*/
		}

		// Step for lambda
		for(size_t i = 0; i < DCTSIZE2; ++i)
		{
			lambdaL[i] = std::max(0.0f, lambdaL[i] - (rho[i] - l[i]) / muL[i]);
			lambdaU[i] = std::max(0.0f, lambdaU[i] - (u[i] - rho[i]) / muU[i]);
		}

		diff = 0;
		for(size_t i = 0; i < DCTSIZE2; ++i)
		{
			diff=std::max(diff, std::max(0.0f, l[i] - rho[i]));
			diff=std::max(diff, std::max(0.0f, rho[i] - u[i]));
		}

		++cnt;
	}

	for(size_t i = 0; i < DCTSIZE2; ++i)
	{
		//cout << lambdaL[i] <<" ";
		if(rho[i] < l[i] || rho[i] > u[i])
		{
			cout << "FAIL";
		}
	}
}

void optimize(
	int iterations
,	float sigma
,	vector<float> orig
,	vector<float> & image
,	vector<block> & rho
,	vector<block> & lower
,	vector<block> & upper
,	int w
,	int h
,	int c
,	int windowSize
,	float lambda3D
,	float tauMatchFirst
,	float tauMatchSecond
)
{
	vector<float> tmp(image.size());
	vector<float> res(image.size());
	vector<block> y;
	vector<block> Pomega;

	initDCT();

	y.resize(rho.size());
	for(size_t i = 0; i < y.size(); ++i)
		y[i].resize(DCTSIZE2);

	for(size_t t = 0; t < iterations; ++t)
	{
		cout << "PSNR " << t <<": " << PSNR(orig, image) << endl;
		cout << "starting bm3d" << endl;
		
		run_bm3d(sigma - t, image, tmp, res, w, h, c, true, true, BIOR, BIOR, YCBCR, windowSize, windowSize, lambda3D, tauMatchFirst, tauMatchSecond);
		
		//res = image;
		cout << "bm3d done" << endl;

		color_space_transform(res, w, h, c, true);
		for(size_t i = 0; i < res.size(); ++i)
		{
			res[i] = (float)round(res[i]);
			res[i] = std::min(255.0f, res[i]);
			res[i] = std::max(0.0f, res[i]);

			res[i]-=128;
		}
		cout << "color converted" << endl;
		imageToBlocks(res, Pomega, w, h, c);
		cout << "image to blocks done" << endl;

		#pragma omp parallel for
		for(int i = 0; i < (int)rho.size(); ++i)
		{
			//y[i] = Pomega[i];
			optimizeForBlock(Pomega[i], rho[i], y[i], lower[i], upper[i]);
		}

		cout << "blocks optimized!" << endl;
		
		blocksToImage(y, image, w, h, c);
		cout << "back to image" << endl;

		for(size_t i = 0; i < image.size(); ++i)
		{
			image[i] += 128;
			image[i] = (float)round(image[i]);
			image[i] = std::min(255.0f, image[i]);
			image[i] = std::max(0.0f, image[i]);
		}

		color_space_transform(image, w, h, c, false);

		for(size_t i = 0; i < image.size(); ++i)
		{
			image[i] = (float)round(image[i]);
			image[i] = std::min(255.0f, image[i]);
			image[i] = std::max(0.0f, image[i]);
		}

		cout << "color space back to normal" << endl;
	}
}