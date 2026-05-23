#include "BufferWriteImpl.h"
#include "BufferReader.h"
#include "DataTypesAndParsers.h"


using namespace SystemVueModelBuilder;

bool BufferWriteImpl::WriteData(int data)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(1, 0)) { // 写入1个元素
        qDebug() << "ERROR: Cannot expand buffer for write operation";
        return false;
    }
    // 获取外部端口map
    auto& externalPorts = m_buffer->GetExternalIntPortsRef();

    // 查找指定端口的std::any
    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取引用
        int& externalData = it->second;
        externalData = data;

        m_buffer->m_totalWritten++;
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }
        return false;
    }
    return false;
}

bool BufferWriteImpl::WriteData(double data)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(1, 0)) { // 写入1个元素
        qDebug() << "ERROR: Cannot expand buffer for write operation";
        return false;
    }
    // 获取外部端口map
    auto& externalPorts = m_buffer->GetExternalDoublePortsRef();

    // 查找指定端口的std::any
    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取引用
        double& externalData = it->second;
        externalData = data;

        m_buffer->m_totalWritten++;
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }
        return true;
    }
    return false;
}

bool BufferWriteImpl::WriteData(float data)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(1, 0)) { // 写入1个元素
        qDebug() << "ERROR: Cannot expand buffer for write operation";
        return false;
    }
    // 获取外部端口map
    auto& externalPorts = m_buffer->GetExternalFloatPortsRef();

    // 查找指定端口的std::any
    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取引用
        float& externalData = it->second;
        externalData = data;

        m_buffer->m_totalWritten++;
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }
        return true;
    }
    return false;
}

bool BufferWriteImpl::WriteData(bool data)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(1, 0)) { // 写入1个元素
        qDebug() << "ERROR: Cannot expand buffer for write operation";
        return false;
    }
    // 获取外部端口map
    auto& externalPorts = m_buffer->GetExternalBoolPortsRef();

    // 查找指定端口的std::any
    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取引用
        bool& externalData = it->second;
        externalData = data;

        m_buffer->m_totalWritten++;
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }
        return true;
    }
    return false;
}

bool BufferWriteImpl::WriteData(std::complex<float> data)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(1, 0)) { // 写入1个元素
        qDebug() << "ERROR: Cannot expand buffer for write operation";
        return false;
    }
    // 获取外部端口map
    auto& externalPorts = m_buffer->GetExternalFComplexPortsRef();

    // 查找指定端口的std::any
    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取引用
        std::complex<float>& externalData = it->second;
        externalData = data;

        m_buffer->m_totalWritten++;
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }
        return true;
    }
    return false;
}

bool BufferWriteImpl::WriteData(std::complex<double> data)
{
    // 先检查并扩容
    if (!SmartExpandIfNeeded(1, 0)) { // 写入1个元素
        qDebug() << "ERROR: Cannot expand buffer for write operation";
        return false;
    }
    // 获取外部端口map
    auto& externalPorts = m_buffer->GetExternalDComplexPortsRef();

    // 查找指定端口的std::any
    auto it = externalPorts.find(m_buffer->m_name);
    if(it != externalPorts.end()) {
        // 直接获取引用
        std::complex<double>& externalData = it->second;
        externalData = data;

        m_buffer->m_totalWritten++;
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }
        return true;
    }
    return false;
}

