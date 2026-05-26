#ifndef RADAR_ADBF_BLOCK_H
#define RADAR_ADBF_BLOCK_H

#include "Block.h"
#include "RADAR_ADBF.h"

#include <complex>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_ADBF_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_ADBF_Block(const std::string& name);
    ~RADAR_ADBF_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    bool DataStreamRun();

    // algorithm helpers
    int getNumX() const;
    int getNumY() const;
    int getNumElements() const;
    void buildSteeringVector(int nx, int ny, double dx, double dy,
                             double thetaDeg, double phiDeg,
                             std::vector<std::complex<double>>& a) const;
    bool solveLinearSystem(std::vector<std::vector<std::complex<double>>> A,
                           const std::vector<std::complex<double>>& b,
                           std::vector<std::complex<double>>& x) const;
    void fallbackConventionalWeight(const std::vector<std::complex<double>>& a,
                                    std::vector<std::complex<double>>& w) const;
    static double deg2rad(double x);

    std::unique_ptr<RADAR_ADBF> m_radar_adbf;

    double m_NumOfXAntElement;
    double m_NumOfYAntElement;
    double m_Dx;
    double m_Dy;
    int    m_NumOfSamples;
    double m_Theta;
    double m_Phi;
    double m_SampleRate;
};

RegAlgo(RADAR_ADBF_Block);

#endif // RADAR_ADBF_BLOCK_H
