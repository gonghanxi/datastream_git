#include "BufferReaderDataReadImpl.h"

using namespace SystemVueModelBuilder;

bool BufferReaderDataReadImpl::ReadData(int &outputData)
{
    if (!m_reader->m_connectedBuffer) {
        LOG_ERROR(" Buffer not connected for reader: " , m_reader->m_name);
        return false;
    }

    // 检查上游是否完成且无数据
    if (m_reader->IsUpstreamDone() && m_reader->GetAvailableDataCount() == 0) {
        m_reader->SetDownstreamDone(true);  // 标记下游完成
        return false;
    }

    //调用输出端的方法读取
    bool success = m_reader->m_connectedBuffer->ReadDataForReader(outputData, m_reader->m_name);

    if (success) {
    } else {
        LOG_ERROR("BufferReader '",m_reader->m_name, "': Read failed");
    }

    return success;
}

bool BufferReaderDataReadImpl::ReadData(double &outputData)
{
    if (!m_reader->m_connectedBuffer) {
        LOG_ERROR(" Buffer not connected for reader: " , m_reader->m_name);
        return false;
    }

    // 检查上游是否完成且无数据
    if (m_reader->IsUpstreamDone() && m_reader->GetAvailableDataCount() == 0) {
        m_reader->SetDownstreamDone(true);  // 标记下游完成
        return false;
    }

    //调用输出端的方法读取
    bool success = m_reader->m_connectedBuffer->ReadDataForReader(outputData, m_reader->m_name);

    if (success) {
    } else {
        LOG_ERROR("BufferReader '",m_reader->m_name, "': Read failed");
    }

    return success;
}

bool BufferReaderDataReadImpl::ReadData(float &outputData)
{
    if (!m_reader->m_connectedBuffer) {
        LOG_ERROR(" Buffer not connected for reader: " , m_reader->m_name);
        return false;
    }

    // 检查上游是否完成且无数据
    if (m_reader->IsUpstreamDone() && m_reader->GetAvailableDataCount() == 0) {
        m_reader->SetDownstreamDone(true);  // 标记下游完成
        return false;
    }

    //调用输出端的方法读取
    bool success = m_reader->m_connectedBuffer->ReadDataForReader(outputData, m_reader->m_name);

    if (success) {
    } else {
        LOG_ERROR("BufferReader '",m_reader->m_name, "': Read failed");
    }

    return success;
}

bool BufferReaderDataReadImpl::ReadData(bool &outputData)
{
    if (!m_reader->m_connectedBuffer) {
        LOG_ERROR(" Buffer not connected for reader: " , m_reader->m_name);
        return false;
    }

    // 检查上游是否完成且无数据
    if (m_reader->IsUpstreamDone() && m_reader->GetAvailableDataCount() == 0) {
        m_reader->SetDownstreamDone(true);  // 标记下游完成
        return false;
    }

    //调用输出端的方法读取
    bool success = m_reader->m_connectedBuffer->ReadDataForReader(outputData, m_reader->m_name);

    if (success) {
    } else {
        LOG_ERROR("BufferReader '",m_reader->m_name, "': Read failed");
    }

    return success;
}

bool BufferReaderDataReadImpl::ReadData(std::complex<float> &outputData)
{
    if (!m_reader->m_connectedBuffer) {
        LOG_ERROR(" Buffer not connected for reader: " , m_reader->m_name);
        return false;
    }

    // 检查上游是否完成且无数据
    if (m_reader->IsUpstreamDone() && m_reader->GetAvailableDataCount() == 0) {
        m_reader->SetDownstreamDone(true);  // 标记下游完成
        return false;
    }

    //调用输出端的方法读取
    bool success = m_reader->m_connectedBuffer->ReadDataForReader(outputData, m_reader->m_name);

    if (success) {
    } else {
        LOG_ERROR("BufferReader '",m_reader->m_name, "': Read failed");
    }

    return success;
}

