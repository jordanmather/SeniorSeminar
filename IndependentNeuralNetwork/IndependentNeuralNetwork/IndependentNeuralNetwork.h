#include <vector>

using namespace std;



struct Connection
{
	double weight;
	double deltaWeight;
};

//the three structs below nest into each other and define how the 
//network weights are constructed when being read from/to a file
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


class Neuron;

typedef vector<Neuron> Layer;



// ****************** class Neuron ******************
class Neuron
{
public:
	Neuron(unsigned numOutputs, unsigned myIndex);
	double getOutputVal(void) const { return m_outputVal; }
	vector<double> getWeights(const Layer &prevLayer);
	void setOutputVal(double val) { m_outputVal = val; }
	void feedForward(const Layer &prevLayer);
	void calcOutputGradients(double targetVal);
	void calcHiddenGradients(const Layer &nextLayer);
	void updateInputWeights(Layer &prevLayer);
	void setWeights(Layer &prevLayer, vector<double> newWeights);

private:
	static double eta;   // [0.0..1.0] overall net training rate
	static double alpha; // [0.0..n] multiplier of last weight change (momentum)
	static double transferFunction(double x);
	static double transferFunctionDerivative(double x);
	static double randomWeight(void) { return rand() / double(RAND_MAX); }
	double sumDOW(const Layer &nextLayer) const;
	double m_outputVal;
	vector<Connection> m_outputWeights;
	unsigned m_myIndex;
	double m_gradient;
};

// ****************** class Net ******************
class Net
{
public:
	Net(const vector<unsigned> &topology);
	double getRecentAverageError(void) const { return m_recentAverageError; }
	NetworkWeights recordWeights();
	NetworkWeights readSavedWeightsFromFile();
	void feedForward(const vector<double> &inputVals);
	void backProp(const vector<double> &targetVals);
	void getResults(vector<double> &resultVals) const;
	void setWeights(NetworkWeights networkWeights);
	void saveWeightsToFile(NetworkWeights networkWeights);

private:
	vector<Layer> m_layers; // m_layers[layerNum][neuronNum]
	double m_error;
	double m_recentAverageError;
	static double m_recentAverageSmoothingFactor;
};