bool BufferWriteImpl::WriteData(const std::vector<int> &data)
{
//    qDebug() << "=== Buffer::WriteData (int) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteIntDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<double> &data)
{
//    qDebug() << "=== Buffer::WriteData (double) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteDoubleDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<float> &data)
{
//    qDebug() << "=== Buffer::WriteData (float) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteFloatDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<bool> &data)
{
//    qDebug() << "=== Buffer::WriteData (bool) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteBoolDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<std::complex<float> > &data)
{
//    qDebug() << "=== Buffer::WriteData (fcomplex) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteFComplexDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<std::complex<double> > &data)
{
//    qDebug() << "=== Buffer::WriteData (dcomplex) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteDComplexDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<int *> &data)
{
    if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
        qDebug() << "ERROR: Cannot expand buffer for write operation";
        return false;
    }
    std::vector<int> convertedData;
    for(const auto& ptr : data) {
        if(ptr)
            convertedData.push_back(*ptr);
    }
    WriteIntDataImpl(convertedData);
    AutoRestoreIfPossible();// 写入后检查是否可以恢复
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<double *> &data)
{
    if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
        qDebug() << "ERROR: Cannot expand buffer for write operation";
        return false;
    }
    std::vector<double> convertedData;
    for(const auto& ptr : data) {
        if(ptr)
            convertedData.push_back(*ptr);
    }

    WriteDoubleDataImpl(convertedData);
    AutoRestoreIfPossible();// 写入后检查是否可以恢复
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<std::complex<double> *> &data)
{
    if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
        qDebug() << "ERROR: Cannot expand buffer for write operation";
        return false;
    }
    std::vector<std::complex<double>> convertedData;
    for(const auto& ptr : data) {
        if(ptr)
            convertedData.push_back(*ptr);
    }
    WriteDComplexDataImpl(convertedData);
    AutoRestoreIfPossible();// 写入后检查是否可以恢复
    return true;
}

bool BufferWriteImpl::WriteData(const CircularBufferBase &data)
{
    if (!SmartExpandIfNeeded(data.GetSize(), 0)) { // 读取需求为0，只检查写入
        qDebug() << "ERROR: Cannot expand buffer for write operation";
        return false;
    }
    size_t bufferSize = data.GetSize();
    if(bufferSize == 0)
        return false;
    //获取对应类型的指针
    auto* intBuffer = dynamic_cast<const SystemVueModelBuilder::IntCircularBuffer*>(&data);
    if(intBuffer) {
        std::vector<int> convertedData;
        convertedData.reserve(bufferSize);
        for(size_t i = 0; i < bufferSize; i++) {
            convertedData.push_back((*intBuffer)[i]);
        }
        WriteIntDataImpl(convertedData);
        // 写入后检查是否可以恢复
        AutoRestoreIfPossible();// 写入后检查是否可以恢复
        return true;
    }
    //获取对应类型的指针
    auto* doubleBuffer = dynamic_cast<const SystemVueModelBuilder::DoubleCircularBuffer*>(&data);
    if(doubleBuffer) {
        std::vector<double> convertedData;
        convertedData.reserve(bufferSize);
        for(size_t i = 0; i < bufferSize; i++) {
            convertedData.push_back((*doubleBuffer)[i]);
        }
        WriteDoubleDataImpl(convertedData);
        AutoRestoreIfPossible();// 写入后检查是否可以恢复
        return true;
    }
    //获取对应类型的指针
    auto* floatBuffer = dynamic_cast<const SystemVueModelBuilder::FloatCircularBuffer*>(&data);
    if(floatBuffer) {
        std::vector<float> convertedData;
        convertedData.reserve(bufferSize);
        for(size_t i = 0; i < bufferSize; i++) {
            convertedData.push_back((*floatBuffer)[i]);
        }
        WriteFloatDataImpl(convertedData);
        AutoRestoreIfPossible();// 写入后检查是否可以恢复
        return true;
    }
    //获取对应类型的指针
    auto* boolBuffer = dynamic_cast<const SystemVueModelBuilder::BoolCircularBuffer*>(&data);
    if(boolBuffer) {
        std::vector<bool> convertedData;
        convertedData.reserve(bufferSize);
        for(size_t i = 0; i < bufferSize; i++) {
            convertedData.push_back((*boolBuffer)[i]);
        }
        WriteBoolDataImpl(convertedData);
        AutoRestoreIfPossible();// 写入后检查是否可以恢复
        return true;
    }
    //获取对应类型的指针
    auto* dcomplexBuffer = dynamic_cast<const SystemVueModelBuilder::DComplexCircularBuffer*>(&data);
    if(dcomplexBuffer) {
        std::vector<std::complex<double>> convertedData;
        convertedData.reserve(bufferSize);
        for(size_t i = 0; i < bufferSize; i++) {
            convertedData.push_back((*dcomplexBuffer)[i]);
        }
        WriteDComplexDataImpl(convertedData);
        AutoRestoreIfPossible();// 写入后检查是否可以恢复
        return true;
    }
    //获取对应类型的指针
    auto* fcomplexBuffer = dynamic_cast<const SystemVueModelBuilder::FComplexCircularBuffer*>(&data);
    if(fcomplexBuffer) {
        std::vector<std::complex<float>> convertedData;
        convertedData.reserve(bufferSize);
        for(size_t i = 0; i < bufferSize; i++) {
            convertedData.push_back((*fcomplexBuffer)[i]);
        }
        WriteFComplexDataImpl(convertedData);
        AutoRestoreIfPossible();// 写入后检查是否可以恢复
        return true;
    }
    return false;
}

bool BufferWriteImpl::WriteData(const EnvelopeSignal &data)
{
    double fc = 0.0;
    // 如果Buffer有连接的Reader，尝试从Reader获取频率
    if (!m_buffer->m_readerObjects.empty()) {
        for(auto Reader : m_buffer->m_readerObjects) {
            if (Reader.second->hasCharacterizationFrequency()) {
                fc = Reader.second->getCharacterizationFrequency();
//                qDebug() << "Got characterization frequency " << fc
//                          << " from reader '" << QString::fromStdString(Reader.first) << "'";
            }
        }
    }
    double EPSILON = 1e-10;
    if (fabs(fc) > EPSILON) {
        m_buffer->setCharacterizationFrequency(fc);
    }

    if (!SmartExpandIfNeeded(1, 0)) { // 读取需求为0，只检查写入
        qDebug() << "ERROR: Cannot expand buffer for write operation";
        return false;
    }
    WriteEnvelopeSignalDataImpl(data);
    AutoRestoreIfPossible();// 写入后检查是否可以恢复
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<EnvelopeSignal> &data)
{
//    qDebug() << "=== Buffer::WriteData (EnvelopeSignal) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteEnvelopeSignalDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<IntMatrix> &data)
{
//    qDebug() << "=== Buffer::WriteData (IntMatrix) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteIntMatrixDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<DoubleMatrix> &data)
{
//    qDebug() << "=== Buffer::WriteData (DoubleMatrix) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteDoubleMatrixDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<FloatMatrix> &data)
{
//    qDebug() << "=== Buffer::WriteData (FloatMatrix) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteFloatMatrixDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<BoolMatrix> &data)
{
//    qDebug() << "=== Buffer::WriteData (BoolMatrix) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteBoolMatrixDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<FComplexMatrix> &data)
{
//    qDebug() << "=== Buffer::WriteData (FComplexMatrix) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteFComplexMatrixDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<DComplexMatrix> &data)
{
//    qDebug() << "=== Buffer::WriteData (DComplexMatrix) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteDComplexMatrixDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

bool BufferWriteImpl::WriteData(const std::vector<EnvelopeMatrix> &data)
{
//    qDebug() << "=== Buffer::WriteData (EnvelopeMatrix) ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", WriterType: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));
    // 根据读取器类型选择读取方式
    if (m_buffer->m_writerType == m_buffer->BUS_MASTER) {
        WriteBusData(data);
    }
    else {
        if (!SmartExpandIfNeeded(data.size(), 0)) { // 读取需求为0，只检查写入
            qDebug() << "ERROR: Cannot expand buffer for write operation";
            return false;
        }
        WriteEnvelopeMatrixDataImpl(data);

        AutoRestoreIfPossible();// 写入后检查是否可以恢复
    }
    return true;
}

void BufferWriteImpl::WriteIntDataImpl(const std::vector<int> &data)
{
//    qDebug() << "=== WriteIntDataImpl BEGIN ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name);
//    qDebug() << "Data size: " << data.size();
//    qDebug() << "Before - TotalWritten: " << m_buffer->m_totalWritten
//              << ", DataCount: " << m_buffer->m_dataCount
//              << ", BufferSize: " << m_buffer->m_bufferSize
//              << ", WritePosition: " << m_buffer->m_writePosition;

    if (data.empty()) {
        return;
    }

    //获取buffer
    auto* buffer = m_buffer->getIntCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get int buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        qDebug() << "ERROR: Insufficient free space! Free: " << freeSpace
                  << ", Need: " << data.size();

        // 如果需要，尝试扩容
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed!";
            return;
        }
        // 重新获取缓冲区
        buffer = m_buffer->getIntCircularBuffer();
    }



    // 写入数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            // 安全边界检查
            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds! " << writeIndex
                          << " >= " << m_buffer->m_bufferSize;
                break;
            }


            (*buffer)[writeIndex] = data[i];
//            qDebug() << "  Writing [" << writeIndex << "] = " << data[i];
        }

        // 关键修复：正确更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        // 重新计算数据计数
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();

        // 确保 slowestReaderPosition 不会超过 totalWritten
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            qDebug() << "WARNING: Slowest reader position " << slowestReaderPosition
                      << " exceeds total written " << m_buffer->m_totalWritten
                      << ". Correcting to total written.";
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        // 计算有效数据量
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "WARNING: DataCount " << m_buffer->m_dataCount
                      << " exceeds buffer size " << m_buffer->m_bufferSize
                      << ". Clamping to buffer size.";
            m_buffer->m_dataCount = m_buffer->m_bufferSize;

            // 需要调整最慢读取器位置
            size_t overflow = m_buffer->m_dataCount - m_buffer->m_bufferSize;
            slowestReaderPosition += overflow;

            // 确保所有读取器位置不会落后太多
            for (auto& reader : m_buffer->m_readerPositions) {
                if (reader.second < slowestReaderPosition) {
                    reader.second = slowestReaderPosition;
                }
            }
        }

//        qDebug() << "After - TotalWritten: " << m_buffer->m_totalWritten
//                  << ", DataCount: " << m_buffer->m_dataCount
//                  << ", WritePosition: " << m_buffer->m_writePosition
//                  << ", SlowestReaderPos: " << slowestReaderPosition;

        // 验证状态一致性
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "ERROR: DataCount " << m_buffer->m_dataCount
                      << " > BufferSize " << m_buffer->m_bufferSize << "!";
        }

        if (m_buffer->m_writePosition >= m_buffer->m_bufferSize) {
            qDebug() << "ERROR: WritePosition " << m_buffer->m_writePosition
                      << " >= BufferSize " << m_buffer->m_bufferSize << "!";
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing data: " << e.what();
        return;
    }

//    qDebug() << "=== WriteIntDataImpl END ===";
}

void BufferWriteImpl::WriteDoubleDataImpl(const std::vector<double> &data)
{

//    qDebug() << "=== WriteDoubleDataImpl BEGIN ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name);
//    qDebug() << "Data size: " << data.size();
//    qDebug() << "Before - TotalWritten: " << m_buffer->m_totalWritten
//              << ", DataCount: " << m_buffer->m_dataCount
//              << ", BufferSize: " << m_buffer->m_bufferSize
//              << ", WritePosition: " << m_buffer->m_writePosition;

    if (data.empty()) {
        qDebug() << "Warning: Empty data vector";
        return;
    }

    //获取buffer
    auto* buffer = m_buffer->getDoubleCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get double buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        qDebug() << "ERROR: Insufficient free space! Free: " << freeSpace
                  << ", Need: " << data.size();

        // 如果需要，尝试扩容
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed!";
            return;
        }
        // 重新获取缓冲区
        buffer = m_buffer->getDoubleCircularBuffer();
    }



    // 写入数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            // 安全边界检查
            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds! " << writeIndex
                          << " >= " << m_buffer->m_bufferSize;
                break;
            }


            (*buffer)[writeIndex] = data[i];
//            qDebug() << "  Writing [" << writeIndex << "] = " << data[i];
        }

        // 关键修复：正确更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        // 重新计算数据计数
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();

        // 确保 slowestReaderPosition 不会超过 totalWritten
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            qDebug() << "WARNING: Slowest reader position " << slowestReaderPosition
                      << " exceeds total written " << m_buffer->m_totalWritten
                      << ". Correcting to total written.";
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        // 计算有效数据量
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "WARNING: DataCount " << m_buffer->m_dataCount
                      << " exceeds buffer size " << m_buffer->m_bufferSize
                      << ". Clamping to buffer size.";
            m_buffer->m_dataCount = m_buffer->m_bufferSize;

            // 需要调整最慢读取器位置
            size_t overflow = m_buffer->m_dataCount - m_buffer->m_bufferSize;
            slowestReaderPosition += overflow;

            // 确保所有读取器位置不会落后太多
            for (auto& reader : m_buffer->m_readerPositions) {
                if (reader.second < slowestReaderPosition) {
                    reader.second = slowestReaderPosition;
                }
            }
        }

