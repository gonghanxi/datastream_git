#ifndef GAININT_BLOCK_H
#define GAININT_BLOCK_H

#include "Block.h"
#include "GainInt.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API GainInt_Block : public SystemVueModelBuilder::Block
{
public:
	GainInt_Block(const std::string& name);
	~GainInt_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(double gain);

	double m_gain;
	std::unique_ptr<GainInt> m_gainInt;
};

RegAlgo(GainInt_Block);

#endif // GAININT_BLOCK_H
