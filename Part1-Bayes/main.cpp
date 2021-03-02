// Part1-Bayes/main.cpp
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>

#include "../Common/sample.h"

// Struct for inputting arguments from command line
struct Arguments {
	DataSet set;
	unsigned seed = 1;
	std::ofstream plotFiles[CLASSES], boundaryParamsFile;
};

double discriminateCase1(observation obs, Vec<CLASSES> mu, double var, double prior);
bool verifyArguments(int argc, char** argv, Arguments& arg, int& err);
void printHelp();

int main(int argc, char** argv) {
	int err;
	Arguments arg;

	if (!verifyArguments(argc, argv, arg, err)) { return err; }

	std::array<sample, CLASSES> samples;
	std::array<observation, CLASSES> means = getMeans(arg.set);
	std::array<observation, CLASSES> vars  = getVars(arg.set);
	std::array<unsigned, CLASSES> sizes    = getSizes(arg.set);
	getSamples(arg.set, samples, arg.seed);

	double totalSize = std::accumulate(sizes.begin(), sizes.end(), 0);

	std::array<unsigned, CLASSES> misclassify = {};
	std::array<double, CLASSES> discriminant;

#pragma omp declare \
	reduction( \
		+ : \
		std::array<unsigned, CLASSES> : \
		std::transform(omp_in.begin(), \
			omp_in.end(), \
			omp_out.begin(), \
			omp_out.end(), \
			std::plus<unsigned>())) \
	initializer(omp_priv(omp_orig))

	for (unsigned i = 0; i < CLASSES; i++) {
#pragma omp parallel for reduction(+ : misclassify)
		for (unsigned j = 0; j < samples[i].size(); j++) {
			for (unsigned k = 0; k < CLASSES; k++) {
				discriminant[k] = discriminateCase1(samples[i][j], means[k],
				                                    vars[k].x, sizes[k] / totalSize);
			}

			if (discriminant[i] !=
			    *std::max_element(discriminant.begin(), discriminant.end())) {
				misclassify[i]++;
			}
		}

		
		if (arg.plotFiles[i]) {
			arg.plotFiles[i] << "#        x           y\n"
			                 << std::fixed << std::setprecision(7);

			for (unsigned j = 0; j < samples[i].size(); j++) {
				for (unsigned k = 0; k < DIM; k++) {
					arg.plotFiles[i] << std::setw(10) << samples[i][j][k] << "  ";
				}
				arg.plotFiles[i] << '\n';
			}
		}
	}

	return 0;
}

double discriminateCase1(observation obs, observation mu, double var, double prior) {
	return (2 * mu.dot(obs) - mu.dot(mu)) / (2 * var) + log(prior);
}

bool verifyArguments(int argc, char** argv, Arguments& arg, int& err) {
	if (argc < 2 ||
	    (argc < 2 && strcmp(argv[1], "-h") && strcmp(argv[1], "--help"))) {
		std::cout << "Missing operand.\n\n";
		err = 1;
		printHelp();
		return false;
	}

	if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		printHelp();
		return false;
	}

	// Required arguments
	if (!strcmp(argv[1], "A") || !strcmp(argv[1], "a")) {
		arg.set = DataSet::A;
	} else if (!strcmp(argv[1], "B") || !strcmp(argv[1], "b")) {
		arg.set = DataSet::B;
	} else {
		std::cout << "Data Set \"" << argv[1] << "\" not recognised.\n\n";
		printHelp();
		err = 1;
		return false;
	}

	using namespace std::string_literals;
	std::regex samplePlotSwitch("-ps([1-"s + std::to_string(CLASSES) + "])"s);
	std::cmatch match;

	// Optional Arguments
	for (unsigned i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "-s")) {
			if (i + 1 >= argc) {
				std::cout << "Missing seed value.\n\n";
				err = 1;
				printHelp();
				return false;
			}

			char* end;
			arg.seed = strtol(argv[i + 1], &end, 10);
			if (end == argv[i + 1]) {
				std::cout << "\"" << argv[i + 1]
				          << "\" could not be interpreted as an integer.\n";
				err = 2;
				return false;
			}

			i++;
		} else if (std::regex_match(argv[i], match, samplePlotSwitch)) {
			char* end;
			unsigned classNum = strtol(match[1].str().c_str(), &end, 10) - 1;

			if (i + 1 >= argc) {
				std::cout << "Missing sample " << classNum << " plot file.\n\n";
				err = 1;
				printHelp();
				return false;
			}

			arg.plotFiles[classNum].open(argv[i + 1]);
			if (!arg.plotFiles[classNum]) {
				std::cout << "Could not open file \"" << argv[i + 1] << "\".\n";
				err = 2;
				return false;
			}

			i++;
		} else if (!strcmp(argv[i], "-pd")) {
			if (i + 1 >= argc) {
				std::cout << "Missing decision boundary parameter file.\n\n";
				err = 1;
				printHelp();
				return false;
			}

			arg.boundaryParamsFile.open(argv[i + 1]);
			if (!arg.boundaryParamsFile) {
				std::cout << "Could not open file \"" << argv[i + 1] << "\".\n";
				err = 2;
				return false;
			}

			i++;
		} else {
			std::cout << "Unrecognised argument \"" << argv[i] << "\".\n";
			printHelp();
			err = 1;
			return false;
		}
	}
	return true;
}

void printHelp() {
	Arguments arg;
	std::cout
	    << "Usage: classify-bayes <data set> [options]                      (1)\n"
	    << "   or: classify-bayes -h                                        (2)\n\n"
	    << "(1) Run a Bayes classifier on a specific data set.\n"
	    << "    Data sets available are 'A' and 'B'.\n"
	    << "(2) Print this help menu.\n\n"
	    << "OPTIONS\n"
	    << "  -s <seed>    Set the seed used to generate samples.\n"
	    << "               Defaults to " << arg.seed << ".\n"
	    << "  -psN <file>  Print all observations from sample N to a file.\n"
	    << "               N can be 1 to " << CLASSES << ".\n"
	    << "  -pd  <file>  Print the parameters of the decision boundary.\n";
}