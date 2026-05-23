#ifndef PEAKDETECTOR_BLOCK_H
#define PEAKDETECTOR_BLOCK_H
#include "PeakDetector.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API PeakDetector_Block : public Block
{
public:
    PeakDetector_Block(const std::string& name);
    ~PeakDetector_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    PeakDetector::SelectedPolarity ConvertStringToSelectedPolarity(const std::string& value);

    void SetDefaultParameters();

    std::unique_ptr<PeakDetector> m_peak;

    double ChargeTimeConstant;
    double DecayTimeConstant;
    double VThreshold;
    double VTransWidth;
    PeakDetector::SelectedPolarity Polarity;

    double polaritySign;
    double VSignal;
    double VDetect;
    double VOut;
    double VTest;
    double SampleRate;
};
RegAlgo(PeakDetector_Block);
#endif // PEAKDETECTOR_BLOCK_H
