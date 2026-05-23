#ifndef GRAYENCODER_BLOCK_H
#define GRAYENCODER_BLOCK_H
#include "GrayEncoder.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API GrayEncoder_Block : public Block
{
public:
    GrayEncoder_Block(const std::string& name);
    ~GrayEncoder_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    GrayEncoder::BitOrder ConvertStringToBitOrderE(const std::string& value);

    std::unique_ptr<GrayEncoder> m_gray;
    int NumBits;
    GrayEncoder::BitOrder m_BitOrder;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<bool> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<bool> m_outputQueue;
    bool m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(GrayEncoder_Block);

#endif // GRAYENCODER_BLOCK_H
