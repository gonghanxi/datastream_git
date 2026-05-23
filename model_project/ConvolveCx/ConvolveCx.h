#pragma once
#include "ModelBuilder.h"
#include "CircularBuffer.h"

#include <complex>
#include <vector>

class ConvolveCx : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(ConvolveCx);

	ConvolveCx();

	bool Setup() override;
	bool Run()   override;
	bool Finalize() override { return true; }

	SystemVueModelBuilder::CircularBuffer< std::complex<double> > inA;
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > inB;
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > out;

	int TruncationDepth;

private:
	std::size_t depth_;
	std::size_t sampleCount_;   // 新增：记录已经接收了多少个输入样点

	std::vector< std::complex<double> > histA_;
	std::vector< std::complex<double> > histB_;
};
