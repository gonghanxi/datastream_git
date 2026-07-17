#include "BufferReadImpl.h"
#include "BufferReader.h"
#include "DataTypesAndParsers.h"

using namespace SystemVueModelBuilder;

bool BufferReadImpl::ReadDataForReader(int &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, 1)) {
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }

    // 从专用存储获取值
    auto externalPorts = m_buffer->GetExternalIntPorts();

    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取存储的值
        outputData = it->second;  // 现在是值，不是引用

        // 检查读取器位置
        if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
            qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
            return false;
        }

        size_t& readerPosition = m_buffer->m_readerPositions[readerName];
        readerPosition += 1;
        m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;

        return true;
    }

    return false;
}

bool BufferReadImpl::ReadDataForReader(double &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, 1)) {
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }

    // 从专用存储获取 double 值
    auto externalPorts = m_buffer->GetExternalDoublePorts();

    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取存储的 double 值
        outputData = it->second;  // 现在是值，不是引用

        // 检查读取器位置
        if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
            qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
            return false;
        }

        size_t& readerPosition = m_buffer->m_readerPositions[readerName];
        readerPosition += 1;
        m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;

        return true;
    }

    return false;
}

bool BufferReadImpl::ReadDataForReader(float &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, 1)) {
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }

    // 从专用存储获取值
    auto externalPorts = m_buffer->GetExternalFloatPorts();

    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取存储的值
        outputData = it->second;  // 现在是值，不是引用

        // 检查读取器位置
        if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
            qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
            return false;
        }

        size_t& readerPosition = m_buffer->m_readerPositions[readerName];
        readerPosition += 1;
        m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;

        return true;
    }

    return false;
}

bool BufferReadImpl::ReadDataForReader(bool &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, 1)) {
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }

    // 从专用存储获取值
    auto externalPorts = m_buffer->GetExternalBoolPorts();

    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取存储的值
        outputData = it->second;  // 现在是值，不是引用

        // 检查读取器位置
        if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
            qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
            return false;
        }

        size_t& readerPosition = m_buffer->m_readerPositions[readerName];
        readerPosition += 1;
        m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;

        return true;
    }

    return false;
}

bool BufferReadImpl::ReadDataForReader(std::complex<float> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, 1)) {
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }

    // 从专用存储获取值
    auto externalPorts = m_buffer->GetExternalFComplexPorts();

    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取存储的值
        outputData = it->second;  // 现在是值，不是引用

        // 检查读取器位置
        if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
            qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
            return false;
        }

        size_t& readerPosition = m_buffer->m_readerPositions[readerName];
        readerPosition += 1;
        m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;

        return true;
    }

    return false;
}

