#pragma once
#include "DtoA.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API DtoA_Block : public SystemVueModelBuilder::Block
{
public:
    DtoA_Block(const std::string& name);
    ~DtoA_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    DtoA::DigFmt ConvertStringToDigFmt(const std::string& value);
    DtoA::HDist ConvertStringToHDist(const std::string& value);
    DtoA::DbRef ConvertStringToDbRef(const std::string& value);

    void SetDefaultParamters();
    //内部函数
    void BuildQuantTable();
    void ParseDataTable();
    double CodeToVolt(int code) const;
    double BasicHarmonics(double y) const;
    double ClockHarmonics(double t_now) const;
    double TableTerm(double t_now, int n, int m, double level_dBc, double phase_deg) const;
    double ApplyRJ(double y_now, double y_prev, double dt) const;

    bool TimeDrivenRun(std::vector<double> outputData);
    bool DataStreamRun(std::vector<double> outputData);

    std::queue<double> m_outputQueue;
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_outputCount;

    std::unique_ptr<DtoA> m_dtoa;

    //参数
    int     m_nbits;
    double  m_vref;
    DtoA::DigFmt  m_inputDigitalFormat;
    int     m_repeatOutput;
    double  m_rjrms;
    double  m_inl;
    double  m_dnl;
    DtoA::HDist   m_harmonicDistortion;

    double  m_dbfs;
    SystemVueModelBuilder::Matrix<double> m_f2_to_f5_dbc;
    SystemVueModelBuilder::Matrix<double> m_c1_to_c5_db;

    DtoA::DbRef   m_dbcReference;
    SystemVueModelBuilder::Matrix<double> m_dataTable;
    double  m_fundamentalFo;
    int     m_setPhase;

    //内部辅助参数
    SimuParameter simulator_param;

    double  m_fc = 0.0;
    double  m_fsOut = 0.0;
    int     m_produced = 0;
    double  m_lsb = 0.0;
    int     m_codeMin = 0;
    int     m_codeMax = 0;
    double  m_i_est = 0.0;
    double  m_q_est = 0.0;
    double  m_alpha = 0.0;
    double  m_afo_est = 1e-12;
    double  m_a_prev = 0.0;

    mutable std::mt19937 m_rng;
    mutable std::normal_distribution<double> m_gauss0;
    std::vector<double> m_code2volt;
    struct Term { int N; int M; double level_dBc; double phase_deg; };
    std::vector<Term> m_terms;

    unsigned long long GetCount() const { return m_iFiringCount; }
    void Advance() { ++m_iFiringCount; }
    unsigned long long m_iFiringCount = 0;

    static inline double Db2Lin(double dB) { return std::pow(10.0, dB / 20.0); }
    static inline double Clip(double x, double lo, double hi) { return std::max(lo, std::min(hi, x)); }
};

RegAlgo(DtoA_Block);
