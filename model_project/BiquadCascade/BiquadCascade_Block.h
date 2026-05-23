#ifndef BIQUADCASCADE_BLOCK_H
#define BIQUADCASCADE_BLOCK_H

#include "Block.h"
#include "BiquadCascade.h"

#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API BiquadCascade_Block : public SystemVueModelBuilder::Block
{
public:
	BiquadCascade_Block(const std::string& name);
	~BiquadCascade_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	bool BuildCascade();

	std::unique_ptr<SystemVueModelBuilder::BiquadCascade> m_biquadCascade;

	std::vector<double> m_taps;
	std::vector<double> m_state1;
	std::vector<double> m_state2;
	std::size_t m_numBiquads;

	struct BiquadBlock
	{
		double b0, b1, b2;
		double a1, a2;
	};

	std::vector<BiquadBlock> m_blocks;
};

RegAlgo(BiquadCascade_Block);

#endif // BIQUADCASCADE_BLOCK_H
