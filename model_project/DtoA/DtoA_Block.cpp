#include "DtoA_Block.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <iostream>
#include <random>
#include <vector>

namespace {
std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}

DtoA_Block::DtoA_Block(const std::string& name)
    : Block(name),
      m_rng(1234567),
      m_gauss0(0.0, 1.0)
{
}

void DtoA_Block::SetDefaultParamters()
{
    m_nbits = 8;
    m_vref = 1.0;
    m_inputDigitalFormat = DtoA::TwosComplement;
    m_repeatOutput = 1;
    m_rjrms = 0.0;
    m_inl = 0.0;
    m_dnl = 0.0;
    m_harmonicDistortion = DtoA::HD_None;

    m_dbfs = -1.0;
    m_f2_to_f5_dbc.Resize(1, 4);
    for (int i = 0; i < 4; ++i) { m_f2_to_f5_dbc(i) = -400.0; }
    m_c1_to_c5_db.Resize(1, 5);
    for (int i = 0; i < 5; ++i) { m_c1_to_c5_db(i) = -400.0; }

    m_dbcReference = DtoA::Ref_SignalFo_to_DBFS;
    m_dataTable.Resize(2, 4);
    m_dataTable(0, 0) = 1; m_dataTable(0, 1) = 3; m_dataTable(0, 2) = -400.0; m_dataTable(0, 3) = 0.0;
    m_dataTable(1, 0) = 1; m_dataTable(1, 1) = -3; m_dataTable(1, 2) = -400.0; m_dataTable(1, 3) = 0.0;
    m_fundamentalFo = 100e6;
    m_setPhase = 0;

    m_fc = 0.0;
    m_fsOut = 0.0;
    m_produced = 0;
    m_lsb = 0.0;
    m_codeMin = 0;
    m_codeMax = 0;
    m_i_est = 0.0;
    m_q_est = 0.0;
    m_alpha = 0.0;
    m_afo_est = 1e-12;
    m_a_prev = 0.0;
    m_terms.clear();
    m_code2volt.clear();
}

void DtoA_Block::SetParameters()
{
    if (!m_dtoa) {
        return;
    }

    m_dtoa->NBits = m_nbits;
    m_dtoa->VRef = m_vref;
    m_dtoa->InputDigitalFormat = m_inputDigitalFormat;
    m_dtoa->RepeatOutput = m_repeatOutput;
    m_dtoa->RJrms = m_rjrms;
    m_dtoa->INL = m_inl;
    m_dtoa->DNL = m_dnl;
    m_dtoa->HarmonicDistortion = m_harmonicDistortion;

    m_dtoa->dBFS = m_dbfs;
    m_dtoa->F2_to_F5_dBc = m_f2_to_f5_dbc;
    m_dtoa->C1_to_C5_dB = m_c1_to_c5_db;

    m_dtoa->dBcReference = m_dbcReference;
    m_dtoa->DataTable = m_dataTable;
    m_dtoa->FundamentalFo = m_fundamentalFo;
    m_dtoa->SetPhase = m_setPhase;
}

