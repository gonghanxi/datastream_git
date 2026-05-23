#ifndef RADAR_UNAMBRANGE_BLOCK_H
#define RADAR_UNAMBRANGE_BLOCK_H

#include "RADAR_UnAmbRange.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_UnAmbRange_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_UnAmbRange_Block(const std::string& name);
    ~RADAR_UnAmbRange_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();

    std::unique_ptr<RADAR_UnAmbRange> m_radarUnAmbRange;

    SystemVueModelBuilder::Matrix<double> m_PRI;
    int m_CPI_Num;
    double m_SampleRate;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<int>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_UnAmbRange_Block);

#endif // RADAR_UNAMBRANGE_BLOCK_H
