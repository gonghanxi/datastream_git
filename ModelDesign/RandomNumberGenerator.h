// Copyright  2011 - 2015 Keysight Technologies, Inc   
#pragma once

#include <random>
#include "SystemVue.h"
//#include "SystemVue/DLL_Export/SystemVueModels.h"

#undef max

namespace SystemVueModelBuilder {

// See comments on warning C4251 in State.h.
#pragma warning ( push )
#pragma warning ( disable: 4251 )

	/// The CUniform01 class implements a uniform random number generator
	/// with values in the range [0,1].
    class CUniform01
	{
	public:

		/// Constructor
		/// <param name="ulSeed">The seed value for the random number generator.</param>
		/// <remarks>A seed value of 0 will result in random value used as a seed.</remarks>
        CUniform01( unsigned long ulSeed = std::mt19937::default_seed ) {
            Initialize(ulSeed);
        }

		/// Initialize
		/// <param name="ulSeed"> Reset the seed value for the random number generator.</param> 
		/// <remarks>A seed value of 0 will result in random value used as a seed.</remarks>
        void Initialize( unsigned long ulSeed ) {
            if (ulSeed == 0) {
                std::random_device rd;
                m_randomInt.seed(rd());
            } else {
                m_randomInt.seed(ulSeed);
            }
        }

		/// operator ( )
		/// <returns>Returns the next random number.</returns>
        double operator( )( ) {
            // 使用 uniform_real_distribution 生成 [0,1] 上的双精度浮点数
            // 注：标准保证 [0,1] 端点都可能出现，但概率极低；性能足够
            static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
            return dist(m_randomInt);
        }

	private:
		std::mt19937 m_randomInt;	// Mersenne twister used to generate the random numbers
	};

#pragma warning ( pop )


	/// The CUniform class implements a uniform random number generator
	/// with values in a user defined range [dLower, dUpper].
    class CUniform
	{
	public:

		/// Constructor
		/// Default dLower = 0, dUpper = 1, ulSeed = std::mt19937::default_seed
        CUniform() : CUniform(0.0, 1.0, std::mt19937::default_seed) {}

		/// Constructor
		/// <param name="dLower">Lower limit of random values returned.</param>
		/// <param name="dUpper">Upper limit of random values returned.</param>
		/// <param name="ulSeed">The seed value for the random number generator.</param>
		/// <remarks>A seed value of 0 will result in random value used as a seed.</remarks>
        CUniform( double dLower, double dUpper, unsigned long ulSeed = std::mt19937::default_seed ) {
            Initialize(dLower, dUpper, ulSeed);
        }

		/// Initialize
		/// <param name="dLower">Lower limit of random values returned.</param>
		/// <param name="dUpper">Upper limit of random values returned.</param>
		/// <param name="ulSeed">The seed value for the random number generator.</param>
		/// <remarks>A seed value of 0 will result in random value used as a seed.</remarks>
        void Initialize( double dLower, double dUpper, unsigned long ulSeed ) {
            m_dScale  = dUpper - dLower;
            m_dOffset = dLower;
            m_Uniform01.Initialize(ulSeed);
        }

		/// operator ( )
		/// <returns>Returns the next random number.</returns>
        double operator( )( ) {
            // 线性变换：x = lower + (upper - lower) * u, u in [0,1]
            return m_dOffset + m_dScale * m_Uniform01();
        }

	private:
		CUniform01 m_Uniform01;		// CUniform01 random number generator used to generate random numbers in [0, 1]
		                           // that are then scaled and offset to produce numbers in the range [dLower, dUpper]
		double m_dScale;				// Scale applied uniform [0, 1] random numbers
		double m_dOffset;				// Offset applied to scaled random numbers
	};


	/// The CNormal class implements a Normal (Gaussian) random number generator
	/// with a user defined mean and variance. The Box-Muller method is used.
    class CNormal
	{
	public:

		/// Constructor
		/// Default dMean = 0, dVar = 1, ulSeed = std::mt19937::default_seed
        CNormal() : CNormal(0.0, 1.0, std::mt19937::default_seed) {}

