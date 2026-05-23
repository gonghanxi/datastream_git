// BetaDistribution.h
#ifndef BETADISTRIBUTION_H
#define BETADISTRIBUTION_H

#include <random>
#include <cmath>
#include <stdexcept>

//#ifdef _WIN32
//#pragma push_macro("min")
//#pragma push_macro("max")
//#undef min
//#undef max
//#endif

template<typename RealType = double>
class BetaDistribution {
public:
    using result_type = RealType;

    explicit BetaDistribution(RealType alpha = 1.0, RealType beta = 1.0)
        : m_alpha(alpha), m_beta(beta) {
        if (alpha <= 0 || beta <= 0) {
            throw std::invalid_argument("Beta distribution parameters must be positive");
        }
    }

    RealType alpha() const { return m_alpha; }
    RealType beta() const { return m_beta; }

    template<typename Generator>
    RealType operator()(Generator& g) {
        std::gamma_distribution<RealType> gamma_alpha(m_alpha, 1.0);
        std::gamma_distribution<RealType> gamma_beta(m_beta, 1.0);

        RealType x = gamma_alpha(g);
        RealType y = gamma_beta(g);

        return x / (x + y);
    }

    template<typename Generator>
    RealType operator()(Generator& g, const RealType& alpha, const RealType& beta) {
        std::gamma_distribution<RealType> gamma_alpha(alpha, 1.0);
        std::gamma_distribution<RealType> gamma_beta(beta, 1.0);

        RealType x = gamma_alpha(g);
        RealType y = gamma_beta(g);

        return x / (x + y);
    }

    void reset() {}

    RealType min() const { return 0.0; }
    RealType max() const { return 1.0; }

private:
    RealType m_alpha;
    RealType m_beta;
};

//#ifdef _WIN32
//#pragma pop_macro("min")
//#pragma pop_macro("max")
//#endif

#endif // BETADISTRIBUTION_H
