// Part1-Bayes/main.cpp
#include <Eigen/Core>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>

#include "../Common/sample.h"

// Struct for inputting arguments from command line
struct Arguments {
	DataSet set;
	unsigned seed             = 1;
	unsigned discriminantCase = 0;
	std::ofstream plotFiles[CLASSES], boundaryParamsFile;
};

double discriminateCase1(const observation& obs, const Vec<CLASSES>& mu,
                         const CovMatrix& varInverse, double logPrior);
double discriminateCase2(const observation& obs, const Vec<CLASSES>& mu,
                         const CovMatrix& varInverse, double logPrior);
double discriminateCase3(const observation& obs, const Vec<CLASSES>& mu,
                         const CovMatrix& varInverse, double logPrior);
bool verifyArguments(int argc, char** argv, Arguments& arg, int& err);
void printHelp();

double (*const discriminantFuncs[])(const observation&, const Vec<CLASSES>&, const CovMatrix&,
                                    double) = {discriminateCase1, discriminateCase2,
                                               discriminateCase3};

int main(int argc, char** argv) {
	int err;
	Arguments arg;

	if (!verifyArguments(argc, argv, arg, err)) { return err; }

	std::array<sample, CLASSES> samples;
	std::array<double, CLASSES> logPriors, varDets;
	std::array<CovMatrix, CLASSES> varInverses;
	std::array<observation, CLASSES> means = getMeans(arg.set);
	std::array<CovMatrix, CLASSES> vars    = getVars(arg.set);
	std::array<unsigned, CLASSES> sizes    = getSizes(arg.set);
	getSamples(arg.set, samples, arg.seed);

	double totalSize = std::accumulate(sizes.begin(), sizes.end(), 0);

	// Compute the log of the priors
	std::transform(sizes.cbegin(), sizes.cend(), logPriors.begin(),
	               [totalSize](unsigned size) { return log(size / totalSize); });

	unsigned overallMisclass = 0;
	double discriminant[CLASSES];

	if (arg.discriminantCase == 0) {
		// Default case is 3, since it covers all other cases as well
		arg.discriminantCase = 3;

		// If all the covariance matrices are equal, then we're in case 2
		if (std::equal(vars.cbegin() + 1, vars.cend(), vars.cbegin())) {
			arg.discriminantCase = 2;

			// If the first covariance matrix is a constant times the identity (diagonal
			// and all elements on the diagonal are the same number), then we're in case 1
			if (vars[0].isDiagonal() && (vars[0].diagonal().array() == vars[0](0, 0)).all()) {
				arg.discriminantCase = 1;
			}
		}

		std::cout << "Detected case " << arg.discriminantCase << "\n\n";
	} else {
		std::cout << "Overriden case " << arg.discriminantCase << "\n\n";
	}

	auto discriminate = discriminantFuncs[arg.discriminantCase - 1];

	switch (arg.discriminantCase) {
		case 1: {
			// Same inverse for all classes, and it's a diagonal matrix, so inverse is easy to find.
			// No need for determinant.
			CovMatrix inverse = vars[0].diagonal().asDiagonal().inverse();
			std::for_each(varInverses.begin(), varInverses.end(),
			              [&inverse](CovMatrix& inv) { inv = inverse; });
			break;
		}
		case 2: {
			// Same inverse for all classes, but this time it's a bit more difficult to find
			// inverse. Covariance matrices are symmetric positive definite, so we can still find
			// inverse quickly. No need for determinant.
			CovMatrix inverse = vars[0].llt().solve(CovMatrix::Identity());
			std::for_each(varInverses.begin(), varInverses.end(),
			              [&inverse](CovMatrix& inv) { inv = inverse; });
			break;
		}
		case 3:
			// Different inverse for each class. They're still symmetric positive-definite, but now
			// we also need determinant and LLT decomposition doesn't give us that. So we use LU
			// decomposition, which takes longer than LLT but gives us both determinant and
			// inverse.
			// std::transform(vars.cbegin(), vars.cend(), varDets.begin(), varInverses, [](const))
			break;
	}

	for (unsigned i = 0; i < CLASSES; i++) {
		unsigned misclass = 0;

#pragma omp parallel for reduction(+ : misclass)
		for (unsigned j = 0; j < samples[i].size(); j++) {
			double maxDiscriminant = discriminate(samples[i][j], means[0], vars[0], logPriors[0]);
			double discriminant;

			for (unsigned k = 1; k < CLASSES; k++) {
				discriminant = discriminate(samples[i][j], means[k], vars[k], logPriors[k]);

				if (k == i && discriminant < maxDiscriminant ||
				    k > i && discriminant > maxDiscriminant) {
					misclass++;
					break;
				}

				maxDiscriminant = std::max(maxDiscriminant, discriminant);
			}
		}

		std::cout << "Misclassification rate for class " << i + 1 << ":\n"
		          << misclass / (double) sizes[i] << "\n\n";

		overallMisclass += misclass;

		if (arg.plotFiles[i]) {
			arg.plotFiles[i] << "#        x           y\n" << std::fixed << std::setprecision(7);

			for (unsigned j = 0; j < samples[i].size(); j++) {
				for (unsigned k = 0; k < DIM; k++) {
					arg.plotFiles[i] << std::setw(10) << samples[i][j][k] << "  ";
				}
				arg.plotFiles[i] << '\n';
			}
		}
	}

	std::cout << "Overall misclassification rate:\n" << overallMisclass / totalSize << '\n';

	return 0;
}