		/// Constructor
		/// <param name="dMean">Mean value of random values returned.</param>
		/// <param name="dVar">Variance of random values returned.</param>
		/// <param name="ulSeed">The seed value for the random number generator.</param>
		/// <remarks>A seed value of 0 will result in random value used as a seed.</remarks>
        CNormal( double dMean, double dVar, unsigned long ulSeed = std::mt19937::default_seed ) {
            Initialize(dMean, dVar, ulSeed);
        }

		/// Initialize
		/// <param name="dMean">Mean value of random values returned.</param>
		/// <param name="dVar">Variance of random values returned.</param>
		/// <param name="ulSeed">The seed value for the random number generator.</param>
		/// <remarks>A seed value of 0 will result in random value used as a seed.</remarks>
        void Initialize( double dMean, double dVar, unsigned long ulSeed ) {
            m_dMean = dMean;
            m_dVar  = dVar;
            m_bHave2 = false;
            // 使用范围为 [-1, 1] 的均匀分布，用于极坐标法
            m_Uniform.Initialize(-1.0, 1.0, ulSeed);
        }

		/// operator ( )
		/// <returns>Returns the next random number.</returns>
        double operator( )( ) {
            // Marsaglia 极坐标法，生成两个标准正态变量并缓存其中一个
            if (m_bHave2) {
                m_bHave2 = false;
                return m_dMean + std::sqrt(m_dVar) * m_dVal2;
            }

            double x, y, s;
            do {
                x = m_Uniform();   // x in [-1, 1]
                y = m_Uniform();   // y in [-1, 1]
                s = x * x + y * y;
            } while (s >= 1.0 || s == 0.0);   // 拒绝 s=0 或 >=1 的点

            double multiplier = std::sqrt(-2.0 * std::log(s) / s);
            double z0 = x * multiplier;   // 标准正态 N(0,1)
            double z1 = y * multiplier;

            m_dVal2 = z1;
            m_bHave2 = true;

            return m_dMean + std::sqrt(m_dVar) * z0;
        }

	private:
		double m_dMean;		// Mean of normal distribution
		double m_dVar;			// Variance of normal distribution
		double m_dVal2;
		bool m_bHave2;
		CUniform m_Uniform;	// CUniform random number generator used to generate random numbers in [-1, 1]
			                  // that are then transformed to normally distributed numbers
	};



	/// The CNegativeExpntl class implements an exponential number generator
	/// with a user defined mean.
    class CNegativeExpntl
	{
	public:

		/// Constructor
		/// Default dMean = 1, ulSeed = std::mt19937::default_seed
        CNegativeExpntl() : CNegativeExpntl(1.0, std::mt19937::default_seed) {}

		/// Constructor
		/// <param name="dMean">Mean value of random values returned.</param>
		/// <param name="ulSeed">The seed value for the random number generator.</param>
		/// <remarks>A seed value of 0 will result in random value used as a seed.</remarks>
        CNegativeExpntl( double dMean, unsigned long ulSeed = std::mt19937::default_seed ) {
            Initialize(dMean, ulSeed);
        }

		/// Initialize
		/// <param name="dMean">Mean value of random values returned.</param>
		/// <param name="ulSeed">The seed value for the random number generator.</param>
		/// <remarks>A seed value of 0 will result in random value used as a seed.</remarks>
        void Initialize( double dMean, unsigned long ulSeed ) {
            m_dMean = dMean;
            m_Uniform01.Initialize(ulSeed);
        }

		/// operator ( )
		/// <returns>Returns the next random number.</returns>
        double operator( )( ) {
            // 指数分布逆变换：X = -mean * ln(1 - U) = -mean * ln(U)  (U 在 (0,1])
            double u;
            do {
                u = m_Uniform01();
            } while (u == 0.0);   // 避免 log(0)
            return -m_dMean * std::log(u);
        }

	private:
		CUniform01 m_Uniform01;		// CUniform01 random number generator used to generate random numbers in [0, 1]
			                        // that are then transformed to exponentially distributed numbers
		double m_dMean;				// Mean of exponential distribution
	};

}
