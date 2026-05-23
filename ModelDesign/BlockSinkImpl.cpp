#include "BlockSinkImpl.h"
#include "Buffer.h"
#include "BufferReader.h"

using namespace SystemVueModelBuilder;

bool BlockSinkImpl::ProcessAsTerminalBlock(const std::string &inputPortName)
{
    BufferReader* reader = m_block->GetInputPort(inputPortName);
    if(!reader || !reader->GetConnectedBuffer()) {
        return false;
    }

    // 获取可用数据量
    size_t availableData = reader->GetAvailableDataCount();
    if (availableData == 0) {
        return false;
    }

    qDebug() << "Block '" << QString::fromStdString(m_block->m_name) << "': Processing terminal data from port '"
              << QString::fromStdString(inputPortName) << "', available data: " << availableData;

    //设置导出文件名称以及时间（用于区分文件）
    auto ms = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch());
    m_block->SetOutputFile(std::to_string(ms.count()) + "_" + m_block->GetName() + ".json");


    // 确保输出文件已打开
    if (m_block->m_outputFilename.empty()) {
        qDebug() << "ERROR: No output filename specified for terminal BlockSinkImpl";
        return false;
    }

    if (!m_block->m_outputFile.is_open()) {
            m_block->m_outputFile.open(m_block->m_outputFilename, std::ios::app);
            if (!m_block->m_outputFile.is_open()) {
                qDebug() << "ERROR: Failed to open output file '" << QString::fromStdString(m_block->m_outputFilename) << "'";
                return false;
            }
            qDebug() << "Output file opened: " << QString::fromStdString(m_block->m_outputFilename);
        }

     // 根据数据类型直接处理并写入文件
     DataType inputDataType = m_block->m_inputPortDataTypes[inputPortName];
     bool success = false;

     switch(inputDataType) {
         case DataType::DOUBLE:
         case DataType::CIRCULAR_BUFFER_DOUBLE:
         case DataType::TIMED_DOUBLE:
             success = ProcessAndWriteDoubles(reader, availableData);
             break;
         case DataType::TIMED_DCOMPLEX:
         case DataType::CIRCULAR_BUFFER_DCOMPLEX:
         case DataType::COMPLEX_DOUBLE:
             success = ProcessAndWriteComplexDoubles(reader, availableData);
             break;
         case DataType::ENVELOPE_SIGNAL:
             success = ProcessAndWriteEnvelopeSignals(reader, availableData);
             break;
         case DataType::TIMED_INT:
         case DataType::CIRCULAR_BUFFER_INT:
         case DataType::INT:
             success = ProcessAndWriteInts(reader, availableData);
             break;
         case DataType::TIMED_FLOAT:
         case DataType::CIRCULAR_BUFFER_FLOAT:
         case DataType::FLOAT:
             success = ProcessAndWriteFloats(reader, availableData);
             break;
         case DataType::TIMED_BOOL:
         case DataType::CIRCULAR_BUFFER_BOOL:
         case DataType::BOOL:
             success = ProcessAndWriteBools(reader, availableData);
             break;
         case DataType::TIMED_FCOMPLEX:
         case DataType::CIRCULAR_BUFFER_FCOMPLEX:
         case DataType::COMPLEX_FLOAT:
             success = ProcessAndWriteComplexFloats(reader, availableData);
             break;
         default:
             qDebug() << "WARNING: Unsupported data type for terminal processing";
             break;
     }

     if (success) {
         m_block->m_outputFile.flush(); // 确保数据写入磁盘
     }

     return success;
}

bool BlockSinkImpl::IsTerminalBlock() const
{
    //终端块没有输出端
    return m_block->m_outputPorts.empty();
}

