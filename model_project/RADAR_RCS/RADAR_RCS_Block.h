#pragma once

#include "RADAR_RCS.h"
#include "Block.h"

#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_RCS_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_RCS_Block(const std::string& name);
    ~RADAR_RCS_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    RADAR_RCS::SelectedType ConvertStringToType(const std::string& value);
    void SetDefaultParamters();

    std::unique_ptr<RADAR_RCS> m_radarRcs;

    RADAR_RCS::SelectedType m_type;
    double m_va;
    double m_vb;
    double m_tStep;
    double m_durationTime;
};

RegAlgo(RADAR_RCS_Block);