bool BufferReadImpl::ReadDataForReader(std::complex<double> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, 1)) {
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }

    // 从专用存储获取值
    auto externalPorts = m_buffer->GetExternalDComplexPorts();

    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取存储的值
        outputData = it->second;  // 现在是值，不是引用

        // 检查读取器位置
        if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
            qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
            return false;
        }

        size_t& readerPosition = m_buffer->m_readerPositions[readerName];
        readerPosition += 1;
        m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;

        return true;
    }

    return false;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<int> &outputData, const std::string &readerName)
{

    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }

    // 执行读取
    bool success = ReadIntDataForReaderImpl(readSize, outputData, readerName);

    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();

    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<double> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }

    bool success = ReadDoubleDataForReaderImpl(readSize, outputData, readerName);

    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();

    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<float> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    bool success =   ReadFloatDataForReaderImpl(readSize, outputData, readerName);

    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();

    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<bool> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    bool success =  ReadBoolDataForReaderImpl(readSize, outputData, readerName);

    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();

    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<std::complex<float> > &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }

    bool success =  ReadFComplexDataForReaderImpl(readSize, outputData, readerName);

    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();

    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<std::complex<double>>& outputData, const std::string& readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) {
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }

    bool success = ReadDComplexDataForReaderImpl(readSize, outputData, readerName);

    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();

    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<int *> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    std::vector<int> tempData;
    if(!ReadIntDataForReaderImpl(readSize, tempData, readerName)) {
        return false;
    }

    for(auto ptr : outputData) {
        delete ptr;
    }
    outputData.clear();
    outputData.reserve(tempData.size());
    for(const auto& value : tempData) {
        int* ptr = new int[value];
        outputData.push_back(ptr);
    }
    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();
    return true;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<double *> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    std::vector<double> tempData;
    if(!ReadDoubleDataForReaderImpl(readSize, tempData, readerName)) {
        return false;
    }

    for(auto ptr : outputData) {
        delete ptr;
    }
    outputData.clear();
    outputData.reserve(tempData.size());
    for(const auto& value : tempData) {
        double* ptr = new double(value);
        outputData.push_back(ptr);
    }
    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();
    return true;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<std::complex<double> *> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    std::vector<std::complex<double>> tempData;
    if(!ReadDComplexDataForReaderImpl(readSize, tempData, readerName)) {
        return false;
    }

    for(auto ptr : outputData) {
        delete ptr;
    }
    outputData.clear();
    outputData.reserve(tempData.size());
    for(const auto& value : tempData) {
        std::complex<double>* ptr = new std::complex<double>(value);
        outputData.push_back(ptr);
    }
    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();
    return true;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, SystemVueModelBuilder::CircularBufferBase &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    //根据不同类型获取不同读取指针
    auto* intBuffer = dynamic_cast<SystemVueModelBuilder::IntCircularBuffer*>(&outputData);
    if(intBuffer) {
        std::vector<int> tempData;
        if(ReadIntDataForReaderImpl(readSize, tempData, readerName)) {
            if(tempData.size() <= intBuffer->GetSize()) {
                for(size_t i = 0; i < tempData.size(); i++) {
                    (*intBuffer)[i] = tempData[i];
                }
                AutoRestoreIfPossible();
                return true;
            }
        }
        return false;
    }
    //根据不同类型获取不同读取指针
    auto* doubleBuffer = dynamic_cast<SystemVueModelBuilder::DoubleCircularBuffer*>(&outputData);
    if(doubleBuffer) {
        std::vector<double> tempData;
        if(ReadDoubleDataForReaderImpl(readSize, tempData, readerName)) {
            if(tempData.size() <= doubleBuffer->GetSize()) {
                for(size_t i = 0; i < tempData.size(); i++) {
                    (*doubleBuffer)[i] = tempData[i];
                }
                AutoRestoreIfPossible();
                return true;
            }
        }
        return false;
    }
    //根据不同类型获取不同读取指针
    auto* floatBuffer = dynamic_cast<SystemVueModelBuilder::FloatCircularBuffer*>(&outputData);
    if(floatBuffer) {
        std::vector<float> tempData;
        if(ReadFloatDataForReaderImpl(readSize, tempData, readerName)) {
            if(tempData.size() <= floatBuffer->GetSize()) {
                for(size_t i = 0; i < tempData.size(); i++) {
                    (*floatBuffer)[i] = tempData[i];
                }
                AutoRestoreIfPossible();
                return true;
            }
        }
        return false;
    }
    //根据不同类型获取不同读取指针
    auto* boolBuffer = dynamic_cast<SystemVueModelBuilder::BoolCircularBuffer*>(&outputData);
    if(boolBuffer) {
        std::vector<bool> tempData;
        if(ReadBoolDataForReaderImpl(readSize, tempData, readerName)) {
            if(tempData.size() <= boolBuffer->GetSize()) {
                for(size_t i = 0; i < tempData.size(); i++) {
                    (*boolBuffer)[i] = tempData[i];
                }
                AutoRestoreIfPossible();
                return true;
            }
        }
        return false;
    }
    //根据不同类型获取不同读取指针
    auto* dcomplexBuffer = dynamic_cast<SystemVueModelBuilder::DComplexCircularBuffer*>(&outputData);
    if(dcomplexBuffer) {
        std::vector<std::complex<double>> tempData;
        if(ReadDComplexDataForReaderImpl(readSize, tempData, readerName)) {
            if(tempData.size() <= dcomplexBuffer->GetSize()) {
                for(size_t i = 0; i < tempData.size(); i++) {
                    (*dcomplexBuffer)[i] = tempData[i];
                }
                AutoRestoreIfPossible();
                return true;
            }
        }
        return false;
    }
    //根据不同类型获取不同读取指针
    auto* fcomplexBuffer = dynamic_cast<SystemVueModelBuilder::FComplexCircularBuffer*>(&outputData);
    if(fcomplexBuffer) {
        std::vector<std::complex<float>> tempData;
        if(ReadFComplexDataForReaderImpl(readSize, tempData, readerName)) {
            if(tempData.size() <= fcomplexBuffer->GetSize()) {
                for(size_t i = 0; i < tempData.size(); i++) {
                    (*fcomplexBuffer)[i] = tempData[i];
                }
                AutoRestoreIfPossible();
                return true;
            }
        }
        return false;
    }
    qDebug() << "WARNING: Buffer '" << QString::fromStdString(m_buffer->m_name) << "': Unknown CircularBufferBase type for reader '"
              << QString::fromStdString(readerName) << "'";
    return false;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<EnvelopeSignal> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    bool success = ReadEnvelopeSignalDataForReaderImpl(readSize, outputData, readerName);

    AutoRestoreIfPossible();

    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, SystemVueModelBuilder::EnvelopeCircularBuffer &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    bool success =  ReadEnvelopeCircularBufferDataForReaderImpl(readSize, outputData, readerName);
    AutoRestoreIfPossible();
    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<IntMatrix> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    // 执行读取
    bool success = ReadIntMatrixDataForReaderImpl(readSize, outputData, readerName);
    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();
    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<DoubleMatrix> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    // 执行读取
    bool success = ReadDoubleMatrixDataForReaderImpl(readSize, outputData, readerName);
    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();
    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<FloatMatrix> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    // 执行读取
    bool success = ReadFloatMatrixDataForReaderImpl(readSize, outputData, readerName);
    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();
    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<BoolMatrix> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    // 执行读取
    bool success = ReadBoolMatrixDataForReaderImpl(readSize, outputData, readerName);
    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();
    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<FComplexMatrix> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    // 执行读取
    bool success = ReadFComplexMatrixDataForReaderImpl(readSize, outputData, readerName);
    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();
    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<DComplexMatrix> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    // 执行读取
    bool success = ReadDComplexMatrixDataForReaderImpl(readSize, outputData, readerName);
    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();
    return success;
}

