
#define _USE_MATH_DEFINES
#include <cmath>

#include "jpeg_utils.h"


static float precalcCos[8][8];
void initDCT()
{
	for(size_t u = 0; u < DCTSIZE; ++u)
	{
		for(size_t x = 0; x < DCTSIZE; ++x)
		{
			precalcCos[x][u] = (float) cos((2*x+1) * u *M_PI / 16);
		}
	}
}

void fdct(block & from, block & to)
{
	for(size_t v = 0; v < DCTSIZE; ++v)
	{
		for(size_t u = 0; u < DCTSIZE; ++u)
		{
			to[v*DCTSIZE + u] = 0;
			for(size_t x = 0; x < DCTSIZE; ++x)
			{
				for(size_t y = 0; y < DCTSIZE; ++y)
				{
					to[v*DCTSIZE + u] += from[y*DCTSIZE + x] * precalcCos[x][u] * precalcCos[y][v];
				}
			}

			float c = 1.0/4.0;
			if(u == 0) c *= (float)M_SQRT1_2;
			if(v == 0) c *= (float)M_SQRT1_2;

			to[v*DCTSIZE + u] *= c;
		}
	}
}

void idct(block &from, block & to)
{
	for(size_t x = 0; x < DCTSIZE; ++x)
	{
		for(size_t y = 0; y < DCTSIZE; ++y)
		{
			to[x*DCTSIZE + y] = 0;
			for(size_t u = 0; u < DCTSIZE; ++u)
			{
				for(size_t v = 0; v < DCTSIZE; ++v)
				{
					float c = 1;
					if(u==0) c *= (float)M_SQRT1_2;
					if(v==0) c *= (float)M_SQRT1_2;

					to[x*DCTSIZE + y] += c * from[v*DCTSIZE+u] * precalcCos[y][u] * precalcCos[x][v];
				}
			}
			to[x*DCTSIZE + y] /= 4;
		}
	}
}