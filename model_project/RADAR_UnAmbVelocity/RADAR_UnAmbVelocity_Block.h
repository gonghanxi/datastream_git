#ifndef RADAR_UNAMBVELOCITY_BLOCK_H
#define RADAR_UNAMBVELOCITY_BLOCK_H

#include "RADAR_UnAmbVelocity.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_UnAmbVelocity_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_UnAmbVelocity_Block(const std::string& name);
    ~RADAR_UnAmbVelocity_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    RADAR_UnAmbVelocity::SelectedDirection ConvertStringToSelectedDirection(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<RADAR_UnAmbVelocity> m_radarUnAmbVelocity;

    SystemVueModelBuilder::Matrix<double> m_PRI;
    int m_CPI_Num;
    double m_fc;
    double m_SampleRate;
    RADAR_UnAmbVelocity::SelectedDirection m_Direction;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<int>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_UnAmbVelocity_Block);

#endif // RADAR_UNAMBVELOCITY_BLOCK_H
