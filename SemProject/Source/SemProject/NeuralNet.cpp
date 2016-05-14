/**
Jordan Mather, April 2016
Attempting to use a neural network to recognize motion gestures

This is a copy of the IndependentNeuralNet program that has been edited down to 
include only the functionality needed by the UE4 program.

// neural-net-tutorial.cpp
// David Miller, http://millermattson.com/dave
// See the associated video for instructions: http://vimeo.com/19569529
*/
#include "SemProject.h"
#include "NeuralNet.h"
#include "CoreMisc.h"
#include <vector>
#include <cassert>

NeuralNet* NeuralNet::net = NULL;
FString filePath = FString("C:/Users/Rorc/Dropbox/School/Senior Year/CSC 491/SemProject/SemProject/Source/SemProject/Weights.txt");
using namespace std;

NeuralNet::NeuralNet()
{
	TrainingData trainData(filePath);
	UE_LOG(LogTemp, Warning, TEXT("textFile size %d"), trainData.textFile.Num());

	// e.g., { 3, 2, 1 }
	vector<unsigned> topology;
	trainData.getTopology(topology); //first line of the Weights file is the topology of the net it refers to
	UE_LOG(LogTemp, Warning, TEXT("NEURALNET: Topology grabbed"));
	

	Net tempNet(topology); //use that info to make the net
	myNet = tempNet;
	UE_LOG(LogTemp, Warning, TEXT("NEURALNET: Net Created and Assigned"));

	NetworkWeights networkWeights = myNet.readSavedWeightsFromFile(trainData.textFile); //read the trained weights from file into the net
	myNet.setWeights(networkWeights); //attach read weights to the net
	UE_LOG(LogTemp, Warning, TEXT("NEURALNET: weights read and set"));
	
}

/*
inputs: the inputs to run through the Neural network
This method takes a vector<double> and feeds it through the neural network, returning 1 if it's a circle and 0 if it's not
*/
int NeuralNet::CheckDataForCircle(vector<double> inputs)
{
	myNet.feedForward(inputs);
	myNet.getResults(results);
	if (results[0] >= .999) //only one output from the net, since we're only recognizing a circle.
	{
		return 1;
	}
	return 0;
}

/*
Copying the setup of NetworkForController, this is a static class to be called by MyPawn to init the Net
*/
NeuralNet* NeuralNet::joyInit() 
{
	net = new NeuralNet();
	return net;
}


NeuralNet::~NeuralNet()
{
}


/*
topology: the vector<unsigned> to store the vector sizes in
Takes the first line of the textFile that is read in during TrainingData construction and 
reads the topology data from that line.  It expects textFile[0] to be a string of format
"topology: x y z" where x y and z are integers describing the size of each layer of the net.
there can be an arbitrary amount of layers, as long as the weights are set up for it.
*/
void TrainingData::getTopology(vector<unsigned> &topology)
{
	FString topologyLine = textFile[0];

	FString line;
	FString label;

	

	topologyLine.Split(":", &label, &line);
	if (label.Compare("topology") != 0) {
		abort();
	}
	UE_LOG(LogTemp, Warning, TEXT("NEURALNET: label: %s"), *label);
	
	TArray<FString> outArray;
	FString deliminator = FString(" ");
	line.ParseIntoArray(outArray, *deliminator, 1);

	for (int32 i = 0; i <= outArray.Num()-1; ++i)
	{
		FString valString = outArray[i];
		topology.push_back(FCString::Atoi(*valString));
	}
}

TrainingData::TrainingData(FString filePath)
{
	
	FFileHelper::LoadANSITextFileToStrings(*filePath, NULL, textFile);
}

double Neuron::transferFunction(double x)
{
	// tanh - output range [-1.0..1.0]

	return tanh(x);
}

void Neuron::feedForward(const Layer &prevLayer)
{
	double sum = 0.0;

	// Sum the previous layer's outputs (which are our inputs)
	// Include the bias node from the previous layer.

	for (unsigned n = 0; n < prevLayer.size(); ++n) {
		sum += prevLayer[n].getOutputVal() *
			prevLayer[n].m_outputWeights[m_myIndex].weight;
	}

	m_outputVal = Neuron::transferFunction(sum);
}

/*
prevLayer: the previous layer of the network
newWeights: a vector of doubles containing the new weights to add
set the weights for the inputs of this neuron.
*/
void Neuron::setWeights(Layer &prevLayer, vector<double> newWeights)
{
	for (unsigned neuron = 0; neuron < prevLayer.size(); ++neuron)
	{
		prevLayer[neuron].m_outputWeights[m_myIndex].weight = newWeights[neuron];
	}
}

