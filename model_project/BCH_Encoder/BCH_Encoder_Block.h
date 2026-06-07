#ifndef BCH_ENCODER_BLOCK_H
#define BCH_ENCODER_BLOCK_H
#include "BCH_Encoder.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API BCH_Encoder_Block : public Block
{
public:
    BCH_Encoder_Block(const std::string& name);
    ~BCH_Encoder_Block() = default;
    bool Run() override;
    bool Setup() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool parseArrayString(const std::string& arrayStr, std::vector<int>& outArray);
    void buildGeneratorBlock();
    void encodeOneBlock(const std::vector<int>& u, std::vector<int>& c_out);

    std::unique_ptr<BCH_Encoder> m_bch;
    int M;
    int K;
    int MsgLength;
    int Ks_ = 0;
    int Ns_ = 0;

    int* GenPoly;
    int  GenPolySize;
    std::vector<int> gendata;
    std::vector<int> g_;
    int parityLen_ = 0;

    bool DataStreamRun();
    bool TimeDrivenRun();
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_MsgBuffer;   // 多输入累积缓冲区
    std::queue<int> m_outputQueue;    // 输出分发队列
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(BCH_Encoder_Block)
#endif // BCH_ENCODER_BLOCK_H
