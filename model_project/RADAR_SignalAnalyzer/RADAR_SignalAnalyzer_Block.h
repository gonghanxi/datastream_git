#ifndef RADAR_SIGNALANALYZER_BLOCK_H
#define RADAR_SIGNALANALYZER_BLOCK_H

#include "Block.h"
#include "RADAR_SignalAnalyzer.h"

#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_SignalAnalyzer_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_SignalAnalyzer_Block(const std::string& name);
    ~RADAR_SignalAnalyzer_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    void ProcessFrame(const std::vector<std::complex<double>>& inputData,
                      std::vector<double>& outputData);

    // ===== 字符串转换 =====
    RADAR_SignalAnalyzer::SelectedAnalyzerType  ConvertStringToAnalyzerType(const std::string& value);
    RADAR_SignalAnalyzer::SelectedWindowType    ConvertStringToWindowType(const std::string& value);
    RADAR_SignalAnalyzer::SelectedCorrType      ConvertStringToCorrType(const std::string& value);
    RADAR_SignalAnalyzer::SelectedNormalizedType ConvertStringToNormalizedType(const std::string& value);
    RADAR_SignalAnalyzer::SelectedFFTShiftType  ConvertStringToFFTShiftType(const std::string& value);

    std::unique_ptr<RADAR_SignalAnalyzer> m_algo;

    // ===== 参数 =====
    RADAR_SignalAnalyzer::SelectedAnalyzerType  m_AnalyzerType;
    RADAR_SignalAnalyzer::SelectedWindowType    m_WindowType;
    double                                      m_WindowParameter;
    RADAR_SignalAnalyzer::SelectedCorrType      m_CorrType;
    RADAR_SignalAnalyzer::SelectedNormalizedType m_NormalizedType;
    RADAR_SignalAnalyzer::SelectedFFTShiftType  m_FFTShiftType;
    int    m_SampleNum;
    int    m_FFTSize;
    double m_SampleRate;

    // ===== TimeDrivenRun =====
    std::vector<std::complex<double>> m_inputBuffer;
    std::queue<double>                m_outputQueue;
};

RegAlgo(RADAR_SignalAnalyzer_Block);

#endif // RADAR_SIGNALANALYZER_BLOCK_H
