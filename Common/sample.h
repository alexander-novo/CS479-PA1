#pragma once
#include "gauss.h"

#define CLASSES 2
#define DIM 2

typedef Vec<DIM> observation;
typedef std::vector<observation> sample;

enum DataSet { A, B };

/**
 * @brief Retreives samples from distributions as labeled by the assignment.
 *
 * @param set      What distributions to use.
 * @param samples
 * @param seed     Seed the RNG. Keep consistent for the same samples.
 */
void getSamples(DataSet set, std::array<sample, CLASSES>& samples, unsigned seed = 1);

/**
 * @brief Retrieve the means of a data set.
 *
 * @param set                                The data set to get the means of.
 * @return std::array<observation, CLASSES>  The means.
 */
std::array<observation, CLASSES> getMeans(DataSet set);

/**
 * @brief Retrieve the variances of a data set. The covariance matrix of all samples
 *        in all data sets are diagonal, so the variances are just the diagonal
 *        elements.
 *
 * @param set                                The data set to get the variances of.
 * @return std::array<observation, CLASSES>  The variances.
 */
std::array<observation, CLASSES> getVars(DataSet set);

/**
 * @brief Retrieve the number of observations for each sample in a data set.
 *
 * @param set                             The data set to get the sizes of.
 * @return std::array<unsigned, CLASSES>  The sizes.
 */
std::array<unsigned, CLASSES> getSizes(DataSet set);

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
		Vec<N> temp = (sample[i] - sampleMean);
		var += correction * temp * temp;
	}

	return var;
}