//        qDebug() << "After - TotalWritten: " << m_buffer->m_totalWritten
//                  << ", DataCount: " << m_buffer->m_dataCount
//                  << ", WritePosition: " << m_buffer->m_writePosition
//                  << ", SlowestReaderPos: " << slowestReaderPosition;

        // 验证状态一致性
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "ERROR: DataCount " << m_buffer->m_dataCount
                      << " > BufferSize " << m_buffer->m_bufferSize << "!";
        }

        if (m_buffer->m_writePosition >= m_buffer->m_bufferSize) {
            qDebug() << "ERROR: WritePosition " << m_buffer->m_writePosition
                      << " >= BufferSize " << m_buffer->m_bufferSize << "!";
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing data: " << e.what();
        return;
    }

//    qDebug() << "=== WriteDoubleDataImpl END ===";
}

void BufferWriteImpl::WriteFloatDataImpl(const std::vector<float> &data)
{
//    qDebug() << "=== WriteFloatDataImpl BEGIN ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name);
//    qDebug() << "Data size: " << data.size();
//    qDebug() << "Before - TotalWritten: " << m_buffer->m_totalWritten
//              << ", DataCount: " << m_buffer->m_dataCount
//              << ", BufferSize: " << m_buffer->m_bufferSize
//              << ", WritePosition: " << m_buffer->m_writePosition;

    if (data.empty()) {
        return;
    }

    //获取buffer
    auto* buffer = m_buffer->getFloatCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get float buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        qDebug() << "ERROR: Insufficient free space! Free: " << freeSpace
                  << ", Need: " << data.size();

        // 如果需要，尝试扩容
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed!";
            return;
        }
        // 重新获取缓冲区
        buffer = m_buffer->getFloatCircularBuffer();
    }

    // 写入数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            // 安全边界检查
            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds! " << writeIndex
                          << " >= " << m_buffer->m_bufferSize;
                break;
            }


            (*buffer)[writeIndex] = data[i];
