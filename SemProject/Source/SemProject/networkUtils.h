// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <stdlib.h>
#include <math.h>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>

//some random number functions

//returns a random integer between x and y
inline int RandInt(int x, int y) { return rand() % (y - x + 1) + x; }

//returns a random float between zero and 1
inline double RandFloat() { return (rand()) / (RAND_MAX + 1.0); }

//returns a random bool
inline bool RandBool()
{
	if (RandInt(0, 1)) return true;
	else return false;
}

//returns a random float in the range -1 < n < 1
inline double RandomClamped() { return RandFloat() - RandFloat(); }


//other useful functions

//converts an int to a std::string
std::string itos(int arg);

//converts a float to a std::string
std::string ftos(float arg);

//clamps the first argument between the second two
void Clamp(double &arg, double min, double max);

//point structure
struct SPoint
{
	float x, y;
	SPoint() {}
	SPoint(float a, float b) :x(a), y(b) {}
};

/**
 * 
 */
class SEMPROJECT_API networkUtils
{
public:
	networkUtils();
	~networkUtils();
};