bool BufferReaderDataReadImpl::ReadData(std::complex<double> &outputData)
{
    if (!m_reader->m_connectedBuffer) {
        LOG_ERROR(" Buffer not connected for reader: " , m_reader->m_name);
        return false;
    }

    // 检查上游是否完成且无数据
    if (m_reader->IsUpstreamDone() && m_reader->GetAvailableDataCount() == 0) {
        m_reader->SetDownstreamDone(true);  // 标记下游完成
        return false;
    }

//    if (!m_reader->HasDataAvailable()) {
//        return false;
//    }

    //调用输出端的方法读取
    bool success = m_reader->m_connectedBuffer->ReadDataForReader(outputData, m_reader->m_name);

    if (success) {
    } else {
        LOG_ERROR("BufferReader '",m_reader->m_name, "': Read failed");
    }

    return success;
}

bool BufferReaderDataReadImpl::ReadData(std::vector<int> &outputData)
{
//    qDebug() << "=== BufferReader::ReadData (int) ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name)
//              << ", ReaderType: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));
    // 根据读取器类型选择读取方式
     if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
//         qDebug() << "Bridge reader reading directly";
         return m_reader->ReadTypedData(outputData);
     }
     else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
//         qDebug() << "Master bus reader collecting from connections";
         return ReadBusData(m_reader->m_readSize, outputData);
     }
     else {
         // 标准读取器
//         qDebug() << "Standard reader";
         return m_reader->ReadTypedData(outputData);
     }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<double> &outputData)
{
//    qDebug() << "=== BufferReader::ReadData (double) ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name)
//              << ", ReaderType: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));
    // 根据读取器类型选择读取方式
     if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
//         qDebug() << "Bridge reader reading directly";
         return m_reader->ReadTypedData(outputData);
     }
     else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
//         qDebug() << "Master bus reader collecting from connections";
         return ReadBusData(m_reader->m_readSize, outputData);
     }
     else {
         // 标准读取器
//         qDebug() << "Standard reader";
         return m_reader->ReadTypedData(outputData);
     }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<float> &outputData)
{
//    qDebug() << "=== BufferReader::ReadData (float) ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name)
//              << ", ReaderType: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));
    // 根据读取器类型选择读取方式
     if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
//         qDebug() << "Bridge reader reading directly";
         return m_reader->ReadTypedData(outputData);
     }
     else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
//         qDebug() << "Master bus reader collecting from connections";
         return ReadBusData(m_reader->m_readSize, outputData);
     }
     else {
         // 标准读取器
//         qDebug() << "Standard reader";
         return m_reader->ReadTypedData(outputData);
     }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<bool> &outputData)
{
//    qDebug() << "=== BufferReader::ReadData (bool) ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name)
//              << ", ReaderType: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));
    // 根据读取器类型选择读取方式
     if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
//         qDebug() << "Bridge reader reading directly";
         return m_reader->ReadTypedData(outputData);
     }
     else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
//         qDebug() << "Master bus reader collecting from connections";
         return ReadBusData(m_reader->m_readSize, outputData);
     }
     else {
         // 标准读取器
//         qDebug() << "Standard reader";
         return m_reader->ReadTypedData(outputData);
     }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<std::complex<float> > &outputData)
{
//    qDebug() << "=== BufferReader::ReadData (complex float) ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name)
//              << ", ReaderType: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));
    // 根据读取器类型选择读取方式
     if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
//         qDebug() << "Bridge reader reading directly";
         return m_reader->ReadTypedData(outputData);
     }
     else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
//         qDebug() << "Master bus reader collecting from connections";
         return ReadBusData(m_reader->m_readSize, outputData);
     }
     else {
         // 标准读取器
//         qDebug() << "Standard reader";
         return m_reader->ReadTypedData(outputData);
     }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<std::complex<double> > &outputData)
{
//    qDebug() << "=== BufferReader::ReadData (complex double) ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name)
//              << ", ReaderType: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));
    // 根据读取器类型选择读取方式
     if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
//         qDebug() << "Bridge reader reading directly";
         return m_reader->ReadTypedData(outputData);
     }
     else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
//         qDebug() << "Master bus reader collecting from connections";
         return ReadBusData(m_reader->m_readSize, outputData);
     }
     else {
         // 标准读取器
//         qDebug() << "Standard reader";
         return m_reader->ReadTypedData(outputData);
     }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<int *> &outputData)
{
    return m_reader->ReadTypedData(outputData);
}