bool BufferReadImpl::ReadDataForReader(size_t readSize, std::vector<EnvelopeMatrix> &outputData, const std::string &readerName)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(0, readSize)) { // 写入需求为0，只检查读取
        qDebug() << "ERROR: Cannot expand buffer for read operation";
        return false;
    }
    // 执行读取
    bool success = ReadEnvelopeMatrixDataForReaderImpl(readSize, outputData, readerName);
    // 读取后检查是否可以恢复
    AutoRestoreIfPossible();
    return success;
}

bool BufferReadImpl::ReadIntDataForReaderImpl(size_t readSize, std::vector<int>& outputData, const std::string& readerName)
{
    //获取Buffer指针
    SystemVueModelBuilder::IntCircularBuffer* buffer = m_buffer->getIntCircularBuffer();
    if(!buffer) {
        //Int与Bool类型兼容
        SystemVueModelBuilder::BoolCircularBuffer* Compatiblebuffer = m_buffer->getBoolCircularBuffer();
        return ReadDataForReaderImpl<bool,int>(readSize,outputData,readerName,Compatiblebuffer);
    }
    return ReadDataForReaderImpl<int,int>(readSize,outputData,readerName,buffer);
}

bool BufferReadImpl::ReadDoubleDataForReaderImpl(size_t readSize, std::vector<double>& outputData, const std::string& readerName)
{
    //获取Buffer指针
    SystemVueModelBuilder::DoubleCircularBuffer* buffer = m_buffer->getDoubleCircularBuffer();
    if(!buffer) {
        //Int与Double类型兼容
        SystemVueModelBuilder::IntCircularBuffer* Compatiblebuffer = m_buffer->getIntCircularBuffer();
        return ReadDataForReaderImpl<int,double>(readSize,outputData,readerName,Compatiblebuffer);
    }
    return ReadDataForReaderImpl<double,double>(readSize,outputData,readerName,buffer);
}

bool BufferReadImpl::ReadFloatDataForReaderImpl(size_t readSize, std::vector<float> &outputData, const std::string &readerName)
{
    //获取Buffer指针
    SystemVueModelBuilder::FloatCircularBuffer* buffer = m_buffer->getFloatCircularBuffer();
    if(!buffer) {
        //Int与Float类型兼容
        SystemVueModelBuilder::IntCircularBuffer* Compatiblebuffer = m_buffer->getIntCircularBuffer();
        return ReadDataForReaderImpl<int,float>(readSize,outputData,readerName,Compatiblebuffer);
    }
    return ReadDataForReaderImpl<float,float>(readSize,outputData,readerName,buffer);
}

bool BufferReadImpl::ReadBoolDataForReaderImpl(size_t readSize, std::vector<bool> &outputData, const std::string &readerName)
{
    //获取Buffer指针
    SystemVueModelBuilder::BoolCircularBuffer* buffer = m_buffer->getBoolCircularBuffer();
    if(!buffer) {
        //Int与Bool类型兼容
        SystemVueModelBuilder::IntCircularBuffer* Compatiblebuffer = m_buffer->getIntCircularBuffer();
        return ReadDataForReaderImpl<int,bool>(readSize,outputData,readerName,Compatiblebuffer);
    }
    return ReadDataForReaderImpl<bool,bool>(readSize,outputData,readerName,buffer);
}

bool BufferReadImpl::ReadFComplexDataForReaderImpl(size_t readSize, std::vector<std::complex<float> > &outputData, const std::string &readerName)
{
    //获取Buffer指针
    SystemVueModelBuilder::FComplexCircularBuffer* buffer = m_buffer->getFComplexCircularBuffer();
    if(!buffer) {
        if(m_buffer->GetDataType() == DataType::CIRCULAR_BUFFER_INT) {
            //Int与FComplex类型兼容
            SystemVueModelBuilder::IntCircularBuffer* Compatiblebuffer = m_buffer->getIntCircularBuffer();
            return ReadDataForReaderImpl<int,std::complex<float>>(readSize,outputData,readerName,Compatiblebuffer);
        }
        else if(m_buffer->GetDataType() == DataType::CIRCULAR_BUFFER_DOUBLE) {
            //double与FComplex类型兼容
            SystemVueModelBuilder::DoubleCircularBuffer* Compatiblebuffer = m_buffer->getDoubleCircularBuffer();
            return ReadDataForReaderImpl<double,std::complex<float>>(readSize,outputData,readerName,Compatiblebuffer);
        }
        else if(m_buffer->GetDataType() == DataType::CIRCULAR_BUFFER_FLOAT) {
            //float与FComplex类型兼容
            SystemVueModelBuilder::FloatCircularBuffer* Compatiblebuffer = m_buffer->getFloatCircularBuffer();
            return ReadDataForReaderImpl<float,std::complex<float>>(readSize,outputData,readerName,Compatiblebuffer);
        }
        LOG_ERROR("get circularbuffer ptr error");
        return false;
    }
    return ReadDataForReaderImpl<std::complex<float>,std::complex<float>>(readSize,outputData,readerName,buffer);
}

