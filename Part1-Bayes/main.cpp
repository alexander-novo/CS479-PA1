// Part1-Bayes/main.cpp
#include <Eigen/Core>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>

#include "../Common/sample.h"

#define PDF_SAMPLES 100

// Struct for inputting arguments from command line
struct Arguments {
	DataSet set;
	unsigned seed             = 1;
	unsigned discriminantCase = 0;
	std::ofstream plotFiles[CLASSES], misclassPlotFiles[CLASSES], boundaryParamsFile, pdfPlotFile;
};

double discriminateCase1(const observation& obs, const Vec<CLASSES>& mu, const CovMatrix& varInverse, double logVarDet,
                         double logPrior);
double discriminateCase2(const observation& obs, const Vec<CLASSES>& mu, const CovMatrix& varInverse, double logVarDet,
                         double logPrior);
double discriminateCase3(const observation& obs, const Vec<CLASSES>& mu, const CovMatrix& varInverse, double logVarDet,
                         double logPrior);
bool verifyArguments(int argc, char** argv, Arguments& arg, int& err);
void printHelp();

double (*const discriminantFuncs[])(const observation&, const Vec<CLASSES>&, const CovMatrix&, double, double) = {
    discriminateCase1, discriminateCase2, discriminateCase3};

int main(int argc, char** argv) {
	int err;
	Arguments arg;

	if (!verifyArguments(argc, argv, arg, err)) { return err; }

	sample misclassifications[CLASSES];
	std::array<sample, CLASSES> samples;
	std::array<double, CLASSES> logPriors, varDets = {}, logVarDets = {};
	std::array<double, (DIM * (DIM + 1)) / 2 + DIM + 1> boundaryCoeffs;
	std::array<CovMatrix, CLASSES> varInverses;
	std::array<observation, CLASSES> means = getMeans(arg.set);
	std::array<CovMatrix, CLASSES> vars    = getVars(arg.set);
	std::array<unsigned, CLASSES> sizes    = getSizes(arg.set);
	getSamples(arg.set, samples, arg.seed);

	observation min = samples[0].front(), max = samples[0].front();

	double totalSize = std::accumulate(sizes.begin(), sizes.end(), 0);

	// Compute the log of the priors
	std::transform(sizes.cbegin(), sizes.cend(), logPriors.begin(),
	               [totalSize](unsigned size) { return log(size / totalSize); });

	// Compute which case we're in from the book
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
			// No need for determinant to discriminate, but we need it if we're plotting the pdf.
			CovMatrix inverse  = vars[0].diagonal().asDiagonal().inverse();
			double determinant = 1;

			// The determinant of a diagonal matrix is the product of the entries on the main diagonal.
			// But since this is a scalar matrix, the entries are all the same so it is the power of that scalar.
			if (arg.pdfPlotFile) { determinant = pow(vars[0](0, 0), DIM); }

			std::for_each(varInverses.begin(), varInverses.end(), [&inverse](CovMatrix& inv) { inv = inverse; });
			std::for_each(varDets.begin(), varDets.end(), [&determinant](double& det) { det = determinant; });
			break;
		}
		case 2: {
			// Same inverse for all classes, but this time it's a bit more difficult to find
			// inverse. Covariance matrices are symmetric positive definite, so we can still find
			// inverse quickly as long as we don't need determinant (which we don't unless we're plotting the pdf).
			CovMatrix inverse;
			double determinant;
			if (!arg.pdfPlotFile) {
				inverse     = vars[0].llt().solve(CovMatrix::Identity());
				determinant = 1;
			} else {
				// See below why we don't use LLT decomposition in this case
				Eigen::PartialPivLU<CovMatrix> varLU = vars[0].lu();
				inverse                              = varLU.inverse();
				determinant                          = varLU.determinant();
			}

			std::for_each(varInverses.begin(), varInverses.end(), [&inverse](CovMatrix& inv) { inv = inverse; });
			std::for_each(varDets.begin(), varDets.end(), [&determinant](double& det) { det = determinant; });
			break;
		}
		case 3:
			// Different inverse for each class. They're still symmetric positive-definite, but now
			// we also need determinant and LLT decomposition doesn't give us that. So we use LU
			// decomposition, which takes longer than LLT but gives us both determinant and
			// inverse.
			std::transform(vars.cbegin(), vars.cend(), varInverses.begin(), varDets.begin(),
			               [](const CovMatrix& var, CovMatrix& inverse) {
				               Eigen::PartialPivLU<CovMatrix> varLU = var.lu();

				               inverse = varLU.inverse();

				               return varLU.determinant();
			               });
			break;
	}

	// Calculate the logs of the determinants
	std::transform(varDets.cbegin(), varDets.cend(), logVarDets.begin(), [](const double& det) { return log(det); });

	unsigned overallMisclass = 0;

	for (unsigned i = 0; i < CLASSES; i++) {
		unsigned misclassCount = 0;
		sample& misclass       = misclassifications[i];

#pragma omp declare reduction(min:observation : omp_out = omp_out.cwiseMin(omp_in)) initializer(omp_priv(omp_orig))
#pragma omp declare reduction(max:observation : omp_out = omp_out.cwiseMax(omp_in)) initializer(omp_priv(omp_orig))
#pragma omp declare reduction(append:sample                                                  \
                              : omp_out.insert(omp_out.end(), omp_in.begin(), omp_in.end())) \
    initializer(omp_priv(omp_orig))
