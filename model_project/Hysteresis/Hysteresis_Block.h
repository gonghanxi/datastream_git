#ifndef HYSTERESIS_BLOCK_H
#define HYSTERESIS_BLOCK_H
#include "Hysteresis.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Hysteresis_Block : public Block
{
public:
    Hysteresis_Block(const std::string& name);
    ~Hysteresis_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<Hysteresis> m_Hysteresis;
    double Bandwidth;
    double Backlash;
    double Gain;

    double SampleRate;
    double InternalState;
    double Difference;

};
RegAlgo(Hysteresis_Block);


#endif // HYSTERESIS_BLOCK_H
