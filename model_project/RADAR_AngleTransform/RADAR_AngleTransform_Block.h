#ifndef RADAR_ANGLETRANSFORM_BLOCK_H
#define RADAR_ANGLETRANSFORM_BLOCK_H

#include "Block.h"
#include "RADAR_AngleTransform.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_AngleTransform_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_AngleTransform_Block(const std::string& name);
    ~RADAR_AngleTransform_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // ---- enum alias and conversion ----
    using TransformTypeEnum = RADAR_AngleTransform::TransformTypeEnum;
    TransformTypeEnum ConvertStringToTransformTypeEnum(const std::string& value);

    // ---- algorithm instance ----
    std::unique_ptr<RADAR_AngleTransform> m_algo;

    // ---- parameters ----
    TransformTypeEnum m_TransformType;

    // ---- inlined private algorithm helpers ----
    static void antennaToRadar(double theta, double phi, double& elevation, double& azimuth);
    static void radarToAntenna(double elevation, double azimuth, double& theta, double& phi);
    static double clampUnit(double x);
    static double sanitize(double x, double fallback);

    // ---- TimeDrivenRun buffers ----
    std::vector<double> m_inElBuffer;
    std::vector<double> m_inAzBuffer;
    std::queue<double>  m_outElQueue;
    std::queue<double>  m_outAzQueue;
};

RegAlgo(RADAR_AngleTransform_Block);

#endif // RADAR_ANGLETRANSFORM_BLOCK_H
