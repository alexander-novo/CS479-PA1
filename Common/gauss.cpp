#include "gauss.h"

void genGaussianSample(double mu, double sigma, std::vector<double>& sample) {
	std::mt19937_64 engine;
	std::normal_distribution<double> dist(mu, sigma);

	for (double& obs : sample) { obs = dist(engine); }
}