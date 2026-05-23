#ifndef DOWNSAMPLE_BLOCK_H
#define DOWNSAMPLE_BLOCK_H

#include "Block.h"
#include "DownSample.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API DownSample_Block : public SystemVueModelBuilder::Block
{
public:
	DownSample_Block(const std::string& name);
	~DownSample_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(int factor, int phase);

	int m_factor;
	int m_phase;

	std::unique_ptr<DownSample> m_downSample;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(DownSample_Block);
#endif // DOWNSAMPLE_BLOCK_H