//            qDebug() << "  Writing [" << writeIndex << "] = " << data[i];
        }

        // 关键修复：正确更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        // 重新计算数据计数
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();

        // 确保 slowestReaderPosition 不会超过 totalWritten
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            qDebug() << "WARNING: Slowest reader position " << slowestReaderPosition
                      << " exceeds total written " << m_buffer->m_totalWritten
                      << ". Correcting to total written.";
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        // 计算有效数据量
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "WARNING: DataCount " << m_buffer->m_dataCount
                      << " exceeds buffer size " << m_buffer->m_bufferSize
                      << ". Clamping to buffer size.";
            m_buffer->m_dataCount = m_buffer->m_bufferSize;

            // 需要调整最慢读取器位置
            size_t overflow = m_buffer->m_dataCount - m_buffer->m_bufferSize;
            slowestReaderPosition += overflow;

            // 确保所有读取器位置不会落后太多
            for (auto& reader : m_buffer->m_readerPositions) {
                if (reader.second < slowestReaderPosition) {
                    reader.second = slowestReaderPosition;
                }
            }
        }

//        qDebug() << "After - TotalWritten: " << m_buffer->m_totalWritten
//                  << ", DataCount: " << m_buffer->m_dataCount
//                  << ", WritePosition: " << m_buffer->m_writePosition
//                  << ", SlowestReaderPos: " << slowestReaderPosition;

        // 验证状态一致性
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "ERROR: DataCount " << m_buffer->m_dataCount
                      << " > BufferSize " << m_buffer->m_bufferSize << "!";
        }

        if (m_buffer->m_writePosition >= m_buffer->m_bufferSize) {
            qDebug() << "ERROR: WritePosition " << m_buffer->m_writePosition
                      << " >= BufferSize " << m_buffer->m_bufferSize << "!";
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing data: " << e.what();
        return;
    }