bool BufferReadImpl::ReadDComplexDataForReaderImpl(size_t readSize, std::vector<std::complex<double> > &outputData, const std::string &readerName)
{
    //获取Buffer指针
    SystemVueModelBuilder::DComplexCircularBuffer* buffer = m_buffer->getDComplexCircularBuffer();
    if(!buffer) {
        if(m_buffer->GetDataType() == DataType::CIRCULAR_BUFFER_INT) {
            //Int与DComplex类型兼容
            SystemVueModelBuilder::IntCircularBuffer* Compatiblebuffer = m_buffer->getIntCircularBuffer();
            return ReadDataForReaderImpl<int,std::complex<double>>(readSize,outputData,readerName,Compatiblebuffer);
        }
        else if(m_buffer->GetDataType() == DataType::CIRCULAR_BUFFER_DOUBLE) {
            //double与DComplex类型兼容
            SystemVueModelBuilder::DoubleCircularBuffer* Compatiblebuffer = m_buffer->getDoubleCircularBuffer();
            return ReadDataForReaderImpl<double,std::complex<double>>(readSize,outputData,readerName,Compatiblebuffer);
        }
        else if(m_buffer->GetDataType() == DataType::CIRCULAR_BUFFER_FLOAT) {
            //float与DComplex类型兼容
            SystemVueModelBuilder::FloatCircularBuffer* Compatiblebuffer = m_buffer->getFloatCircularBuffer();
            return ReadDataForReaderImpl<float,std::complex<double>>(readSize,outputData,readerName,Compatiblebuffer);
        }
        LOG_ERROR("get circularbuffer ptr error");
        return false;
    }
    return ReadDataForReaderImpl<std::complex<double>,std::complex<double>>(readSize,outputData,readerName,buffer);
}

bool BufferReadImpl::ReadEnvelopeSignalDataForReaderImpl(size_t readSize, std::vector<EnvelopeSignal> &outputData, const std::string &readerName)
{
    //获取Buffer指针
    auto* buffer = m_buffer->getEnvelopeCircularBuffer();
    if(!buffer) {
        if(m_buffer->GetDataType() == DataType::CIRCULAR_BUFFER_INT) {
            //Int与Envelope类型兼容
            SystemVueModelBuilder::IntCircularBuffer* Compatiblebuffer = m_buffer->getIntCircularBuffer();
            return ReadDataForReaderImpl<int,EnvelopeSignal>(readSize,outputData,readerName,Compatiblebuffer);
        }
        else if(m_buffer->GetDataType() == DataType::CIRCULAR_BUFFER_DOUBLE) {
            //double与DComplex类型兼容
            SystemVueModelBuilder::DoubleCircularBuffer* Compatiblebuffer = m_buffer->getDoubleCircularBuffer();
            return ReadDataForReaderImpl<double,EnvelopeSignal>(readSize,outputData,readerName,Compatiblebuffer);
        }
        else if(m_buffer->GetDataType() == DataType::CIRCULAR_BUFFER_FLOAT) {
            //float与DComplex类型兼容
            SystemVueModelBuilder::FloatCircularBuffer* Compatiblebuffer = m_buffer->getFloatCircularBuffer();
            return ReadDataForReaderImpl<float,EnvelopeSignal>(readSize,outputData,readerName,Compatiblebuffer);
        }
        LOG_ERROR("get circularbuffer ptr error");
        return false;
    }
    return ReadDataForReaderImpl<EnvelopeSignal,EnvelopeSignal>(readSize,outputData,readerName,buffer);
}

bool BufferReadImpl::ReadEnvelopeCircularBufferDataForReaderImpl(size_t readSize, EnvelopeCircularBuffer &outputData, const std::string &readerName)
{
    // 获取 CircularBuffer
    auto* buffer = m_buffer->getEnvelopeCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get complex double buffer for EnvelopeSignal";
        return false;
    }

    // 检查读取器是否注册
    if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
        qDebug() << "Buffer '" << QString::fromStdString(m_buffer->m_name) << "': Reader '" << QString::fromStdString(readerName) << "' not registered";
        return false;
    }

    size_t& readerPos = m_buffer->m_readerPositions[readerName];
    size_t available = m_buffer->m_totalWritten - readerPos;

    // 调整实际读取大小
    size_t actualReadSize = std::min(readSize, available);
    if (actualReadSize == 0) {
        qDebug() << "No data available to read";
        return false;
    }

    outputData.ResizeMemory(1,true);

    try {
        for (size_t i = 0; i < actualReadSize; i++) {
            size_t readIndex = (readerPos + i) % m_buffer->m_bufferSize;
            // 从缓冲区读取复数数据
            SystemVueModelBuilder::EnvelopeSignal complexValue = (*buffer)[readIndex];

            // 转换为 EnvelopeSignal
            SystemVueModelBuilder::EnvelopeSignal envSignal(complexValue);
            outputData[i] = envSignal;
        }
        readerPos += actualReadSize;

        // 验证和修正reader位置
        if (readerPos > m_buffer->m_totalWritten) {
            qDebug() << "WARNING: Correcting reader position from " << readerPos
                      << " to " << m_buffer->m_totalWritten;
            readerPos = m_buffer->m_totalWritten;
        }
    } catch (const std::exception& e) {
        qDebug() << "ERROR reading EnvelopeSignal data: " << e.what();
        return false;
    }

    return true;
}

