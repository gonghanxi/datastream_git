#ifndef CODERRS_BLOCK_H
#define CODERRS_BLOCK_H
#include "CoderRs.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API CoderRS_Block : public Block
{
public:
    CoderRS_Block(const std::string& name);
    ~CoderRS_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    bool parseArrayString(const std::string& arrayStr, std::vector<int>& outArray);
    void buildFieldBlock();
    void buildGeneratorBlock();
    int  gf_add(int a, int b) const;
    int  gf_mul(int a, int b) const;

    std::unique_ptr<CoderRS> m_code;

    int   GF;
    int   CodeLength;
    int   MessageLength;

    int*  PrimPoly;
    int   PrimPolySize;
    std::vector<int> primdata;

    int   Root;

    int n_ = 0;
    int k_ = 0;
    int fieldMask_ = 0;
    int maxExp_ = 0;
    std::vector<int> alpha_to_;
    std::vector<int> index_of_;
    std::vector<int> g_;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<int> m_outputQueue;    // 输出分发队列
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(CoderRS_Block)
#endif // CODERRS_BLOCK_H