//    qDebug() << "=== WriteFloatDataImpl END ===";
}

void BufferWriteImpl::WriteBoolDataImpl(const std::vector<bool> &data)
{
//    qDebug() << "=== WriteBoolDataImpl BEGIN ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name);
//    qDebug() << "Data size: " << data.size();
//    qDebug() << "Before - TotalWritten: " << m_buffer->m_totalWritten
//              << ", DataCount: " << m_buffer->m_dataCount
//              << ", BufferSize: " << m_buffer->m_bufferSize
//              << ", WritePosition: " << m_buffer->m_writePosition;

    if (data.empty()) {
        return;
    }

    //获取buffer
    auto* buffer = m_buffer->getBoolCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get bool buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        qDebug() << "ERROR: Insufficient free space! Free: " << freeSpace
                  << ", Need: " << data.size();

        // 如果需要，尝试扩容
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed!";
            return;
        }
        // 重新获取缓冲区
        buffer = m_buffer->getBoolCircularBuffer();
    }



    // 写入数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            // 安全边界检查
            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds! " << writeIndex
                          << " >= " << m_buffer->m_bufferSize;
                break;
            }


            (*buffer)[writeIndex] = data[i];
//            qDebug() << "  Writing [" << writeIndex << "] = " << data[i];
        }

        // 关键修复：正确更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        // 重新计算数据计数
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();

        // 确保 slowestReaderPosition 不会超过 totalWritten
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            qDebug() << "WARNING: Slowest reader position " << slowestReaderPosition
                      << " exceeds total written " << m_buffer->m_totalWritten
                      << ". Correcting to total written.";
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        // 计算有效数据量
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "WARNING: DataCount " << m_buffer->m_dataCount
                      << " exceeds buffer size " << m_buffer->m_bufferSize
                      << ". Clamping to buffer size.";
            m_buffer->m_dataCount = m_buffer->m_bufferSize;

            // 需要调整最慢读取器位置
            size_t overflow = m_buffer->m_dataCount - m_buffer->m_bufferSize;
            slowestReaderPosition += overflow;

            // 确保所有读取器位置不会落后太多
            for (auto& reader : m_buffer->m_readerPositions) {
                if (reader.second < slowestReaderPosition) {
                    reader.second = slowestReaderPosition;
                }
            }
        }

//        qDebug() << "After - TotalWritten: " << m_buffer->m_totalWritten
//                  << ", DataCount: " << m_buffer->m_dataCount
//                  << ", WritePosition: " << m_buffer->m_writePosition
//                  << ", SlowestReaderPos: " << slowestReaderPosition;

        // 验证状态一致性
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "ERROR: DataCount " << m_buffer->m_dataCount
                      << " > BufferSize " << m_buffer->m_bufferSize << "!";
        }

        if (m_buffer->m_writePosition >= m_buffer->m_bufferSize) {
            qDebug() << "ERROR: WritePosition " << m_buffer->m_writePosition
                      << " >= BufferSize " << m_buffer->m_bufferSize << "!";
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing data: " << e.what();
        return;
    }

//    qDebug() << "=== WriteBoolDataImpl END ===";
}

void BufferWriteImpl::WriteFComplexDataImpl(const std::vector<std::complex<float> > &data)
{
//    qDebug() << "=== WriteFComplexDataImpl BEGIN ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name);
//    qDebug() << "Data size: " << data.size();
//    qDebug() << "Before - TotalWritten: " << m_buffer->m_totalWritten
//              << ", DataCount: " << m_buffer->m_dataCount
//              << ", BufferSize: " << m_buffer->m_bufferSize
//              << ", WritePosition: " << m_buffer->m_writePosition;

    if (data.empty()) {
        return;
    }

    //获取buffer
    auto* buffer = m_buffer->getFComplexCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get complex float buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        qDebug() << "ERROR: Insufficient free space! Free: " << freeSpace
                  << ", Need: " << data.size();

        // 如果需要，尝试扩容
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed!";
            return;
        }
        // 重新获取缓冲区
        buffer = m_buffer->getFComplexCircularBuffer();
    }



    // 写入数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            // 安全边界检查
            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds! " << writeIndex
                          << " >= " << m_buffer->m_bufferSize;
                break;
            }


            (*buffer)[writeIndex] = data[i];
//            qDebug() << "  Writing [" << writeIndex << "] = " << QString("(%1, %2)").arg(data[i].real()).arg(data[i].imag());
        }

        // 关键修复：正确更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        // 重新计算数据计数
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();

        // 确保 slowestReaderPosition 不会超过 totalWritten
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            qDebug() << "WARNING: Slowest reader position " << slowestReaderPosition
                      << " exceeds total written " << m_buffer->m_totalWritten
                      << ". Correcting to total written.";
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        // 计算有效数据量
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "WARNING: DataCount " << m_buffer->m_dataCount
                      << " exceeds buffer size " << m_buffer->m_bufferSize
                      << ". Clamping to buffer size.";
            m_buffer->m_dataCount = m_buffer->m_bufferSize;

            // 需要调整最慢读取器位置
            size_t overflow = m_buffer->m_dataCount - m_buffer->m_bufferSize;
            slowestReaderPosition += overflow;

            // 确保所有读取器位置不会落后太多
            for (auto& reader : m_buffer->m_readerPositions) {
                if (reader.second < slowestReaderPosition) {
                    reader.second = slowestReaderPosition;
                }
            }
        }

