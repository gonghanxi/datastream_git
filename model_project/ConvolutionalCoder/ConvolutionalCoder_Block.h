#ifndef CONVOLUTIONALCODER_BLOCK_H
#define CONVOLUTIONALCODER_BLOCK_H
#include "ConvolutionalCoder.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API ConvolutionalCoder_Block : public Block
{
public:
    ConvolutionalCoder_Block(const std::string& name);
    ~ConvolutionalCoder_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool parseArrayString(const std::string& arrayStr, std::vector<int>& outArray);
    bool ModelSetup();
    ConvolutionalCoder::CodingRateEnum ConvertStringToCodingRateEnum(const std::string& value);
    ConvolutionalCoder::ZeroTailEnum ConvertStringToZeroTailEnum(const std::string& value);

    std::unique_ptr<ConvolutionalCoder> m_con;


    ConvolutionalCoder::CodingRateEnum CodingRate;
    int           ConstraintLength;

    int* Polynomial;
    int  PolynomialSize;
    std::vector<int> primdata;

    ConvolutionalCoder::ZeroTailEnum ZeroTail;
    int         BitSequenceLength;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<bool> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<bool> m_outputQueue;    // 输出分发队列
    bool m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(ConvolutionalCoder_Block)
#endif // CONVOLUTIONALCODER_BLOCK_H
