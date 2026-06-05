#pragma once

#include "ModelBuilder.h"
#include "Matrix.h"

#include <complex>
#include <cmath>

class SYSTEMVUEMODELBUILDER_API RADAR_Detector_M : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE(RADAR_Detector_M);

    // ===== 枚举：顺序与帮助文档一致 =====
    enum SelectedDetectorType
    {
        Envelop = 0,
        Square = 1,
        LogSquare = 2,
        Log = 3
    };

	// Constructor to initialize parameters
	RADAR_Detector_M();

	//-------- Function Overloads --------
	virtual bool Run();

	// ===== 端口 =====
	// input  : complex matrix
	// output : real matrix
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix< std::complex<double> > > input;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix< double > > output;

	// ===== 参数 =====
	// 帮助文档中参数名为 Type
	SelectedDetectorType Type;

	// Log detector law: y = a ln(bx)
	double Log_Coefb;
	double Log_Coefa;
};
