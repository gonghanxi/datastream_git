// Copyright 2011 - 2014 Keysight Technologies, Inc
#pragma once

#include "ModelBuilder.h"
#include "SystemVueModels.h"
#include "Matrix.h"

namespace SystemVueModelBuilder
{
    class SYSTEMVUEMODELBUILDER_API  SVD_M : public SystemVueModelBuilder::DFModel
	{
	public:
		// input and output
		CircularBuffer< Matrix<double> > input; // Input stream
		CircularBuffer< Matrix<double> > svals; // The singular values of input - vector form (diag(W))
		CircularBuffer< Matrix<double> > rsvec; // Right singular vectors of input
		CircularBuffer< Matrix<double> > lsvec; // Left singular vectors of input

		// parameters
		double Threshold;      // Threshold for similarities
		int    MaxIterations;  // Maximum iterations for SVD convergence

		enum GenerateLeftE { DoNotGenerateLeft, GenerateLeft };
		GenerateLeftE m_GenerateLeft;

		enum GenerateRightE { DoNotGenerateRight, GenerateRight };
		GenerateRightE m_GenerateRight;

		DECLARE_MODEL_INTERFACE(SVD_M);

		SVD_M();

		bool Initialize();
		bool Run();
		bool Finalize();

	protected:
		size_t nrows, ncols;
		Matrix<double> U, W, V;

		double hypot(double, double);

		void calc_svd(const Matrix<double>& A,
			Matrix<double>& Uo,
			Matrix<double>& Wo,
			Matrix<double>& Vo,
			double threshold,
			int maxIters,
			int needV);

	private:
		void transpose(const Matrix<double>& A, Matrix<double>& AT);

		// --- Ϊ�ˡ�ÿ֡�ȶ����������÷���ϰ�ߡ���״̬ ---
		bool m_hasPrevV;
		Matrix<double> m_prevV;
	};
}
