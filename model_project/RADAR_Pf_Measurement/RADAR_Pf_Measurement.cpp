#include "RADAR_Pf_Measurement.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_Pf_Measurement )
{	
	SET_MODEL_DESCRIPTION("False Alarm Rate Estimation");
	SET_MODEL_CATEGORY("Measurement");

	ADD_MODEL_INPUT(input);

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Start);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetDescription("Data collection start index");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Stop);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1000");
		param.SetDescription("Data collection stop index when EstRelVariance is not met");
	}

	{
		SystemVueModelBuilder::DFParam fName = ADD_MODEL_PARAM(FileName);
		fName.SetDescription("Data file name");
		fName.SetSchematicDisplay(0);
		fName.SetParamAsFile();
	}

	//{
	//	SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(ControlSimulation, SelectedControlSimulation);
	//	enumParam.AddEnumeration("NO", NO);
	//	enumParam.AddEnumeration("YES", YES);
	//	enumParam.SetDefaultValue("1");
	//}
	return true;
}
#endif

RADAR_Pf_Measurement::RADAR_Pf_Measurement()
{
	
}

bool RADAR_Pf_Measurement::Setup()
{
	bool bStatus = true;

	if (Start < 0)
	{
		POST_ERROR("Start must be >= 0.");
		bStatus = false;
	}
	if (Stop <= Start)
	{
		POST_ERROR("Stop must be > Start.");
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
			bStatus = m_control.Initialize(this, Start, Stop);
		}
	}
	FalseCount = 0;

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_Pf_Measurement::Run()
{
	if (m_control.CollectData()) // Check if we should still collect data
	{
		if (input[0] > 0)
		{
			FalseCount++;
		}
	}

	return true;
}

bool RADAR_Pf_Measurement::Finalize()
{
	outputFile << "[" << "\r\n";
	outputFile << "\t{" << "\r\n";
	outputFile << "\t\t" << R"("Index": )" << 1 << "," << "\r\n";
	outputFile << "\t\t" << R"("Pf":)" << 1.0*FalseCount / (Stop - Start) << "\r\n";
	outputFile << "\t}" << "\r\n";
	outputFile << "]" << "\r\n";

	// Close the file
	outputFile.close();

	return true;
}