Neuron::Neuron(unsigned numOutputs, unsigned myIndex)
{
	for (unsigned c = 0; c < numOutputs; ++c) {
		m_outputWeights.push_back(Connection());
		m_outputWeights.back().weight = randomWeight();
	}

	m_myIndex = myIndex;
}


// ****************** class Net ******************


void Net::getResults(vector<double> &resultVals) const
{
	resultVals.clear();
	for (unsigned n = 0; n < m_layers.back().size() - 1; ++n) {
		resultVals.push_back(m_layers.back()[n].getOutputVal());
	}
}


/*
inputVals: the inputs to feed into the neural net
attaches the input values to the inputs of the neural net
then runs the net and calculates the output
*/
void Net::feedForward(const vector<double> &inputVals)
{
	assert(inputVals.size() == m_layers[0].size() - 1);

	// Assign (latch) the input values into the input neurons
	for (unsigned i = 0; i < inputVals.size(); ++i) {
		m_layers[0][i].setOutputVal(inputVals[i]);
	}

	// forward propagate
	for (unsigned layerNum = 1; layerNum < m_layers.size(); ++layerNum) {
		Layer &prevLayer = m_layers[layerNum - 1];
		for (unsigned n = 0; n < m_layers[layerNum].size() - 1; ++n) {
			m_layers[layerNum][n].feedForward(prevLayer);
		}
	}
}

/*
networkWeights: A NetworkWeights that contains the weights to assign to this network
Method essentially recreates a network that the weights were saved from.
*/
void Net::setWeights(NetworkWeights networkWeights)
{
	for (unsigned layerNum = 1; layerNum < m_layers.size(); ++layerNum)
	{
		Layer &prevLayer = m_layers[layerNum - 1];
		for (unsigned neuron = 0; neuron < m_layers[layerNum].size() - 1; ++neuron)
		{
			vector<double> newWeights = networkWeights.networkWeights[layerNum - 1].layerWeights[neuron].neuronWeights;
			if (prevLayer.size() != newWeights.size())
			{
				UE_LOG(LogTemp, Error, TEXT("The sizes are not the same."));
			}
			m_layers[layerNum][neuron].setWeights(prevLayer, newWeights);
		}

	}
}

/*
returns NetworkWeights
creates a NetworkWeights from textFile that can then be inserted
into a network with setWeights()
*/
NetworkWeights Net::readSavedWeightsFromFile(TArray<FString> textFile)
{
	NetworkWeights readWeights;
	int currLayer = -1;
	int currNeuron = -1;
	for (int32 i = 1; i <= textFile.Num()-1; ++i)
	{
		FString line;
		FString label;

		textFile[i].Split(":", &label, &line);

		if (label.Compare("Layer") == 0)
		{
			LayerWeights newLayer;
			readWeights.networkWeights.push_back(newLayer);
			currLayer++;
			currNeuron = -1;
		}
		else if (label.Compare("Neuron") == 0)
		{
			NeuronWeights newNeuron;
			readWeights.networkWeights[currLayer].layerWeights.push_back(newNeuron);
			currNeuron++;
		}
		else if (label.Compare("Weight") == 0)
		{
			double newWeight;
			newWeight = FCString::Atof(*line);
			readWeights.networkWeights[currLayer].layerWeights[currNeuron].neuronWeights.push_back(newWeight);
		
		}
		else if(label.Compare("END") == 0)
		{
			//end of weights file
			break;
		}
	}
	return readWeights;
}

Net::Net(const vector<unsigned> &topology)
{
	unsigned numLayers = topology.size();
	for (unsigned layerNum = 0; layerNum < numLayers; ++layerNum) {
		m_layers.push_back(Layer());
		unsigned numOutputs = layerNum == topology.size() - 1 ? 0 : topology[layerNum + 1];

		// We have a new layer, now fill it with neurons, and
		// add a bias neuron in each layer.
		for (unsigned neuronNum = 0; neuronNum <= topology[layerNum]; ++neuronNum) {
			m_layers.back().push_back(Neuron(numOutputs, neuronNum));
			//cout << "Made a Neuron!" << endl;
		}

		// Force the bias node's output to 1.0 (it was the last neuron pushed in this layer):
		m_layers.back().back().setOutputVal(1.0);
	}
}



