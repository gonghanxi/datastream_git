#ifndef RADAR_WAVEGATE_BLOCK_H
#define RADAR_WAVEGATE_BLOCK_H
#include "RADAR_WaveGate.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RADAR_WaveGate_Block : public Block
{
public:
    RADAR_WaveGate_Block(const std::string& name);
    ~RADAR_WaveGate_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();

    std::unique_ptr<RADAR_WaveGate> m_radar;

    double PRF;
    double StartTime;
    double GateTime;
    double SampleRate;
};
RegAlgo(RADAR_WaveGate_Block);
#endif // RADAR_WAVEGATE_BLOCK_H
