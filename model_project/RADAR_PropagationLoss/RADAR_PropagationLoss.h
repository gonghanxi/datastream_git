#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

#include <cmath>

/*
 * RADAR_PropagationLoss V2
 *
 * 功能：
 *   通过目标距离 range 计算雨、雪或 77GHz 车载雷达雨/水膜场景下的传播衰减。
 *
 * V2 修正：
 *   1. dB 输出采用内置约定：负衰减 dB；
 *   2. Linear Power Loss 输出功率保留比例：10^(atten_dB/10)；
 *   3. Linear Amplitude Loss 输出幅度保留比例：10^(atten_dB/20)；
 *   4. RainfallRate / SnowfallRate 必须大于 0；dw=0 时水膜损耗严格为 0。
 *
 * 帮助文档要点：
 *   - Domain: Untimed
 *   - 输入端口 range：real，单位 m，Rate = 1
 *   - 输出端口 attenuate：real，Rate = 1
 *   - OutputUnit 只影响输出单位，不影响参数显隐
 *   - PropagationLossType 控制 Rainfall / Snowfall / 77GHz Rainfall 三类参数显隐
 *   - 77GHz Rainfall 下 RainLoss77GHzType 进一步控制 Near / Mid / Antenna Water Layer 参数显隐
 *
 * 说明：
 *   公开帮助文档没有给出完整经验系数表，因此本版本按帮助文档结构和公开雨衰/雪衰模型
 *   做工程近似。端口、参数、枚举、显隐和输出单位转换优先对齐内置模块；
 *   具体 77GHz Near/Mid/WaterLayer 分支建议后续通过黑盒测试进一步修正经验系数。
 */

class SYSTEMVUEMODELBUILDER_API RADAR_PropagationLoss : public SystemVueModelBuilder::DFModel
{
public:
	DECLARE_MODEL_INTERFACE(RADAR_PropagationLoss);

	RADAR_PropagationLoss();

	virtual bool Setup();
	virtual bool Run();

	// ============================================================
	// OutputUnit:
	//   0: dB
	//   1: Linear Power Loss
	//   2: Linear Amplitude Loss
	// ============================================================
	enum OutputUnitEnum
	{
		Output_dB = 0,
		Linear_Power_Loss = 1,
		Linear_Amplitude_Loss = 2
	};

	// ============================================================
	// PropagationLossType:
	//   0: Rainfall
	//   1: Snowfall
	//   2: 77GHz Rainfall
	// ============================================================
	enum PropagationLossTypeEnum
	{
		Rainfall = 0,
		Snowfall = 1,
		Rainfall_77GHz = 2
	};

	// ============================================================
	// RainLoss77GHzType:
	//   0: Near Range Loss
	//   1: Mid Range Loss
	//   2: Antenna Water Layer Loss
	// ============================================================
	enum RainLoss77GHzTypeEnum
	{
		Near_Range_Loss = 0,
		Mid_Range_Loss = 1,
		Antenna_Water_Layer_Loss = 2
	};

	// ============================================================
	// TempAntWtLyLoss:
	//   帮助文档为枚举：-10, 0, 10, 20, 30, 40, 50 摄氏度
	// ============================================================
	enum TempAntWtLyLossEnum
	{
		Temp_minus10 = 0,
		Temp_0 = 1,
		Temp_10 = 2,
		Temp_20 = 3,
		Temp_30 = 4,
		Temp_40 = 5,
		Temp_50 = 6
	};

	// ============================================================
	// 端口
	// Port 1: range     目标距离，real，单位 m
	// Port 2: attenuate 输出损耗值，单位由 OutputUnit 决定
	// ============================================================
	SystemVueModelBuilder::CircularBuffer<double> range;
	SystemVueModelBuilder::CircularBuffer<double> attenuate;

	// ============================================================
	// 参数
	// ============================================================
	OutputUnitEnum OutputUnit;
	PropagationLossTypeEnum PropagationLossType;
	RainLoss77GHzTypeEnum RainLoss77GHzType;

	double Frequency;       // Hz
	double RainfallRate;    // mm/hour
	double AntTheta;        // deg
	double AntPhi;          // deg
	double AntHeight;       // m
	double Bandwidth;       // Hz
	double TarRCS;          // m^2

	TempAntWtLyLossEnum TempAntWtLyLoss;
	double dw;              // m，天线罩水膜厚度

	double SnowfallRate;    // mm/hour，等效雨水含量

	double computeLossDb_(double rangeMeter) const;
	double convertOutputUnit_(double lossDb) const;

	double genericRainLossDb_(double rangeMeter) const;
	double genericSnowLossDb_(double rangeMeter) const;

	double rain77GHzLossDb_(double rangeMeter) const;
	double rain77GHzNearLossDb_(double rangeMeter) const;
	double rain77GHzMidLossDb_(double rangeMeter) const;
	double antennaWaterLayerLossDb_() const;

	// ITU-R P.838 风格的比雨衰计算，返回 dB/km。
	// 本模块没有极化参数，使用水平极化系数作为默认近似。
	double ituRainSpecificAttenDbPerKm_(double freqGHz, double rainRate) const;

	// 77GHz 中距离经验比衰减，返回 dB/km。
	double rain77MidSpecificAttenDbPerKm_(double rainRate) const;

	double tempCelsius_() const;

	static double deg2rad_(double x);
	static double clamp_(double x, double lo, double hi);
	static double safePositive_(double x, double fallback);
	static double log10Safe_(double x);
	static double interp1_(const double* xs, const double* ys, int n, double x);
};
