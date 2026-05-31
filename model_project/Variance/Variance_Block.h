#ifndef VARIANCE_BLOCK_H
#define VARIANCE_BLOCK_H

#include "Variance.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Variance_Block : public SystemVueModelBuilder::Block
{
public:
	Variance_Block(const std::string& name);
	~Variance_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(int blockSize);

	std::unique_ptr<Variance> m_variance;

	int m_blockSize = 1;
	double m_sum = 0.0;
	double m_sumSqr = 0.0;
	int m_sumN = 0;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inBuffer;
    std::queue<double> m_meanQueue;
    std::queue<double> m_varianceQueue;
    double m_lastmean;
    double m_lastvariance;
    int m_inputCount;
    int m_outputCount;
};

RegAlgo(Variance_Block);

#endif // VARIANCE_BLOCK_H
