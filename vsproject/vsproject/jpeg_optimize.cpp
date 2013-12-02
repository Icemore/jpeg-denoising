
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

			//float left = 1 / sigmaSqr;
			//float right = rho[i] / sigmaSqr;

			//if(right / left >= u[i] - muU[i] * lambdaU[i])
			//{
			//	left += 1 / muU[i];
			//	right += u[i] / muU[i] - lambdaU[i];
			//}

			//if(right / left <= l[i] + muL[i] * lambdaL[i])
			//{
			//	left += 1 / muL[i];
			//	right += l[i] / muL[i] + lambdaL[i];
			//}

			//rho[i] = right / left;
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

	//idct(rho, y);

	//if(cnt > 1)
	//{
	//	cout <<cnt <<endl;
	//}

	for(size_t i = 0; i < DCTSIZE2; ++i)
	{
		//cout << lambdaL[i] <<" ";
		if(rho[i] < l[i] || rho[i] > u[i])
		{
			cout << "FAIL";
		}
	}
}

void optimize(vector<float> & image, vector<block> & rho, vector<block> & lower, vector<block> & upper, int w, int h, int c)
{
	vector<float> tmp(image.size());
	vector<float> res(image.size());
	vector<block> y;
	vector<block> Pomega;

	initDCT();

	y.resize(rho.size());
	for(size_t i = 0; i < y.size(); ++i)
		y[i].resize(DCTSIZE2);

	for(size_t t = 0; t < 5; ++t)
	{
		cout << "starting bm3d" << endl;
		run_bm3d(20, image, tmp, res, w, h, c, true, true, DCT, DCT, YCBCR);
		//res = image;
		cout << "bm3d done" << endl;

		color_space_transform(res, w, h, c, true);
		cout << "color converted" << endl;
		imageToBlocks(res, Pomega, w, h, c);
		cout << "image to blocks done" << endl;

		for(size_t i = 0; i < rho.size(); ++i)
		{
			//y[i] = Pomega[i];
			optimizeForBlock(Pomega[i], rho[i], y[i], lower[i], upper[i]);
		}

		cout << "blocks optimized!" << endl;
		
		blocksToImage(y, image, w, h, c);
		cout << "back to image" << endl;

		color_space_transform(image, w, h, c, false);
		cout << "color space back to normal" << endl;
	}
}