bool BlockSinkImpl::ProcessAndWriteDoubles(BufferReader *reader, size_t availableData)
{
    std::vector<double> inputData;
    // 临时调整读取大小以适应可用数据
    size_t originalReadSize = reader->GetReadSize();
    reader->SetReadSize(std::min(originalReadSize, availableData));
    bool readSuccess = reader->ReadData(inputData);
    reader->SetReadSize(originalReadSize);

     if (!readSuccess || inputData.empty()) {
         return false;
     }

     qDebug() << "Processing " << inputData.size() << " double values";
         // 根据 Sink 的运行模式处理
     for (size_t i = 0; i < inputData.size(); i++) {
         double value = inputData[i];
         double timestamp = 0.0; // 需要获取仿真软件的时间步长

         // 根据 Sink 的三种模式
         switch (m_block->m_terminalMode) {
             case TerminalMode::AUTO:
                 // Auto模式：根据时间范围
                 if (m_block->m_processedCount <= m_block->m_simuPara.samplingRate) {
                     WriteDoubleToJson(m_block->m_processedCount, value, timestamp);
                     m_block->m_processedCount++;
                 }
                 break;

             case TerminalMode::SAMPLES:
                 // Samples模式：根据采样索引范围
                 if (m_block->m_processedCount >= m_block->m_sampleStart && m_block->m_processedCount <= m_block->m_sampleStop) {
                     WriteDoubleToJson(m_block->m_processedCount, value, timestamp);
                 }
                 m_block->m_processedCount++;
                 break;

             case TerminalMode::TIME:
                 // Time模式：根据时间范围
                 if (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop) {
                     WriteDoubleToJson(m_block->m_processedCount, value, timestamp);
                     m_block->m_processedCount++;
                 }
                 break;

             default:
                 // 默认直接写入所有数据
                 WriteDoubleToJson(m_block->m_processedCount, value, timestamp);
                 m_block->m_processedCount++;
                 break;
         }
     }

     return true;
}

bool BlockSinkImpl::ProcessAndWriteComplexDoubles(BufferReader* reader, size_t availableData)
{
    std::vector<std::complex<double>> inputData;
    size_t originalReadSize = reader->GetReadSize();

    // 临时调整读取大小以适应可用数据
    reader->SetReadSize(std::min(originalReadSize, availableData));
    bool readSuccess = reader->ReadData(inputData);
    reader->SetReadSize(originalReadSize);

    if (!readSuccess || inputData.empty()) {
        return false;
    }

    qDebug() << "Processing " << inputData.size() << " complex values";

    for (size_t i = 0; i < inputData.size(); i++) {
        const std::complex<double>& value = inputData[i];
        double timestamp = 0.0; // 需要获取仿真软件的时间步长

        // 根据 Sink 的三种模式
        bool shouldWrite = false;

        switch (m_block->m_terminalMode) {
            case TerminalMode::AUTO:
            // Auto模式：根据时间范围
                shouldWrite = (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop);
                break;
            case TerminalMode::SAMPLES:
            // Samples模式：根据采样索引范围
                shouldWrite = (m_block->m_processedCount >= m_block->m_sampleStart && m_block->m_processedCount <= m_block->m_sampleStop);
                m_block->m_processedCount++;
                break;
            case TerminalMode::TIME:
            // Time模式：根据时间范围
                shouldWrite = (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop);
                if (shouldWrite) m_block->m_processedCount++;
                break;
            default:
                shouldWrite = true;
                m_block->m_processedCount++;
                break;
        }

        if (shouldWrite) {
            WriteDComplexToJson(i, value, timestamp);
        }
    }

    return true;
}