bool BufferReaderDataReadImpl::ReadData(std::vector<double *> &outputData)
{
    return m_reader->ReadTypedData(outputData);
}

bool BufferReaderDataReadImpl::ReadData(std::vector<std::complex<double> *> &outputData)
{
    return m_reader->ReadTypedData(outputData);
}

bool BufferReaderDataReadImpl::ReadData(SystemVueModelBuilder::CircularBufferBase &outputData)
{
    if (!m_reader->m_connectedBuffer) {
        qDebug() << "BufferReader '" << QString::fromStdString(m_reader->m_name) << "': Read failed - not connected to buffer";
        return false;
    }

//    if (!m_reader->HasDataAvailable()) {
//        qDebug() << "BufferReader '" << QString::fromStdString(m_reader->m_name) << "': No data available";
//        return false;
//    }

    bool success = m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);

    if (success) {
//        qDebug() << "BufferReader '" << QString::fromStdString(m_reader->m_name) << "': Successfully read CircularBufferBase data";
    } else {
//        qDebug() << "BufferReader '" << QString::fromStdString(m_reader->m_name) << "': Read CircularBufferBase failed";

    }

    return success;
}



bool BufferReaderDataReadImpl::ReadData(std::vector<EnvelopeSignal> &outputData)
{
//    qDebug() << "=== EnvelopeSignal::ReadData enter ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name)
//              << ", ReaderType: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));

    // 读取时，从上游的buffer获取其中的表征频率
    // 从Buffer获取表征频率
    if (m_reader->propagateCharacterizationFrequencyFromBuffer()) {
//        qDebug() << "从上游获取到表征频率: "
//                 << m_reader->getCharacterizationFrequency();
    }


    // 根据读取器类型选择读取方式
     if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
//         qDebug() << "Bridge reader reading directly";
         return m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);
     }
     else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
//         qDebug() << "Master bus reader collecting from connections";
         return ReadBusData(m_reader->m_readSize, outputData);
     }
     else {
         // 标准读取器
//         qDebug() << "Standard reader";
         return m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);
     }
}

bool BufferReaderDataReadImpl::ReadData(EnvelopeCircularBuffer &outputData)
{
//    qDebug() << "=== EnvelopeCircularBuffer::ReadData enter ===";
    if (!m_reader->m_connectedBuffer) {
        qDebug() << "ERROR: Buffer not connected";
        return false;
    }

    // 从Buffer获取表征频率
    if (m_reader->propagateCharacterizationFrequencyFromBuffer()) {
//        qDebug() << "Got characterization frequency from buffer: "
//                  << m_reader->getCharacterizationFrequency();
    }

//    if (!m_reader->HasDataAvailable()) {
//        qDebug() << "BufferReader '" << QString::fromStdString(m_reader->m_name) << "': No data available";
//        return false;
//    }

    // 根据读取器类型选择读取方式
     if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
//         qDebug() << "Bridge reader reading directly";
         return m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);
     }
     else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
//         qDebug() << "Master bus reader collecting from connections";
         return ReadBusData(m_reader->m_readSize, outputData);
     }
     else {
         // 标准读取器
//         qDebug() << "Standard reader";
         return m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);
     }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<IntMatrix> &outputData)
{
    // 根据读取器类型选择读取方式
    if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
        return m_reader->ReadTypedData(outputData);
    }
    else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
        return ReadBusData(m_reader->m_readSize, outputData);
    }
    else {
        // 标准读取器
        return m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);
    }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<DoubleMatrix> &outputData)
{
    // 根据读取器类型选择读取方式
    if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
        return m_reader->ReadTypedData(outputData);
    }
    else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
        return ReadBusData(m_reader->m_readSize, outputData);
    }
    else {
        // 标准读取器
        return m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);
    }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<FloatMatrix> &outputData)
{
    // 根据读取器类型选择读取方式
    if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
        return m_reader->ReadTypedData(outputData);
    }
    else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
        return ReadBusData(m_reader->m_readSize, outputData);
    }
    else {
        // 标准读取器
        return m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);
    }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<BoolMatrix> &outputData)
{
    // 根据读取器类型选择读取方式
    if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
        return m_reader->ReadTypedData(outputData);
    }
    else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
        return ReadBusData(m_reader->m_readSize, outputData);
    }
    else {
        // 标准读取器
        return m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);
    }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<FComplexMatrix> &outputData)
{
    // 根据读取器类型选择读取方式
    if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
        return m_reader->ReadTypedData(outputData);
    }
    else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
        return ReadBusData(m_reader->m_readSize, outputData);
    }
    else {
        // 标准读取器
        return m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);
    }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<DComplexMatrix> &outputData)
{
    // 根据读取器类型选择读取方式
    if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
        return m_reader->ReadTypedData(outputData);
    }
    else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
        return ReadBusData(m_reader->m_readSize, outputData);
    }
    else {
        // 标准读取器
        return m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);
    }
}

