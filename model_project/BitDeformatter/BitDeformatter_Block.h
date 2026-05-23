#ifndef BITDEFORMATTER_BLOCK_H
#define BITDEFORMATTER_BLOCK_H
#include "BitDeformatter.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API BitDeformatter_Block : public Block
{
public:
    BitDeformatter_Block(const std::string& name);
    ~BitDeformatter_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    BitDeformatter::SelectedFormat ConvertStringToSelectedFormat(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<BitDeformatter> m_BitDeformatter;

    BitDeformatter::SelectedFormat m_Format;
    int m_SamplesPerBit;
    double m_LogicZeroLevel;
    double m_LogicOneLevel;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<bool> m_outputQueue;    // 输出分发队列
    bool m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(BitDeformatter_Block);
#endif // BITDEFORMATTER_BLOCK_H
