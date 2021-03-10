// Part1-Bayes/main.cpp
#include "main.h"

int main(int argc, char** argv) {
	int err;
	Arguments arg;

	if (!verifyArguments(argc, argv, arg, err)) { return err; }

	sample misclassifications[CLASSES];
	array<sample, CLASSES> samples;
	array<observation, CLASSES> means = getMeans(arg.set);
	array<CovMatrix, CLASSES> vars    = getVars(arg.set);
	array<unsigned, CLASSES> sizes    = getSizes(arg.set);
	getSamples(arg.set, samples, arg.seed);

	observation min = samples[0].front(), max = samples[0].front();

	double totalSize = std::accumulate(sizes.begin(), sizes.end(), 0);

	std::cout << "Classifying data set \"" << dataSetName(arg.set) << "\" - " << CLASSES << " classes.\n";

	double sigmaSquared = vars[0](0, 0);

	unsigned overallMisclass = 0;

	for (unsigned i = 0; i < CLASSES; i++) {
		sample& misclass = misclassifications[i];

		unsigned misclassCount = classifySample(samples[i], i, misclassifications[i], min, max, means, sigmaSquared,
		                                        !arg.misclassPlotFiles[i].fail());

		std::cout << "Misclassification rate for class " << i + 1 << ":\n"
		          << misclassCount / (double) sizes[i] << "\n\n";

		overallMisclass += misclassCount;

		if (arg.plotFiles[i]) { printPlotFile(arg.plotFiles[i], samples[i]); }

		if (arg.misclassPlotFiles[i]) { printPlotFile(arg.misclassPlotFiles[i], misclassifications[i]); }
	}

	std::cout << "Overall misclassification rate:\n" << overallMisclass / totalSize << "\n\n";

	if (arg.boundaryParamsFile) { printParamsFile(arg.boundaryParamsFile, min, max, means, sigmaSquared); }

	return 0;
}

double discriminate(const observation& obs, const Vec<CLASSES>& mu, double sigmaSquared) {
	// Euclidean Distance
	return -(obs - mu).norm();
}

unsigned classifySample(const sample& samp, unsigned correctClass, sample& misclass, observation& min, observation& max,
                        const array<observation, CLASSES>& means, double sigmaSquared, bool plotMisclassifications) {
	unsigned misclassCount = 0;

#pragma omp declare reduction(min:observation : omp_out = omp_out.cwiseMin(omp_in)) initializer(omp_priv(omp_orig))
#pragma omp declare reduction(max:observation : omp_out = omp_out.cwiseMax(omp_in)) initializer(omp_priv(omp_orig))
#pragma omp declare reduction(append:sample                                                  \
                              : omp_out.insert(omp_out.end(), omp_in.begin(), omp_in.end())) \
    initializer(omp_priv(omp_orig))
#pragma omp parallel for reduction(+ : misclassCount) reduction(min : min) reduction(max : max) reduction(append : misclass)
	for (unsigned j = 0; j < samp.size(); j++) {
		double maxDiscriminant = discriminate(samp[j], means[0], sigmaSquared);
		double discriminant;

		for (unsigned k = 1; k < CLASSES; k++) {
			discriminant = discriminate(samp[j], means[k], sigmaSquared);

			if (k == correctClass && discriminant < maxDiscriminant ||
			    k > correctClass && discriminant > maxDiscriminant) {
				misclassCount++;

				if (plotMisclassifications) { misclass.push_back(samp[j]); }

				break;
			}

			maxDiscriminant = std::max(maxDiscriminant, discriminant);
		}

		min = min.cwiseMin(samp[j]);
		max = max.cwiseMax(samp[j]);
	}

	return misclassCount;
}

void printPlotFile(std::ofstream& plotFile, const sample& samp) {
	plotFile << "#        x           y\n" << std::fixed << std::setprecision(7);

	for (unsigned j = 0; j < samp.size(); j++) {
		for (unsigned k = 0; k < DIM; k++) { plotFile << std::setw(10) << samp[j][k] << "  "; }
		plotFile << '\n';
	}
}

