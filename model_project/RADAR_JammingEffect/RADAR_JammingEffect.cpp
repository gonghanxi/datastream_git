#include "RADAR_JammingEffect.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_JammingEffect )
{	
	SET_MODEL_DESCRIPTION("Jamming Effect Evaluation");
	SET_MODEL_CATEGORY("EW");

	ADD_MODEL_INPUT(input);

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(JammingType, SelectedJammingType);
		enumParam.SetDescription("The jamming type to evaluate");
		enumParam.AddEnumeration("CoverJamming", CoverJamming);
		enumParam.AddEnumeration("DeceptionJamming", DeceptionJamming);
		enumParam.SetDefaultValue("0");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Start);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetDescription("Data collection start index");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI_NUM);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("10000");
		param.SetDescription("number of samples in PRI");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FFT_Size);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("16");
		param.SetDescription("Number of Pulse for coherent detection");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(DetectionNum);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetDescription("number of detections");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TargetsInPRI);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetDescription("number of targets in pri");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FalseTargetNum);
		param.SetDescription("The False Target Number.");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetHideCondition("JammingType ~= 1");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(TargetThreshold);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1e-8");
		param.SetDescription("Target Threshold");
	}

	{
		SystemVueModelBuilder::DFParam fName = ADD_MODEL_PARAM(FileName);
		fName.SetDescription("Data file name");
		fName.SetSchematicDisplay(0);
		fName.SetParamAsFile();
	}
	return true;
}
#endif

RADAR_JammingEffect::RADAR_JammingEffect()
{
	
}

bool RADAR_JammingEffect::Setup()
{
	bool bStatus = true;

	if (Start < 0)
	{
		POST_ERROR("Start must be >= 0");
		bStatus = false;
	}
	if (PRI_NUM <= 0)
	{
		POST_ERROR("PRI_NUM must be > 0");
		bStatus = false;
	}
	if (FFT_Size <= 0)
	{
		POST_ERROR("FFT_Size must be > 0");
		bStatus = false;
	}
	if (DetectionNum <= 0)
	{
		POST_ERROR("DetectionNum must be > 0");
		bStatus = false;
	}
	if (TargetsInPRI <= 0)
	{
		POST_ERROR("TargetsInPRI must be > 0");
		bStatus = false;
	}
	if (FalseTargetNum <= 0 && JammingType == RADAR_JammingEffect::DeceptionJamming)
	{
		POST_ERROR("FalseTargetNum must be > 0");
		bStatus = false;
	}
	if (TargetThreshold <= 0)
	{
		POST_ERROR("TargetThreshold must be > 0");
		bStatus = false;
	}

	// If parameters are valid, open the file
	if (bStatus)
	{
		outputFile.open(FileName, std::ios::binary | std::ios::out);
		if (outputFile.is_open() == false)
		{
			POST_ERROR("Cannot open file.");
			bStatus = false;
		}

		if (bStatus)
		{
			// Initialize the data collection controller - it will tract the data collected
			// and declare to SystemVue when the sink is done collecting data.
			bStatus = m_control.Initialize(this, Start, Start + PRI_NUM * FFT_Size * DetectionNum);
		}
	}

	DetectCount = 0;
	DetectStatus = false;

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_JammingEffect::Run()
{
	if (m_control.CollectData()) // Check if we should still collect data
	{
		// 连续的过阈值点视作一个目标
		if (input[0] > TargetThreshold && !DetectStatus)
		{
			DetectStatus = true;
			DetectCount++; // 信号过阈值的上升沿时计数
		}
		if (input[0] <= TargetThreshold && DetectStatus)
		{
			DetectStatus = false;
		}
	}
	return true;
}

bool RADAR_JammingEffect::Finalize()
{
	outputFile << "[" << "\r\n";
	outputFile << "\t{" << "\r\n";
	outputFile << "\t\t" << R"("Index": )" << 1 << "," << "\r\n";
	outputFile << "\t\t" << R"("Detection Hit":)" << DetectCount << "," << "\r\n";
	outputFile << "\t\t" << R"("True Target Count":)" << TargetsInPRI * DetectionNum << "," << "\r\n";

	switch (JammingType)
	{
	case RADAR_JammingEffect::CoverJamming:
		outputFile << "\t\t" << R"("Pd":)" << 1.0*DetectCount / (TargetsInPRI*DetectionNum) << "\r\n";
		break;
	case RADAR_JammingEffect::DeceptionJamming:
		outputFile << "\t\t" << R"("False Target Count":)" << FalseTargetNum * DetectionNum << "\r\n";
		break;
	default:
		break;
	}

	outputFile << "\t}" << "\r\n";
	outputFile << "]" << "\r\n";

	// Close the file
	outputFile.close();

	return true;
}