bool BufferReadImpl::ReadIntMatrixDataForReaderImpl(size_t readSize, std::vector<IntMatrix> &outputData, const std::string &readerName)
{
     // 获取内部矩阵缓冲区指针
     auto* buffer = m_buffer->getIntMatrixCircularBuffer();
     if(!buffer) {
         //Matrix Bool与Matrix Int兼容
         if(m_buffer->GetDataType() == DataType::MATRIX_BOOL) {
             BoolMatrixCircularBuffer* Compatiblebuffer = m_buffer->getBoolMatrixCircularBuffer();
             if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
                 qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
                 return false;
             }

             size_t& readerPosition = m_buffer->m_readerPositions[readerName];

             // 检查reader位置有效性
             if (readerPosition == SIZE_MAX) {
                 qDebug() << "ERROR: Invalid reader position for '" << QString::fromStdString(readerName) << "'";
                 readerPosition = 0;
                 return false;
             }

             //获取可用数据量
             size_t available = m_buffer->m_totalWritten - readerPosition;

             //时间驱动与数据流驱动 区别
             //1.时间驱动每次读取实际数据量
             //2.数据流驱动每次读取读指针数据量

             if(m_buffer->IsVariableMode()) {
                 // 读取矩阵数据
                 outputData.resize(available);
                 for (size_t i = 0; i < available; i++) {
                     size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                     BoolMatrix ReadData = (*Compatiblebuffer)[readIndex];
                     IntMatrix OutputValue(ReadData.NumRows(),ReadData.NumColumns());
                     for (size_t row = 0; row < ReadData.NumRows(); ++row) {
                         for (size_t col = 0; col < ReadData.NumColumns(); ++col) {
                             // 从 IntMatrix 读取整数，转换为 double 后存入 DoubleMatrix
                             OutputValue(row, col) = static_cast<double>(ReadData(row, col));
                         }
                     }
                     outputData[i] = OutputValue;
                 }
                 // 更新读指针位置
                 readerPosition += available;
                 m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
                 return true;
             }
             else {
                 // 读取矩阵数据
                 outputData.resize(readSize);
                 for (size_t i = 0; i < readSize; i++) {
                     size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                     BoolMatrix ReadData = (*Compatiblebuffer)[readIndex];
                     IntMatrix OutputValue(ReadData.NumRows(),ReadData.NumColumns());
                     for (size_t row = 0; row < ReadData.NumRows(); ++row) {
                         for (size_t col = 0; col < ReadData.NumColumns(); ++col) {
                             // 从 IntMatrix 读取整数，转换为 double 后存入 DoubleMatrix
                             OutputValue(row, col) = static_cast<double>(ReadData(row, col));
                         }
                     }
                     outputData[i] = OutputValue;
                 }
                 // 更新读指针位置
                 readerPosition += readSize;
                 m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
                 return true;
             }

         }
         LOG_ERROR("get circularbuffer ptr error");
         return false;
     }
     return ReadDataForReaderImpl<IntMatrix, IntMatrix>(readSize, outputData, readerName, buffer);
}

bool BufferReadImpl::ReadDoubleMatrixDataForReaderImpl(size_t readSize, std::vector<DoubleMatrix> &outputData, const std::string &readerName)
{
     // 获取内部矩阵缓冲区指针
     auto* buffer = m_buffer->getDoubleMatrixCircularBuffer();
     if(!buffer) {
         //Matrix Int与Matrix Double兼容
         if(m_buffer->GetDataType() == DataType::MATRIX_INT) {
             IntMatrixCircularBuffer* Compatiblebuffer = m_buffer->getIntMatrixCircularBuffer();
             if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
                 qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
                 return false;
             }

             size_t& readerPosition = m_buffer->m_readerPositions[readerName];

             // 检查reader位置有效性
             if (readerPosition == SIZE_MAX) {
                 qDebug() << "ERROR: Invalid reader position for '" << QString::fromStdString(readerName) << "'";
                 readerPosition = 0;
                 return false;
             }
             //获取可用数据量
             size_t available = m_buffer->m_totalWritten - readerPosition;

             //时间驱动与数据流驱动 区别
             //1.时间驱动每次读取实际数据量
             //2.数据流驱动每次读取读指针数据量

             if(m_buffer->IsVariableMode()) {
                 // 读取矩阵数据
                 outputData.resize(available);
                 for (size_t i = 0; i < available; i++) {
                     size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                     IntMatrix ReadData = (*Compatiblebuffer)[readIndex];
                     DoubleMatrix OutputValue(ReadData.NumRows(),ReadData.NumColumns());
                     for (size_t row = 0; row < ReadData.NumRows(); ++row) {
                         for (size_t col = 0; col < ReadData.NumColumns(); ++col) {
                             // 从 IntMatrix 读取整数，转换为 double 后存入 DoubleMatrix
                             OutputValue(row, col) = static_cast<double>(ReadData(row, col));
                         }
                     }
                     outputData[i] = OutputValue;
                 }
                 // 更新读指针位置
                 readerPosition += available;
                 m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
                 return true;
             }
             else {
                 // 读取矩阵数据
                 outputData.resize(readSize);
                 for (size_t i = 0; i < readSize; i++) {
                     size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                     IntMatrix ReadData = (*Compatiblebuffer)[readIndex];
                     DoubleMatrix OutputValue(ReadData.NumRows(),ReadData.NumColumns());
                     for (size_t row = 0; row < ReadData.NumRows(); ++row) {
                         for (size_t col = 0; col < ReadData.NumColumns(); ++col) {
                             // 从 IntMatrix 读取整数，转换为 double 后存入 DoubleMatrix
                             OutputValue(row, col) = static_cast<double>(ReadData(row, col));
                         }
                     }
                     outputData[i] = OutputValue;
                 }
                 // 更新读指针位置
                 readerPosition += readSize;
                 m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
                 return true;
             }

         }
         LOG_ERROR("get circularbuffer ptr error");
         return false;
     }
     return ReadDataForReaderImpl<DoubleMatrix, DoubleMatrix>(readSize, outputData, readerName, buffer);
}

