#ifndef GAINENV_BLOCK_H
#define GAINENV_BLOCK_H

#include "Block.h"
#include "GainEnv.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API GainEnv_Block : public SystemVueModelBuilder::Block
{
public:
	GainEnv_Block(const std::string& name);
	~GainEnv_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(double gain);

	double m_gain;
	std::unique_ptr<GainEnv> m_gainEnv;
};

RegAlgo(GainEnv_Block);

#endif // GAINENV_BLOCK_H
