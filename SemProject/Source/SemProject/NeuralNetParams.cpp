// Fill out your copyright notice in the Description page of Project Settings.

#include "SemProject.h"
#include "NeuralNetParams.h"

// because we will always be loading in the settings from an ini file
//we can just initialize everything to zero
int NeuralNetParams::iNumInputs = 0;
int NeuralNetParams::iNumHidden = 0;
int NeuralNetParams::iNeuronsPerHiddenLayer = 0;
int NeuralNetParams::iNumOutputs = 0;
double NeuralNetParams::dActivationResponse = 0;
double NeuralNetParams::dBias = 0;

NeuralNetParams::NeuralNetParams()
{
	if (!LoadInParameters("params.ini"))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot find .ini file!"));
	}
}

bool NeuralNetParams::LoadInParameters(char* szFileName)
{
	std::ifstream grab(szFileName);

	//check file exists
	if (!grab)
	{
		return false;
	}

	//load in from the file
	char ParamDescription[40];

	grab >> ParamDescription;
	grab >> iNumInputs;
	grab >> ParamDescription;
	grab >> iNumHidden;
	grab >> ParamDescription;
	grab >> iNeuronsPerHiddenLayer;
	grab >> ParamDescription;
	grab >> iNumOutputs;
	grab >> ParamDescription;
	grab >> dActivationResponse;
	grab >> ParamDescription;
	grab >> dBias;


	return true;
}

NeuralNetParams::~NeuralNetParams()
{
}