bool BufferReadImpl::ReadFloatMatrixDataForReaderImpl(size_t readSize, std::vector<FloatMatrix> &outputData, const std::string &readerName)
{
     // 获取内部矩阵缓冲区指针
     auto* buffer = m_buffer->getFloatMatrixCircularBuffer();
     if(!buffer) {
         //Matrix Int与Matrix Float兼容
         if(m_buffer->GetDataType() == DataType::MATRIX_INT) {
             IntMatrixCircularBuffer* Compatiblebuffer = m_buffer->getIntMatrixCircularBuffer();
             if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
                 qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
                 return false;
             }

             size_t& readerPosition = m_buffer->m_readerPositions[readerName];

             // 检查reader位置有效性
             if (readerPosition == SIZE_MAX) {
                 qDebug() << "ERROR: Invalid reader position for '" << QString::fromStdString(readerName) << "'";
                 readerPosition = 0;
                 return false;
             }
             //获取可用数据量
             size_t available = m_buffer->m_totalWritten - readerPosition;

             //时间驱动与数据流驱动 区别
             //1.时间驱动每次读取实际数据量
             //2.数据流驱动每次读取读指针数据量

             if(m_buffer->IsVariableMode()) {
                 // 读取矩阵数据
                 outputData.resize(available);
                 for (size_t i = 0; i < available; i++) {
                     size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                     IntMatrix ReadData = (*Compatiblebuffer)[readIndex];
                     FloatMatrix OutputValue(ReadData.NumRows(),ReadData.NumColumns());
                     for (size_t row = 0; row < ReadData.NumRows(); ++row) {
                         for (size_t col = 0; col < ReadData.NumColumns(); ++col) {
                             OutputValue(row, col) = static_cast<double>(ReadData(row, col));
                         }
                     }
                     outputData[i] = OutputValue;
                 }
                 // 更新读指针位置
                 readerPosition += available;
                 m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
                 return true;
             }
             else {
                 // 读取矩阵数据
                 outputData.resize(readSize);
                 for (size_t i = 0; i < readSize; i++) {
                     size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                     IntMatrix ReadData = (*Compatiblebuffer)[readIndex];
                     FloatMatrix OutputValue(ReadData.NumRows(),ReadData.NumColumns());
                     for (size_t row = 0; row < ReadData.NumRows(); ++row) {
                         for (size_t col = 0; col < ReadData.NumColumns(); ++col) {
                             OutputValue(row, col) = static_cast<double>(ReadData(row, col));
                         }
                     }
                     outputData[i] = OutputValue;
                 }
                 // 更新读指针位置
                 readerPosition += readSize;
                 m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
                 return true;
             }


         }
         LOG_ERROR("get circularbuffer ptr error");
         return false;
     }

     return ReadDataForReaderImpl<FloatMatrix, FloatMatrix>(readSize, outputData, readerName, buffer);
}