//        qDebug() << "After - TotalWritten: " << m_buffer->m_totalWritten
//                  << ", DataCount: " << m_buffer->m_dataCount
//                  << ", WritePosition: " << m_buffer->m_writePosition
//                  << ", SlowestReaderPos: " << slowestReaderPosition;

        // 验证状态一致性
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "ERROR: DataCount " << m_buffer->m_dataCount
                      << " > BufferSize " << m_buffer->m_bufferSize << "!";
        }

        if (m_buffer->m_writePosition >= m_buffer->m_bufferSize) {
            qDebug() << "ERROR: WritePosition " << m_buffer->m_writePosition
                      << " >= BufferSize " << m_buffer->m_bufferSize << "!";
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing data: " << e.what();
        return;
    }

//    qDebug() << "=== WriteFComplexDataImpl END ===";
}

void BufferWriteImpl::WriteDComplexDataImpl(const std::vector<std::complex<double> > &data)
{
//    qDebug() << "=== WriteDComplexDataImpl BEGIN ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name);
//    qDebug() << "Data size: " << data.size();
//    qDebug() << "Before - TotalWritten: " << m_buffer->m_totalWritten
//              << ", DataCount: " << m_buffer->m_dataCount
//              << ", BufferSize: " << m_buffer->m_bufferSize
//              << ", WritePosition: " << m_buffer->m_writePosition;

    if (data.empty()) {
        return;
    }

    //获取buffer
    auto* buffer = m_buffer->getDComplexCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get complex double buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        qDebug() << "ERROR: Insufficient free space! Free: " << freeSpace
                  << ", Need: " << data.size();

        // 如果需要，尝试扩容
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed!";
            return;
        }
        // 重新获取缓冲区
        buffer = m_buffer->getDComplexCircularBuffer();
    }



    // 写入数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            // 安全边界检查
            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds! " << writeIndex
                          << " >= " << m_buffer->m_bufferSize;
                break;
            }


            (*buffer)[writeIndex] = data[i];
//            qDebug() << "  Writing [" << writeIndex << "] = " << QString("(%1, %2)").arg(data[i].real()).arg(data[i].imag());
        }

        // 关键修复：正确更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        // 重新计算数据计数
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();

        // 确保 slowestReaderPosition 不会超过 totalWritten
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            qDebug() << "WARNING: Slowest reader position " << slowestReaderPosition
                      << " exceeds total written " << m_buffer->m_totalWritten
                      << ". Correcting to total written.";
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        // 计算有效数据量
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "WARNING: DataCount " << m_buffer->m_dataCount
                      << " exceeds buffer size " << m_buffer->m_bufferSize
                      << ". Clamping to buffer size.";
            m_buffer->m_dataCount = m_buffer->m_bufferSize;

            // 需要调整最慢读取器位置
            size_t overflow = m_buffer->m_dataCount - m_buffer->m_bufferSize;
            slowestReaderPosition += overflow;

            // 确保所有读取器位置不会落后太多
            for (auto& reader : m_buffer->m_readerPositions) {
                if (reader.second < slowestReaderPosition) {
                    reader.second = slowestReaderPosition;
                }
            }
        }

//        qDebug() << "After - TotalWritten: " << m_buffer->m_totalWritten
//                  << ", DataCount: " << m_buffer->m_dataCount
//                  << ", WritePosition: " << m_buffer->m_writePosition
//                  << ", SlowestReaderPos: " << slowestReaderPosition;

        // 验证状态一致性
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "ERROR: DataCount " << m_buffer->m_dataCount
                      << " > BufferSize " << m_buffer->m_bufferSize << "!";
        }

        if (m_buffer->m_writePosition >= m_buffer->m_bufferSize) {
            qDebug() << "ERROR: WritePosition " << m_buffer->m_writePosition
                      << " >= BufferSize " << m_buffer->m_bufferSize << "!";
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing data: " << e.what();
        return;
    }

//    qDebug() << "=== WriteDComplexDataImpl END ===";
}

void BufferWriteImpl::WriteEnvelopeSignalDataImpl(const EnvelopeSignal &data)
{
    std::vector<SystemVueModelBuilder::EnvelopeSignal> tempData;
    tempData.push_back(data);
    WriteEnvelopeSignalDataImpl(tempData);
}

void BufferWriteImpl::WriteEnvelopeSignalDataImpl(const std::vector<EnvelopeSignal> &data)
{
    if (data.empty()) {
        return;
    }

    //获取buffer
    auto* buffer = m_buffer->getEnvelopeCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get EnvelopeSignal buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        qDebug() << "ERROR: Insufficient free space! Free: " << freeSpace
                  << ", Need: " << data.size();

        // 如果需要，尝试扩容
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed!";
            return;
        }
        // 重新获取缓冲区
        buffer = m_buffer->getEnvelopeCircularBuffer();
    }