bool DtoA_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool DtoA_Block::Run()
{
    const std::string inputPort = GetInputPortName(0);

    auto inputData = ReadInputData<int>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    const int repeat = std::max(1, m_repeatOutput);

    int d = inputData[0U];
    d = static_cast<int>(Clip(d, m_codeMin, m_codeMax));
    const double a_quant = CodeToVolt(d);

    const double dt = (m_fsOut > 0.0) ? (1.0 / m_fsOut) : 0.0;

    double a_base = ApplyRJ(a_quant, m_a_prev, dt);
    m_a_prev = a_quant;

    std::vector<double> outputData;
    outputData.reserve(static_cast<size_t>(repeat));
    const double fs = simulator_param.samplingRate;
    const double baseCount = static_cast<double>(GetCount());
    for (int k = 0; k < repeat; ++k) {
        const double t = (fs > 0.0)
            ? (simulator_param.startTime + (baseCount + static_cast<double>(k)) / fs)
            : 0.0;

        double y = a_base;
        if (m_harmonicDistortion == DtoA::HD_Basic) {
            y = BasicHarmonics(y);
        }

        if (m_harmonicDistortion == DtoA::HD_Table) {
            const double c = std::cos(2.0 * DtoA::kPI * m_fundamentalFo * t);
            const double s = std::sin(2.0 * DtoA::kPI * m_fundamentalFo * t);
            m_i_est = m_alpha * m_i_est + (1.0 - m_alpha) * (a_base * c);
            m_q_est = m_alpha * m_q_est + (1.0 - m_alpha) * (a_base * s);
            m_afo_est = std::max(1e-12, std::sqrt(m_i_est * m_i_est + m_q_est * m_q_est));

            double sum = 0.0;
            for (const auto& term : m_terms) {
                sum += TableTerm(t, term.N, term.M, term.level_dBc, term.phase_deg);
            }
            y = Clip(a_base + sum, -m_vref, m_vref);
        }

        if (m_harmonicDistortion != DtoA::HD_None) {
            y = Clip(y + ClockHarmonics(t), -m_vref, m_vref);
        }

        outputData.push_back(y);
    }
    if(IsVariableStepMode()) return TimeDrivenRun(outputData);
    return DataStreamRun(outputData);
}
bool DtoA_Block::TimeDrivenRun(std::vector<double> outputData)
{
    const std::string outputPort = GetOutputPortName(0);
    for(const auto& val : outputData) m_outputQueue.push(val);
    if (!m_outputQueue.empty())
    {
        double outputValue = m_outputQueue.front();
        m_outputQueue.pop();

        WriteOutputData(outputPort, std::vector<double>{outputValue});
        m_lastOutput = outputValue;
        m_produced++;
        Advance();

        qDebug() << "[DtoA_Block] :"
                 << " value:" << outputValue;
    }
    return true;
}

bool DtoA_Block::DataStreamRun(std::vector<double> outputData)
{
    const std::string outputPort = GetOutputPortName(0);
    WriteOutputData(outputPort, outputData);

    m_produced++;
    Advance();
    return true;
}

