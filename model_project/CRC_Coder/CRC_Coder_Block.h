#ifndef CRC_CODER_BLOCK_H
#define CRC_CODER_BLOCK_H
#include "CRC_Coder.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API CRC_Coder_Block : public Block
{
public:
    CRC_Coder_Block(const std::string& name);
    ~CRC_Coder_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    void SetParameters();
private:
    void SetDefaultParameters();
    bool ModelSetup();
    CRC_Coder::ParityPositionEnum ConvertStringToParityPositionEnum(const std::string& value);
    CRC_Coder::YesNoEnum ConvertStringToYesNoEnum(const std::string& value);

    std::unique_ptr<CRC_Coder> m_crc;

    CRC_Coder::ParityPositionEnum ParityPosition;
    CRC_Coder::YesNoEnum ReverseData;
    CRC_Coder::YesNoEnum ReverseParity;
    CRC_Coder::YesNoEnum ComplementParity;

    int MessageLength;
    int InitialState;
    int Polynomial;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<bool> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<bool> m_outputQueue;    // 输出分发队列
    bool m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(CRC_Coder_Block);
#endif // CRC_CODER_BLOCK_H