bool BufferReaderDataReadImpl::ReadData(std::vector<EnvelopeMatrix> &outputData)
{
    // 根据读取器类型选择读取方式
    if (m_reader->m_readerType == m_reader->BUS_BRIDGE) {
        return m_reader->ReadTypedData(outputData);
    }
    else if (m_reader->m_readerType == m_reader->BUS_MASTER) {
        return ReadBusData(m_reader->m_readSize, outputData);
    }
    else {
        // 标准读取器
        return m_reader->m_connectedBuffer->ReadDataForReader(m_reader->m_readSize, outputData, m_reader->m_name);
    }
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<int> &outputData)
{
//    qDebug() << "=== BufferReader::ReadBusData ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name) << ", Type: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));

    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }

    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }

    outputData.clear();
    if(readSize != 0) {

    }

    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
//        qDebug() << "Processing connection " << i;

        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }

        // 关键：桥接读取器应该直接读取，不检查总线连接
//        qDebug() << "Calling bridge reader: " << QString::fromStdString(connection.bridgeReader->GetName());

        std::vector<int> connectionData;

        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);

        if (success && !connectionData.empty()) {
//            qDebug() << "Read " << connectionData.size()
//                      << " samples from bridge reader";

            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
//                qDebug() << "Added sample: " << connectionData[0]
//                         << " from connection " << i;
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }

    bool success = !outputData.empty();
//    qDebug() << "ReadBusData completed: " << (success ? "success" : "failed")
//              << ", samples read: " << outputData.size();

    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<double> &outputData)
{
//    qDebug() << "=== BufferReader::ReadBusData ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name) << ", Type: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));

    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }

    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }

    outputData.clear();
    if(readSize != 0) {

    }

    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
//        qDebug() << "Processing connection " << i;

        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }

        // 关键：桥接读取器应该直接读取，不检查总线连接
//        qDebug() << "Calling bridge reader: " << QString::fromStdString(connection.bridgeReader->GetName());

        std::vector<double> connectionData;

        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);

        if (success && !connectionData.empty()) {
//            qDebug() << "Read " << connectionData.size()
//                      << " samples from bridge reader";

            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
//                qDebug() << "Added sample: " << connectionData[0]
//                         << " from connection " << i;
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }

    bool success = !outputData.empty();
//    qDebug() << "ReadBusData completed: " << (success ? "success" : "failed")
//              << ", samples read: " << outputData.size();

    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<float> &outputData)
{
//    qDebug() << "=== BufferReader::ReadBusData ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name) << ", Type: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));

    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }

    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }

    outputData.clear();
    if(readSize != 0) {

    }

    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
//        qDebug() << "Processing connection " << i;

        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }

        // 关键：桥接读取器应该直接读取，不检查总线连接
//        qDebug() << "Calling bridge reader: " << QString::fromStdString(connection.bridgeReader->GetName());

        std::vector<float> connectionData;

        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);

        if (success && !connectionData.empty()) {
//            qDebug() << "Read " << connectionData.size()
//                      << " samples from bridge reader";

            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
//                qDebug() << "Added sample: " << connectionData[0]
//                         << " from connection " << i;
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }

    bool success = !outputData.empty();