double discriminateCase1(const observation& obs, const Vec<CLASSES>& mu,
                         const CovMatrix& varInverse, double logPrior) {
	// Note that varInverse is what is passed, and the inverse of a diagonal matrix is also a
	// diagonal matrix with the diagonal elements being the reciprocal of the original elements. So
	// instead of dividing by sigma^2, we multiply by 1/sigma^2.
	return (varInverse(0, 0) * mu).dot(obs) - varInverse(0, 0) * mu.dot(mu) / 2.0 + logPrior;
}

double discriminateCase2(const observation& obs, const Vec<CLASSES>& mu,
                         const CovMatrix& varInverse, double logPrior) {
	return (-1 / 2.0) * (obs - mu).transpose() * varInverse * (obs - mu) + logPrior;
}

double discriminateCase3(const observation& obs, const Vec<CLASSES>& mu,
                         const CovMatrix& varInverse, double logPrior) {
	// return obs.transpose() * (-1 / 2.0 * varInverse) * obs + (varInverse * mu).transpose() * obs
	// -
	//        1 / 2.0 * mu.transpose() * varInverse * mu - 1 / 2.0 * ln()
	return 0;
}

bool verifyArguments(int argc, char** argv, Arguments& arg, int& err) {
	if (argc < 2 || (argc < 2 && strcmp(argv[1], "-h") && strcmp(argv[1], "--help"))) {
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
				std::cout << "\"" << argv[i + 1] << "\" could not be interpreted as an integer.\n";
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
		} else if (!strcmp(argv[i], "-c")) {
			if (i + 1 >= argc) {
				std::cout << "Missing case number.\n\n";
				err = 1;
				printHelp();
				return false;
			}

			char* end;
			arg.discriminantCase = strtol(argv[i + 1], &end, 10);
			if (end == argv[i + 1]) {
				std::cout << "\"" << argv[i + 1] << "\" could not be interpreted as an integer.\n";
				err = 2;
				return false;
			} else if (arg.discriminantCase < 1 || arg.discriminantCase > 3) {
				std::cout << "Discriminant case number must be between 1 and 3 (inclusive).\n";
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
	std::cout << "Usage: classify-bayes <data set> [options]                            (1)\n"
	          << "   or: classify-bayes -h                                              (2)\n\n"
	          << "(1) Run a Bayes classifier on a specific data set.\n"
	          << "    Data sets available are 'A' and 'B'.\n"
	          << "(2) Print this help menu.\n\n"
	          << "OPTIONS\n"
	          << "  -s <seed>    Set the seed used to generate samples.\n"
	          << "               Defaults to " << arg.seed << ".\n"
	          << "  -psN <file>  Print all observations from sample N to a file.\n"
	          << "               N can be 1 to " << CLASSES << ".\n"
	          << "  -pd  <file>  Print the parameters of the decision boundary.\n"
	          << "  -c  <case>   Override which discriminant case is to be used.\n"
	          << "               <case> can be 1-3. Higher numbers are more computationally\n"
	          << "               expensive, but are correct more of the time.\n"
	          << "               By default, the case will be chosen automatically.\n";
}