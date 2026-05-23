#ifndef REVERSEINT_BLOCK_H
#define REVERSEINT_BLOCK_H

#include "Block.h"
#include "ReverseInt.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ReverseInt_Block : public SystemVueModelBuilder::Block
{
public:
	ReverseInt_Block(const std::string& name);
	~ReverseInt_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(int n);

	int m_n;
	std::unique_ptr<ReverseInt> m_reverseInt;
};

RegAlgo(ReverseInt_Block);

#endif // REVERSEINT_BLOCK_H
