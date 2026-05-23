#ifndef UPSAMPLEENV_BLOCK_H
#define UPSAMPLEENV_BLOCK_H

#include "UpSampleEnv.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API UpSampleEnv_Block : public SystemVueModelBuilder::Block
{
public:
    UpSampleEnv_Block(const std::string& name);
    ~UpSampleEnv_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;



    void SetParameters(int factor = 5,
                       int phase = 0,
                       UpSampleEnv::ModeEnum mode = UpSampleEnv::Holdsample);
private:
    void UpdateCharacterizationFrequency();

    void SetDefaultParameters();

    UpSampleEnv::ModeEnum ConvertStringToModeEnum(const std::string& value);

    std::unique_ptr<UpSampleEnv> m_upsampleEnv;

    int m_factor;
    int m_phase;
    UpSampleEnv::ModeEnum m_mode;

    double FcOut;
    std::complex<double> m_prevEnv;
    SystemVueModelBuilder::EnvelopeCircularBuffer interpEnv;
};

RegAlgo(UpSampleEnv_Block);

#endif // UPSAMPLEENV_BLOCK_H
