#pragma once

#include "AsyncCommutatorCx.h"
#include "Block.h"
#include "DataTypesAndParsers.h"
#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AsyncCommutatorCx_Block : public SystemVueModelBuilder::Block
{
public:
    AsyncCommutatorCx_Block(const std::string& name);
    ~AsyncCommutatorCx_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParamters();

    std::unique_ptr<AsyncCommutatorCx> m_asyncCommutatorCx;
    SystemVueModelBuilder::Matrix<int> m_blockSizes;

    bool ModelSetup();
    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<std::complex<double>>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(AsyncCommutatorCx_Block);
