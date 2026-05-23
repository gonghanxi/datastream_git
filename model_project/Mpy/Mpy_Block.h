#ifndef MPY_BLOCK_H
#define MPY_BLOCK_H

#include "Block.h"
#include "Mpy.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Mpy_Block : public SystemVueModelBuilder::Block
{
public:
	Mpy_Block(const std::string& name);
	~Mpy_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();

	std::unique_ptr<Mpy> m_mpy;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(Mpy_Block);
#endif // MPY_BLOCK_H