//    qDebug() << "ReadBusData completed: " << (success ? "success" : "failed")
//              << ", samples read: " << outputData.size();

    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<bool> &outputData)
{
//    qDebug() << "=== BufferReader::ReadBusData ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name) << ", Type: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));

    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }

    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }

    outputData.clear();
    if(readSize != 0) {

    }

    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
//        qDebug() << "Processing connection " << i;

        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }

        // 关键：桥接读取器应该直接读取，不检查总线连接
//        qDebug() << "Calling bridge reader: " << QString::fromStdString(connection.bridgeReader->GetName());

        std::vector<bool> connectionData;

        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);

        if (success && !connectionData.empty()) {
//            qDebug() << "Read " << connectionData.size()
//                      << " samples from bridge reader";

            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
//                qDebug() << "Added sample: " << connectionData[0]
//                         << " from connection " << i;
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }

    bool success = !outputData.empty();
//    qDebug() << "ReadBusData completed: " << (success ? "success" : "failed")
//              << ", samples read: " << outputData.size();

    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<std::complex<double>>& outputData)
{
//    qDebug() << "=== BufferReader::ReadBusData ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name) << ", Type: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));

    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }

    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }

    outputData.clear();
    if(readSize != 0) {

    }

    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
//        qDebug() << "Processing connection " << i;

        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }

        // 关键：桥接读取器应该直接读取，不检查总线连接
//        qDebug() << "Calling bridge reader: " << QString::fromStdString(connection.bridgeReader->GetName());

        std::vector<std::complex<double>> connectionData;

        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);

        if (success && !connectionData.empty()) {
//            qDebug() << "Read " << connectionData.size()
//                      << " samples from bridge reader";

            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
//                qDebug() << "Added sample:  (" << connectionData[i].real() << "," << connectionData[i].imag()
//                         << ") from connection " << i;
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }

    bool success = !outputData.empty();
//    qDebug() << "ReadBusData completed: " << (success ? "success" : "failed")
//              << ", samples read: " << outputData.size();

    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<std::complex<float> > &outputData)
{
//    qDebug() << "=== BufferReader::ReadBusData ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name) << ", Type: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));

    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }

    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }

    outputData.clear();
    if(readSize != 0) {

    }

    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
//        qDebug() << "Processing connection " << i;

        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }

        // 关键：桥接读取器应该直接读取，不检查总线连接
//        qDebug() << "Calling bridge reader: " << QString::fromStdString(connection.bridgeReader->GetName());

        std::vector<std::complex<float>> connectionData;

        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);

        if (success && !connectionData.empty()) {
//            qDebug() << "Read " << connectionData.size()
//                      << " samples from bridge reader";

            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
//                qDebug() << "Added sample:  (" << connectionData[i].real() << "," << connectionData[i].imag()
//                         << ") from connection " << i;
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }

    bool success = !outputData.empty();
//    qDebug() << "ReadBusData completed: " << (success ? "success" : "failed")
//              << ", samples read: " << outputData.size();

    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<EnvelopeSignal> &outputData)
{
//    qDebug() << "=== BufferReader::ReadBusData ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name) << ", Type: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));

    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }

    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }

    outputData.clear();
    if(readSize != 0) {

    }

    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
//        qDebug() << "Processing connection " << i;

        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }

        // 关键：桥接读取器应该直接读取，不检查总线连接
//        qDebug() << "Calling bridge reader: " << QString::fromStdString(connection.bridgeReader->GetName());

        std::vector<SystemVueModelBuilder::EnvelopeSignal> connectionData;

        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);

        if (success && !connectionData.empty()) {
//            qDebug() << "Read " << connectionData.size()
//                      << " samples from bridge reader";

            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
//                qDebug() << "Added sample:  (" << connectionData[i].real() << "," << connectionData[i].imag()
//                         << ") from connection " << i;
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }

    bool success = !outputData.empty();