bool DtoA_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_dtoa = std::make_unique<DtoA>();

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_nbits = std::stoi(getParameter("NBits").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NBits', using default value."); }
    try { m_vref = std::stod(getParameter("VRef").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'VRef', using default value."); }
    try { m_inputDigitalFormat = ConvertStringToDigFmt(getParameter("InputDigitalFormat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'InputDigitalFormat', using default value."); }
    try { m_repeatOutput = std::stoi(getParameter("RepeatOutput").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RepeatOutput', using default value."); }
    try { m_rjrms = std::stod(getParameter("RJrms").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RJrms', using default value."); }
    try { m_inl = std::stod(getParameter("INL").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'INL', using default value."); }
    try { m_dnl = std::stod(getParameter("DNL").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DNL', using default value."); }
    try { m_harmonicDistortion = ConvertStringToHDist(getParameter("HarmonicDistortion").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'HarmonicDistortion', using default value."); }

    try { m_dbfs = std::stod(getParameter("dBFS").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'dBFS', using default value."); }
    try { m_f2_to_f5_dbc = DataTypesAndParsers::ParseStringToMatrixDouble(getParameter("F2_to_F5_dBc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'F2_to_F5_dBc', using default value."); }
    try { m_c1_to_c5_db = DataTypesAndParsers::ParseStringToMatrixDouble(getParameter("C1_to_C5_dB").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'C1_to_C5_dB', using default value."); }

    try { m_dbcReference = ConvertStringToDbRef(getParameter("dBcReference").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'dBcReference', using default value."); }
    try { m_dataTable = DataTypesAndParsers::ParseStringToMatrixDouble(getParameter("DataTable").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DataTable', using default value."); }
    try { m_fundamentalFo = std::stod(getParameter("FundamentalFo").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FundamentalFo', using default value."); }
    try { m_setPhase = std::stoi(getParameter("SetPhase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SetPhase', using default value."); }

    SetParameters();

    const int repeat = std::max(1, m_repeatOutput);
    AddInputPort("D", m_dtoa->D, 1, Block::DataType::TIMED_INT);
    AddOutputPort("A", m_dtoa->A, static_cast<size_t>(repeat), Block::DataType::TIMED_DOUBLE);

    m_dtoa->A.SetRate(static_cast<size_t>(repeat));
    m_dtoa->A.SetSampleRate(std::max(1, m_repeatOutput) * simulator_param.samplingRate);
    m_dtoa->A.SetStartTime(simulator_param.startTime);

    m_fc = simulator_param.samplingRate;
    if (m_fc <= 0.0) {
        m_fc = 1.0;
    }
    m_fsOut = std::max(1, m_repeatOutput) * m_fc;

    if (m_inputDigitalFormat == DtoA::OffsetBinary) {
        m_codeMin = 0;
        m_codeMax = (1 << m_nbits) - 1;
    } else {
        m_codeMin = -(1 << (m_nbits - 1));
        m_codeMax = (1 << (m_nbits - 1)) - 1;
    }
    m_lsb = 2.0 * m_vref / static_cast<double>(1 << m_nbits);

    BuildQuantTable();
    ParseDataTable();

    m_alpha = 0.999;
    m_produced = 0;
    m_afo_est = 1e-12;
    m_i_est = 0.0;
    m_q_est = 0.0;
    m_a_prev = 0.0;

    return true;
}

void DtoA_Block::BuildQuantTable()
{
    const int ncode = m_codeMax - m_codeMin + 1;
    m_code2volt.assign(ncode, 0.0);
    for (int c = m_codeMin; c <= m_codeMax; ++c) {
        int idx = c - m_codeMin;
        double mid = -m_vref + (idx + 0.5) * m_lsb;
        m_code2volt[idx] = Clip(mid, -m_vref, m_vref);
    }
    if (m_dnl > 0.0 || m_inl > 0.0) {
        for (int i = 0; i < ncode; ++i) {
            double wig = (m_dnl > 0.0 ? m_gauss0(m_rng) * 0.25 * m_dnl * m_lsb : 0.0)
                + (m_inl > 0.0 ? 0.5 * m_inl * m_lsb * std::sin(2 * DtoA::kPI * (i + 0.5) / ncode) : 0.0);
            m_code2volt[i] = Clip(m_code2volt[i] + wig, -m_vref, m_vref);
        }
    }
}

double DtoA_Block::CodeToVolt(int code) const
{
    code = static_cast<int>(Clip(code, m_codeMin, m_codeMax));
    return m_code2volt[code - m_codeMin];
}

void DtoA_Block::ParseDataTable()
{
    m_terms.clear();
    const int r = static_cast<int>(m_dataTable.NumRows());
    const int c = static_cast<int>(m_dataTable.NumColumns());
    if (r <= 0 || c < 3) {
        return;
    }
    for (int row = 0; row < r; ++row) {
        Term t{};
        t.N = static_cast<int>(std::floor(m_dataTable(row, 0) + 0.5));
        t.M = static_cast<int>(std::floor(m_dataTable(row, 1) + 0.5));
        t.level_dBc = m_dataTable(row, 2);
        t.phase_deg = (c >= 4 ? m_dataTable(row, 3) : 0.0);
        m_terms.push_back(t);
    }
}

double DtoA_Block::ApplyRJ(double y_now, double y_prev, double dt) const
{
    if (m_rjrms <= 0.0) {
        return y_now;
    }
    const double jitter = m_gauss0(m_rng) * m_rjrms;
    const double dydt = (dt > 0.0) ? (y_now - y_prev) / dt : 0.0;
    return Clip(y_now + dydt * jitter, -m_vref, m_vref);
}

double DtoA_Block::BasicHarmonics(double y) const
{
    if (m_harmonicDistortion != DtoA::HD_Basic) {
        return y;
    }
    double u = (m_vref > 0.0) ? Clip(y / m_vref, -1.0, 1.0) : 0.0;
    const double t1 = u;
    const double t2 = 2 * u * u - 1;
    const double t3 = 2 * u * t2 - t1;
    const double t4 = 2 * u * t3 - t2;
    const double t5 = 2 * u * t4 - t3;

    const double aref = m_vref * Db2Lin(m_dbfs);
    const double a1 = (std::fabs(aref) > 1e-12 ? std::fabs(y) / aref : 0.0);
    const double h2 = Db2Lin(m_f2_to_f5_dbc(0)) * std::pow(a1, 1.0);
    const double h3 = Db2Lin(m_f2_to_f5_dbc(1)) * std::pow(a1, 2.0);
    const double h4 = Db2Lin(m_f2_to_f5_dbc(2)) * std::pow(a1, 3.0);
    const double h5 = Db2Lin(m_f2_to_f5_dbc(3)) * std::pow(a1, 4.0);

    double y2 = y
        + m_vref * (-h2) * t2
        + m_vref * (-h3) * t3
        + m_vref * (-h4) * t4
        + m_vref * (-h5) * t5;
    return Clip(y2, -m_vref, m_vref);
}

double DtoA_Block::ClockHarmonics(double t_now) const
{
    if (m_harmonicDistortion == DtoA::HD_None) {
        return 0.0;
    }
    const double twopi = 2.0 * DtoA::kPI;
    double acc = 0.0;
    for (int n = 1; n <= 5; ++n) {
        double amp = m_vref * Db2Lin(m_c1_to_c5_db(n - 1));
        if (amp <= 0.0) {
            continue;
        }
        acc += amp * std::sin(twopi * (n * m_fc) * t_now);
    }
    return acc;
}

double DtoA_Block::TableTerm(double t_now, int n, int m, double level_dBc, double phase_deg) const
{
    const double twopi = 2.0 * DtoA::kPI;
    const double f = n * m_fc + m * m_fundamentalFo;
    double amp = 0.0;
    if (m_dbcReference == DtoA::Ref_SignalFo_to_DBFS) {
        amp = m_vref * Db2Lin(m_dbfs) * Db2Lin(level_dBc);
    } else {
        amp = m_afo_est * Db2Lin(level_dBc);
    }
    const double ph = (m_setPhase ? phase_deg * (twopi / 360.0) : 0.0);
    return amp * std::sin(twopi * f * t_now + ph);
}
DtoA::DigFmt DtoA_Block::ConvertStringToDigFmt(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "offsetbinary" || lower == "offset binary" || lower == "0") {
        return DtoA::OffsetBinary;
    }
    if (lower == "twoscomplement" || lower == "1") {
        return DtoA::TwosComplement;
    }
    return DtoA::TwosComplement;
}

DtoA::HDist DtoA_Block::ConvertStringToHDist(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "hd_basic" || lower == "basic distortion" || lower == "basicdistortion" || lower == "1") {
        return DtoA::HD_Basic;
    }
    if (lower == "hd_table" || lower == "settable with data table" || lower == "settablewithdatatable" || lower == "2") {
        return DtoA::HD_Table;
    }
    if (lower == "hd_none" || lower == "0") {
        return DtoA::HD_None;
    }
    return DtoA::HD_None;
}

DtoA::DbRef DtoA_Block::ConvertStringToDbRef(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "ref_signalfo_only" || lower == "signal fo only" || lower == "signalfoonly" || lower == "1") {
        return DtoA::Ref_SignalFo_only;
    }
    if (lower == "ref_signalfo_to_dbfs" || lower == "signal fo to dbfs" || lower == "signalfotodbfs" || lower == "0") {
        return DtoA::Ref_SignalFo_to_DBFS;
    }
    return DtoA::Ref_SignalFo_to_DBFS;
}