bool BlockSinkImpl::ProcessAndWriteEnvelopeSignals(BufferReader* reader, size_t availableData)
{
    std::vector<SystemVueModelBuilder::EnvelopeSignal> inputData;
//    SystemVueModelBuilder::EnvelopeCircularBuffer inputData;
    size_t originalReadSize = reader->GetReadSize();

    // 临时调整读取大小以适应可用数据
    reader->SetReadSize(std::min(originalReadSize, availableData));
    bool readSuccess = reader->ReadData(inputData);
    reader->SetReadSize(originalReadSize);

    if (!readSuccess || inputData.empty()) {
//     if (!readSuccess) {
        return false;
    }

    qDebug() << "Processing " << inputData.size() << " envelope signals";

    for (size_t i = 0; i < inputData.size(); i++) {
        const auto& envelope = inputData[i];
        double timestamp = 0.0; // 需要获取仿真软件的时间步长

        // 根据模式处理
        bool shouldWrite = false;

        switch (m_block->m_terminalMode) {
            case TerminalMode::AUTO:
            // Auto模式：根据时间范围
                shouldWrite = (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop);
                break;
            case TerminalMode::SAMPLES:
            // Samples模式：根据采样索引范围
                shouldWrite = (m_block->m_processedCount >= m_block->m_sampleStart && m_block->m_processedCount <= m_block->m_sampleStop);
                m_block->m_processedCount++;
                break;
            case TerminalMode::TIME:
            // Time模式：根据时间范围
                shouldWrite = (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop);
                if (shouldWrite) m_block->m_processedCount++;
                break;
            default:
                shouldWrite = true;
                m_block->m_processedCount++;
                break;
        }

        if (shouldWrite) {
            WriteEnvelopeSignalToJson(i, envelope, timestamp);
        }
    }

    return true;
}

bool BlockSinkImpl::ProcessAndWriteInts(BufferReader *reader, size_t availableData)
{
    std::vector<int> inputData;
     size_t originalReadSize = reader->GetReadSize();

     // 临时调整读取大小以适应可用数据
     reader->SetReadSize(std::min(originalReadSize, availableData));
     bool readSuccess = reader->ReadData(inputData);
     reader->SetReadSize(originalReadSize);

     if (!readSuccess || inputData.empty()) {
         return false;
     }

     qDebug() << "Processing " << inputData.size() << " double values";

     // 根据 Sink 的运行模式处理
     for (size_t i = 0; i < inputData.size(); i++) {
         double value = inputData[i];
         double timestamp = 0.0; // 需要获取仿真软件的时间步长

         // 模拟 Sink 的三种模式
         switch (m_block->m_terminalMode) {
             case TerminalMode::AUTO:
                 // Auto模式：根据时间范围
                 if (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop) {
                     WriteIntToJson(i, value, timestamp);
                     m_block->m_processedCount++;
                 }
                 break;

             case TerminalMode::SAMPLES:
                 // Samples模式：根据采样索引范围
                 if (m_block->m_processedCount >= m_block->m_sampleStart && m_block->m_processedCount <= m_block->m_sampleStop) {
                     WriteIntToJson(m_block->m_processedCount, value, timestamp);
                 }
                 m_block->m_processedCount++;
                 break;

             case TerminalMode::TIME:
                 // Time模式：根据时间范围
                 if (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop) {
                     WriteIntToJson(m_block->m_processedCount, value, timestamp);
                     m_block->m_processedCount++;
                 }
                 break;

             default:
                 // 默认直接写入所有数据
                 WriteIntToJson(m_block->m_processedCount, value, timestamp);
                 m_block->m_processedCount++;
                 break;
         }
     }

     return true;
}

