
#include <vector>
#include <tuple>
#include <cmath>
#include <iostream>
#include <algorithm>

namespace zaphod {
namespace _legendre {

// Adapted from https://thoughts-on-coding.com/2019/04/25/numerical-methods-in-cpp-part-2-gauss-legendre-integration/

const long double EPSILON = 1e-15;

std::tuple<long double, long double> calculate_legendre_value_and_derivative(long double x, unsigned int n) {
    // Implemented here because
    //  1) llvm (underlying clang) still hasn't implemented std::legendre() as of July 2026
    //  2) In any case, the spec says behavior is undefined when n > 128, which encompasses a lot of the use case here

    long double value = x;
    long double value_minus_1 = 1;
    long double derivative = 0;
    
    const long double f = 1 / (x*x - 1);
    for (unsigned int step = 2; step <= n; step++) {
        const long double val = ((2 * step - 1) * x * value - (step - 1) * value_minus_1) / step;
        derivative = step * f * (x * val - value);

        value_minus_1 = value;
        value = val;
    }

    return std::make_tuple(value, derivative);
}

std::vector<double> calculate_legendre_roots(unsigned int n) {
    std::vector<double> roots(n);

    for (unsigned int step = 0; step < n; step++) {
        // https://math.stackexchange.com/questions/1278772/gauss-legendre-quadrature-computation-of-the-abscissas-and-weights
        // According to ^ that link, the arccos of the ith root lies in the interval [ π (2i - 1) / (2n + 1), π (2i) / (2n + 1) ] (i = 1, 2, ..., n)
        //  So pick the middle of that interval as the first guess for the root.
        
        long double root = cos(M_PI * (step + 0.75) / (n + 0.5));
        long double value, derivative;

        std::tie(value, derivative) = calculate_legendre_value_and_derivative(root, n);

        long double newton_raphson_ratio;
        do {
            newton_raphson_ratio = value / derivative;
            root -= newton_raphson_ratio;
            std::tie(value, derivative) = calculate_legendre_value_and_derivative(root, n);
        } while (fabs(newton_raphson_ratio) > EPSILON);

        roots[step] = root;
    }

    return roots;
}
}

}
