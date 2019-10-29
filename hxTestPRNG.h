#pragma once
// Copyright 2017 Adrian Johnston

#include "hatchling.h"

// ----------------------------------------------------------------------------
// The linear congruential generator from Numerical Recipes.

struct hxTestPRNG {
public:
	hxTestPRNG(uint32_t seed=1u) : m_seed(seed) { }
	uint32_t operator()() { return (m_seed = 1664525u * m_seed + 1013904223u); }
	uint32_t m_seed;
};