bool BlockSinkImpl::ProcessAndWriteFloats(BufferReader *reader, size_t availableData)
{
    std::vector<float> inputData;
     size_t originalReadSize = reader->GetReadSize();

     // 临时调整读取大小以适应可用数据
     reader->SetReadSize(std::min(originalReadSize, availableData));
     bool readSuccess = reader->ReadData(inputData);
     reader->SetReadSize(originalReadSize);

     if (!readSuccess || inputData.empty()) {
         return false;
     }

     qDebug() << "Processing " << inputData.size() << " double values";

     // 根据 Sink 的运行模式处理
     for (size_t i = 0; i < inputData.size(); i++) {
         double value = inputData[i];
         double timestamp = 0.0; // 需要获取仿真软件的时间步长

         // 模拟 Sink 的三种模式
         switch (m_block->m_terminalMode) {
             case TerminalMode::AUTO:
                 // Auto模式：根据时间范围
                 if (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop) {
                     WriteFloatToJson(i, value, timestamp);
                     m_block->m_processedCount++;
                 }
                 break;

             case TerminalMode::SAMPLES:
                 // Samples模式：根据采样索引范围
                 if (m_block->m_processedCount >= m_block->m_sampleStart && m_block->m_processedCount <= m_block->m_sampleStop) {
                     WriteFloatToJson(m_block->m_processedCount, value, timestamp);
                 }
                 m_block->m_processedCount++;
                 break;

             case TerminalMode::TIME:
                 // Time模式：根据时间范围
                 if (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop) {
                     WriteFloatToJson(m_block->m_processedCount, value, timestamp);
                     m_block->m_processedCount++;
                 }
                 break;

             default:
                 // 默认直接写入所有数据
                 WriteFloatToJson(m_block->m_processedCount, value, timestamp);
                 m_block->m_processedCount++;
                 break;
         }
     }

     return true;
}

bool BlockSinkImpl::ProcessAndWriteBools(BufferReader *reader, size_t availableData)
{
    std::vector<bool> inputData;
     size_t originalReadSize = reader->GetReadSize();

     // 临时调整读取大小以适应可用数据
     reader->SetReadSize(std::min(originalReadSize, availableData));
     bool readSuccess = reader->ReadData(inputData);
     reader->SetReadSize(originalReadSize);

     if (!readSuccess || inputData.empty()) {
         return false;
     }

     qDebug() << "Processing " << inputData.size() << " double values";

     // 根据 Sink 的运行模式处理
     for (size_t i = 0; i < inputData.size(); i++) {
         double value = inputData[i];
         double timestamp = 0.0; // 需要获取仿真软件的时间步长

         // 模拟 Sink 的三种模式
         switch (m_block->m_terminalMode) {
             case TerminalMode::AUTO:
                 // Auto模式：根据时间范围
                 if (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop) {
                     WriteBoolToJson(i, value, timestamp);
                     m_block->m_processedCount++;
                 }
                 break;

             case TerminalMode::SAMPLES:
                 // Samples模式：根据采样索引范围
                 if (m_block->m_processedCount >= m_block->m_sampleStart && m_block->m_processedCount <= m_block->m_sampleStop) {
                     WriteBoolToJson(m_block->m_processedCount, value, timestamp);
                 }
                 m_block->m_processedCount++;
                 break;

             case TerminalMode::TIME:
                 // Time模式：根据时间范围
                 if (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop) {
                     WriteBoolToJson(m_block->m_processedCount, value, timestamp);
                     m_block->m_processedCount++;
                 }
                 break;

             default:
                 // 默认直接写入所有数据
                 WriteBoolToJson(m_block->m_processedCount, value, timestamp);
                 m_block->m_processedCount++;
                 break;
         }
     }

     return true;
}

bool BlockSinkImpl::ProcessAndWriteComplexFloats(BufferReader *reader, size_t availableData)
{
    std::vector<std::complex<float>> inputData;
    size_t originalReadSize = reader->GetReadSize();

    // 临时调整读取大小以适应可用数据
    reader->SetReadSize(std::min(originalReadSize, availableData));
    bool readSuccess = reader->ReadData(inputData);
    reader->SetReadSize(originalReadSize);

    if (!readSuccess || inputData.empty()) {
        return false;
    }

    qDebug() << "Processing " << inputData.size() << " complex values";

    for (size_t i = 0; i < inputData.size(); i++) {
        std::complex<float>& value = inputData[i];
        double timestamp = 0.0; // 需要获取仿真软件的时间步长

        // 根据模式处理
        bool shouldWrite = false;

        switch (m_block->m_terminalMode) {
            case TerminalMode::AUTO:
            // Auto模式：根据时间范围
                shouldWrite = (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop);
                break;
            case TerminalMode::SAMPLES:
            // Samples模式：根据采样索引范围
                shouldWrite = (m_block->m_processedCount >= m_block->m_sampleStart && m_block->m_processedCount <= m_block->m_sampleStop);
                m_block->m_processedCount++;
                break;
            case TerminalMode::TIME:
            // Time模式：根据时间范围
                shouldWrite = (timestamp >= m_block->m_timeStart && timestamp <= m_block->m_timeStop);
                if (shouldWrite) m_block->m_processedCount++;
                break;
            default:
                shouldWrite = true;
                m_block->m_processedCount++;
                break;
        }

        if (shouldWrite) {
            WriteFComplexToJson(i, value, timestamp);
        }
    }

    return true;
}