bool BufferReadImpl::ReadBoolMatrixDataForReaderImpl(size_t readSize, std::vector<BoolMatrix> &outputData, const std::string &readerName)
{
     // 获取内部矩阵缓冲区指针
     auto* buffer = m_buffer->getBoolMatrixCircularBuffer();
     if(!buffer) {
         //Matrix Int与Matrix Bool兼容
         if(m_buffer->GetDataType() == DataType::MATRIX_INT) {
             IntMatrixCircularBuffer* Compatiblebuffer = m_buffer->getIntMatrixCircularBuffer();
             if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
                 qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
                 return false;
             }

             size_t& readerPosition = m_buffer->m_readerPositions[readerName];

             // 检查reader位置有效性
             if (readerPosition == SIZE_MAX) {
                 qDebug() << "ERROR: Invalid reader position for '" << QString::fromStdString(readerName) << "'";
                 readerPosition = 0;
                 return false;
             }

             //获取可用数据量
             size_t available = m_buffer->m_totalWritten - readerPosition;

             //时间驱动与数据流驱动 区别
             //1.时间驱动每次读取实际数据量
             //2.数据流驱动每次读取读指针数据量

             if(m_buffer->IsVariableMode()) {
                 // 读取矩阵数据
                 outputData.resize(available);
                 for (size_t i = 0; i < available; i++) {
                     size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                     IntMatrix ReadData = (*Compatiblebuffer)[readIndex];
                     BoolMatrix OutputValue(ReadData.NumRows(),ReadData.NumColumns());
                     for (size_t row = 0; row < ReadData.NumRows(); ++row) {
                         for (size_t col = 0; col < ReadData.NumColumns(); ++col) {
                             OutputValue(row, col) = static_cast<double>(ReadData(row, col));
                         }
                     }
                     outputData[i] = OutputValue;
                 }
                 // 更新读指针位置
                 readerPosition += available;
                 m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
                 return true;
             }
             else {
                 // 读取矩阵数据
                 outputData.resize(readSize);
                 for (size_t i = 0; i < readSize; i++) {
                     size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                     IntMatrix ReadData = (*Compatiblebuffer)[readIndex];
                     BoolMatrix OutputValue(ReadData.NumRows(),ReadData.NumColumns());
                     for (size_t row = 0; row < ReadData.NumRows(); ++row) {
                         for (size_t col = 0; col < ReadData.NumColumns(); ++col) {
                             OutputValue(row, col) = static_cast<double>(ReadData(row, col));
                         }
                     }
                     outputData[i] = OutputValue;
                 }
                 // 更新读指针位置
                 readerPosition += readSize;
                 m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
                 return true;
             }
         }
         LOG_ERROR("get circularbuffer ptr error");
         return false;
     }

     return ReadDataForReaderImpl<BoolMatrix, BoolMatrix>(readSize, outputData, readerName, buffer);
}

bool BufferReadImpl::ReadFComplexMatrixDataForReaderImpl(size_t readSize, std::vector<FComplexMatrix> &outputData, const std::string &readerName)
{
     // 获取内部矩阵缓冲区指针
     auto* buffer = m_buffer->getFComplexMatrixCircularBuffer();
     if(!buffer) {
         // 兼容: IntMatrix -> FComplexMatrix (int作为实部，虚部为0)
         if(m_buffer->GetDataType() == DataType::MATRIX_INT) {
             IntMatrixCircularBuffer* compatBuf = m_buffer->getIntMatrixCircularBuffer();
             if(!compatBuf) { LOG_ERROR("getIntMatrixCircularBuffer failed"); return false; }
             if(m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
                 qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
                 return false;
             }
             size_t& readerPosition = m_buffer->m_readerPositions[readerName];
             if(readerPosition == SIZE_MAX) { readerPosition = 0; return false; }
             size_t available = m_buffer->m_totalWritten - readerPosition;
             size_t processSize = m_buffer->IsVariableMode() ? available : readSize;
             outputData.resize(processSize);
             for(size_t i = 0; i < processSize; i++) {
                 size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                 IntMatrix readData = (*compatBuf)[readIndex];
                 FComplexMatrix outValue(readData.NumRows(), readData.NumColumns());
                 for(size_t row = 0; row < readData.NumRows(); ++row)
                     for(size_t col = 0; col < readData.NumColumns(); ++col)
                         outValue(row, col) = std::complex<float>(static_cast<float>(readData(row, col)), 0.0f);
                 outputData[i] = outValue;
             }
             readerPosition += processSize;
             m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
             return true;
         }
         // 兼容: DoubleMatrix -> FComplexMatrix (double作为实部，虚部为0)
         if(m_buffer->GetDataType() == DataType::MATRIX_DOUBLE) {
             DoubleMatrixCircularBuffer* compatBuf = m_buffer->getDoubleMatrixCircularBuffer();
             if(!compatBuf) { LOG_ERROR("getDoubleMatrixCircularBuffer failed"); return false; }
             if(m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
                 qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
                 return false;
             }
             size_t& readerPosition = m_buffer->m_readerPositions[readerName];
             if(readerPosition == SIZE_MAX) { readerPosition = 0; return false; }
             size_t available = m_buffer->m_totalWritten - readerPosition;
             size_t processSize = m_buffer->IsVariableMode() ? available : readSize;
             outputData.resize(processSize);
             for(size_t i = 0; i < processSize; i++) {
                 size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                 DoubleMatrix readData = (*compatBuf)[readIndex];
                 FComplexMatrix outValue(readData.NumRows(), readData.NumColumns());
                 for(size_t row = 0; row < readData.NumRows(); ++row)
                     for(size_t col = 0; col < readData.NumColumns(); ++col)
                         outValue(row, col) = std::complex<float>(static_cast<float>(readData(row, col)), 0.0f);
                 outputData[i] = outValue;
             }
             readerPosition += processSize;
             m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
             return true;
         }
         LOG_ERROR("get circularbuffer ptr error");
         return false;
     }
     return ReadDataForReaderImpl<FComplexMatrix, FComplexMatrix>(readSize, outputData, readerName, buffer);
}

