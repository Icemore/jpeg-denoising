#pragma once

#include "jpeg_utils.h"

extern double precalcCos[8][8];

void initDCT();
void fdct(block & from, block & to);
void idct(block &from, block & to);