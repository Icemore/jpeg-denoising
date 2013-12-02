#pragma once

#include "jpeg_utils.h"

void initDCT();
void fdct(block & from, block & to);
void idct(block &from, block & to);