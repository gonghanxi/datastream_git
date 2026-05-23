#include "Sink_Block.h"

Sink_Block::Sink_Block(const std::string &name)
    :Block(name), m_totalSamplesProcessed(0), m_fcExtracted(false)
{
    Initialize();
}

Sink_Block::~Sink_Block()
{
    WriteJsonOutput();
}

void Sink_Block::Setup()
{
    Block::Setup();
}

void Sink_Block::Run()
{
    ProcessEnvelopeData();


}

void Sink_Block::SetCharacterizationFrequency(double fc)
{
    m_fc = fc;
}

void Sink_Block::PrintParameters() const
{

}

void Sink_Block::Initialize()
{
    SetBlockType(Block::BlockType::SINK);

    SetOutputFile("envelope_output.json");

    AddInputPort("envelope_input", 1, Block::DataType::ENVELOPE_SIGNAL);
}

void Sink_Block::ProcessEnvelopeData()
{
    BufferReader* envelopeReader = GetInputPort("envelope_input");

    if (!envelopeReader) {
        std::cout << "SinkBlock: Missing input port" << std::endl;
        return;
    }

    if (!envelopeReader->HasDataAvailable()) {
        std::cout << "SinkBlock: No envelope data available" << std::endl;
        return;
    }

    // 读取包络数据
    std::vector<SystemVueModelBuilder::EnvelopeSignal> newData;
    if (!envelopeReader->ReadData(newData) || newData.empty()) {
        std::cout << "SinkBlock: Failed to read envelope data" << std::endl;
        return;
    }

    // 过滤掉所有的 Fc 标记
    std::vector<SystemVueModelBuilder::EnvelopeSignal> filteredData;
    for (size_t i = 0; i < newData.size(); ++i) {
        std::complex<double> sample = newData[i].complex();

        // 检查是否是 Fc 标记（虚部为 0，实部为正数）
        if (sample.imag() == 0 && sample.real() > 0 && sample.real() > 1000) {
            // 如果是第一个 Fc 标记，保存为 m_fc
            m_fc = sample.real();
            m_fcExtracted = true;
            std::cout << "SinkBlock: Extracted Fc = " << m_fc << " Hz from input stream" << std::endl;

            // 跳过所有的 Fc 标记，不加入数据
            continue;
        }

        // 如果是真实数据，加入 filteredData
        filteredData.push_back(newData[i]);
    }

    if (filteredData.empty()) {
        std::cout << "SinkBlock: No real data after filtering Fc markers" << std::endl;
        return;
    }

    // 保存数据和时间戳
    for (size_t i = 0; i < newData.size(); ++i) {
        m_envelopeData.push_back(newData[i]);
        // 每个样本的时间戳递增，与radar_cw_block保持一致
        m_timestamps.push_back(GetSourceTimestamp());
    }

    m_totalSamplesProcessed += filteredData.size();
}

void Sink_Block::WriteJsonOutput()
{
    if(m_envelopeData.empty()) {
        return;
    }

    nlohmann::json outputJson;

    outputJson["Fc"] = m_fc;

    nlohmann::json valueArray = nlohmann::json::array();

    for(size_t i = 0; i < m_envelopeData.size(); i++) {
        const auto& envSignal = m_envelopeData[i];
        const auto& timeStamp = m_timestamps[i];

        std::complex<double> complexValue = envSignal.complex();

        nlohmann::json datajson;
        datajson["Index"] = i + 1;
        datajson["_Index"] = i;
        datajson["re"] = complexValue.real();
        datajson["im"] = complexValue.imag();
        datajson["timestamp"] = timeStamp.count();

        valueArray.push_back(datajson);
    }

    outputJson["Values"] = valueArray;

    std::ofstream outputFile(GetOutputFile());
    if(outputFile.is_open()) {
        outputFile << std::setw(4) << outputJson << std::endl;
        outputFile.close();
    }
    else {
        std::cout << "SinkBlock: Failed to open output file '" << GetOutputFile() << "'" << std::endl;
    }

    m_envelopeData.clear();
    m_timestamps.clear();
}
