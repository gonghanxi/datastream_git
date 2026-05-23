#ifndef RADAR_PF_MEASUREMENT_BLOCK_H
#define RADAR_PF_MEASUREMENT_BLOCK_H
#include "RADAR_Pf_Measurement.h"
#include "Block.h"
#include "SimulationControl.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RADAR_Pf_Measurement_Block : public Block
{
public:
    RADAR_Pf_Measurement_Block(const std::string& name);
    ~RADAR_Pf_Measurement_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;
    bool Done() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    void cleanup();
    bool openFileForAppend();
    bool ModelSetup();

    std::unique_ptr<RADAR_Pf_Measurement> m_radar;

    int Start;
    int Stop;
    char* FileName;

    std::string m_UserId;

    QFile m_qfile;
    QTextStream m_stream;
    QString m_fullPath;
    QString m_WritePath;

    int FalseCount;

    SinkControl m_control;
};
RegAlgo(RADAR_Pf_Measurement_Block);

#endif // RADAR_PF_MEASUREMENT_BLOCK_H
