#ifndef SUB_BLOCK_H
#define SUB_BLOCK_H

#include "Block.h"
#include "Sub.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Sub_Block : public SystemVueModelBuilder::Block
{
public:
	Sub_Block(const std::string& name);
	~Sub_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();

	std::unique_ptr<Sub> m_sub;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_posBuffer;
    std::map<BufferReader*, std::vector<double>> m_negBuffer;
    std::queue<double> m_outputQueue;
    double m_lastOutput;
    int m_inputCount;
    int m_outputCount;
};
RegAlgo(Sub_Block);
#endif // SUB_BLOCK_H
