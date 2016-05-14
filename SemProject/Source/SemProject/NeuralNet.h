// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <vector>
/**
 * 
 */
using namespace std;

class Net;
class Neuron;

struct Connection
{
	double weight;
	double deltaWeight;
};

struct NeuronWeights
{
	vector<double> neuronWeights;
};

struct LayerWeights
{
	vector<NeuronWeights> layerWeights;
};

struct NetworkWeights
{
	vector<LayerWeights> networkWeights;
};

typedef vector<Neuron> Layer;

// ****************** class TrainingData ******************
class TrainingData
{
public:
	TrainingData(FString filePath);
	void getTopology(vector<unsigned> &topology);

public:
	TArray<FString> textFile;
};

// ****************** class Neuron ******************
class Neuron
{
public:
	Neuron(unsigned numOutputs, unsigned myIndex);
	void setOutputVal(double val) { m_outputVal = val; }
	double getOutputVal(void) const { return m_outputVal; }
	void feedForward(const Layer &prevLayer);
	void setWeights(Layer &prevLayer, vector<double> newWeights);

private:
	static double transferFunction(double x);
	static double randomWeight(void) { return rand() / double(RAND_MAX); }
	double m_outputVal;
	vector<Connection> m_outputWeights;
	unsigned m_myIndex;
	double m_gradient;
};

// ****************** class Net ******************
class Net
{
public:
	Net() {}; //an empty constructor that UE4 needs in order to compile
	Net(const vector<unsigned> &topology);
	void feedForward(const vector<double> &inputVals);
	void getResults(vector<double> &resultVals) const;
	void setWeights(NetworkWeights networkWeights);
	NetworkWeights readSavedWeightsFromFile(TArray<FString> textFile);

private:
	vector<Layer> m_layers; // m_layers[layerNum][neuronNum]
};

class SEMPROJECT_API NeuralNet
{
public:
	NeuralNet();
	~NeuralNet();
	static NeuralNet* joyInit();
	int CheckDataForCircle(vector<double> inputs);
	
			

private:
	Net myNet;
	vector<double> results;
	static NeuralNet* net;

};