bool BufferReadImpl::ReadDComplexMatrixDataForReaderImpl(size_t readSize, std::vector<DComplexMatrix> &outputData, const std::string &readerName)
{
     // 获取内部矩阵缓冲区指针
     auto* buffer = m_buffer->getDComplexMatrixCircularBuffer();
     if(!buffer) {
         // 兼容: IntMatrix -> DComplexMatrix (int作为实部，虚部为0)
         if(m_buffer->GetDataType() == DataType::MATRIX_INT) {
             IntMatrixCircularBuffer* compatBuf = m_buffer->getIntMatrixCircularBuffer();
             if(!compatBuf) { LOG_ERROR("getIntMatrixCircularBuffer failed"); return false; }
             if(m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
                 qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
                 return false;
             }
             size_t& readerPosition = m_buffer->m_readerPositions[readerName];
             if(readerPosition == SIZE_MAX) { readerPosition = 0; return false; }
             size_t available = m_buffer->m_totalWritten - readerPosition;
             size_t processSize = m_buffer->IsVariableMode() ? available : readSize;
             outputData.resize(processSize);
             for(size_t i = 0; i < processSize; i++) {
                 size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                 IntMatrix readData = (*compatBuf)[readIndex];
                 DComplexMatrix outValue(readData.NumRows(), readData.NumColumns());
                 for(size_t row = 0; row < readData.NumRows(); ++row)
                     for(size_t col = 0; col < readData.NumColumns(); ++col)
                         outValue(row, col) = std::complex<double>(static_cast<double>(readData(row, col)), 0.0);
                 outputData[i] = outValue;
             }
             readerPosition += processSize;
             m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
             return true;
         }
         // 兼容: DoubleMatrix -> DComplexMatrix (double作为实部，虚部为0)
         if(m_buffer->GetDataType() == DataType::MATRIX_DOUBLE) {
             DoubleMatrixCircularBuffer* compatBuf = m_buffer->getDoubleMatrixCircularBuffer();
             if(!compatBuf) { LOG_ERROR("getDoubleMatrixCircularBuffer failed"); return false; }
             if(m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
                 qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
                 return false;
             }
             size_t& readerPosition = m_buffer->m_readerPositions[readerName];
             if(readerPosition == SIZE_MAX) { readerPosition = 0; return false; }
             size_t available = m_buffer->m_totalWritten - readerPosition;
             size_t processSize = m_buffer->IsVariableMode() ? available : readSize;
             outputData.resize(processSize);
             for(size_t i = 0; i < processSize; i++) {
                 size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                 DoubleMatrix readData = (*compatBuf)[readIndex];
                 DComplexMatrix outValue(readData.NumRows(), readData.NumColumns());
                 for(size_t row = 0; row < readData.NumRows(); ++row)
                     for(size_t col = 0; col < readData.NumColumns(); ++col)
                         outValue(row, col) = std::complex<double>(readData(row, col), 0.0);
                 outputData[i] = outValue;
             }
             readerPosition += processSize;
             m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
             return true;
         }
         LOG_ERROR("get circularbuffer ptr error");
         return false;
     }
     return ReadDataForReaderImpl<DComplexMatrix, DComplexMatrix>(readSize, outputData, readerName, buffer);
}

bool BufferReadImpl::ReadEnvelopeMatrixDataForReaderImpl(size_t readSize, std::vector<EnvelopeMatrix> &outputData, const std::string &readerName)
{
     // 获取内部矩阵缓冲区指针
     auto* buffer = m_buffer->getEnvelopeMatrixCircularBuffer();
     if(!buffer) {
         return false;
     }

     if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
         qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
         return false;
     }

     size_t& readerPosition = m_buffer->m_readerPositions[readerName];

     // 检查reader位置有效性
     if (readerPosition == SIZE_MAX) {
         qDebug() << "ERROR: Invalid reader position for '" << QString::fromStdString(readerName) << "'";
         readerPosition = 0;
         return false;
     }

     //获取可用数据量
     size_t available = m_buffer->m_totalWritten - readerPosition;

     //时间驱动与数据流驱动 区别
     //1.时间驱动每次读取实际数据量
     //2.数据流驱动每次读取读指针数据量

     //1.时间驱动
     if(m_buffer->IsVariableMode()) {
         //读取读指针数据量
         outputData.resize(available);
         for (size_t i = 0; i < available; i++) {
             size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
             outputData[i] = (*buffer)[readIndex];
         }

         //更新这个读指针位置和当前数据量
         readerPosition += available;
         m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
     }
     //2.数据流驱动
     else {
         //读取读指针数据量
         outputData.resize(readSize);
         for (size_t i = 0; i < readSize; i++) {
             size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
             outputData[i] = (*buffer)[readIndex];
         }

         //更新这个读指针位置和当前数据量
         readerPosition += readSize;
         m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
     }
     return true;
}



bool BufferReadImpl::SmartExpandIfNeeded(size_t writeSize, size_t readSize)
{
    return m_buffer->SmartExpandIfNeeded(writeSize, readSize);
}

void BufferReadImpl::AutoRestoreIfPossible()
{
    m_buffer->AutoRestoreIfPossible();
}

bool BufferReadImpl::CheckAllBusReaderHaveData(const std::string &readerName, size_t readSize)
{
    return m_buffer->CheckAllBusReaderHaveData(readerName, readSize);
}
