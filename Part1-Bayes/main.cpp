// Part1-Bayes/main.cpp
#include <cstring>
#include <iostream>

#include "../Common/sample.h"

// Struct for inputting arguments from command line
struct Arguments {
	DataSet set;
};

bool verifyArguments(int argc, char** argv, Arguments& arg, int& err);
void printHelp();

int main(int argc, char** argv) {
	int err;
	Arguments arg;

	if (!verifyArguments(argc, argv, arg, err)) { return err; }

	std::vector<Vec<2>> sample1, sample2;
	getSample(arg.set, sample1, sample2);

	return 0;
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

	return true;
}

void printHelp() {
	std::cout
	    << "Usage: classify-bayes <data set>                                (1)\n"
	    << "   or: classify-bayes -h                                        (2)\n\n"
	    << "(1) Run a Bayes classifier on a specific data set.\n"
	    << "    Data sets available are 'A' and 'B'.\n"
	    << "(2) Print this help menu.\n";
}