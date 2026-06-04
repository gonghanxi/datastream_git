#pragma once

#include "SystemVue.h"          // ERESULT
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "TimedCircularBuffer.h"
#include "EnvelopeSignal.h"
#include "Matrix.h"

#include <complex>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cerrno>
#include <cstring>

class SYSTEMVUEMODELBUILDER_API RADAR_MultiCH_Rx : public SystemVueModelBuilder::TimedDFModel
{
public:
	using Cx = std::complex<double>;
	using EnvSig = SystemVueModelBuilder::EnvelopeSignal;

	// 输入：multiple envelope（Timed bus）
	using EnvBus = SystemVueModelBuilder::EnvelopeCircularBufferBus;

	// 输出：multiple complex（Timed bus）
	using CxBuf = SystemVueModelBuilder::TimedCircularBuffer<Cx>;
	using CxBus = SystemVueModelBuilder::CircularBufferBusT<CxBuf>;

	DECLARE_MODEL_INTERFACE(RADAR_MultiCH_Rx);
	RADAR_MultiCH_Rx();

	bool Setup() override;
	bool Run() override;

	ERESULT CalculateLatency() override;

	// 端口（bus）
	EnvBus input;
	CxBus  output;

	// 参数
	double RefFreq;
	double NDensity;

	SystemVueModelBuilder::Matrix<double> Sensitivity;
	SystemVueModelBuilder::Matrix<double> Phase;
	SystemVueModelBuilder::Matrix<double> IQGainImbalance;
	SystemVueModelBuilder::Matrix<double> IQPhaseImbalance;

	int NumOfCh;
	SystemVueModelBuilder::Matrix<Cx> ImbalanceCoef;

private:
	static constexpr double kPi = 3.1415926535897932384626433832795;
	static constexpr double kTwoPi = 6.283185307179586476925286766559;

	size_t inBusSize_ = 0;
	size_t outBusSize_ = 0;

	int nChExpected_ = 0;

	std::vector<double> sens_;
	std::vector<double> phaseDeg_;
	std::vector<double> iqGainDb_;
	std::vector<double> iqPhaseDeg_;
	std::vector<Cx>     imbCoef_;

	double ts0_ = 0.0;   // 用于噪声/兜底的 time step（取第0路）
	double fs0_ = 0.0;   // 用于噪声/兜底的 sample rate（取第0路）

	uint32_t rngState_ = 1;
	bool   haveSpare_ = false;
	double spare_ = 0.0;
	uint64_t sampleIndex_ = 0;

	static inline double deg2rad(double deg) { return deg * kPi / 180.0; }

	inline uint32_t lcg_()
	{
		rngState_ = rngState_ * 214013u + 2531011u;
		return (rngState_ >> 16) & 0x7fffu;
	}

	inline double uniform01_()
	{
		return (static_cast<double>(lcg_()) + 1.0) / 32768.0;
	}

	double randn_()
	{
		if (haveSpare_)
		{
			haveSpare_ = false;
			return spare_;
		}
		const double u1 = uniform01_();
		const double u2 = uniform01_();
		const double r = std::sqrt(-2.0 * std::log(u1));
		const double th = kTwoPi * u2;
		spare_ = r * std::sin(th);
		haveSpare_ = true;
		return r * std::cos(th);
	}

	bool buildCache_();
	void applyOutputTiming_();

	static Cx applyIQImbalance_(const Cx& z, double gainDb, double phaseDeg);
	Cx makeNoise_(double fs);
};
