#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "TimedCircularBuffer.h"
#include "Matrix.h"
#include "SystemVue.h"
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

class SYSTEMVUEMODELBUILDER_API DtoA : public SystemVueModelBuilder::DFModel {
public:
    static constexpr double kPI = 3.14159265358979323846;

    enum DigFmt { OffsetBinary = 0, TwosComplement = 1 };
    enum HDist { HD_None = 0, HD_Basic = 1, HD_Table = 2 };
    enum DbRef { Ref_SignalFo_to_DBFS = 0, Ref_SignalFo_only = 1 };

    DECLARE_MODEL_INTERFACE(DtoA);
    DtoA();

    bool Setup() override;
    bool Run() override;

    SystemVueModelBuilder::TimedCircularBuffer<int>     D;
    SystemVueModelBuilder::TimedCircularBuffer<double>  A;

    int     NBits;
    double  VRef;
    DigFmt  InputDigitalFormat;
    int     RepeatOutput;
    double  RJrms;
    double  INL;
    double  DNL;
    HDist   HarmonicDistortion;

    double  dBFS;
    SystemVueModelBuilder::Matrix<double> F2_to_F5_dBc;
    SystemVueModelBuilder::Matrix<double> C1_to_C5_dB;

    DbRef   dBcReference;
    SystemVueModelBuilder::Matrix<double> DataTable;
    double  FundamentalFo;
    int     SetPhase;

private:
    double  Fc_ = 0.0;
    double  FsOut_ = 0.0;
    int     produced_ = 0;
    double  LSB_ = 0.0;
    int     codeMin_ = 0, codeMax_ = 0;

    mutable std::mt19937 rng_;
    mutable std::normal_distribution<double> gauss0_;

    std::vector<double> code2volt_;

    struct Term { int N; int M; double level_dBc; double phase_deg; };
    std::vector<Term> terms_;

    double i_est_ = 0.0, q_est_ = 0.0, alpha_ = 0.0;
    double Afo_est_ = 1e-12;

    static inline double db2lin(double dB) { return std::pow(10.0, dB / 20.0); }
    static inline double clip(double x, double lo, double hi) { return std::max(lo, std::min(hi, x)); }

    void   buildQuantTable_();
    double codeToVolt_(int code) const;

    void   parseDataTable_();
    double dBcAbs_SignalFoToDBFS_(double dBc) const { return VRef * db2lin(dBFS) * db2lin(dBc); }
    double dBcAbs_SignalFoOnly_(double dBc)  const { return Afo_est_ * db2lin(dBc); }
    double tableTerm_(const Term& t, double t_now) const;

    double basicHarmonics_(double y_base) const;

    double clockHarmonics_(double t_now) const;

    double applyRJ_(double y_now, double y_prev, double dt) const;
};
