#include "RADAR_Detector_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_Detector_M)
{
	SET_MODEL_DESCRIPTION("RADAR Detector Matrix");
	SET_MODEL_CATEGORY("Signal Processing");

	// ===== 端口 =====
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("Input signal");
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("Output signal");
	}

	// ===== 参数：Type =====
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Type, SelectedDetectorType);
		enumParam.SetName("Type");
		enumParam.AddEnumeration("Envelop", Envelop);
		enumParam.AddEnumeration("Square", Square);
		enumParam.AddEnumeration("Log Square", LogSquare);
		enumParam.AddEnumeration("Log", Log);
		enumParam.SetDefaultValue("1");  // Square
		enumParam.SetDescription("The type of the detector: Envelop, Square, Log Square, Log");
	}

	// ===== 参数：Log_Coefb =====
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Log_Coefb);
		param.SetName("Log_Coefb");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetDescription("The coefficient value b in the log detector law y = aln(bx)");
	param.SetHideCondition("Type ~= 3");
	}

	// ===== 参数：Log_Coefa =====
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Log_Coefa);
		param.SetName("Log_Coefa");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetDescription("The coefficient value a in the log detector law y = aln(bx)");
		param.SetHideCondition("Type ~= 3");
	}

	return true;
}
#endif

RADAR_Detector_M::RADAR_Detector_M()
{
}

//-----------------------------------------------------------------------------------
//  Run
//      矩阵版 RADAR_Detector：
//      input  为 complex matrix
//      output 为 real matrix
//
//      核心计算方式完全沿用普通 RADAR_Detector：
//      Envelop   : y = |x|
//      Square    : y = |x| * |x|
//      LogSquare : y = a * log(|b*x| * |b*x|)
//      Log       : y = a * log(|b*x|)
//-----------------------------------------------------------------------------------
bool RADAR_Detector_M::Run()
{
	const SystemVueModelBuilder::Matrix< std::complex<double> >& inMat = input[0];

	const size_t nRows = inMat.NumRows();
	const size_t nCols = inMat.NumColumns();

	SystemVueModelBuilder::Matrix< double > outMat;
	outMat.Resize(nRows, nCols);

	for (size_t r = 0; r < nRows; ++r)
	{
		for (size_t c = 0; c < nCols; ++c)
		{
			const std::complex<double>& x = inMat(r, c);

			switch (Type)
			{
			case RADAR_Detector_M::Envelop:
				outMat(r, c) = std::abs(x);
				break;

			case RADAR_Detector_M::Square:
				outMat(r, c) = std::abs(x) * std::abs(x);
				break;

			case RADAR_Detector_M::LogSquare:
				outMat(r, c) = Log_Coefa * std::log(
					std::abs(Log_Coefb * x) * std::abs(Log_Coefb * x)
				);
				break;

			case RADAR_Detector_M::Log:
				outMat(r, c) = Log_Coefa * std::log(std::abs(Log_Coefb * x));
				break;

			default:
				outMat(r, c) = 0.0;
				break;
			}
		}
	}

	output[0] = outMat;

	return true;
}