void BlockSinkImpl::WriteDoubleToJson(size_t index, double value, double timestamp)
{
    std::lock_guard<std::mutex> lock(m_block->m_fileMutex);
    //第一个参数：索引
    m_block->m_outputFile << "    {" << std::endl;
    m_block->m_outputFile << "        " << R"("Index": )" << index << "," << std::endl;
    //第二个参数：根据模式不同
    //Samples模式下为Sink_Index
    //Time模式下为Sink_Time
    //自动模式下根据数据类型是普通Circularbuffer还是时域TimedCircularbuffer，参数不同
    if (m_block->m_terminalMode == TerminalMode::SAMPLES) {
        m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::TIME){
        m_block->m_outputFile << "        " << R"("Sink_Time": )"
                     << std::setprecision(4) << timestamp << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::AUTO) {
        if(m_block->m_dataType == DataType::DOUBLE || m_block->m_dataType == DataType::CIRCULAR_BUFFER_DOUBLE) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
        }
        else if(m_block->m_dataType == DataType::TIMED_DOUBLE) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << m_block->m_samplingRate << "," << std::endl;
            m_block->m_samplingRate += m_block->m_samplingRateIncrement;
        }
    }

    m_block->m_outputFile << "        " << R"("Sink_Data": )"
                 << std::setprecision(4) << value << std::endl;
    m_block->m_outputFile << "    }," << std::endl;
}

void BlockSinkImpl::WriteDComplexToJson(size_t index, std::complex<double> value, double timestamp)
{
    std::lock_guard<std::mutex> lock(m_block->m_fileMutex);
    //第一个参数：索引
    m_block->m_outputFile << "    {" << std::endl;
    m_block->m_outputFile << "        " << R"("Index": )" << index << "," << std::endl;
    //第二个参数：根据模式不同
    //Samples模式下为Sink_Index
    //Time模式下为Sink_Time
    //自动模式下根据数据类型是普通Circularbuffer还是时域TimedCircularbuffer，参数不同
    if (m_block->m_terminalMode == TerminalMode::SAMPLES) {
        m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::TIME){
        m_block->m_outputFile << "        " << R"("Sink_Time": )"
                     << std::setiosflags(std::ios::scientific)
                     << std::setprecision(4) << timestamp << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::AUTO) {
        if(m_block->m_dataType == DataType::COMPLEX_DOUBLE || m_block->m_dataType == DataType::CIRCULAR_BUFFER_DCOMPLEX) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
        }
        else if(m_block->m_dataType == DataType::TIMED_DCOMPLEX) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << m_block->m_samplingRate << "," << std::endl;
            m_block->m_samplingRate += m_block->m_samplingRateIncrement;
        }
    }

    m_block->m_outputFile << "        " << R"("Sink_Data_Real": )"
                 << std::setiosflags(std::ios::fixed)
                 << std::setprecision(4) << value.real() << "," << std::endl;
    m_block->m_outputFile << "        " << R"("Sink_Data_Imag": )"
                 << std::setiosflags(std::ios::fixed)
                 << std::setprecision(4) << value.imag() << std::endl;
    m_block->m_outputFile << "    }," << std::endl;
}

