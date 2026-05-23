#ifndef RADAR_RANGEMEAS_BLOCK_H
#define RADAR_RANGEMEAS_BLOCK_H

#include "RADAR_RangeMeas.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_RangeMeas_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_RangeMeas_Block(const std::string& name);
    ~RADAR_RangeMeas_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();

    std::unique_ptr<RADAR_RangeMeas> m_radarRangeMeas;

    double m_PRI;
    int m_CPI_Num;
    double m_SampleRate;

    int m_PRINum;
    int m_portRate;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_RangeQueue;    // 输出分发队列
    std::queue<int> m_IndexQueue;
    double m_lastRange;                 // 上次输出值（用于保持）
    int m_lastIndex;
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_RangeMeas_Block);

#endif // RADAR_RANGEMEAS_BLOCK_H
