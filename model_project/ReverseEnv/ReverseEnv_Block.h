#ifndef REVERSEENV_BLOCK_H
#define REVERSEENV_BLOCK_H

#include "Block.h"
#include "ReverseEnv.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ReverseEnv_Block : public SystemVueModelBuilder::Block
{
public:
	ReverseEnv_Block(const std::string& name);
	~ReverseEnv_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(int n);

	int m_n;
	std::unique_ptr<ReverseEnv> m_reverseEnv;
};

RegAlgo(ReverseEnv_Block);

#endif // REVERSEENV_BLOCK_H
