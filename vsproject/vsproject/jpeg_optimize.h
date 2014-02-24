#pragma once

#include "jpeg_utils.h"

void optimize(
	int iterations
,	float sigma
,	std::vector<float> orig
,	std::vector<float> & image
,	std::vector<block> & rho
,	std::vector<block> & lower
,	std::vector<block> & upper
,	int w
,	int h
,	int c
,	int windowSize = 16
,	float lambda3D = 2.7f
,	float tauMatchFirst = -1
,	float tauMatchSecond = -1
);
void optimizeForBlock(block const & Pomega, block & rho, block & y, block const & l, block const & u);