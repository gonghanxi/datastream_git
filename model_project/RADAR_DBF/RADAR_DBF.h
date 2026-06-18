#pragma once

#include "ModelBuilder.h"
#include "CircularBuffer.h"

#include <complex>

/*
 * RADAR_DBF
 *
 * 功能：
 *   Digital Beamforming，数字波束成形。
 *
 * 与内置帮助文档对齐：
 *   - Domain: Untimed
 *   - 输入端口：
 *       input   : multiple complex，输入各通道信号
 *       weight  : multiple complex，每个输入通道对应的数字波束形成权值
 *   - 输出端口：
 *       output  : complex，加权求和后的波束形成输出
 *   - 端口速率：
 *       input  rate = 1
 *       weight rate = 1
 *       output rate = 1
 *
 * 实现说明：
 *   RADAR_DBF 本身不估计权值，也不根据角度/阵列间距生成 steering vector。
 *   权值由 RADAR_ADBF 等其他模块产生，本模块只执行逐通道复数乘加：
 *
 *       output = sum_i input[i] * weight[i]
 *
 *   当前版本采用“直接使用 weight”方式，不对 weight 取共轭，也不做阵元数归一化。
 *   若后续黑盒验证发现内置采用 w^H*x，可只将乘法改为 input[i] * conj(weight[i])。
 */

class SYSTEMVUEMODELBUILDER_API RADAR_DBF : public SystemVueModelBuilder::DFModel
{
public:
	typedef std::complex<double> Cx;

	typedef SystemVueModelBuilder::CircularBufferBusT<SystemVueModelBuilder::DComplexCircularBuffer> CxBus;
	typedef SystemVueModelBuilder::DComplexCircularBuffer CxBuf;

	DECLARE_MODEL_INTERFACE(RADAR_DBF);

	RADAR_DBF();

	virtual bool Setup();
	virtual bool Run();

	// ============================================================
	// 端口定义
	// Port 1：input，multiple complex，各阵元/各通道输入信号
	// Port 2：weight，multiple complex，各通道对应的数字波束形成权值
	// Port 3：output，complex，波束形成后的单路输出
	// ============================================================
	CxBus input;
	CxBus weight;
	CxBuf output;

private:
	int inputBusSize_;
	int weightBusSize_;
	int activeBusSize_;

private:
	bool validateAndPrepare_();
	void applyInputRates_();

	Cx getInputCx_(int index);
	Cx getWeightCx_(int index);
};
