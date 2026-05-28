#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"

#include <complex>
#include <vector>

namespace SystemVueModelBuilder
{
    class SYSTEMVUEMODELBUILDER_API Demapper : public SystemVueModelBuilder::DFModel
	{
	public:
		DECLARE_MODEL_INTERFACE(Demapper);
		
		Demapper();
		virtual ~Demapper() = default;
		
		virtual bool Initialize();
		virtual bool Setup();
		virtual bool Run();
		virtual bool Finalize();
		
		// ---- 获取内部状态 (供 Block 使用) ----
		const std::vector<std::complex<double>>& GetConstellationTable() const { return m_table; }
		int GetConstellationSize() const { return m_M; }
		int GetSymbolLength() const { return m_symbolLength; }
		const std::vector<int>& GetIndexToState() const { return m_indexToState; }

		// Ports
		CircularBuffer<std::complex<double> > m_input;
		CircularBuffer<bool>                  m_bits;
		CircularBuffer<std::complex<double> > m_node;

		// Parameters 
		enum ModTypeEnum
		{
			BPSK = 0,
			QPSK,
			PSK8,
			PSK16,
			QAM16,
			QAM32,
			QAM64,
			QAM128,
			QAM256,
			User_Defined,
			QAM512,
			QAM1024,
			QAM2048,
			QAM4096,
			APSK16,
			APSK32,
			Star16QAM,
			Star32QAM,
			CustomAPSK
		};

		enum BitOrderEnum
		{
			LSB_first = 0,
			MSB_first
		};

		enum DefaultStateEnum
		{
			FALSE_ = 0,
			TRUE_
		};

		ModTypeEnum      ModType;
		BitOrderEnum     BitOrder;

		// User Defined
		DComplexMatrix   MappingTable;

		// APSK /Star ratios
		double           Ratio_R2_R1;
		double           Ratio_R3_R1;
		double           Ratio_R4_R1;

		// Custom APSK
		IntMatrix        RingStates;
		DoubleMatrix     RingMagnitudes;
		DoubleMatrix     RinginitialPhases;
		DefaultStateEnum DefaultState;
		IntMatrix        States;

	private:
		template<typename Buffer>
		static auto TrySetRate(Buffer& b, int r, int) -> decltype(b.SetRate(r), void())
		{
			b.SetRate(static_cast<unsigned>(r));
		}
		template<typename Buffer>
		static void TrySetRate(Buffer&, int, ...)
		{
		}

		bool RebuildConstellation();
		bool ShouldNormalize() const;
		int  FindNearestIndex(const std::complex<double>& x) const;
		void WriteBitsFromState(int state);

		void BuildBPSK(std::vector<std::complex<double> >& table) const;
		void BuildPSK(int M, double phase0_rad, std::vector<std::complex<double> >& table) const;
		void BuildQAM(int M, std::vector<std::complex<double> >& table) const;
		void BuildUserDefined(std::vector<std::complex<double> >& table) const;
		void BuildAPSK16(std::vector<std::complex<double> >& table) const;
		void BuildAPSK32(std::vector<std::complex<double> >& table) const;
		void BuildStar16(std::vector<std::complex<double> >& table) const;
		void BuildStar32(std::vector<std::complex<double> >& table) const;
		bool BuildCustomAPSK(std::vector<std::complex<double> >& table);
		bool BuildIndexToStateMap(int M);

		static unsigned int GrayEncode(unsigned int v);
		static unsigned int GrayDecode(unsigned int g);
		static bool         IsPowerOfTwo(int n);
		static int          ILog2Pow2(int n);
		static double       DegToRad(double deg);
		static std::complex<double> Polar(double r, double phase);
		static std::complex<double> RotateByQuadrant(int q2, double I, double Q);
		static std::complex<double> RotateByQuadrantGray(int q2, double I, double Q);
		static void         NormalizeTable(std::vector<std::complex<double> >& table);

		template<typename TMatrix>
		static int MatrixNumElements(const TMatrix& m)
		{
			return static_cast<int>(m.NumElements());
		}

		template<typename TMatrix>
		static double MatrixGetAsDouble(const TMatrix& m, int idx)
		{
			return static_cast<double>(m(static_cast<size_t>(idx)));
		}

		static std::complex<double> MatrixGetAsComplex(const DComplexMatrix& m, int idx)
		{
			return m(static_cast<size_t>(idx));
		}

	private:
		int m_symbolLength;
		int m_M;
		std::vector<std::complex<double> > m_table;
		std::vector<int> m_indexToState; 
	};
}
