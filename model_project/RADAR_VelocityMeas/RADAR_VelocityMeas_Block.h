#ifndef RADAR_VELOCITYMEAS_BLOCK_H
#define RADAR_VELOCITYMEAS_BLOCK_H

#include "RADAR_VelocityMeas.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_VelocityMeas_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_VelocityMeas_Block(const std::string& name);
    ~RADAR_VelocityMeas_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();

    std::unique_ptr<RADAR_VelocityMeas> m_radarVelocityMeas;

    double m_PRI;
    int m_CPI_Num;
    double m_SampleRate;
    double m_fc;

    int m_PRINum;
    int m_portRate;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_VelocitytQueue;    // 输出分发队列
    std::queue<int> m_IndextQueue;

    double m_lastVelocity;
    int m_lastIndex;

    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_VelocityMeas_Block);

#endif // RADAR_VELOCITYMEAS_BLOCK_H
