#pragma once
#include "ModelBuilder.h"
#include "TimedDFModel.h"
#include "EnvelopeSignal.h"
#include "RandomNumberGenerator.h"
#include "Matrix.h"

#include <vector>
#include <cmath>
#include <complex>
#include <algorithm>
#include <limits>

class Amplifier : public SystemVueModelBuilder::TimedDFModel
{
public:
	// ===== 枚举：名字/顺序用于 HideCondition，与内置参数顺序保持一致 =====
	enum GainUnitEnum {
		voltage = 0,
		dB = 1
	};

	enum QuantizationEnum {
		NO = 0,
		Number_of_Bits_Uniform = 1,
		Custom_Levels = 2
	};

	enum GainErrorEnum {
		None = 0,
		Normal = 1,
		Uniform = 2,
		Custom_Error = 3
	};

	enum GCTypeEnum {
		none = 0,
		TOI = 1,
		dBc1 = 2,
		TOI_dBc1 = 3,
		PSat_GCSat_TOI = 4,
		PSat_GCSat_dBc1 = 5,
		PSat_GCSat_TOI_dBc1 = 6,
		RappNonlinearity = 7,
		Gain_compression_vs_input_power = 8,
		AM_AM_and_AMPM_vs_input_power = 9
	};

	DECLARE_MODEL_INTERFACE(Amplifier);
	Amplifier();

	bool    Setup() override;
	bool    Run()   override;
	ERESULT PropagateCharacterizationFrequency() override;

	// ===== 端口 =====
	SystemVueModelBuilder::EnvelopeCircularBuffer input;      // 黑：包络输入
	SystemVueModelBuilder::CircularBuffer<double> control;    // 蓝：可选实数控制输入
	SystemVueModelBuilder::EnvelopeCircularBuffer output;     // 黑：包络输出

	// ===== 参数 =====
	GainUnitEnum     GainUnit;        // voltage / dB
	double           Gain;            // 默认 1，control 连接时忽略

	QuantizationEnum Quantization;    // NO / Number of Bits (Uniform) / Custom Levels
	int              NumBits;         // 默认 6
	double           StepSize;        // 默认 0.5，UI 用 W 表示，数值按 dB
	double           MaxGain;         // 默认 10，UI 用 W 表示，数值按 dB
	SystemVueModelBuilder::Matrix<double> Levels; // Custom Levels，UI 用 W 表示，数值按 dB

	GainErrorEnum    GainError;       // None / Normal / Uniform / Custom Error
	double           StdDev;          // Normal std，单位随 GainUnit，UI 用 W 表示
	double           Min;             // Uniform min，单位随 GainUnit，UI 用 W 表示
	double           Max;             // Uniform max，单位随 GainUnit，UI 用 W 表示
	double           CustomError;     // Custom Error，单位随 GainUnit，UI 用 W 表示

	double           NoiseFigure;     // 输入噪声系数 dB，UI 用 W 表示

	GCTypeEnum       GCType;          // 非线性类型
	double           TOIout;          // W，输出三阶截点功率
	double           dBc1out;         // W，输出 1dB 压缩功率
	double           PSat;            // W，饱和功率
	double           GCSat;           // dB，饱和处增益压缩，UI 用 W 表示
	int              RappS;           // Rapp 平滑因子
	SystemVueModelBuilder::Matrix<double> GComp; // 三元组表

	double           RefR;            // ohm

    // ===== 噪声 =====
    bool updateNoiseSigmaIfNeeded(double fc);
    SystemVueModelBuilder::EnvelopeSignal addInputNoise(
        const SystemVueModelBuilder::EnvelopeSignal& xin,
        double fc);
    double computeSmallSignalGainLin(double gainSrc, double& outGainDb);
    double getInputAmplitude(const SystemVueModelBuilder::EnvelopeSignal& xin,
        double fc) const;

    // ===== 非线性幅度输出 =====
    double applyTOI(double ain, double c1) const;
    double applydBc1(double ain, double c1) const;
    double applyTOIdBc1(double ain, double c1) const;
    double applyPSatGCSat(double ain, double c1, double fc) const;
    double applyRapp(double ain, double c1) const;