#pragma omp parallel for reduction(+ : misclassCount) reduction(min : min) reduction(max : max) reduction(append : misclass)
		for (unsigned j = 0; j < samples[i].size(); j++) {
			double maxDiscriminant = discriminate(samples[i][j], means[0], varInverses[0], logVarDets[0], logPriors[0]);
			double discriminant;

			for (unsigned k = 1; k < CLASSES; k++) {
				discriminant = discriminate(samples[i][j], means[k], varInverses[k], logVarDets[k], logPriors[k]);

				if (k == i && discriminant < maxDiscriminant || k > i && discriminant > maxDiscriminant) {
					misclassCount++;

					if (arg.misclassPlotFiles[i]) { misclass.push_back(samples[i][j]); }

					break;
				}

				maxDiscriminant = std::max(maxDiscriminant, discriminant);
			}

			min = min.cwiseMin(samples[i][j]);
			max = max.cwiseMax(samples[i][j]);
		}

		std::cout << "Misclassification rate for class " << i + 1 << ":\n"
		          << misclassCount / (double) sizes[i] << "\n\n";

		overallMisclass += misclassCount;

		if (arg.plotFiles[i]) {
			arg.plotFiles[i] << "#        x           y\n" << std::fixed << std::setprecision(7);

			for (unsigned j = 0; j < samples[i].size(); j++) {
				for (unsigned k = 0; k < DIM; k++) { arg.plotFiles[i] << std::setw(10) << samples[i][j][k] << "  "; }
				arg.plotFiles[i] << '\n';
			}
		}

		if (arg.misclassPlotFiles[i]) {
			arg.misclassPlotFiles[i] << "#        x           y\n" << std::fixed << std::setprecision(7);

			for (unsigned j = 0; j < misclassifications[i].size(); j++) {
				for (unsigned k = 0; k < DIM; k++) {
					arg.misclassPlotFiles[i] << std::setw(10) << misclassifications[i][j][k] << "  ";
				}
				arg.misclassPlotFiles[i] << '\n';
			}
		}
	}

	std::cout << "Overall misclassification rate:\n" << overallMisclass / totalSize << '\n';

	double pdfMax = 0;
	if (arg.pdfPlotFile) {
		std::array<double, CLASSES> varDetRoots, priors;
		std::transform(varDets.cbegin(), varDets.cend(), varDetRoots.begin(), [](double det) { return sqrt(det); });
		std::transform(logPriors.cbegin(), logPriors.cend(), priors.begin(),
		               [](double logPrior) { return exp(logPrior); });

		arg.pdfPlotFile << "#        x           y           z       class\n" << std::fixed << std::setprecision(7);
#pragma omp parallel for ordered collapse(2) reduction(max : pdfMax)
		for (unsigned x = 0; x < PDF_SAMPLES; x++) {
			for (unsigned y = 0; y < PDF_SAMPLES; y++) {
				double xmod = min.x() + x * (max.x() - min.x()) / PDF_SAMPLES;
				double ymod = min.y() + y * (max.y() - min.y()) / PDF_SAMPLES;

				observation vecX = {xmod, ymod};

				double densities[] = {gaussianDensity<DIM>(vecX, means[0], varInverses[0], varDetRoots[0]) * priors[0],
				                      gaussianDensity<DIM>(vecX, means[1], varInverses[1], varDetRoots[1]) * priors[1]};

				pdfMax = std::max(pdfMax, densities[0] + densities[1]);

#pragma omp ordered
				{
					arg.pdfPlotFile << std::setw(10) << xmod << "  " << std::setw(10) << ymod << "  " << std::setw(10)
					                << densities[0] + densities[1] << "  " << std::setw(10)
					                << ((densities[0] > densities[1]) ? 1 : 2);

					if (x != PDF_SAMPLES - 1 || y < PDF_SAMPLES - 1) { arg.pdfPlotFile << '\n'; }

					// gnuplot requires an extra blank line between rows on surface plots
					if (y == PDF_SAMPLES - 1 && x != PDF_SAMPLES - 1) { arg.pdfPlotFile << '\n'; }
				}
			}
		}
	}

	if (arg.boundaryParamsFile) {
		CovMatrix diffW   = -1 / 2.0 * (varInverses[0] - varInverses[1]);
		observation diffw = varInverses[0] * means[0] - varInverses[1] * means[1];

		// Terms of degree 2 first, in alphabetical order
		// e.g. x^2 + xy + y^2,
		// or   x^2 + xy + xz + y^2 + yz + z^2
		for (unsigned i = 0; i < DIM; i++) {
			// Diagonals are the coefficients on squared terms
			boundaryCoeffs[i * DIM] = diffW(i, i);
			// Off-Diagonals are the coefficients on non-squared terms, and since the matrix is symmetric and xy = yx,
			// they get doubled.
			for (unsigned j = 1; j < DIM - i; j++) { boundaryCoeffs[i * DIM + j] = 2 * diffW(i, j + i); }
		}

		// Then terms of degree 1, once again in alphabetical order
		for (unsigned i = 0; i < DIM; i++) { boundaryCoeffs[(DIM * (DIM + 1)) / 2 + i] = diffw[i]; }

		// Then finally the constant term
		boundaryCoeffs.back() =
		    (-1 / 2.0 * means[0].dot(varInverses[0] * means[0]) - 1 / 2.0 * logVarDets[0] + logPriors[0]) -
		    (-1 / 2.0 * means[1].dot(varInverses[1] * means[1]) - 1 / 2.0 * logVarDets[1] + logPriors[1]);

		for (unsigned i = 0; i < boundaryCoeffs.size(); i++) {
			arg.boundaryParamsFile << std::setw(10) << (char) ('a' + i) << "  ";
		}
		arg.boundaryParamsFile << std::setw(10) << "xmin"
		                       << "  " << std::setw(10) << "xmax"
		                       << "  " << std::setw(10) << "ymin"
		                       << "  " << std::setw(10) << "ymax"
		                       << "  " << std::setw(10) << "zmax\n"
		                       << std::fixed << std::setprecision(7);

		for (double coeff : boundaryCoeffs) { arg.boundaryParamsFile << std::setw(10) << coeff << "  "; }

		arg.boundaryParamsFile << std::setw(10) << min[0] << "  " << std::setw(10) << max[0] << "  " << std::setw(10)
		                       << min[1] << "  " << std::setw(10) << max[1] << "  " << std::setw(10)
		                       << round(pdfMax * 100) / 100;
	}

	return 0;
}

