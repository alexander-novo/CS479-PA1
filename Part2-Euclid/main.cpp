// Part2-Euclid/main.cpp
#include <cstring>
#include <iostream>

#include "../Common/sample.h"

// Struct for inputting arguments from command line
struct Arguments {
	DataSet set;
	unsigned seed = 1;
};

typedef Vec<DIM> observation;

bool verifyArguments(int argc, char** argv, Arguments& arg, int& err);
void printHelp();

int main(int argc, char** argv) {
	int err;
	Arguments arg;

	if (!verifyArguments(argc, argv, arg, err)) { return err; }

	std::array<sample, CLASSES> samples;
	getSamples(arg.set, samples, arg.seed);

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

	// Optional Arguments
	for (unsigned i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "-s")) {
			if (i + 1 >= argc) {
				std::cout << "Missing seed value\n\n";
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
	std::cout << "Usage: classify-euclid <data set>                                (1)\n"
	          << "   or: classify-euclid -h                                        (2)\n\n"
	          << "(1) Run a Euclidean distance classifier on a specific data set.\n"
	          << "    Data sets available are 'A' and 'B'.\n"
	          << "(2) Print this help menu.\n\n"
	          << "OPTIONS\n"
	          << "  -s <seed>    Set the seed used to generate samples.\n"
	          << "               Defaults to " << arg.seed << ".\n";
}