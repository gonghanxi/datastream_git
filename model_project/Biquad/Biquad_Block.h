#ifndef BIQUAD_BLOCK_H
#define BIQUAD_BLOCK_H

#include "Block.h"
#include "Biquad.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Biquad_Block : public SystemVueModelBuilder::Block
{
public:
	Biquad_Block(const std::string& name);
	~Biquad_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters();

	std::unique_ptr<SystemVueModelBuilder::Biquad> m_biquad;

	double m_dD1;
	double m_dD2;
	double m_dN0;
	double m_dN1;
	double m_dN2;
	double m_dState1;
	double m_dState2;
};

RegAlgo(Biquad_Block);

#endif // BIQUAD_BLOCK_H
