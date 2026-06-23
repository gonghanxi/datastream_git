#ifndef GAINFXP_BLOCK_H
#define GAINFXP_BLOCK_H

#include "Block.h"
#include "GainFxp.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API GainFxp_Block : public SystemVueModelBuilder::Block
{
public:
    GainFxp_Block(const std::string& name);
    ~GainFxp_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters(double gain = 1.0);

private:
    void SetDefaultParameters();

    std::unique_ptr<GainFxp> m_GainFxp;
    double m_gain;
    int m_fxpPos;
};

RegAlgo(GainFxp_Block);

#endif // GAINFXP_BLOCK_H
