#ifndef TIMESYNCHRONIZER_BLOCK_H
#define TIMESYNCHRONIZER_BLOCK_H

#include "Block.h"
#include "TimeSynchronizer.h"
#include <memory>
#include <deque>
#include <map>
#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API TimeSynchronizer_Block : public SystemVueModelBuilder::Block
{
public:
    TimeSynchronizer_Block(const std::string& name);
    ~TimeSynchronizer_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    TimeSynchronizer::ModeEnum ConvertStringToModeEnum(const std::string& value);
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<TimeSynchronizer> m_TimeSynchronizer;
    TimeSynchronizer::ModeEnum m_Mode;

    // ========== 同步算法内部状态 ==========
    struct SampleD { double v; double t; };
    std::vector<std::deque<SampleD>> m_fifos;
    std::vector<double> m_lastValue;
    unsigned long long m_firingCount = 0;

    // ========== 时间驱动多通道累积缓冲区 ==========
    std::map<BufferReader*, std::vector<double>> m_inputChannelBuffer;
    std::queue<std::vector<double>> m_outputQueue;

    static inline double eps() { return 1e-15; }
};

RegAlgo(TimeSynchronizer_Block);

#endif // TIMESYNCHRONIZER_BLOCK_H
