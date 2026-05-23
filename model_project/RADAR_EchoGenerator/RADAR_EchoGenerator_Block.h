#ifndef RADAR_ECHOGENERATOR_BLOCK_H
#define RADAR_ECHOGENERATOR_BLOCK_H
#include "RADAR_EchoGenerator.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RADAR_EchoGenerator_Block : public Block
{
public:
    RADAR_EchoGenerator_Block(const std::string& name);
    ~RADAR_EchoGenerator_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    RADAR_EchoGenerator::SelectedIncludePropagationEffect ConvertStringToSelectedIncludePropagationEffect(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<RADAR_EchoGenerator> m_radar;

    double SampleRate;
    double SystemLoss;
    RADAR_EchoGenerator::SelectedIncludePropagationEffect IncludePropagationEffect;
    double RF_Freq;
    int SimulationSampleNum;

    int	TargetNum;
    int TxPlatformNum;
    int RxPlatformNum;
    int ChannelNum;

    int Index;
    SystemVueModelBuilder::DComplexMatrix TargetDelayBuffer;
    SystemVueModelBuilder::DComplexMatrix outDelayBuffer;
    SystemVueModelBuilder::DComplexMatrix RxDelayBuffer;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*,std::vector<EnvelopeSignal>> m_inSignalBuffer;   // 多输入累积缓冲区
    std::map<BufferReader*,std::vector<DoubleMatrix>> m_TxPlatformLocBuffer;   // 多输入累积缓冲区
    std::map<BufferReader*,std::vector<DoubleMatrix>> m_RxPlatformLocBuffer;   // 多输入累积缓冲区
    std::map<BufferReader*,std::vector<DoubleMatrix>> m_TargetScatterLocBuffer;   // 多输入累积缓冲区
    std::map<BufferReader*,std::vector<double>> m_TargetScatterRCSBuffer;   // 多输入累积缓冲区



    std::queue<EnvelopeSignal> m_TargetSignalQueue;    // 输出分发队列
    std::queue<EnvelopeSignal> m_outSignalQueue;    // 输出分发队列
    std::queue<EnvelopeSignal> m_RxSignalQueue;    // 输出分发队列

    EnvelopeSignal m_lastTargetSignal;                 // 上次输出值（用于保持）
    EnvelopeSignal m_lastoutSignal;
    EnvelopeSignal m_lastRxSignal;
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数

    template<typename T>
    bool ReadDataToBuffer(BufferReader* port, std::map<BufferReader*, std::vector<T>>& buffer) {
        auto readers = port->GetBusConnections();
        bool allEmpty = true;

        for(const auto& bridge_reader : readers) {
            std::vector<T> inputData;
            bridge_reader.bridgeReader->ReadData(inputData);

            if(!inputData.empty()) {
                allEmpty = false;
                for(size_t i = 0; i < inputData.size(); i++) {
                    buffer[bridge_reader.bridgeReader].push_back(inputData[i]);
                }
            }
        }

        return allEmpty;
    }

    // 检查缓冲区是否有足够数据的辅助函数
    template<typename T>
    bool CheckBufferReady(const std::map<BufferReader*, std::vector<T>>& buffer, size_t minSize = 1) {
        for(auto it = buffer.begin(); it != buffer.end(); ++it) {
            if(it->second.size() < minSize) {
                return false;
            }
        }
        return true;
    }
};

RegAlgo(RADAR_EchoGenerator_Block);

#endif // RADAR_ECHOGENERATOR_BLOCK_H
