#ifndef BITFORMATTER_BLOCK_H
#define BITFORMATTER_BLOCK_H

#include "BitFormatter.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API BitFormatter_Block : public SystemVueModelBuilder::Block
{
public:
    BitFormatter_Block(const std::string& name);
    ~BitFormatter_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(int samplesperbit = 1, BitFormatter::SelectedFormat format = BitFormatter::NRZ,
                       double logiczerolevel = -1, double logiconelevel = 1);
private:
    BitFormatter::SelectedFormat ConvertStringToSelectedFormat(const std::string& value);

    void SetDefaultParameters();

    std::unique_ptr<BitFormatter> m_bitformatter;

    int m_SamplesPerBit;
    BitFormatter::SelectedFormat m_Format;
    double m_LogicZeroLevel;
    double m_LogicOneLevel;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<bool> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(BitFormatter_Block);

#endif // BITFORMATTER_BLOCK_H
