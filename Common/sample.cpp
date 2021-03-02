#include "sample.h"

void getSamples(DataSet set, std::array<std::vector<Vec<DIM>>, CLASSES>& samples,
                unsigned seed) {
	std::array<unsigned, CLASSES> sizes = getSizes(set);
	std::array<Vec<DIM>, CLASSES> means = getMeans(set);
	std::array<Vec<DIM>, CLASSES> vars  = getVars(set);
	for (unsigned i = 0; i < CLASSES; i++) {
		samples[i].resize(sizes[i]);
		genGaussianSample(means[i], vars[i], samples[i], seed);
	}
}

std::array<Vec<DIM>, CLASSES> getMeans(DataSet set) {
	switch (set) {
		case DataSet::A:
			return {{{{1, 1}}, {{4, 4}}}};
		case DataSet::B:
			return {{{{1, 1}}, {{4, 4}}}};
	}
}

std::array<Vec<DIM>, CLASSES> getVars(DataSet set) {
	switch (set) {
		case DataSet::A:
			return {{{{1, 1}}, {{1, 1}}}};
		case DataSet::B:
			return {{{{1, 1}}, {{2, sqrt(8)}}}};
	}
}

std::array<unsigned, CLASSES> getSizes(DataSet set) {
	switch (set) {
		case DataSet::A:
			return {60'000, 140'000};
		case DataSet::B:
			return {40'000, 160'000};
	}
}