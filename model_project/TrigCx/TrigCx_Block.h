#ifndef TRIGCX_BLOCK_H
#define TRIGCX_BLOCK_H
#include "TrigCx.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API TrigCx_Block : public Block
{
public:
    TrigCx_Block(const std::string& name);
    ~TrigCx_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    TrigCx::SelectedFunctionType ConvertStringToSelectedFunctionType(const std::string& value);

    void SetDefaultParameters();

    std::unique_ptr<TrigCx> m_TrigCx;

    TrigCx::SelectedFunctionType	m_FunctionType;
};
RegAlgo(TrigCx_Block);

#endif // TRIGCX_BLOCK_H
