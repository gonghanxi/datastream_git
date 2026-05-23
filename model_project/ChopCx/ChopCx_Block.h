#ifndef ChopCx_Block_H
#define ChopCx_Block_H

#include "Block.h"
#include "ChopCx.h"
#include <cstddef>
#include <deque>
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ChopCx_Block : public SystemVueModelBuilder::Block
{
public:
    ChopCx_Block(const std::string& name);
    ~ChopCx_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();
    QueryEnum ConvertStringToQueryEnum(const std::string& value);
    bool InitializeRanges();

    std::unique_ptr<SystemVueModelBuilder::ChopCx> m_chop;

    int m_nRead;
    int m_nWrite;
    int m_offset;
    QueryEnum m_usePastInputs;

    std::size_t m_iReadFrom;
    std::size_t m_iReadNum;
    std::size_t m_iReadBufSize;
    std::size_t m_iWriteTo;
    std::size_t m_iWriteNum;
    std::size_t m_iWriteBufSize;
    std::size_t m_iZeroPadFrom;
    std::size_t m_iZeroPadNum;

    std::deque<std::complex<double>> m_history;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(ChopCx_Block);

#endif // ChopCx_Block_H

