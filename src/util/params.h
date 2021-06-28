#ifndef __params_h__
#define __params_h__
#include <random>
#include <ctime>
#include <functional>
static std::default_random_engine generator(time(nullptr));
static std::uniform_real_distribution<float> distribution(0,1);
static auto rroulette = std::bind(distribution,generator);

#endif
