#ifndef DOWNSAMPLECX_BLOCK_H
#define DOWNSAMPLECX_BLOCK_H

#include "Block.h"
#include "DownSampleCx.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API DownSampleCx_Block : public SystemVueModelBuilder::Block
{
public:
	DownSampleCx_Block(const std::string& name);
	~DownSampleCx_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters();

	std::unique_ptr<DownSampleCx> m_downSampleCx;
	int m_factor;
	int m_phase;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(DownSampleCx_Block);
#endif // DOWNSAMPLECX_BLOCK_H
