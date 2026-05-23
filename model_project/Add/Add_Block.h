#ifndef ADD_BLOCK_H
#define ADD_BLOCK_H

#include "Block.h"
#include "Add.h"
#include <queue>

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Add_Block : public SystemVueModelBuilder::Block
{
public:
	Add_Block(const std::string& name);
	~Add_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();

    bool DataStreamRun();
    bool TimeDrivenRun();

	std::unique_ptr<Add> m_add;



    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数

    // ========== 时间驱动配置 ==========
    double m_samplePeriod;               // 模型采样周期
};
RegAlgo(Add_Block);
#endif // ADD_BLOCK_H
