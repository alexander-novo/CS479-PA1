#include "sample.h"

void getSample(DataSet set, std::vector<Vec<2>>& sample1,
               std::vector<Vec<2>>& sample2) {
	switch (set) {
		case DataSet::A:
			sample1.resize(60'000);
			sample2.resize(140'000);
			genGaussianSample({{1, 1}}, {{1, 1}}, sample1);
			genGaussianSample({{4, 4}}, {{1, 1}}, sample2);
			break;
		case DataSet::B:
			sample1.resize(40'000);
			sample2.resize(160'000);
			genGaussianSample({{1, 1}}, {{1, 1}}, sample1);
			genGaussianSample({{4, 4}}, {{2, sqrt(8)}}, sample2);
			break;
	}
}