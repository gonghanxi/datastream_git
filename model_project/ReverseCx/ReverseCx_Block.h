#ifndef REVERSECX_BLOCK_H
#define REVERSECX_BLOCK_H

#include "Block.h"
#include "ReverseCx.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ReverseCx_Block : public SystemVueModelBuilder::Block
{
public:
	ReverseCx_Block(const std::string& name);
	~ReverseCx_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(int n);

	int m_n;
	std::unique_ptr<ReverseCx> m_reverseCx;
};

RegAlgo(ReverseCx_Block);

#endif // REVERSECX_BLOCK_H
