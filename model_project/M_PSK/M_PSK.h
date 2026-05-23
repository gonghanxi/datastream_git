#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"
#include "SystemVueModels.h"

#include <complex>
#include <vector>
#include <cstddef>

namespace SystemVueModelBuilder {

    class M_PSK : public DFModel
	{
	public:
		enum ModType
		{
			BPSK = 0,
			QPSK,
			PSK8,
			PSK16,
			PSK32,
			PSK64,
			PSK128,
			PSK256,
			PSK512,
			MOD_COUNT
		};

		enum BitOrder
		{
			LSB_first = 0,
			MSB_first = 1
		};

	public:
		DECLARE_MODEL_INTERFACE(M_PSK);

		M_PSK();

		bool Setup() override;
		bool Initialize() override;
		bool Run() override;

		bool UpdateDynamicParameters() override;

//	protected:
		bool ValidateParameters();

		// 生成 Gray 映射表：m_grayTable[grayLabel] = phaseIndex
		void GenerateGrayCoding();

		// 将 n 个 bit 组装成整数（按 BitOrder）
		int ConvertBitsToInt(const std::vector<int>& bits) const;

		// 建立星座查表：float step*k + cosf/sinf
		void BuildConstellationTable();

//	public:
		CircularBuffer<int> m_in;
		std::complex<double> m_out;


		ModType  m_modType;
		BitOrder m_bitOrder;

//	protected:
		std::size_t m_modOrder;
		std::size_t m_modBits;

		std::vector<int> m_grayTable;

		std::vector<std::complex<float>> m_constTable;
		float m_angleStep;

		ModType m_setupModType;
	};

} 
