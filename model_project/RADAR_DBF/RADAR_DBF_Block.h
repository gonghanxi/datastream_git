#ifndef RADAR_DBF_BLOCK_H
#define RADAR_DBF_BLOCK_H

#include "RADAR_DBF.h"
#include "Block.h"
#include <queue>
#include <map>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_DBF_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_DBF_Block(const std::string& name);
    ~RADAR_DBF_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    std::unique_ptr<RADAR_DBF> m_dbf;

    bool DataStreamRun();
    bool TimeDrivenRun();

    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<std::complex<double>>> m_inputBuffer;   // input bus 多通道累积缓冲区
    std::map<BufferReader*, std::vector<std::complex<double>>> m_weightBuffer;  // weight bus 多通道累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_DBF_Block);

#endif // RADAR_DBF_BLOCK_H
