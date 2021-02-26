#include "gauss.h"

enum DataSet { A, B };

/**
 * @brief Retreives two samples from distributions as labeled by the assignment.
 *
 * @param set     What distributions to use.
 * @param sample1
 * @param sample2
 */
void getSample(DataSet set, std::vector<Vec<2>>& sample1,
               std::vector<Vec<2>>& sample2);

/**
 * @brief Calculate the sample mean from a sample.
 *
 * @tparam N       The number of features in the sample.
 * @param sample   The sample to calculate from.
 * @return Vec<N>  The sample mean.
 */
template <unsigned N>
Vec<N> sampleMean(const std::vector<Vec<N>>& sample) {
	Vec<N> mean       = 0;
	double correction = 1.0 / sample.size();

#pragma omp parallel for reduction(+ : mean)
	for (unsigned i = 0; i < sample.size(); i++) { mean += correction * sample[i]; }

	return mean;
}

/**
 * @brief Calculate the sample variance from a sample. Uses Bessel's correction for
 *        unbiased-ness.
 *
 * @tparam N          The number of features in the sample.
 * @param sample      The sample to calculate from.
 * @param sampleMean  The sample mean.
 *                    See sampleMean(sample).
 * @return            The sample variance.
 */
template <unsigned N>
Vec<N> sampleVariance(const std::vector<Vec<N>>& sample, const Vec<N>& sampleMean) {
	Vec<N> var        = 0;
	double correction = 1.0 / (sample.size() - 1);

#pragma omp parallel for reduction(+ : var)
	for (unsigned i = 0; i < sample.size(); i++) {
		double temp = (sample[i] - sampleMean);
		var += correction * temp * temp;
	}

	return var;
}