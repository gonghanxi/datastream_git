#include "SinkEnv.h"

// Buffer size to speed up writing of data
#define FILEWRITER_BUFFER_SIZE 1000000

SinkEnv::SinkEnv()
{
	Index = 1;
	FileName = 0;
	m_pdBuffer = 0;
	m_iBuffer = 0;
}

SinkEnv::~SinkEnv()
{
	// delete the buffer
	delete[] m_pdBuffer;
}

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( SinkEnv )
{	
	SET_MODEL_DESCRIPTION("Data Sink");
	SET_MODEL_SYMBOL("SYM_Sink");
	SET_MODEL_CATEGORY("Sinks");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
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

bool SinkEnv::Setup()
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

	{
		switch (StartStopOption)
		{
		case SinkEnv::Auto:
			SampleStart = TimeStart * SampleRate;
			SampleStop = TimeStop * SampleRate;
			break;
		case SinkEnv::Samples:
			break;
		case SinkEnv::Time:
			SampleStart = TimeStart * SampleRate;
			SampleStop = TimeStop * SampleRate;
			break;
		default:
			break;
		}

		// Windows limits files to be 2 gigabytes in size 2^31 = 1 << 31
		unsigned long iMaxCollect = (unsigned(1) << 31) / sizeof(std::complex<double>);
		if (SampleStop - SampleStart + 1 > iMaxCollect)
		{
			char str[128];
			sprintf(str, "SampleStop - SampleStart + 1  must be less than or equal to %d", iMaxCollect);
			POST_ERROR(str);
			bStatus = false;
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

		if (bStatus)
		{
			// If all OK - now we initialize the write buffer
			m_pdBuffer = new std::complex<double>[FILEWRITER_BUFFER_SIZE];
			m_iBuffer = 0;
		}

		if (bStatus)
		{
			// json文件预处理：写入开头的中括号
			outputFile << "[" << "\r\n";
		}
	}

	return bStatus;
}

ERESULT SinkEnv::PropagateCharacterizationFrequency()
{
	double Fc = input.GetCharacterizationFrequency();

	outputFile << "\t{" << "\r\n";
	outputFile << "\t\t" << R"("Fc": )" << Fc << "\r\n";
	outputFile << "\t}," << "\r\n";
	return true;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool SinkEnv::Run()
{
	if (m_control.CollectData()) // Check if we should still collect data
	{
		// Write data into buffer
		m_pdBuffer[m_iBuffer++] = input[0].complex();

		// If buffer is full, write it out to disk
		if (m_iBuffer == FILEWRITER_BUFFER_SIZE)
		{
			m_iBuffer = 0;
			for (int i = 0; i < FILEWRITER_BUFFER_SIZE; i++)
			{
				outputFile << "\t{" << "\r\n";
				outputFile << "\t\t" << R"("Index": )" << Index << "," << "\r\n";
				switch (StartStopOption)
				{
				case SinkEnv::Auto:
					outputFile << "\t\t" << R"("Sink_Time": )" << TimeStart + (Index - 1) / SampleRate << "," << "\r\n";
					break;
				case SinkEnv::Samples:
					outputFile << "\t\t" << R"("Sink_Index": )" << SampleStart + Index - 1 << "," << "\r\n";
					break;
				case SinkEnv::Time:
					outputFile << "\t\t" << R"("Sink_Time": )" << TimeStart + (Index - 1) / SampleRate << "," << "\r\n";
					break;
				default:
					break;
				}
				outputFile << "\t\t" << R"("(re)Sink_Data": )" << m_pdBuffer[i].real() << "\r\n";
				outputFile << "\t\t" << R"("(im)Sink_Data": )" << m_pdBuffer[i].imag() << "\r\n";
				outputFile << "\t}," << "\r\n";

				Index++;
			}
		}
	}
	return true;
}

bool SinkEnv::Finalize()
{
	// Flush the rest of the buffer to disk
	for (int i = 0; i < m_iBuffer; i++)
	{
		outputFile << "\t{" << "\r\n";
		outputFile << "\t\t" << R"("Index": )" << Index << "," << "\r\n";
		switch (StartStopOption)
		{
		case SinkEnv::Auto:
			outputFile << "\t\t" << R"("Sink_Time": )" << TimeStart + (Index - 1) / SampleRate << "," << "\r\n";
			break;
		case SinkEnv::Samples:
			outputFile << "\t\t" << R"("Sink_Index": )" << SampleStart + Index - 1 << "," << "\r\n";
			break;
		case SinkEnv::Time:
			outputFile << "\t\t" << R"("Sink_Time": )" << TimeStart + (Index - 1) / SampleRate << "," << "\r\n";
			break;
		default:
			break;
		}
		outputFile << "\t\t" << R"("(re)Sink_Data": )" << m_pdBuffer[i].real() << "," << "\r\n";
		outputFile << "\t\t" << R"("(im)Sink_Data": )" << m_pdBuffer[i].imag() << "\r\n";
		outputFile << "\t}," << "\r\n";

		Index++;
	}

	// json后处理：删除多余逗号并补全末尾中括号
	outputFile.seekp(-4, std::ios::end);	// 如果是LF换行则填-4
	outputFile << "\r\n" << "]" << "\r\n";

	// Delete the buffer
	delete[] m_pdBuffer;
	m_pdBuffer = 0;

	// Close the file
	outputFile.close();
	return true;
}