void printParamsFile(std::ofstream& boundaryParamsFile, const observation& min, const observation& max,
                     const array<observation, CLASSES>& means, double sigmaSquared) {
	array<double, (DIM * (DIM + 1)) / 2 + DIM + 1> boundaryCoeffs;
	// Corresponds to the difference in the vectors "w_i" in the book
	observation diffw = (means[0] - means[1]) / sigmaSquared;

	// Terms of degree 2 first, in alphabetical order
	// e.g. x^2 + xy + y^2,
	// or   x^2 + xy + xz + y^2 + yz + z^2
	for (unsigned i = 0; i < DIM; i++) {
		// We never have any quadratic terms in this case
		for (unsigned j = 1; j < DIM - i; j++) { boundaryCoeffs[i * DIM + j] = 0; }
	}

	// Then terms of degree 1, once again in alphabetical order
	for (unsigned i = 0; i < DIM; i++) { boundaryCoeffs[(DIM * (DIM + 1)) / 2 + i] = diffw[i]; }

	// Then finally the constant term
	boundaryCoeffs.back() =
	    (-1 / 2.0 * means[0].dot(means[0] / sigmaSquared)) - (-1 / 2.0 * means[1].dot(means[1] / sigmaSquared));

	// Print parameter headers. All coefficients are single letters in alphabetical order, starting with 'a'
	for (unsigned i = 0; i < boundaryCoeffs.size(); i++) {
		boundaryParamsFile << std::setw(10) << (char) ('a' + i) << "  ";
	}
	// Then misc. parameter headers
	boundaryParamsFile << std::setw(10) << "xmin"
	                   << "  " << std::setw(10) << "xmax"
	                   << "  " << std::setw(10) << "ymin"
	                   << "  " << std::setw(10) << "ymax" << '\n'
	                   << std::fixed << std::setprecision(7);

	// Print parameters, starting with coefficients calculated above
	for (double coeff : boundaryCoeffs) { boundaryParamsFile << std::setw(10) << coeff << "  "; }
	// Then misc. parameters
	boundaryParamsFile << std::setw(10) << min[0] << "  " << std::setw(10) << max[0] << "  " << std::setw(10) << min[1]
	                   << "  " << std::setw(10) << max[1];
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
	std::regex misclassPlotSwitch("-pm([1-"s + std::to_string(CLASSES) + "])"s);
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
		} else if (std::regex_match(argv[i], match, misclassPlotSwitch)) {
			char* end;
			unsigned classNum = strtol(match[1].str().c_str(), &end, 10) - 1;

			if (i + 1 >= argc) {
				std::cout << "Missing misclassifications of sample " << classNum << " plot file.\n\n";
				err = 1;
				printHelp();
				return false;
			}

			arg.misclassPlotFiles[classNum].open(argv[i + 1]);
			if (!arg.misclassPlotFiles[classNum]) {
				std::cout << "Could not open file \"" << argv[i + 1] << "\".\n";
				err = 2;
				return false;
			}

			i++;
		} else if (!strcmp(argv[i], "-pdb")) {
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
	std::cout << "Usage: classify-euclid <data set> [options]                            (1)\n"
	          << "   or: classify-euclid -h                                              (2)\n\n"
	          << "(1) Run a Bayes classifier on a specific data set.\n"
	          << "    Data sets available are 'A' and 'B'.\n"
	          << "(2) Print this help menu.\n\n"
	          << "OPTIONS\n"
	          << "  -s   <seed>  Set the seed used to generate samples.\n"
	          << "               Defaults to " << arg.seed << ".\n"
	          << "  -psN <file>  Print all observations from sample N to a file.\n"
	          << "               N can be 1 to " << CLASSES << ".\n"
	          << "  -pmN <file>  Print all misclassified observations from sample N to a file.\n"
	          << "               N can be 1 to " << CLASSES << ".\n"
	          << "  -pdb <file>  Print the parameters of the decision boundary.\n";
}