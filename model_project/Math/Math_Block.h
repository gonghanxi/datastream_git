#pragma once

#include "MathModel.h"
#include "Block.h"

#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Math_Block : public SystemVueModelBuilder::Block
{
public:
    Math_Block(const std::string& name);
    ~Math_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(Math::SelectedFunctionType functionType);

private:
    Math::SelectedFunctionType ConvertStringToFunctionType(const std::string& value);
    void SetDefaultParamters();

    std::unique_ptr<Math> m_math;
    Math::SelectedFunctionType m_functionType;
};

RegAlgo(Math_Block);
