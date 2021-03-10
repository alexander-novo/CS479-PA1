// Part1-Bayes/main.h
#include <Eigen/Core>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>

#include "../Common/sample.h"

using Eigen::PartialPivLU;

// Struct for inputting arguments from command line
struct Arguments {
	DataSet set;
	unsigned seed = 1;
	std::ofstream plotFiles[CLASSES], misclassPlotFiles[CLASSES], boundaryParamsFile;
};

double discriminate(const observation& obs, const Vec<CLASSES>& mu, double sigmaSquared);
unsigned classifySample(const sample& samp, unsigned correctClass, sample& misclass, observation& min, observation& max,
                        const array<observation, CLASSES>& means, double sigmaSquared,
                        bool plotMisclassifications = false);
void printPlotFile(std::ofstream& plotFile, const sample& samp);
void printParamsFile(std::ofstream& boundaryParamsFile, const observation& min, const observation& max,
                     const array<observation, CLASSES>& means, double sigmaSquared);
bool verifyArguments(int argc, char** argv, Arguments& arg, int& err);
void printHelp();
