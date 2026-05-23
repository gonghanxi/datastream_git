#pragma once

#include "LinearQuantizer.h"
#include "Block.h"

#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API LinearQuantizer_Block : public SystemVueModelBuilder::Block
{
public:
    LinearQuantizer_Block(const std::string& name);
    ~LinearQuantizer_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParamters();

    std::unique_ptr<LinearQuantizer> m_linearQuantizer;
    int m_levels;
    double m_low;
    double m_high;
};

RegAlgo(LinearQuantizer_Block);
