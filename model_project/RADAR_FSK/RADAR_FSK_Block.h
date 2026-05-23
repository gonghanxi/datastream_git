#ifndef RADAR_FSK_BLOCK_H
#define RADAR_FSK_BLOCK_H

#include "Block.h"
#include "RADAR_FSK.h"
#include "DataTypesAndParsers.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_FSK_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_FSK_Block(const std::string& name);
    ~RADAR_FSK_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    RADAR_FSK::Types ConvertStringToType(const std::string& value);
    RADAR_FSK::CodeLengthEnum ConvertStringToCodeLength(const std::string& value);

    std::unique_ptr<RADAR_FSK> m_radarFsk;

    RADAR_FSK::Types m_type;
    double m_pri;
    SystemVueModelBuilder::Matrix<double> m_fhSequence;
    SystemVueModelBuilder::Matrix<double> m_fskpskSequence;
    SystemVueModelBuilder::Matrix<double> m_timeIntervals;
    double m_fskpskSubTimePeriod;
    RADAR_FSK::CodeLengthEnum m_codeLength;
    double m_sampleRate;
};

RegAlgo(RADAR_FSK_Block);

#endif // RADAR_FSK_BLOCK_H
