#include "RADAR_CoIntgr_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_CoIntgr_M)
{
	SET_MODEL_DESCRIPTION("Signal Coherent Integration for Matrix signals");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The signal after coherent integration");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(NumOfPulse);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("32");
		param.SetDescription("Number of pulses for one coherent integration");
	}

	return true;
}
#endif

RADAR_CoIntgr_M::RADAR_CoIntgr_M()
	: NumOfPulse(32)
{
}

//-----------------------------------------------------------------------------------
//  Setup
//      Matrix 版本每次输入/输出 1 个 Matrix。
//      由于矩阵行列数可以在仿真运行中动态变化，所以不在 Setup 中固定矩阵尺寸。
//-----------------------------------------------------------------------------------
bool RADAR_CoIntgr_M::Setup()
{
	bool bStatus = true;

	if (NumOfPulse > 0)
	{
		input.SetRate(1U);
		output.SetRate(1U);
	}
	else
	{
		POST_ERROR("RADAR_CoIntgr_M: NumOfPulse must be greater than 0.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//  Run
//      不改变 RADAR_CoIntgr 的核心算法：
//          output[i] = sum(input[PulseIndex * PRN + i])
//      只是将一维 complex 样本流换成 complex matrix 的线性元素序列。
//
//      输入矩阵总元素数 = PRN * NumOfPulse
//      输出矩阵总元素数 = PRN
//
//      这里按 SystemVue Matrix 的线性索引顺序访问矩阵元素，
//      与 Pack_M / Unpack_M 打包后的矩阵元素顺序保持一致。
//-----------------------------------------------------------------------------------
bool RADAR_CoIntgr_M::Run()
{
	const SystemVueModelBuilder::Matrix<std::complex<double> >& inMat = input[0];

	const int totalNum = static_cast<int>(inMat.NumElements());

	if (totalNum <= 0)
	{
		POST_ERROR("RADAR_CoIntgr_M: input matrix must contain at least one element.");
		return false;
	}

	if (NumOfPulse <= 0)
	{
		POST_ERROR("RADAR_CoIntgr_M: NumOfPulse must be greater than 0.");
		return false;
	}

	if ((totalNum % NumOfPulse) != 0)
	{
		POST_ERROR("RADAR_CoIntgr_M: input matrix element count must be an integer multiple of NumOfPulse.");
		return false;
	}

	const int PRN = totalNum / NumOfPulse; // 每个脉冲对应的矩阵元素数

	SystemVueModelBuilder::Matrix<std::complex<double> > outMat;
	outMat.Resize(1, PRN);                // 输出为 1 x PRN 行向量矩阵

	for (int i = 0; i < PRN; i++)
	{
		outMat(i).real(0.0);
		outMat(i).imag(0.0);

		// 对 NumOfPulse 个脉冲做相参积累
		for (int PulseIndex = 0; PulseIndex < NumOfPulse; PulseIndex++)
		{
			outMat(i) += inMat(PulseIndex * PRN + i);
		}
	}

	output[0] = outMat;

	return true;
}
