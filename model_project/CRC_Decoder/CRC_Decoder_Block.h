#ifndef CRC_DECODER_BLOCK_H
#define CRC_DECODER_BLOCK_H
#include "CRC_Decoder.h"
#include "Block.h"
#include <cstdint>
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API CRC_Decoder_Block : public Block
{
public:
    CRC_Decoder_Block(const std::string& name);
    ~CRC_Decoder_Block();

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    void SetParameters();
private:
    void SetDefaultParameters();
    CRC_Decoder::ParityPositionEnum ConvertStringToParityPositionEnum(const std::string& value);
    CRC_Decoder::YesNoEnum ConvertStringToYesNoEnum(const std::string& value);

    // ========== 内联的原算法方法 ==========
    int  boundaryCheckBlock(char functionTag);
    int  computeCRCLengthBlock(int poly) const;
    void computePolynomialMasksBlock();
    void crcComputeRemainderBitsBlock(const bool* msgLogical, bool* crcBits);

    std::unique_ptr<CRC_Decoder> m_crc;

    CRC_Decoder::ParityPositionEnum ParityPosition;
    CRC_Decoder::YesNoEnum ReverseData;
    CRC_Decoder::YesNoEnum ReverseParity;
    CRC_Decoder::YesNoEnum ComplementParity;

    int MessageLength;
    int InitialState;
    int Polynomial;

    // ========== 原算法内部状态 ==========
    int      m_InputFrmLen;
    int      m_CRCLength;
    uint32_t m_crcMask;
    uint32_t m_polyNoMsb;
    bool*    m_msgFrame;
    bool*    m_msgLogical;
    bool*    m_crcRx;
    bool*    m_crcExp;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<bool> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<bool> m_outputQueue;    // 输出分发队列
    bool m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(CRC_Decoder_Block);

#endif // CRC_DECODER_BLOCK_H
