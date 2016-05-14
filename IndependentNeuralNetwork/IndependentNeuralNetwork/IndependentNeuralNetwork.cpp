// neural-net-tutorial.cpp
// David Miller, http://millermattson.com/dave
// See the associated video for instructions: http://vimeo.com/19569529
// Altered by Jordan Mather to save/load weights

#include "stdafx.h"
#include "IndependentNeuralNetwork.h"
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace std;

//this should be in the .h file, but it breaks the program to move it
//and I ran out of time to figure out why before project submission.
class TrainingData
{
public:
	TrainingData(const string filename);
	bool isEof(void) { return m_trainingDataFile.eof(); }
	void getTopology(vector<unsigned> &topology);

	// Returns the number of input values read from the file:
	unsigned getNextInputs(vector<double> &inputVals);
	unsigned getTargetOutputs(vector<double> &targetOutputVals);

public:
	ifstream m_trainingDataFile;  //save the stream of the training file we're using
};



/*
topology: an unsigned vector that will hold the size and number of network layers
This method reads a line from m_trainingDataFile, checks that it is the topology line,
and enters the values for the network in topology
*/

void TrainingData::getTopology(vector<unsigned> &topology)
{
	string line;
	string label;

	getline(m_trainingDataFile, line);
	stringstream ss(line);
	ss >> label;
	if (this->isEof() || label.compare("topology:") != 0) {
		abort();
	}

	while (!ss.eof()) {
		unsigned n;
		ss >> n;
		topology.push_back(n);
	}

	return;
}

TrainingData::TrainingData(const string filename)
{
	m_trainingDataFile.open(filename.c_str());
}

/*
inputVals: a vector of doubles that will hold the input values to run through the network
This method reads a line from m_trainingDataFile, checks that it is an input line,
and enters the values for the inputs in inputVals
*/
unsigned TrainingData::getNextInputs(vector<double> &inputVals)
{
	inputVals.clear();

	string line;
	getline(m_trainingDataFile, line);
	stringstream ss(line);

	string label;
	ss >> label;
	if (label.compare("in:") == 0) {
		double oneValue;
		while (ss >> oneValue) {
			inputVals.push_back(oneValue);
		}
	}

	return inputVals.size();
}

/*
targetOutputVals: a vector of doubles that will hold the output values to compare to the network result
This method reads a line from m_trainingDataFile, checks that it is an output line,
and enters the values for the outputs in targetOutputVals
*/
unsigned TrainingData::getTargetOutputs(vector<double> &targetOutputVals)
{
	targetOutputVals.clear();

	string line;
	getline(m_trainingDataFile, line);
	stringstream ss(line);

	string label;
	ss >> label;
	if (label.compare("out:") == 0) {
		double oneValue;
		while (ss >> oneValue) {
			targetOutputVals.push_back(oneValue);
		}
	}

	return targetOutputVals.size();
}


double Neuron::eta = 0.4;    // overall net learning rate, [0.0..1.0]
double Neuron::alpha = 0.5;   // momentum, multiplier of last deltaWeight, [0.0..1.0]

/*
prevLayer: the previous layer of the network
Given the previous layer, run through each neuron and adjust the weights
*/
void Neuron::updateInputWeights(Layer &prevLayer)
{
	// The weights to be updated are in the Connection container
	// in the neurons in the preceding layer

	for (unsigned n = 0; n < prevLayer.size(); ++n) {
		Neuron &neuron = prevLayer[n];
		double oldDeltaWeight = neuron.m_outputWeights[m_myIndex].deltaWeight;

		double newDeltaWeight =
			// Individual input, magnified by the gradient and train rate:
			eta
			* neuron.getOutputVal()
			* m_gradient
			// Also add momentum = a fraction of the previous delta weight;
			+ alpha
			* oldDeltaWeight;

		neuron.m_outputWeights[m_myIndex].deltaWeight = newDeltaWeight;
		neuron.m_outputWeights[m_myIndex].weight += newDeltaWeight;
	}
}

double Neuron::sumDOW(const Layer &nextLayer) const
{
	double sum = 0.0;

	// Sum our contributions of the errors at the nodes we feed.

	for (unsigned n = 0; n < nextLayer.size() - 1; ++n) {
		sum += m_outputWeights[n].weight * nextLayer[n].m_gradient;
	}

	return sum;
}

void Neuron::calcHiddenGradients(const Layer &nextLayer)
{
	double dow = sumDOW(nextLayer);
	m_gradient = dow * Neuron::transferFunctionDerivative(m_outputVal);
}

