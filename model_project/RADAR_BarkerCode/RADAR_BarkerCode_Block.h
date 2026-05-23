#ifndef RADAR_BARKERCODE_BLOCK_H
#define RADAR_BARKERCODE_BLOCK_H

#include "Block.h"
#include "RADAR_BarkerCode.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_BarkerCode_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_BarkerCode_Block(const std::string& name);
    ~RADAR_BarkerCode_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    RADAR_BarkerCode::CodeLengthEnum ConvertStringToCodeLength(const std::string& value);

    std::unique_ptr<RADAR_BarkerCode> m_radarBarker;

    double m_pri;
    double m_subPulseWidth;
    RADAR_BarkerCode::CodeLengthEnum m_codeLength;
    double m_sampleRate;
};

RegAlgo(RADAR_BarkerCode_Block);

#endif // RADAR_BARKERCODE_BLOCK_H
