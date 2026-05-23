#ifndef RADAR_DOA_BLOCK_H
#define RADAR_DOA_BLOCK_H

#include "RADAR_DOA.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_DOA_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_DOA_Block(const std::string& name);
    ~RADAR_DOA_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    RADAR_DOA::SelectedMTI_Type ConvertStringToSelectedMTI_Type(const std::string& value);

    void SetDefaultParameters();

    // 参数定义
    double m_Fc;		// 中心频率
    double m_D;		// 阵元间距
    int m_NumOfCh;		// 通道数/阵元数
    int m_SnapShotLen;	// 快拍长度
    RADAR_DOA::SelectedMTI_Type m_MTI_Type;		// 算法类型
    std::unique_ptr<RADAR_DOA> m_RADAR_DOA;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*,std::vector<std::complex<double>>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(RADAR_DOA_Block);
#endif // RADAR_DOA_BLOCK_H