    // ===== 输出封装：按幅度改写，保持输入相位；fc==0 时保持符号 =====
    SystemVueModelBuilder::EnvelopeSignal makeOutputWithAmplitude(
        const SystemVueModelBuilder::EnvelopeSignal& xin,
        double fc,
        double aout) const;

    struct TableData
    {
        std::vector<double> pinDbm;   // 输入功率 dBm
        std::vector<double> gcDb;     // 增益变化 dB，或由 AM2AM 递推得到
        std::vector<double> pcDeg;    // 相位变化 deg，仅保留，不作用输出
        bool valid = false;

        // AM/AM 全线性表格时，高功率端继续线性；
        // GComp 或非线性 AM/AM 表格时，高功率端固定最后 Pout。
        bool highLinearExtension = false;

        // 只给 Gain compression vs input power 使用：
        // 黑盒 10A/10B 表明最后一个表格端点斜率更接近 0.5 * 最后一段割线斜率。
        // AM/AM 模式必须保持 false，否则 11B 会被破坏。
        bool halfLastSlope = false;

        // 只给 Gain compression vs input power 使用：
        // 当首点 GC 已经明显非 0 时，10B 黑盒结果显示首端点斜率既不是 sec[0]，
        // 也不是 0.5*sec[0]，而更接近相邻两段割线斜率的平均值。
        // AM/AM 模式必须保持 false，否则 11B 会被破坏。
        bool averageFirstSlope = false;
    };
    double tableOutputAmplitude(double ain,
        double c1,
        double gainDb,
        const TableData& table) const;

    // ===== 表格缓存 =====
    TableData gcompTable_;
    TableData amamTable_;


private:
	static constexpr double kPI = 3.14159265358979323846;
	static constexpr double kBoltz = 1.3806504e-23;
	static constexpr double kT0 = 290.0;
	static constexpr double kOneDbVoltageRatio = 0.8912509381337456; // 10^(-1/20)

private:
	// ===== 基础换算 =====
	static double dbToLin(double db);
	static double linToDb(double lin);
	static double wattToDbm(double w);
	static double dbmToWatt(double dbm);
	static double wattToPeakVoltage(double w, double r);
	static double peakVoltageToWatt(double v, double r);
	static double peakVoltageToDbm(double v, double r);
	static double dbmToPeakVoltage(double dbm, double r);

	// ===== 增益处理 =====

	double quantizeGainDb(double gainDb) const;



	// ===== GComp / AMAM 表 =====
	bool prepareGCompTable();
	bool prepareAMAMTable();
	bool parseGCompTriples(std::vector<double>& a,
		std::vector<double>& b,
		std::vector<double>& c) const;





	// ===== 多项式工具 =====
	static double evalOddPolynomial(double x, const std::vector<double>& coeff);
	static double evalOddDerivative(double x, const std::vector<double>& coeff);
	static double findFirstPeakX(const std::vector<double>& coeff, double hint);
	static bool   solve4x4(double a[4][4], double b[4], double x[4]);

	bool computePSatPolynomialCoeffs(double c1,
		double fc,
		std::vector<double>& coeff,
		double& xs,
		double& ys) const;





private:
	// ===== 本次仿真固定的增益误差 =====
	// GainUnit=dB 时表示 dB；GainUnit=voltage 时表示线性电压增益偏差
	double gainErrOnce_ = 0.0;

	SystemVueModelBuilder::CNormal  rngGainErrN_;
	SystemVueModelBuilder::CUniform rngGainErrU_;

	// ===== NoiseFigure 噪声 =====
	bool   noisePrepared_ = false;
	bool   noiseEnabled_ = false;
	double noiseSigma_ = 0.0;       // 当前链路每路噪声 sigma

	SystemVueModelBuilder::CNormal rngNoiseI_;
	SystemVueModelBuilder::CNormal rngNoiseQ_;


};
