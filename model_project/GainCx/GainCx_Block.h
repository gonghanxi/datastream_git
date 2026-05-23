#ifndef GAINCX_BLOCK_H
#define GAINCX_BLOCK_H

#include "Block.h"
#include "GainCx.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API GainCx_Block : public SystemVueModelBuilder::Block
{
public:
	GainCx_Block(const std::string& name);
	~GainCx_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(double gain);

	double m_gain;
	std::unique_ptr<GainCx> m_gainCx;
};

RegAlgo(GainCx_Block);

#endif // GAINCX_BLOCK_H