//    qDebug() << "=== WriteEnvelopeSignalDataImpl BEGIN ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name);
//    qDebug() << "Data size: " << data.size();
//    qDebug() << "Before - TotalWritten: " << m_buffer->m_totalWritten
//              << ", DataCount: " << m_buffer->m_dataCount
//              << ", BufferSize: " << m_buffer->m_bufferSize
//              << ", WritePosition: " << m_buffer->m_writePosition;

    // 写入数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            // 安全边界检查
            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds! " << writeIndex
                          << " >= " << m_buffer->m_bufferSize;
                break;
            }

            // 将 EnvelopeSignal 转换为复数并写入缓冲区
            std::complex<double> complexValue = data[i].complex();
            (*buffer)[writeIndex] = complexValue;
//            qDebug() << "Writing to index " << writeIndex
//                                  << ": (" << complexValue.real() << ", "
//                                  << complexValue.imag() << ")";
        }

        // 关键修复：正确更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        // 重新计算数据计数
        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();

        // 确保 slowestReaderPosition 不会超过 totalWritten
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            qDebug() << "WARNING: Slowest reader position " << slowestReaderPosition
                      << " exceeds total written " << m_buffer->m_totalWritten
                      << ". Correcting to total written.";
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        // 计算有效数据量
        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "WARNING: DataCount " << m_buffer->m_dataCount
                      << " exceeds buffer size " << m_buffer->m_bufferSize
                      << ". Clamping to buffer size.";
            m_buffer->m_dataCount = m_buffer->m_bufferSize;

            // 需要调整最慢读取器位置
            size_t overflow = m_buffer->m_dataCount - m_buffer->m_bufferSize;
            slowestReaderPosition += overflow;

            // 确保所有读取器位置不会落后太多
            for (auto& reader : m_buffer->m_readerPositions) {
                if (reader.second < slowestReaderPosition) {
                    reader.second = slowestReaderPosition;
                }
            }
        }

//        qDebug() << "After - TotalWritten: " << m_buffer->m_totalWritten
//                  << ", DataCount: " << m_buffer->m_dataCount
//                  << ", WritePosition: " << m_buffer->m_writePosition
//                  << ", SlowestReaderPos: " << slowestReaderPosition;

        // 验证状态一致性
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            qDebug() << "ERROR: DataCount " << m_buffer->m_dataCount
                      << " > BufferSize " << m_buffer->m_bufferSize << "!";
        }

        if (m_buffer->m_writePosition >= m_buffer->m_bufferSize) {
            qDebug() << "ERROR: WritePosition " << m_buffer->m_writePosition
                      << " >= BufferSize " << m_buffer->m_bufferSize << "!";
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing data: " << e.what();
        return;
    }

    //    qDebug() << "=== WriteEnvelopeSignalDataImpl END ===";
}

void BufferWriteImpl::WriteIntMatrixDataImpl(const std::vector<IntMatrix> &data)
{

    if (data.empty()) {
        return;
    }

    // 获取内部矩阵缓冲区
    auto* buffer = m_buffer->getIntMatrixCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get complex double matrix buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed for matrix write!";
            return;
        }
        buffer = m_buffer->getIntMatrixCircularBuffer();
    }

    // 写入矩阵数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds!";
                break;
            }

            (*buffer)[writeIndex] = data[i];
        }

        // 更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            m_buffer->m_dataCount = m_buffer->m_bufferSize;
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing matrix data: " << e.what();
        return;
    }
}

void BufferWriteImpl::WriteDoubleMatrixDataImpl(const std::vector<DoubleMatrix> &data)
{

    if (data.empty()) {
        return;
    }

    // 获取内部矩阵缓冲区
    auto* buffer = m_buffer->getDoubleMatrixCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get complex double matrix buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed for matrix write!";
            return;
        }
        buffer = m_buffer->getDoubleMatrixCircularBuffer();
    }

    // 写入矩阵数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds!";
                break;
            }

            (*buffer)[writeIndex] = data[i];
        }

        // 更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            m_buffer->m_dataCount = m_buffer->m_bufferSize;
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing matrix data: " << e.what();
        return;
    }
}

void BufferWriteImpl::WriteFloatMatrixDataImpl(const std::vector<FloatMatrix> &data)
{
    if (data.empty()) {
        return;
    }

    // 获取内部矩阵缓冲区
    auto* buffer = m_buffer->getFloatMatrixCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get complex double matrix buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed for matrix write!";
            return;
        }
        buffer = m_buffer->getFloatMatrixCircularBuffer();
    }

    // 写入矩阵数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds!";
                break;
            }

            (*buffer)[writeIndex] = data[i];
        }

        // 更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            m_buffer->m_dataCount = m_buffer->m_bufferSize;
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing matrix data: " << e.what();
        return;
    }
}

void BufferWriteImpl::WriteBoolMatrixDataImpl(const std::vector<BoolMatrix> &data)
{
    if (data.empty()) {
        return;
    }

    // 获取内部矩阵缓冲区
    auto* buffer = m_buffer->getBoolMatrixCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get complex double matrix buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed for matrix write!";
            return;
        }
        buffer = m_buffer->getBoolMatrixCircularBuffer();
    }

    // 写入矩阵数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds!";
                break;
            }

            (*buffer)[writeIndex] = data[i];
        }

        // 更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            m_buffer->m_dataCount = m_buffer->m_bufferSize;
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing matrix data: " << e.what();
        return;
    }
}