void BlockSinkImpl::WriteFComplexToJson(size_t index, std::complex<float> value, double timestamp)
{
    std::lock_guard<std::mutex> lock(m_block->m_fileMutex);
    //第一个参数：索引
    m_block->m_outputFile << "    {" << std::endl;
    m_block->m_outputFile << "        " << R"("Index": )" << index << "," << std::endl;
    //第二个参数：根据模式不同
    //Samples模式下为Sink_Index
    //Time模式下为Sink_Time
    //自动模式下根据数据类型是普通Circularbuffer还是时域TimedCircularbuffer，参数不同
    if (m_block->m_terminalMode == TerminalMode::SAMPLES) {
        m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::TIME){
        m_block->m_outputFile << "        " << R"("Sink_Time": )"
                     << std::setiosflags(std::ios::scientific)
                     << std::setprecision(4) << timestamp << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::AUTO) {
        if(m_block->m_dataType == DataType::COMPLEX_FLOAT || m_block->m_dataType == DataType::CIRCULAR_BUFFER_FCOMPLEX) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
        }
        else if(m_block->m_dataType == DataType::TIMED_FCOMPLEX) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << m_block->m_samplingRate << "," << std::endl;
            m_block->m_samplingRate += m_block->m_samplingRateIncrement;
        }
    }

    m_block->m_outputFile << "        " << R"("Sink_Data_Real": )"
                 << std::setiosflags(std::ios::fixed)
                 << std::setprecision(4) << value.real() << "," << std::endl;
    m_block->m_outputFile << "        " << R"("Sink_Data_Imag": )"
                 << std::setiosflags(std::ios::fixed)
                 << std::setprecision(4) << value.imag() << std::endl;
    m_block->m_outputFile << "    }," << std::endl;
}

void BlockSinkImpl::WriteEnvelopeSignalToJson(size_t index, const SystemVueModelBuilder::EnvelopeSignal& envelope, double timestamp)
{
    std::lock_guard<std::mutex> lock(m_block->m_fileMutex);

    auto complexValue = envelope.complex();
    //第一个参数：索引
    m_block->m_outputFile << "    {" << std::endl;
    m_block->m_outputFile << "        " << R"("Index": )" << index << "," << std::endl;
    //第二个参数：根据模式不同
    //Samples模式下为Sink_Index
    //Time模式下为Sink_Time
    //自动模式下根据数据类型是普通Circularbuffer还是时域TimedCircularbuffer，参数不同
    if (m_block->m_terminalMode == TerminalMode::SAMPLES) {
        m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::TIME) {
        m_block->m_outputFile << "        " << R"("Sink_Time": )"
                     << std::setiosflags(std::ios::scientific)
                     << std::setprecision(4) << timestamp << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::AUTO) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << m_block->m_samplingRate << "," << std::endl;
            m_block->m_samplingRate += m_block->m_samplingRateIncrement;
    }

    m_block->m_outputFile << "        " << R"("Sink_Data_Real": )"
                 << std::setiosflags(std::ios::fixed)
                 << std::setprecision(4) << complexValue.real() << "," << std::endl;
    m_block->m_outputFile << "        " << R"("Sink_Data_Imag": )"
                 << std::setiosflags(std::ios::fixed)
                 << std::setprecision(4) << complexValue.imag() << "," << std::endl;
    m_block->m_outputFile << "    }," << std::endl;
}

