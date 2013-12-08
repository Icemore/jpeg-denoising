#pragma once

#include "jpeg_utils.h"

void optimize(std::vector<float> & image, std::vector<block> & rho, std::vector<block> & lower, std::vector<block> & upper, int w, int h, int c);
void optimizeForBlock(block const & Pomega, block & rho, block & y, block const & l, block const & u);