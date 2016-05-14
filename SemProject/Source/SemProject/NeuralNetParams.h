// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <fstream>
#include <windows.h>

/**
 * 
 */
class SEMPROJECT_API NeuralNetParams
{

public:
	static int iNumInputs;
	static int iNumHidden;
	static int iNeuronsPerHiddenLayer;
	static int iNumOutputs;

	//for tweaking the sigmoid function
	static double dActivationResponse;

	//bias value
	static double dBias;
public:
	NeuralNetParams();
	bool LoadInParameters(char* szFileName);
	~NeuralNetParams();
};
