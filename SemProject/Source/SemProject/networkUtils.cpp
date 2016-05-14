// Fill out your copyright notice in the Description page of Project Settings.

#include "SemProject.h"
#include "networkUtils.h"
#include <math.h>

networkUtils::networkUtils()
{
}

networkUtils::~networkUtils()
{
}

//convert an integer to a string
std::string itos(int arg)
{
	std::ostringstream buffer;

	//send the int to the ostringstream
	buffer << arg;

	//capture the string
	return buffer.str();
}

//converts a float to a string
std::string ftos(float arg)
{
	std::ostringstream buffer;

	//send the int to the ostringstream
	buffer << arg;

	//capture the string
	return buffer.str();
}

//clamps the first argument between the second two
void Clamp(double &arg, double min, double max)
{
	if (arg < min)
	{
		arg = min;
	}
	else if (arg > max)
	{
		arg = max;
	}
}