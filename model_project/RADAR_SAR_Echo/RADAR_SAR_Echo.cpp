#include "RADAR_SAR_Echo.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_SAR_Echo )
{	
	SET_MODEL_DESCRIPTION("SAR Echo Generation");

	SET_MODEL_CATEGORY("Signal Source");

	ADD_MODEL_OUTPUT( output );
	
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(SAR_Mode, SelectedSAR_Mode);
		enumParam.SetDescription("The SAR Work Mode: Stripmap");
		enumParam.AddEnumeration("Stripmap", Stripmap);
		enumParam.SetDefaultValue("0");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Fc);
		param.SetDescription("Carrier Frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("1e9");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Xmin);
		param.SetDescription("Target Area Min");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("0");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Xmax);
		param.SetDescription("Target Area Max");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("50");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Yc);
		param.SetDescription("Center of Imaged Area");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("10000");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Y0);
		param.SetDescription("Half Target Area Width");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("500");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(H);
		param.SetDescription("Radar Height");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("5000");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Vr);
		param.SetDescription("Radar Velocity");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("100");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(D);
		param.SetDescription("Radar Antenna Aperture Size");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("4");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Tr);
		param.SetDescription("LFM Pulse Width");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("5e-6");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Br);
		param.SetDescription("LFM Bandwidth");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("30e6");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDescription("Waveform Baseband Sampling Rate in Range Dimension");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("100e6");
	}
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(EchoGenerate_Mode, SelectedEchoGenerate_Mode);
		enumParam.SetDescription("The Echo Generation Mode: Point_Target");
		enumParam.AddEnumeration("Point_Target", Point_Target);
		enumParam.SetDefaultValue("0");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(TargetInfo);
		param.SetDescription("Point Target Information [range_n, cross_n, rcs_0;]");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("[0,0,1]");
	}
	return true;
}
#endif

RADAR_SAR_Echo::RADAR_SAR_Echo()
{
	
}

bool RADAR_SAR_Echo::Setup()
{
	bool bStatus = true;

	if (Fc <= 0)
	{
		POST_ERROR("Fc must be > 0");
		bStatus = false;
	}
	if (D <= 0)
	{
		POST_ERROR("D must be > 0");
		bStatus = false;
	}
	if (Tr <= 0)
	{
		POST_ERROR("Tr must be > 0");
		bStatus = false;
	}
	if (Br <= 0)
	{
		POST_ERROR("Br must be > 0");
		bStatus = false;
	}
	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}

	const double c = 3e8;
	// 波长
	lambda = c / Fc;
	// 景中心斜距（零多普勒位置校准后）
	R0 = std::sqrt(Yc*Yc + H * H);
	// ——天线参数——
	// 合成孔径长度
	Lsar = lambda * R0 / D;
	// 波束穿越时间
	Tsar = Lsar / Vr;
	// ——慢时间维参数——
	// 多普勒调频率
	Ka = -2 * Vr*Vr / lambda / R0;
	// 多普勒带宽
	Ba = std::abs(Ka*Tsar);
	// 脉冲重复频率
	PRF = Ba;
	// 脉冲重复周期
	PRI = 1 / PRF;
	// 慢时间维采样间隔
	ds = PRI;
	// 慢时间维样点数量
	Nslow = std::ceil((Xmax - Xmin + Lsar) / Vr / ds);

	// ——快时间维参数——
	// 线性调频斜率
	Kr = Br / Tr;
	// 快时间维采样频率
	Fsr = SampleRate;
	// 快时间维采样间隔
	dt = 1 / Fsr;
	// 快时间维距离下限
	Rmin = std::sqrt((Yc - Y0)*(Yc - Y0) + H * H);
	// 快时间维距离上限
	Rmax = std::sqrt((Yc + Y0)*(Yc + Y0) + H * H);
	// 快时间维样点数量
	Nfast = std::ceil(2 * (Rmax - Rmin) / c / dt + Tr / dt);


    output.SetRate(Nslow*Nfast);
    outputData.resize(Nslow*Nfast);

    m_Nslow = Nslow;
    m_Nfast = Nfast;

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_SAR_Echo::Run()
{
	const double c = 3e8;
	const double PI = std::acos(-1);
	const std::complex<double> imag_I(0, 1);

	// 慢时间轴
	SystemVueModelBuilder::Matrix<double> sn(1, Nslow);
	for (int i = 0; i < Nslow; i++)
	{
		sn(i) = (Xmin - Lsar / 2) / Vr + ds * i;
	}

	// 快时间轴
	SystemVueModelBuilder::Matrix<double> tm(1, Nfast);
	for (int i = 0; i < Nfast; i++)
	{
		tm(i) = 2 * Rmin / c + dt;
	}

	SystemVueModelBuilder::Matrix<double> Dslow(1, Nslow);
	SystemVueModelBuilder::Matrix<double> R(1, Nslow);
	SystemVueModelBuilder::Matrix<double> tau(1, Nslow);
	SystemVueModelBuilder::Matrix<double> Dfast(Nslow, Nfast);
	SystemVueModelBuilder::Matrix<double> phase(Nslow, Nfast);

	// 输入的目标信息需要符合格式
	if (TargetInfo.NumElements() % 3)
	{
        LOG_ERROR("Point Target Information should format as [range_n, cross_n, rcs_n;]");
		return false;
	}

	// 点目标数量
	int Ntarget = TargetInfo.NumElements() / 3;

	// 清空输出缓存
	for (int i = 0; i < Nslow*Nfast; i++)
	{
        outputData[i] = 0;
//        output[i] = 0;
	}

	for (int i = 0; i < Ntarget; i++)
	{
		// 拆分目标信息
		double range_n = TargetInfo.NumRows() == 1 ? TargetInfo(i * 3) : TargetInfo(i, 0);
		double cross_n = TargetInfo.NumRows() == 1 ? TargetInfo(i * 3 + 1) : TargetInfo(i, 1);
		double rcs_n = TargetInfo.NumRows() == 1 ? TargetInfo(i * 3 + 2) : TargetInfo(i, 2);
		
		Dslow = sn * Vr - range_n;
		for (int n = 0; n < Nslow; n++)
		{
			R(n) = std::sqrt(Dslow(n)*Dslow(n) + cross_n * cross_n + H * H);
			tau(n) = 2 * R(n) / c;
		}
		for (int n = 0; n < Nslow; n++)
		{
			for (int m = 0; m < Nfast; m++)
			{
				Dfast(n, m) = tm(m) - tau(n);
				phase(n, m) = PI * Kr*Dfast(n, m)*Dfast(n, m) - 4 * PI / lambda * R(n);
			}
		}
		// 输出
		for (int m = 0; m < Nfast; m++)
		{
			for (int n = 0; n < Nslow; n++)
			{
                outputData[m*Nslow+n] += rcs_n * std::exp(imag_I*phase(n, m));
//                output[m*Nslow+n] += rcs_n * std::exp(imag_I*phase(n, m));
//                output[n*Nfast + m] += rcs_n * std::exp(imag_I*phase(n, m))*(1.0*((Dfast(n, m) > 0) & (Dfast(n, m) < Tr))*(std::abs(Dslow(n)) < (Lsar / 2)));
			}
		}
	}


	return true;
}
