#pragma once

#include "MathCx.h"
#include "Block.h"

#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API MathCx_Block : public SystemVueModelBuilder::Block
{
public:
    MathCx_Block(const std::string& name);
    ~MathCx_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(MathCx::SelectedFunctionType functionType);

private:
    MathCx::SelectedFunctionType ConvertStringToFunctionType(const std::string& value);
    void SetDefaultParamters();

    std::unique_ptr<MathCx> m_mathCx;
    MathCx::SelectedFunctionType m_functionType;
};

RegAlgo(MathCx_Block);
