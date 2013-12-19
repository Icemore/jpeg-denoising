
#define _USE_MATH_DEFINES
#include <cmath>

#include "jpeg_utils.h"


float precalcCos[8][8];
void initDCT()
{
	for(size_t x = 0; x < DCTSIZE; ++x)
	{
		for(size_t u = 0; u < DCTSIZE; ++u)
		{
			float t = (2.0f*(float)x+1.0f) * (float)u * (float)M_PI / 16.0f;
			precalcCos[x][u] = cos(t);
		}
	}
}

void fdct(block & from, block & to)
{
	block tmp(DCTSIZE2);

	for(size_t v = 0; v < DCTSIZE; ++v)
	{
		for(size_t u = 0; u < DCTSIZE; ++u)
		{
			tmp[v*DCTSIZE + u] = 0;
			for(size_t x = 0; x < DCTSIZE; ++x)
			{
				for(size_t y = 0; y < DCTSIZE; ++y)
				{
					tmp[v*DCTSIZE + u] += from[y*DCTSIZE + x] * precalcCos[x][u] * precalcCos[y][v];
				}
			}

			float c = 1.0f/4.0f;
			if(u == 0) c *= (float)1.0f/sqrt(2.0f);
			if(v == 0) c *= (float)1.0f/sqrt(2.0f);

			tmp[v*DCTSIZE + u] *= c;
		}
	}

	for(size_t i = 0; i < DCTSIZE2; ++i)
	{
		to[i] = tmp[i];
	}
}

void idct(block &from, block & to)
{
	block tmp(DCTSIZE2);
	for(size_t x = 0; x < DCTSIZE; ++x)
	{
		for(size_t y = 0; y < DCTSIZE; ++y)
		{
			tmp[y*DCTSIZE + x] = 0;
			for(size_t u = 0; u < DCTSIZE; ++u)
			{
				for(size_t v = 0; v < DCTSIZE; ++v)
				{
					float c = 1;
					if(u==0) c *= (float)1.0f/sqrt(2.0f);
					if(v==0) c *= (float)1.0f/sqrt(2.0f);

					tmp[y*DCTSIZE + x] += c * from[v*DCTSIZE+u] * precalcCos[x][u] * precalcCos[y][v];
				}
			}
			tmp[y*DCTSIZE + x] /= 4.0f;
		}
	}

	for(size_t i = 0; i < DCTSIZE2; ++i)
	{
		to[i]=tmp[i];
	}
}

//void idct(block &from, block & to)
//{
//	for(size_t x = 0; x < DCTSIZE; ++x)
//	{
//		for(size_t y = 0; y < DCTSIZE; ++y)
//		{
//			to[x*DCTSIZE + y] = 0;
//			for(size_t u = 0; u < DCTSIZE; ++u)
//			{
//				for(size_t v = 0; v < DCTSIZE; ++v)
//				{
//					float c = 1;
//					if(u==0) c *= (float)M_SQRT1_2;
//					if(v==0) c *= (float)M_SQRT1_2;
//
//					to[x*DCTSIZE + y] += c * from[v*DCTSIZE+u] * precalcCos[y][u] * precalcCos[x][v];
//				}
//			}
//			to[x*DCTSIZE + y] /= 4;
//		}
//	}
//}