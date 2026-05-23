#include "Quantizer2D.h"

#include <cmath>
#include <limits>

using SystemVueModelBuilder::DFParam;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Quantizer2D)
{
	SET_MODEL_DESCRIPTION("2-Dimensional Quantizer using a threshold list");
	SET_MODEL_SYMBOL("SYM_Quantizer2D");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetDescription("Input signal");
	}

	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("Output signal");
	}

	{
		DFParam p = ADD_MODEL_PARAM(VxMax);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");
		p.SetDescription("Maximum real output level");
	}

	{
		DFParam p = ADD_MODEL_PARAM(VxMin);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("-1");
		p.SetDescription("Minimum real output level");
	}

	{
		DFParam p = ADD_MODEL_PARAM(Nx);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("16");
		p.SetDescription("Number of real output levels");
	}

	{
		DFParam p = ADD_MODEL_PARAM(VyMax);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");
		p.SetDescription("Maximum imaginary output level");
	}

	{
		DFParam p = ADD_MODEL_PARAM(VyMin);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("-1");
		p.SetDescription("Minimum imaginary output level");
	}

	{
		DFParam p = ADD_MODEL_PARAM(Ny);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("16");
		p.SetDescription("Number of imaginary output levels");
	}

	{
		DFParam p = ADD_MODEL_PARAM(QuantList);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDescription("User-defined quantization points");
	}

	return true;
}
#endif 

Quantizer2D::Quantizer2D()
	: input(0.0, 0.0)
	, output(0.0, 0.0)
	, VxMax(1.0)
	, VxMin(-1.0)
	, Nx(16.0)
	, VyMax(1.0)
	, VyMin(-1.0)
	, Ny(16.0)
	, xDelta(0.0)
	, yDelta(0.0)
{
}

bool Quantizer2D::ValidateParameters()
{
	const std::size_t numPoints = QuantList.NumElements();
	if (numPoints > 0) {
		return true;
	}

	bool ok = true;

	if (Nx < 1.0) {
		POST_ERROR("Quantizer2D: Nx (number of real output levels) must be >= 1.");
		ok = false;
	}
	if (Ny < 1.0) {
		POST_ERROR("Quantizer2D: Ny (number of imaginary output levels) must be >= 1.");
		ok = false;
	}
	if (!(VxMax > VxMin)) {
		POST_ERROR("Quantizer2D: VxMax must be greater than VxMin.");
		ok = false;
	}
	if (!(VyMax > VyMin)) {
		POST_ERROR("Quantizer2D: VyMax must be greater than VyMin.");
		ok = false;
	}

	return ok;
}

bool Quantizer2D::UpdateDynamicParameters()
{
	if (!ValidateParameters()) {
		return false;
	}

	const std::size_t numPoints = QuantList.NumElements();
	if (numPoints == 0) {
		const int nx = static_cast<int>(std::floor(Nx + 0.5)); 
		const int ny = static_cast<int>(std::floor(Ny + 0.5));

		if (nx > 1) {
			xDelta = (VxMax - VxMin) / static_cast<double>(nx - 1);
		}
		else {
			xDelta = 0.0; 
		}

		if (ny > 1) {
			yDelta = (VyMax - VyMin) / static_cast<double>(ny - 1);
		}
		else {
			yDelta = 0.0; 
		}
	}

	return true;
}

bool Quantizer2D::Setup()
{
	return UpdateDynamicParameters();
}

bool Quantizer2D::Run()
{
	const std::complex<double> x = input;

	const std::size_t numPoints = QuantList.NumElements();

	if (numPoints > 0) {
		double bestDist2 = std::numeric_limits<double>::infinity();
		std::complex<double> bestPoint(0.0, 0.0);

		for (std::size_t i = 0; i < numPoints; ++i) {
			const std::complex<double>& q = QuantList(i); 
			const double dx = x.real() - q.real();
			const double dy = x.imag() - q.imag();
			const double d2 = dx * dx + dy * dy;

			if (d2 < bestDist2) {
				bestDist2 = d2;
				bestPoint = q;
			}
		}

		output = bestPoint;
		return true;
	}

	const int nx = static_cast<int>(std::floor(Nx + 0.5));
	const int ny = static_cast<int>(std::floor(Ny + 0.5));

	double tx = 0.0;
	if (VxMax != VxMin) {
		tx = (x.real() - VxMin) / (VxMax - VxMin) * static_cast<double>(nx - 1);
	}
	int ix = static_cast<int>(std::floor(tx + 0.5));
	if (ix < 0)        ix = 0;
	else if (ix > nx - 1) ix = nx - 1;

	double ty = 0.0;
	if (VyMax != VyMin) {
		ty = (x.imag() - VyMin) / (VyMax - VyMin) * static_cast<double>(ny - 1);
	}
	int iy = static_cast<int>(std::floor(ty + 0.5));
	if (iy < 0)        iy = 0;
	else if (iy > ny - 1) iy = ny - 1;

	const double xr = VxMin + static_cast<double>(ix) * xDelta;
	const double yi = VyMin + static_cast<double>(iy) * yDelta;

	output = std::complex<double>(xr, yi);
	return true;
}
