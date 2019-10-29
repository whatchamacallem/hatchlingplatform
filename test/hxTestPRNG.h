#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/hatchling.h>

// ----------------------------------------------------------------------------
// The linear congruential generator from Numerical Recipes.

struct hxTestPRNG {
public:
	hxTestPRNG(uint32_t seed=1u) : m_seed(seed) { }
	uint32_t operator()() { return (m_seed = 1664525u * m_seed + 1013904223u); }
	uint32_t m_seed;
};
