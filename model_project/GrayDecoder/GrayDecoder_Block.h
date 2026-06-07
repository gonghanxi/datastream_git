#ifndef GRAYDECODER_BLOCK_H
#define GRAYDECODER_BLOCK_H
#include "GrayDecoder.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API GrayDecoder_Block : public Block
{
public:
    GrayDecoder_Block(const std::string& name);
    ~GrayDecoder_Block();
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    void FreeBuffersBlock();
    GrayDecoder::BitOrderE ConvertStringToBitOrderE(const std::string& value);

    std::unique_ptr<GrayDecoder> m_gray;
    int NumBits;
    GrayDecoder::BitOrderE m_BitOrder;

    // ========== 原算法内部缓冲区 ==========
    bool* inBits;
    bool* outBits;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<bool> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<bool> m_outputQueue;
    bool m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数

};
RegAlgo(GrayDecoder_Block);

#endif // GRAYDECODER_BLOCK_H
