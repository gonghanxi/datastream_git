#ifndef GAIN_BLOCK_H
#define GAIN_BLOCK_H

#include "Block.h"
#include "Gain.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Gain_Block : public SystemVueModelBuilder::Block
{
public:
    Gain_Block(const std::string& name);
    ~Gain_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;


    void SetParameters(double gain = 1.0);


private:
    void SetDefaultParameters();

    std::unique_ptr<Gain> m_Gain;
    double m_gain;
};


RegAlgo(Gain_Block);


#endif // GAIN_BLOCK_H
