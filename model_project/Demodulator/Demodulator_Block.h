#ifndef DEMODULATOR_BLOCK_H
#define DEMODULATOR_BLOCK_H

#include "Block.h"
#include "Demodulator.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Demodulator_Block : public SystemVueModelBuilder::Block
{
public:
	Demodulator_Block(const std::string& name);
	~Demodulator_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	Demodulator::OutputTypeEnum ConvertStringToOutputType(const std::string& value);
	Demodulator::MirrorEnum ConvertStringToMirror(const std::string& value);
	Demodulator::IQImpEnum ConvertStringToIQImp(const std::string& value);
	void SetDefaultParamters();
	void SetParameters();

	static double deg2rad(double d);
	double unwrapPhase(double rawThetaRad);

	Demodulator::OutputTypeEnum m_outputType;
	double m_ampSensitivity;
	double m_phaseSensitivity;
	double m_freqSensitivity;
	double m_fCarrier;
	double m_initialPhase;
	Demodulator::MirrorEnum m_mirrorSignal;
	Demodulator::IQImpEnum m_showIQImpairments;
	double m_gainImbalance;
	double m_phaseImbalance;
	double m_iOriginOffset;
	double m_qOriginOffset;
	double m_iqRotation;

	double m_prevThetaRad;
    double m_prevTime;
    bool m_havePrev;
    unsigned long long GetCount() const { return m_iFiringCount; }
    void Advance() { ++m_iFiringCount; }
    unsigned long long m_iFiringCount = 0;

	std::unique_ptr<Demodulator> m_demodulator;
};
RegAlgo(Demodulator_Block);
#endif // DEMODULATOR_BLOCK_H