void Neuron::calcOutputGradients(double targetVal)
{
	double delta = targetVal - m_outputVal;
	m_gradient = delta * Neuron::transferFunctionDerivative(m_outputVal);
}

double Neuron::transferFunction(double x)
{
	// tanh - output range [-1.0..1.0]

	return tanh(x);
}

double Neuron::transferFunctionDerivative(double x)
{
	// tanh derivative
	return 1.0 - x * x;
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
given the previous layer, get the weights for the inputs of this neuron.
*/
vector<double> Neuron::getWeights(const Layer &prevLayer)
{
	vector<double> returnedWeights;
	for (unsigned neuron = 0; neuron < prevLayer.size(); ++neuron)
	{
		returnedWeights.push_back(prevLayer[neuron].m_outputWeights[m_myIndex].weight);
	}
	return returnedWeights;
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



double Net::m_recentAverageSmoothingFactor = 100.0; // Number of training samples to average over

/*
resultVals: a vector of doubles to hold the outputs of the final layer
gets the final outputs of the network and stores them in resultVals
*/
void Net::getResults(vector<double> &resultVals) const
{
	resultVals.clear();
	for (unsigned n = 0; n < m_layers.back().size() - 1; ++n) {
		resultVals.push_back(m_layers.back()[n].getOutputVal());
	}
}

/*
targetVals: the expected output of the network
This method takes the target value of the test input and calculates the error of the network
it then adjusts the weights accordingly.
*/
void Net::backProp(const vector<double> &targetVals)
{
	// Calculate overall net error (RMS of output neuron errors)

	Layer &outputLayer = m_layers.back();
	m_error = 0.0;

	for (unsigned n = 0; n < outputLayer.size() - 1; ++n) {
		double delta = targetVals[n] - outputLayer[n].getOutputVal();
		m_error += delta * delta;
	}
	m_error /= outputLayer.size() - 1; // get average error squared
	m_error = sqrt(m_error); // RMS

							 // Implement a recent average measurement

	m_recentAverageError =
		(m_recentAverageError * m_recentAverageSmoothingFactor + m_error)
		/ (m_recentAverageSmoothingFactor + 1.0);

	// Calculate output layer gradients

	for (unsigned n = 0; n < outputLayer.size() - 1; ++n) {
		outputLayer[n].calcOutputGradients(targetVals[n]);
	}

	// Calculate hidden layer gradients

	for (unsigned layerNum = m_layers.size() - 2; layerNum > 0; --layerNum) {
		Layer &hiddenLayer = m_layers[layerNum];
		Layer &nextLayer = m_layers[layerNum + 1];

		for (unsigned n = 0; n < hiddenLayer.size(); ++n) {
			hiddenLayer[n].calcHiddenGradients(nextLayer);
		}
	}

	// For all layers from outputs to first hidden layer,
	// update connection weights

	for (unsigned layerNum = m_layers.size() - 1; layerNum > 0; --layerNum) {
		Layer &layer = m_layers[layerNum];
		Layer &prevLayer = m_layers[layerNum - 1];

		for (unsigned n = 0; n < layer.size() - 1; ++n) {
			layer[n].updateInputWeights(prevLayer);
		}
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
returns a NetworkWeights with all the weights of the network
*/
NetworkWeights Net::recordWeights()
{
	NetworkWeights networkWeights;
	for (unsigned layerNum = 1; layerNum < m_layers.size(); ++layerNum)
	{
		Layer &prevLayer = m_layers[layerNum - 1];
		NeuronWeights neuronWeights;
		LayerWeights layerWeights;
		for (unsigned neuron = 0; neuron < m_layers[layerNum].size() - 1; ++neuron)
		{
			neuronWeights.neuronWeights = m_layers[layerNum][neuron].getWeights(prevLayer);
			layerWeights.layerWeights.push_back(neuronWeights);
		}
		networkWeights.networkWeights.push_back(layerWeights);
	}
	return networkWeights;
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
			vector<double> newWeights = networkWeights.networkWeights[layerNum-1].layerWeights[neuron].neuronWeights;
			if (prevLayer.size() != newWeights.size())
			{
				cout << "The sizes are not the same" << endl;
			}
			m_layers[layerNum][neuron].setWeights(prevLayer, newWeights);
		}
		
	}
}

/*
networkWeights: the weights to save to file
This method takes a NetworkWeights and saves them to a local file called "savedWeights.txt"
this file can then be read in by readSavedWeightsFromFile() back to a NetworkWeights
*/
void Net::saveWeightsToFile(NetworkWeights networkWeights)
{
	ofstream newFile;
	newFile.open("savedWeights.txt");
	unsigned numLayers = networkWeights.networkWeights.size(); //amount of layers
	for (unsigned i = 0; i <= numLayers - 1; i++)
	{
		unsigned numNeurons = networkWeights.networkWeights[i].layerWeights.size(); //amount of neurons in this layer
		newFile << "Layer: " << i << "\n";
		for (unsigned k = 0; k <= numNeurons - 1; k++)
		{
			unsigned numWeights = networkWeights.networkWeights[i].layerWeights[k].neuronWeights.size(); //amount of weights in the neuron
			newFile << "Neuron: " << k << "\n";
			for (unsigned j = 0; j <= numWeights - 1; j++)
			{
				double weight = networkWeights.networkWeights[i].layerWeights[k].neuronWeights[j];
				newFile << "Weight: " << weight << "\n";
			}
		}
	}
	newFile.close();
}

/*
returns NetworkWeights
reads a local file called "Weights.txt" and creates a NetworkWeights that can then be inserted
into a network with setWeights()
*/
NetworkWeights Net::readSavedWeightsFromFile()
{
	TrainingData weightFile("Weights.txt");

	vector<unsigned> topology;
	weightFile.getTopology(topology);
	NetworkWeights readWeights;
	int currLayer = -1;
	int currNeuron = -1;
	while (!weightFile.isEof())
	{
		string line;
		getline(weightFile.m_trainingDataFile, line);
		stringstream ss(line);

		string label;
		ss >> label;

		if (label.compare("Layer:")==0)
		{
			int oneValue;
			while (ss >> oneValue) {
				/*spin to end of line I guess?*/
			}
			LayerWeights newLayer;
			readWeights.networkWeights.push_back(newLayer);
			currLayer++;
			currNeuron = -1;
		}
		else if (label.compare("Neuron:")==0)
		{
			int oneValue;
			while (ss >> oneValue) { 
				/*spin to end of line I guess?*/
			}
			NeuronWeights newNeuron;
			readWeights.networkWeights[currLayer].layerWeights.push_back(newNeuron);
			currNeuron++;
		}
		else if (label.compare("Weight:")==0)
		{
			double newWeight;
			while (ss >> newWeight)
			{
				readWeights.networkWeights[currLayer].layerWeights[currNeuron].neuronWeights.push_back(newWeight);
			}
		}
		else
		{
			cout << "Found nothing: " << label << "\n";
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


void showVectorVals(string label, vector<double> &v)
{
	cout << label << " ";
	for (unsigned i = 0; i < v.size(); ++i) {
		cout << v[i] << " ";
	}

	cout << endl;
}


int main()
{
	TrainingData trainData("TestData.txt");

	// e.g., { 3, 2, 1 }
	vector<unsigned> topology;
	trainData.getTopology(topology);

	Net myNet(topology);

	vector<double> inputVals, targetVals, resultVals;

	//The below comments would be used if the user wanted to 
	//start the network with a certain set of weights read in from Weights.txt
	/*NetworkWeights networkWeights = myNet.readSavedWeightsFromFile();
	myNet.setWeights(networkWeights);*/

	int trainingPass = 0; //count the amount of training data we have reviewed

	while (!trainData.isEof()) {
		++trainingPass;
		cout << endl << "Pass " << trainingPass << endl;

		// Get new input data and feed it forward:
		if (trainData.getNextInputs(inputVals) != topology[0]) {
			cout << "bad topology!";
			break;
		}
		//showVectorVals("Inputs:", inputVals);  //uncomment if you want to see the inputvalues.
		myNet.feedForward(inputVals);

		// Collect the net's actual output results:
		myNet.getResults(resultVals);
		showVectorVals("Outputs:", resultVals);

		// Train the net what the outputs should have been:
		trainData.getTargetOutputs(targetVals);
		showVectorVals("Targets:", targetVals);
		assert(targetVals.size() == topology.back());

		myNet.backProp(targetVals);

		// Report how well the training is working, average over recent samples:
		cout << "Net recent average error: "
			<< myNet.getRecentAverageError() << endl;
	}

	//uncomment if you want to save the weights to file for review or later use
	/*NetworkWeights networkWeights = myNet.recordWeights();
	cout << endl << "Weights recorded!" << endl;
	myNet.saveWeightsToFile(networkWeights);*/

	cout << endl << "Done" << endl;
}