double discriminateCase1(const observation& obs, const Vec<CLASSES>& mu, const CovMatrix& varInverse, double logVarDet,
                         double logPrior) {
	// Note that varInverse is what is passed, and the inverse of a diagonal matrix is also a
	// diagonal matrix with the diagonal elements being the reciprocal of the original elements. So
	// instead of dividing by sigma^2, we multiply by 1/sigma^2.
	return (varInverse(0, 0) * mu).dot(obs) - varInverse(0, 0) * mu.dot(mu) / 2.0 + logPrior;
}

double discriminateCase2(const observation& obs, const Vec<CLASSES>& mu, const CovMatrix& varInverse, double logVarDet,
                         double logPrior) {
	observation inverseMu = varInverse * mu;
	return inverseMu.dot(obs) - mu.dot(inverseMu) / 2.0 + logPrior;
}

double discriminateCase3(const observation& obs, const Vec<CLASSES>& mu, const CovMatrix& varInverse, double logVarDet,
                         double logPrior) {
	return (obs.dot((-1 / 2.0 * varInverse) * obs) + (varInverse * mu).dot(obs)) - 1 / 2.0 * mu.dot(varInverse * mu) -
	       logVarDet / 2.0 + logPrior;
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
		} else if (!strcmp(argv[i], "-pdf")) {
			if (i + 1 >= argc) {
				std::cout << "Missing probability density function file.\n\n";
				err = 1;
				printHelp();
				return false;
			}

			arg.pdfPlotFile.open(argv[i + 1]);
			if (!arg.pdfPlotFile) {
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
	          << "  -s   <seed>  Set the seed used to generate samples.\n"
	          << "               Defaults to " << arg.seed << ".\n"
	          << "  -psN <file>  Print all observations from sample N to a file.\n"
	          << "               N can be 1 to " << CLASSES << ".\n"
	          << "  -pmN <file>  Print all misclassified observations from sample N to a file.\n"
	          << "               N can be 1 to " << CLASSES << ".\n"
	          << "  -pdf <file>  Print a graph of the probability density function to a file.\n"
	          << "               There will be an extra column which shows which class is more\n"
	          << "               likely at that point. Will also allow correct calculation of\n"
	          << "               zmax in the -pdb file.\n"
	          << "  -pdb <file>  Print the parameters of the decision boundary.\n"
	          << "  -c   <case>  Override which discriminant case is to be used.\n"
	          << "               <case> can be 1-3. Higher numbers are more computationally\n"
	          << "               expensive, but are correct more of the time.\n"
	          << "               By default, the case will be chosen automatically.\n";
}