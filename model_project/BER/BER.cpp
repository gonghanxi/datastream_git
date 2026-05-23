#include "BER.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( BER )
{	
	SET_MODEL_DESCRIPTION(" Bit Error Rate Measurement");
	SET_MODEL_SYMBOL("SYM_BER_FER");
	SET_MODEL_CATEGORY("Sinks");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(ref);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(test);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(StartStopOption, SelectedStartStopOption);
		enumParam.SetDescription("Sink collection mode: Auto, Samples, Time");
		enumParam.AddEnumeration("Auto", Auto);
		enumParam.AddEnumeration("Samples", Samples);
		enumParam.AddEnumeration("Time", Time);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleStart);
		param.SetDescription("Sample number to start data collection");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
		param.SetHideCondition("StartStopOption == 2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleStop);
		param.SetDescription("Sample number to stop data collection");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("Num_Samples - 1");
		param.SetSchematicDisplay(0);
		param.SetHideCondition("StartStopOption == 2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(TimeStart);
		param.SetDescription("Time to start data collection");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("Start_Time");
		param.SetSchematicDisplay(0);
		param.SetHideCondition("StartStopOption == 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(TimeStop);
		param.SetDescription("Time to stop data collection");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("Stop_Time");
		param.SetSchematicDisplay(0);
		param.SetHideCondition("StartStopOption == 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(StatusUpdatePeriod);
		param.SetDescription("Status update period in number of bits");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1000");
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam fName = ADD_MODEL_PARAM(FileName);
		fName.SetDescription("Data file name");
		fName.SetSchematicDisplay(0);
		fName.SetParamAsFile();
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDefaultValue("Sample_Rate");
		param.SetSchematicDisplay(0);
		param.SetHideCondition("1");
	}
	return true;
}
#endif

BER::BER()
{

}

bool BER::Setup()
{
	bool bStatus = true;

	if (SampleStart < 0)
	{
		POST_ERROR("SampleStart must be >= 0");
		bStatus = false;
	}

	if (SampleStop < SampleStart)
	{
		POST_ERROR("SampleStop must be >= SampleStart");
		bStatus = false;
	}

	if (TimeStart < 0)
	{
		POST_ERROR("TimeStart must be >= 0");
		bStatus = false;
	}

	if (TimeStop < TimeStart)
	{
		POST_ERROR("TimeStop must be >= TimeStart");
		bStatus = false;
	}

	if (StatusUpdatePeriod <= 0)
	{
		POST_ERROR("StatusUpdatePeriod must be > 0");
		bStatus = false;
	}
	if (SampleStop - SampleStart + 1 < StatusUpdatePeriod)
	{
		POST_WARNING("Not enough samples to yeild BER (SampleStop - SampleStart + 1 < StatusUpdatePeriod), the result may be empty.");
	}


	{
		switch (StartStopOption)
		{
		case BER::Auto:
			SampleStart = TimeStart * SampleRate;
			SampleStop = TimeStop * SampleRate;
			break;
		case BER::Samples:
			break;
		case BER::Time:
			SampleStart = TimeStart * SampleRate;
			SampleStop = TimeStop * SampleRate;
			break;
		default:
			break;
		}
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
			bStatus = m_control.Initialize(this, SampleStart, SampleStop);
		}

		// 写入json文件开头的中括号
		outputFile << "[" << "\r\n";
	}

	SinkIndex = 0;
	ResultIndex = 1;
	PeriodIndex = 0;
	BitErrorCount = 0;

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool BER::Run()
{
	if (m_control.CollectData()) // Check if we should still collect data
	{
		// 异或运算统计误码
		if (ref[0] ^ test[0])
		{
			BitErrorCount++;
		}
		PeriodIndex++;

		// 收集满检测周期个点后输出
		if (PeriodIndex >= StatusUpdatePeriod)
		{
			if (SinkIndex == 0)
			{

			}
			outputFile << "\t{" << "\r\n";
			outputFile << "\t\t" << R"("Index": )" << ResultIndex << "," << "\r\n";
			outputFile << "\t\t" << R"("BER":)" << 1.0*BitErrorCount / StatusUpdatePeriod << "\r\n";
			if (SinkIndex >= (SampleStop - SampleStart) - (SampleStop - SampleStart + 1) % StatusUpdatePeriod)
			{
				// 若为最后一组数据，去除多余逗号
				outputFile << "\t}" << "\r\n";
			}
			else
			{
				outputFile << "\t}," << "\r\n";
			}
			// 重置计数器
			BitErrorCount = 0;
			PeriodIndex = 0;
			// 输出索引步进
			ResultIndex++;
		}

		// 当前收集到的点的索引，每个Run计数加一
		SinkIndex++;
	}
	return true;
}

bool BER::Finalize()
{
	// 写入json文件结尾的中括号
	outputFile << "]" << "\r\n";

	// Close the file
	outputFile.close();
	return true;
}
