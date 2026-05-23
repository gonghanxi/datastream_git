#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"
#include <complex>
#include <cstddef>

class SYSTEMVUEMODELBUILDER_API Quantizer2D : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(Quantizer2D);

	Quantizer2D();

	bool Setup() override;
	bool Run() override;
	bool UpdateDynamicParameters() override;

protected:
	bool ValidateParameters();

public:
	std::complex<double> input;
	std::complex<double> output;

	double VxMax;
	double VxMin;
	double Nx;
	double VyMax;
	double VyMin;
	double Ny;

	SystemVueModelBuilder::DComplexMatrix QuantList;

protected:
	double xDelta;
	double yDelta;
};
