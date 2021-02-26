#include <random>

/**
 * @brief Generate a normally-distributed random sample with the given parameters.
 *
 * @param      mu      The mean of the distribution
 * @param      sigma   The standard deviation of the distribution
 * @param[out] sample  Where the sample outputs are stored. The sample size
 *                     is the size of the vector.
 */
void genGaussianSample(double mu, double sigma, std::vector<double>& sample);