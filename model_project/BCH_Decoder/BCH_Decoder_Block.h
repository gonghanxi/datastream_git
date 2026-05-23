#ifndef BCH_DECODER_BLOCK_H
#define BCH_DECODER_BLOCK_H
#include "BCH_Decoder.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API BCH_Decoder_Block : public Block
{
public:
    BCH_Decoder_Block(const std::string& name);
    ~BCH_Decoder_Block() = default;

    bool Run() override;
    bool Setup() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool parseArrayString(const std::string& arrayStr, std::vector<int>& outArray);
    BCH_Decoder::EraseEnum ConvertStringToEraseEnum(const std::string& value);

    bool ModelSetup();
    void buildField();
    int  parsePrimitivePolynomial() const;
    void decodeCore(const std::vector<int> &r_in,
        std::vector<int>       &c_out,
        std::vector<int>       &msg_out);

    std::unique_ptr<BCH_Decoder> m_bch;

    int   M;
    int   K;
    int   T;
    int   CodeLength;

    int* PrimPoly;
    int  PrimPolySize;

    BCH_Decoder::EraseEnum Erase;

    int* ErasePosition;
    int  ErasePositionSize;

    std::vector<int> primdata;
    std::vector<int> erasedata;

    int  N_;
    int  Ns_;
    int  Ks_;
    bool eraseFlagConnected_;
    std::vector<int> alpha_to_;
    std::vector<int> index_of_;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_CodeBuffer;   // 多输入累积缓冲区
    std::vector<int> m_EraseBuffer;
    std::queue<int> m_outputQueue;    // 输出分发队列
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(BCH_Decoder_Block)
#endif // BCH_DECODER_BLOCK_H
