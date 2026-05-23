#ifndef MAPPER_BLOCK_H
#define MAPPER_BLOCK_H
#include "Mapper.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Mapper_Block : public Block
{
public:
    Mapper_Block(const std::string& name);
    ~Mapper_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    Mapper::ModTypeEnum ConvertStringToModTypeEnum(const std::string& value);
    Mapper::DefaultStateEnum ConvertStringToDefaultStateEnum(const std::string& value);
    Mapper::BitOrderEnum ConvertStringToBitOrderEnum(const std::string& value);
    void SetDefaultParameters();
    bool ModelSetup();

    std::unique_ptr<Mapper> m_mapper;

    //参数
    Mapper::ModTypeEnum ModType;
    Mapper::DefaultStateEnum DefaultState;
    Mapper::BitOrderEnum     BitOrder;

    DComplexMatrix   MappingTable;
    double           Ratio_R2_R1;  // R2/R1
    double           Ratio_R3_R1;  // R3/R1
    double           Ratio_R4_R1;  // R4/R1
    IntMatrix        RingStates;
    DoubleMatrix     RingMagnitudes;
    DoubleMatrix     RinginitialPhases;
    IntMatrix        States;

    int m_symbolLength;
    int m_M;
    std::vector<std::complex<double> > m_table;
    std::vector<int> m_stateToIndex; // only used when CustomAPSK && DefaultState==FALSE

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<bool> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(Mapper_Block)
#endif // MAPPER_BLOCK_H