void BufferWriteImpl::WriteFComplexMatrixDataImpl(const std::vector<FComplexMatrix> &data)
{
    if (data.empty()) {
        return;
    }

    // 获取内部矩阵缓冲区
    auto* buffer = m_buffer->getFComplexMatrixCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get complex double matrix buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed for matrix write!";
            return;
        }
        buffer = m_buffer->getFComplexMatrixCircularBuffer();
    }

    // 写入矩阵数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds!";
                break;
            }

            (*buffer)[writeIndex] = data[i];
        }

        // 更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            m_buffer->m_dataCount = m_buffer->m_bufferSize;
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing matrix data: " << e.what();
        return;
    }
}

void BufferWriteImpl::WriteDComplexMatrixDataImpl(const std::vector<DComplexMatrix> &data)
{
    if (data.empty()) {
        return;
    }

    // 获取内部矩阵缓冲区
    auto* buffer = m_buffer->getDComplexMatrixCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get complex double matrix buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed for matrix write!";
            return;
        }
        buffer = m_buffer->getDComplexMatrixCircularBuffer();
    }

    // 写入矩阵数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds!";
                break;
            }

            (*buffer)[writeIndex] = data[i];
        }

        // 更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            m_buffer->m_dataCount = m_buffer->m_bufferSize;
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing matrix data: " << e.what();
        return;
    }
}

void BufferWriteImpl::WriteEnvelopeMatrixDataImpl(const std::vector<EnvelopeMatrix> &data)
{

    if (data.empty()) {
        return;
    }

    // 获取内部矩阵缓冲区
    auto* buffer = m_buffer->getEnvelopeMatrixCircularBuffer();
    if (!buffer) {
        qDebug() << "ERROR: Failed to get complex double matrix buffer";
        return;
    }

    // 检查是否有足够空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    if (freeSpace < data.size()) {
        if (!SmartExpandIfNeeded(data.size(), 0)) {
            qDebug() << "ERROR: Buffer expansion failed for matrix write!";
            return;
        }
        buffer = m_buffer->getEnvelopeMatrixCircularBuffer();
    }

    // 写入矩阵数据
    try {
        for (size_t i = 0; i < data.size(); i++) {
            size_t writeIndex = (m_buffer->m_writePosition + i) % m_buffer->m_bufferSize;

            if (writeIndex >= m_buffer->m_bufferSize) {
                qDebug() << "ERROR: Write index out of bounds!";
                break;
            }

            (*buffer)[writeIndex] = data[i];
        }

        // 更新状态
        m_buffer->m_writePosition = (m_buffer->m_writePosition + data.size()) % m_buffer->m_bufferSize;
        m_buffer->m_totalWritten += data.size();

        size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
        if (slowestReaderPosition > m_buffer->m_totalWritten) {
            slowestReaderPosition = m_buffer->m_totalWritten;
        }

        if (m_buffer->m_totalWritten > slowestReaderPosition) {
            m_buffer->m_dataCount = m_buffer->m_totalWritten - slowestReaderPosition;
        } else {
            m_buffer->m_dataCount = 0;
        }

        // 确保数据计数不超过缓冲区大小
        if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
            m_buffer->m_dataCount = m_buffer->m_bufferSize;
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR writing matrix data: " << e.what();
        return;
    }
}

bool BufferWriteImpl::SmartExpandIfNeeded(size_t writeSize, size_t readSize)
{
    return m_buffer->SmartExpandIfNeeded(writeSize, readSize);
}

void BufferWriteImpl::AutoRestoreIfPossible()
{
    m_buffer->AutoRestoreIfPossible();// 写入后检查是否可以恢复
}

template<typename T>
void BufferWriteImpl::WriteBusData(const std::vector<T> &data)
{
//    qDebug() << "=== Buffer::WriteBusData ===";
//    qDebug() << "Buffer: " << QString::fromStdString(m_buffer->m_name)
//             << ", Type: " << QString::fromStdString(m_buffer->WriterTypeToString(m_buffer->m_writerType));

    if (m_buffer->m_writerType != m_buffer->BUS_MASTER) {
        qDebug() << "ERROR: Only master bus readers should call ReadBusData";
        return;
    }

    if (m_buffer->m_busConnections.empty()) {
        qDebug() << "ERROR: No bus connections available";
        return;
    }

    // 从每个连接写入数据
    for (size_t i = 0; i < m_buffer->m_busConnections.size(); ++i) {
        const auto& connection = m_buffer->m_busConnections[i];

        if (!connection.bridgeWriter) {
            qDebug() << "ERROR: Bridge reader is null";
            continue;
        }
        // 检查允许写入标志
        if (!connection.PermitWrite) {
            // 跳过该连接的写入
            continue;
        }

        // 直接调用写入方法，让桥接器自己处理
        connection.bridgeWriter->WriteData(data);

    }
    return;
}