//    qDebug() << "ReadBusData completed: " << (success ? "success" : "failed")
//              << ", samples read: " << outputData.size();

    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, EnvelopeCircularBuffer &outputData)
{
//    qDebug() << "=== BufferReader::ReadBusData ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_reader->m_name) << ", Type: " << QString::fromStdString(m_reader->ReaderTypeToString(m_reader->m_readerType));

    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }

    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }

    outputData.Zero(0,outputData.GetSize(),nullptr);
    if(readSize != 0) {

    }

    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
//        qDebug() << "Processing connection " << i;

        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }

        // 关键：桥接读取器应该直接读取，不检查总线连接
//        qDebug() << "Calling bridge reader: " << QString::fromStdString(connection.bridgeReader->GetName());

        SystemVueModelBuilder::EnvelopeCircularBuffer connectionData;

        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);

        if (success && connectionData.GetSize() != 0) {
//            qDebug() << "Read " << connectionData.GetSize()
//                      << " samples from bridge reader";

            // 取数据
            for(size_t i = 0; i < connectionData.GetSize(); i++) {
                outputData[0] = connectionData[0];
//                qDebug() << "Added sample:  (" << connectionData[0].real() << "," << connectionData[0].imag()
//                         << ") from connection " << i;
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }

    bool success = (outputData.GetSize() != 0);
//    qDebug() << "ReadBusData completed: " << (success ? "success" : "failed")
//              << ", samples read: " << outputData.GetSize();

    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<IntMatrix> &outputData)
{
    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }
    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }
    outputData.clear();
    if(readSize != 0) {

    }
    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }
        std::vector<IntMatrix> connectionData;
        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);
        if (success && !connectionData.empty()) {
            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }
    bool success = !outputData.empty();
    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<DoubleMatrix> &outputData)
{
    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }
    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }
    outputData.clear();
    if(readSize != 0) {

    }
    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }
        std::vector<DoubleMatrix> connectionData;
        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);
        if (success && !connectionData.empty()) {
            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }
    bool success = !outputData.empty();
    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<FloatMatrix> &outputData)
{
    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }
    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }
    outputData.clear();
    if(readSize != 0) {

    }
    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }
        std::vector<FloatMatrix> connectionData;
        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);
        if (success && !connectionData.empty()) {
            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }
    bool success = !outputData.empty();
    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<BoolMatrix> &outputData)
{
    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }
    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }
    outputData.clear();
    if(readSize != 0) {

    }
    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }
        std::vector<BoolMatrix> connectionData;
        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);
        if (success && !connectionData.empty()) {
            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }
    bool success = !outputData.empty();
    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<FComplexMatrix> &outputData)
{
    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }
    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }
    outputData.clear();
    if(readSize != 0) {

    }
    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }
        std::vector<FComplexMatrix> connectionData;
        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);
        if (success && !connectionData.empty()) {
            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }
    bool success = !outputData.empty();
    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<DComplexMatrix> &outputData)
{
    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }
    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }
    outputData.clear();
    if(readSize != 0) {

    }
    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }
        std::vector<DComplexMatrix> connectionData;
        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);
        if (success && !connectionData.empty()) {
            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }
    bool success = !outputData.empty();
    return success;
}

bool BufferReaderDataReadImpl::ReadBusData(size_t readSize, std::vector<EnvelopeMatrix> &outputData)
{
    if (m_reader->m_readerType != m_reader->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return false;
    }
    if (m_reader->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return false;
    }
    outputData.clear();
    if(readSize != 0) {

    }
    // 从每个连接读取数据
    for (size_t i = 0; i < m_reader->m_busConnections.size(); ++i) {
        const auto& connection = m_reader->m_busConnections[i];
        if (!connection.bridgeReader) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }
        std::vector<EnvelopeMatrix> connectionData;
        // 直接调用读取方法，让桥接读取器自己处理
        bool success = connection.bridgeReader->ReadData(connectionData);
        if (success && !connectionData.empty()) {
            // 取数据
            for(size_t i = 0; i < connectionData.size(); i++) {
                outputData.push_back(connectionData[i]);
            }
        } else {
            qDebug() << "WARNING: Failed to read data from bridge reader";
            // 即使一个连接失败，也继续处理其他连接
        }
    }
    bool success = !outputData.empty();
    return success;
}
