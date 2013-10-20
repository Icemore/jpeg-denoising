#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <string.h>

#include "bm3d.h"
#include "utilities.h"
#include "jpeg_utils.h"

using namespace std;


int main(int argc, char **argv)
{
	size_t h, w, c;
    vector<float> jpegData;
	CoeffBlocks coeffs;
	QuantTables qtables;
	SamplingFactors sfactors;

	read_JPEG_file(argv[1], jpegData, w, h, c);
	read_JPEG_coefficients(argv[1], coeffs, qtables, sfactors);

	cout<<coeffs.size()<<endl;
	cout<<coeffs[0].size()<<endl;
	cout<<coeffs[0][0].size()<<endl;
	cout<<coeffs[0][0][0].size()<<endl;

	for (int i = 0; i < sfactors.size(); i++)
	{
		cout<<sfactors[i].first<<" "<<sfactors[i].second<<endl;
	}

    write_JPEG_file(argv[2], jpegData, w, h, c, 10);

	return EXIT_SUCCESS;
}
