#ifndef CHOP_BLOCK_H
#define CHOP_BLOCK_H

#include "Block.h"
#include "Chop.h"
#include <cstddef>
#include <deque>
#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Chop_Block : public SystemVueModelBuilder::Block
{
public:
    Chop_Block(const std::string& name);
    ~Chop_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();
    QueryEnum ConvertStringToQueryEnum(const std::string& value);
    bool InitializeRanges();

    std::unique_ptr<SystemVueModelBuilder::Chop> m_chop;

    int m_nRead;
    int m_nWrite;
    int m_offset;
    QueryEnum m_usePastInputs;

    size_t m_iReadFrom;
    size_t m_iReadNum;
    size_t m_iReadBufSize;
    size_t m_iWriteTo;
    size_t m_iWriteNum;
    size_t m_iWriteBufSize;
    size_t m_iZeroPadFrom;
    size_t m_iZeroPadNum;

    std::deque<double> m_history;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(Chop_Block);

#endif // CHOP_BLOCK_H
