#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"

#include <complex>
#include <vector>

namespace SystemVueModelBuilder
{
    class SYSTEMVUEMODELBUILDER_API Mapper : public DFModel
	{
	public:
		DECLARE_MODEL_INTERFACE(Mapper);

		Mapper();
		virtual ~Mapper() = default;

		virtual bool Initialize();
		virtual bool Setup();
		virtual bool Run();
		virtual bool Finalize();

		// Ports
		CircularBuffer<bool>                  m_input;
		CircularBuffer<std::complex<double> > m_output;

		// -------- Parameters --------
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

		ModTypeEnum      ModType;     // Modulation type
		BitOrderEnum     BitOrder;    // Bit order

		// User Defined
		DComplexMatrix   MappingTable; // Constellation table

		// APSK / Star ratios
		double           Ratio_R2_R1;  // R2/R1
		double           Ratio_R3_R1;  // R3/R1
		double           Ratio_R4_R1;  // R4/R1

		// Custom APSK
		IntMatrix        RingStates;         // Number of phase states on each ring
		DoubleMatrix     RingMagnitudes;     // Relative radius of each ring
		DoubleMatrix     RinginitialPhases;  // deg
		DefaultStateEnum DefaultState;       // Use default states or not
		IntMatrix        States;             // States corresponding to constellation points

		template<typename Buffer>
		static auto TrySetRate(Buffer& b, int r, int) -> decltype(b.SetRate(r), void())
		{
			b.SetRate(static_cast<unsigned>(r));
		}
		template<typename Buffer>
		static void TrySetRate(Buffer&, int, ...)
		{
		}

		int  GetTableIndex(const bool* bits, int symbolLength) const;
		bool RebuildConstellation();
		bool ShouldNormalize() const;

		// Table builders
		void BuildBPSK(std::vector<std::complex<double> >& table) const;
		void BuildPSK(int M, double phase0_rad, std::vector<std::complex<double> >& table) const;
		void BuildQAM(int M, std::vector<std::complex<double> >& table) const;
		void BuildUserDefined(std::vector<std::complex<double> >& table) const;
		void BuildAPSK16(std::vector<std::complex<double> >& table) const;
		void BuildAPSK32(std::vector<std::complex<double> >& table) const;
		void BuildStar16(std::vector<std::complex<double> >& table) const;
		void BuildStar32(std::vector<std::complex<double> >& table) const;
		bool BuildCustomAPSK(std::vector<std::complex<double> >& table);
		bool BuildStateToIndexMap(int M);

		// Utilities
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

		int m_symbolLength;
		int m_M;
		std::vector<std::complex<double> > m_table;
		std::vector<int> m_stateToIndex; // only used when CustomAPSK && DefaultState==FALSE
	};
}
