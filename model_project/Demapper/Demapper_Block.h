#ifndef DEMAPPER_BLOCK_H
#define DEMAPPER_BLOCK_H

#include "Block.h"
#include "Demapper.h"

#include <complex>
#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Demapper_Block : public SystemVueModelBuilder::Block
{
public:
    Demapper_Block(const std::string& name);
    ~Demapper_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();

    // ---- enum aliases ----
    using ModTypeEnum    = SystemVueModelBuilder::Demapper::ModTypeEnum;
    using BitOrderEnum   = SystemVueModelBuilder::Demapper::BitOrderEnum;
    using DefaultStateEnum = SystemVueModelBuilder::Demapper::DefaultStateEnum;

    ModTypeEnum      ConvertStringToModTypeEnum(const std::string& value);
    BitOrderEnum     ConvertStringToBitOrderEnum(const std::string& value);
    DefaultStateEnum ConvertStringToDefaultStateEnum(const std::string& value);

    // ---- algorithm instance ----
    std::unique_ptr<SystemVueModelBuilder::Demapper> m_demapper;

    // ---- parameters ----
    ModTypeEnum      m_ModType;
    BitOrderEnum     m_BitOrder;

    DComplexMatrix   m_MappingTable;

    double           m_Ratio_R2_R1;
    double           m_Ratio_R3_R1;
    double           m_Ratio_R4_R1;

    IntMatrix        m_RingStates;
    DoubleMatrix     m_RingMagnitudes;
    DoubleMatrix     m_RinginitialPhases;
    DefaultStateEnum m_DefaultState;
    IntMatrix        m_States;

    // ---- symbol length cache (set after Setup) ----
    int m_symbolLength;

    // ---- constellation table (built in Initialize via Setup, used in DataStreamRun) ----
    int                      m_M;
    std::vector<std::complex<double>> m_constellationTable;
    std::vector<int>         m_indexToState;
};

RegAlgo(Demapper_Block);

#endif // DEMAPPER_BLOCK_H
