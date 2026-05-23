#ifndef AMPLIFIER_BLOCK_H
#define AMPLIFIER_BLOCK_H

#include "Amplifier.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Amplifier_Block : public SystemVueModelBuilder::Block
{
public:
    Amplifier_Block(const std::string& name);
    ~Amplifier_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();
    bool UpdateCharacterizationFrequency(double& fc);

    Amplifier::GainUnitEnum ConvertStringToGainUnit(const std::string& value);
    Amplifier::QuantizationEnum ConvertStringToQuantization(const std::string& value);
    Amplifier::GainErrorEnum ConvertStringToGainError(const std::string& value);
    Amplifier::GCTypeEnum ConvertStringToGCType(const std::string& value);

    std::unique_ptr<Amplifier> m_amplifier;

    Amplifier::GainUnitEnum m_gainUnit;
    double m_gain;

    Amplifier::QuantizationEnum m_quantization;
    int m_numBits;
    double m_stepSize;
    double m_maxGain;
    SystemVueModelBuilder::Matrix<double> m_levels;

    Amplifier::GainErrorEnum m_gainError;
    double m_stdDev;
    double m_min;
    double m_max;
    double m_customError;

    double m_noiseFigure;
    Amplifier::GCTypeEnum m_gcType;
    double           TOIout;          // W 输出三阶截点功率
    double           dBc1out;         // W 输出 1dB 压缩功率
    double           PSat;            // W 饱和功率
    double           GCSat;           // dB 饱和处增益压缩，UI 用 W 表示
    int              RappS;           // Rapp 平滑因子
    SystemVueModelBuilder::Matrix<double> GComp; // 三元组表

    double           RefR;            // ohm

    bool m_runtimeInitialized = false;
    SimuParameter simulator_param;
};

RegAlgo(Amplifier_Block);

#endif // AMPLIFIER_BLOCK_H