void BlockSinkImpl::WriteIntToJson(size_t index, int value, double timestamp)
{
    std::lock_guard<std::mutex> lock(m_block->m_fileMutex);
    //第一个参数：索引
    m_block->m_outputFile << "    {" << std::endl;
    m_block->m_outputFile << "        " << R"("Index": )" << index << "," << std::endl;
    //第二个参数：根据模式不同
    //Samples模式下为Sink_Index
    //Time模式下为Sink_Time
    //自动模式下根据数据类型是普通Circularbuffer还是时域TimedCircularbuffer，参数不同
    if (m_block->m_terminalMode == TerminalMode::SAMPLES) {
        m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::TIME){
        m_block->m_outputFile << "        " << R"("Sink_Time": )"
                     << std::setiosflags(std::ios::scientific)
                     << std::setprecision(4) << timestamp << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::AUTO) {
        if(m_block->m_dataType == DataType::INT || m_block->m_dataType == DataType::CIRCULAR_BUFFER_INT) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
        }
        else if(m_block->m_dataType == DataType::TIMED_INT) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << m_block->m_samplingRate << "," << std::endl;
            m_block->m_samplingRate += m_block->m_samplingRateIncrement;
        }
    }

    m_block->m_outputFile << "        " << R"("Sink_Data": )"
                 << std::setprecision(4) << value << std::endl;
    m_block->m_outputFile << "    }," << std::endl;
}

void BlockSinkImpl::WriteFloatToJson(size_t index, float value, double timestamp)
{
    std::lock_guard<std::mutex> lock(m_block->m_fileMutex);
    //第一个参数：索引
    m_block->m_outputFile << "    {" << std::endl;
    m_block->m_outputFile << "        " << R"("Index": )" << index << "," << std::endl;
    //第二个参数：根据模式不同
    //Samples模式下为Sink_Index
    //Time模式下为Sink_Time
    //自动模式下根据数据类型是普通Circularbuffer还是时域TimedCircularbuffer，参数不同
    if (m_block->m_terminalMode == TerminalMode::SAMPLES) {
        m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::TIME){
        m_block->m_outputFile << "        " << R"("Sink_Time": )"
                     << std::setiosflags(std::ios::scientific)
                     << std::setprecision(4) << timestamp << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::AUTO) {
        if(m_block->m_dataType == DataType::FLOAT || m_block->m_dataType == DataType::CIRCULAR_BUFFER_FLOAT) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
        }
        else if(m_block->m_dataType == DataType::TIMED_FLOAT) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << m_block->m_samplingRate << "," << std::endl;
            m_block->m_samplingRate += m_block->m_samplingRateIncrement;
        }
    }

    m_block->m_outputFile << "        " << R"("Sink_Data": )"
                 << std::setprecision(4) << value << std::endl;
    m_block->m_outputFile << "    }," << std::endl;
}

void BlockSinkImpl::WriteBoolToJson(size_t index, bool value, double timestamp)
{
    std::lock_guard<std::mutex> lock(m_block->m_fileMutex);
    //第一个参数：索引
    m_block->m_outputFile << "    {" << std::endl;
    m_block->m_outputFile << "        " << R"("Index": )" << index << "," << std::endl;
    //第二个参数：根据模式不同
    //Samples模式下为Sink_Index
    //Time模式下为Sink_Time
    //自动模式下根据数据类型是普通Circularbuffer还是时域TimedCircularbuffer，参数不同
    if (m_block->m_terminalMode == TerminalMode::SAMPLES) {
        m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::TIME){
        m_block->m_outputFile << "        " << R"("Sink_Time": )"
                     << std::setiosflags(std::ios::scientific)
                     << std::setprecision(4) << timestamp << "," << std::endl;
    } else if(m_block->m_terminalMode == TerminalMode::AUTO) {
        if(m_block->m_dataType == DataType::BOOL || m_block->m_dataType == DataType::CIRCULAR_BUFFER_BOOL) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << index << "," << std::endl;
        }
        else if(m_block->m_dataType == DataType::TIMED_BOOL) {
            m_block->m_outputFile << "        " << R"("Sink_Index": )" << m_block->m_samplingRate << "," << std::endl;
            m_block->m_samplingRate += m_block->m_samplingRateIncrement;
        }
    }

    m_block->m_outputFile << "        " << R"("Sink_Data": )"
                 << std::setprecision(4) << value << std::endl;
    m_block->m_outputFile << "    }," << std::endl;
}
