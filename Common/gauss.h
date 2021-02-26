#include <omp.h>

#include <iomanip>
#include <random>

template <unsigned N, typename T = double>
struct Vec {
	T v[N];
	T& x = v[0];
	T& y = v[1];
	T& z = v[2];

	Vec() {}
	Vec(const T (&values)[N]) {
		for (unsigned i = 0; i < N; i++) { v[i] = values[i]; }
	}

	T& operator[](unsigned i) { return v[i]; }
	const T& operator[](unsigned i) const { return v[i]; }
};

template <typename T>
struct Vec<1, T> {
	T v[1];
	T& x = v[0];

	Vec() {}
	Vec(const T (&values)[1]) { v[0] = values[0]; }
	Vec(const T value) { v[0] = value; }
	T& operator[](unsigned i) { return v[i]; }
	const T& operator[](unsigned i) const { return v[i]; }
};

template <typename T>
struct Vec<2, T> {
	T v[2];
	T& x = v[0];
	T& y = v[1];

	Vec() {}
	Vec(const T (&values)[2]) {
		for (unsigned i = 0; i < 2; i++) v[i] = values[i];
	}
	T& operator[](unsigned i) { return v[i]; }
	const T& operator[](unsigned i) const { return v[i]; }
};

template <unsigned N, typename T>
Vec<N, T> operator+(const Vec<N, T>& lhs, const Vec<N, T>& rhs) {
	Vec<N, T> re;
	for (unsigned i = 0; i < N; i++) { re.v[i] = lhs[i] + rhs[i]; }
}

template <unsigned N, typename T>
Vec<N, T> operator-(const Vec<N, T>& lhs, const Vec<N, T>& rhs) {
	Vec<N, T> re;
	for (unsigned i = 0; i < N; i++) { re.v[i] = lhs[i] - rhs[i]; }
}

template <unsigned N, typename T>
Vec<N, T> operator*(const T& lhs, const Vec<N, T>& rhs) {
	Vec<N, T> re;
	for (unsigned i = 0; i < N; i++) { re.v[i] = lhs * rhs[i]; }
}

template <unsigned N, typename T>
Vec<N, T> operator*(const Vec<N, T>& lhs, const T& rhs) {
	Vec<N, T> re;
	for (unsigned i = 0; i < N; i++) { re.v[i] = rhs[i] * rhs; }
}

template <unsigned N, typename T>
std::ostream& operator<<(std::ostream& lhs, const Vec<N, T>& rhs) {
	lhs << '(';
	for (unsigned i = 0; i < N - 1; i++) { lhs << rhs[i] << ", "; }
	lhs << rhs[N - 1] << ')';
	return lhs;
}

/**
 * @brief Generate a multivariate-normally-distributed random sample with the given
 *        parameters. Special case with the covariance matrix diagonal (i.e. the
 *        variables are uncorrelated).
 *
 * @param      mu      The mean of the distribution
 * @param      sigma   The standard deviation of the distribution
 * @param[out] sample  Where the sample outputs are stored. The sample size
 *                     is the size of the vector.
 * @param      seed    What to seed the RNG with. Defaults to 1.
 */
template <unsigned N>
void genGaussianSample(Vec<N> mu, Vec<N> sigma, std::vector<Vec<N>>& sample,
                       unsigned seed = 1) {
#pragma omp parallel
	{
		std::mt19937_64 engine(seed + omp_get_thread_num());
		std::vector<std::normal_distribution<double>> dists;

		// Initialise distributions with given information
		for (unsigned i = 0; i < N; i++) { dists.emplace_back(mu[i], sigma[i]); }

#pragma omp for
		// Generate random observations. Since the variables are uncorrelated
		// from a multivariate normal distribution, they are also independent.
		for (unsigned j = 0; j < sample.size(); j++) {
			for (unsigned i = 0; i < N; i++) { sample[j][i] = dists[i](engine); }
		}
	}
}