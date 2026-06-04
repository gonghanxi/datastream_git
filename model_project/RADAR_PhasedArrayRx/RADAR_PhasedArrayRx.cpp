#include "RADAR_PhasedArrayRx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_PhasedArrayRx )
{	
	SET_MODEL_DESCRIPTION("This model is used to model the phased array antenna");

	SET_MODEL_CATEGORY("Array TR");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(ArrayInput);
		port.SetDescription("The array input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(TargetThetaIn);
		port.SetDescription("The angle subtended from the z axis to the point of target in antenna coordinate in radians");
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(TargetPhiIn);
		port.SetDescription("The angle between the projection of beam direction onto the x–y axis and the x axis in antenna coordinate in radians");
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(BeamThetaIn);
		port.SetDescription("The angle subtended from the z axis to the point of beam direction in antenna coordinate in radians");
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(BeamPhiIn);
		port.SetDescription("The angle between the projection of beam direction onto the x–y axis and the x axis in antenna coordinate in radians");
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(ArrayOutput);
		port.SetDescription("The transmitted signal of phased array antenna");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Configuration, SelectedConfiguration);
		enumParam.SetDescription("The phased array manifold: Uniform Linear Array, Uniform Rectangular Array");
		enumParam.AddEnumeration("UniformLinearArray", UniformLinearArray);
		enumParam.AddEnumeration("UniformRectangularArray", UniformRectangularArray);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(AxisType, SelectedAxisType);
		enumParam.SetDescription("The axis of coordinate: X, Y, Z");
		enumParam.AddEnumeration("X", X);
		enumParam.AddEnumeration("Y", Y);
		enumParam.AddEnumeration("Z", Z);
		enumParam.SetDefaultValue("0");
		enumParam.SetHideCondition("Configuration ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Array2DShapeType, SelectedArray2DShapeType);
		enumParam.SetDescription("The shape of 2-D Array: Full, Customized");
		enumParam.AddEnumeration("Full", Full);
		enumParam.AddEnumeration("Customized", Customized);
		enumParam.SetDefaultValue("0");
		enumParam.SetHideCondition("Configuration ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumOfAnt1D);
		param.SetDescription("Number of Antenna Element for 1-D Array");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("8");
		param.SetHideCondition("Configuration ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumOfAnt2D_H);
		param.SetDescription("Number of Horizontal Antenna Element for 2-D Array");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("8");
		param.SetHideCondition("Configuration ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumOfAnt2D_V);
		param.SetDescription("Number of Vertical Antenna Element for 2-D Array");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("8");
		param.SetHideCondition("Configuration ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(ElementFactor);
		param.SetDescription("The element factor of array, assuming the element pattern is (cos(theta))^(ElementFactor/2)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(SpaceType, SelectedSpaceType);
		enumParam.SetDescription("The space of element is uniform or non-uniform : Uniform, NonUniform");
		enumParam.AddEnumeration("Uniform", Uniform);
		enumParam.AddEnumeration("NonUniform", NonUniform);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(GridType, SelectedGridType);
		enumParam.SetDescription("The type of grid : Rectangular, Triangular");
		enumParam.AddEnumeration("Rectangular", Rectangular);
		enumParam.AddEnumeration("Triangular", Triangular);
		enumParam.SetDefaultValue("0");
		enumParam.SetHideCondition("Configuration ~= 1 || SpaceType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(D);
		param.SetDescription("The distance between elements in wavelength");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0.5");
		param.SetHideCondition("Configuration ~= 0 || SpaceType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(D_H);
		param.SetDescription("The element distance of Horizontal Antenna Element for 2-D Array in wavelength");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0.5");
		param.SetHideCondition("Configuration ~= 1 || SpaceType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(D_V);
		param.SetDescription("The element distance of Vertical Antenna Element for 2-D Array in wavelength");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0.5");
		param.SetHideCondition("Configuration ~= 1 || SpaceType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(D_array);
		param.SetDescription("The distance between elements in meters");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("[-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5]");
		param.SetHideCondition("Configuration ~= 0 || SpaceType ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(D_H_array);
		param.SetDescription("The element distance of Horizontal Antenna Element for 2-D Array in meters");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("[-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5]");
		param.SetHideCondition("Configuration ~= 1 ||SpaceType ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(D_V_array);
		param.SetDescription("The element distance of Vertical Antenna Element for 2-D Array in meters");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("[-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5,-3.5,-2.5,-1.5,-0.5,0.5,1.5,2.5,3.5]");
		param.SetHideCondition("Configuration ~= 1 ||SpaceType ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(mask_array);
		param.SetDescription("The mask array for customized shape array. The array element reprents whether the element at this postion is enable. 1: enable, 0: disable");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]");
		param.SetHideCondition("Configuration ~= 1 ||NumOfAnt2D_H ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(ReliabilityType, SelectedReliabilityType);
		enumParam.SetDescription("To test the pattern with/without element failures : NoFailures, RandomElement");
		enumParam.AddEnumeration("NoFailures", NoFailures);
		enumParam.AddEnumeration("RandomElement", RandomElement);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FailureProbability);
		param.SetDescription("The failure probability of element");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0.1");
		param.SetHideCondition("ReliabilityType ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(TargetTheta);
		param.SetDescription("The angle subtended from the z axis to the point of target in antenna coordinate");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(TargetPhi);
		param.SetDescription("The angle between the projection of target onto the x–y axis and the x axis in antenna coordinate");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(WindowType, SelectedWindowType);
		enumParam.SetDescription("The type of window function: Rectangle, Bartlett, Hanning, Hamming, Blackman, SteepBlackman, Kaiser, Taylor");
		enumParam.AddEnumeration("Rectangle", Rectangle);			// 0
		enumParam.AddEnumeration("Bartlett", Bartlett);				// 1
		enumParam.AddEnumeration("Hanning", Hanning);				// 2
		enumParam.AddEnumeration("Hamming", Hamming);				// 3
		enumParam.AddEnumeration("Blackman", Blackman);				// 4
		enumParam.AddEnumeration("SteepBlackman", SteepBlackman);	// 5
		enumParam.AddEnumeration("Kaiser", Kaiser);					// 6
		enumParam.AddEnumeration("Taylor", Taylor);					// 7
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(KaiserWindowParameter);
		param.SetDescription("The alpha value for Kaiser window function");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
		param.SetHideCondition("WindowType ~= 6");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Sidelobe_Levels);
		param.SetDescription("Sidelobe_levels in dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("-20");
		param.SetHideCondition("WindowType ~= 7");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(nBar);
		param.SetDescription("This parameter is used to generate the Taylor distribution");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("2");
		param.SetHideCondition("WindowType ~= 7");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(IsPhaseShift, SelectedYesorNo);
		enumParam.SetDescription("With/Without PhaseShifter: Yes, No");
		enumParam.AddEnumeration("Yes", Yes);
		enumParam.AddEnumeration("No", No);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BeamTheta);
		param.SetDescription("The angle subtended from the z axis to the point of beam direction in antenna coordinate");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(BeamPhi);
		param.SetDescription("The angle between the projection of beam direction onto the x–y axis and the x axis in antenna coordinate");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(QuantizationType, SelectedYesorNo);
		enumParam.SetDescription("QuantizationType: Yes, No");
		enumParam.AddEnumeration("Yes", Yes);
		enumParam.AddEnumeration("No", No);
		enumParam.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PhaseShifterBitwidth);
		param.SetDescription("The quantized bitwidth of phase shifter");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("5");
		param.SetHideCondition("QuantizationType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(PhaseShiftType, SelectedPhaseShiftType);
		enumParam.SetDescription("The phase shift value calculation method: Calculate by theta and phi, DesiredPhaseShift");
		enumParam.AddEnumeration("CalculateByThetaAndPhi", CalculateByThetaAndPhi);
		enumParam.AddEnumeration("DesiredPhaseShift", DesiredPhaseShift);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(DesiredPhaseShiftAngle);
		param.SetDescription("Desired phase shift angles");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]");
		param.SetHideCondition("PhaseShiftType ~= 1");
	}
	return true;
}
#endif

RADAR_PhasedArrayRx::RADAR_PhasedArrayRx()
{

}

bool RADAR_PhasedArrayRx::Setup()
{
	bool bStatus = true;

	// 参数校验
	if (NumOfAnt1D <= 0 && Configuration == 0)
	{
		POST_ERROR("NumOfAnt1D must be > 0");
		bStatus = false;
	}
	if (NumOfAnt2D_H <= 0 && Configuration == 1)
	{
		POST_ERROR("NumOfAnt2D_H must be > 0");
		bStatus = false;
	}
	if (NumOfAnt2D_V <= 0 && Configuration == 1)
	{
		POST_ERROR("NumOfAnt2D_V must be > 0");
		bStatus = false;
	}
	if (D <= 0 && Configuration == 0 && SpaceType == 0)
	{
		POST_ERROR("D must be > 0");
		bStatus = false;
	}
	if (D_H <= 0 && Configuration == 1 && SpaceType == 0)
	{
		POST_ERROR("D_H must be > 0");
		bStatus = false;
	}
	if (D_V <= 0 && Configuration == 1 && SpaceType == 0)
	{
		POST_ERROR("D_V must be > 0");
		bStatus = false;
	}
	if ((FailureProbability < 0 || FailureProbability > 1) && ReliabilityType == 1)
	{
		POST_ERROR("FailureProbability must be >= 0 and <= 1");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_PhasedArrayRx::Run()
{
	// 若通过端口输入参数，则以端口输入为准
	if (TargetPhiIn.IsConnected())
	{
		TargetPhi = TargetPhiIn[0];
	}
	if (TargetThetaIn.IsConnected())
	{
		TargetTheta = TargetThetaIn[0];
	}
	if (BeamPhiIn.IsConnected())
	{
		BeamPhi = BeamPhiIn[0];
	}
	if (BeamThetaIn.IsConnected())
	{
		BeamTheta = BeamThetaIn[0];
	}

	double theta1 = std::sin(TargetPhi)*std::cos(TargetTheta);
	double theta2 = std::sin(TargetTheta);



	switch (Configuration)
	{
		// 线阵
	case RADAR_PhasedArrayRx::UniformLinearArray:
	{
		// NumOfAnt1D 阵元数量
		// D 阵元间距
		// 声明阵元坐标
		SystemVueModelBuilder::Matrix<double> ElementsLoc(1, NumOfAnt1D);

		switch (SpaceType)
		{
		case RADAR_PhasedArrayRx::Uniform:

			// 均匀线阵求阵元坐标
			for (int i = 0; i < NumOfAnt1D; i++)
			{
				ElementsLoc(i) = D * i - D * (NumOfAnt1D - 1) / 2;
			}
			break;
		case RADAR_PhasedArrayRx::NonUniform:

			// 用户定义的阵元坐标数量需要与阵元数量对应
			if (D_array.NumElements() != NumOfAnt1D)
			{
				POST_ERROR("The size of D_array should be same as NumOfAnt1D");
				return false;
			}
			// 非均匀线阵由用户定义阵元坐标
			for (int i = 0; i < NumOfAnt1D; i++)
			{
				ElementsLoc(i) = D_array(i);
			}
			break;
		default:
			break;
		}


		break;
	}

	// 面阵
	case RADAR_PhasedArrayRx::UniformRectangularArray:
	{
		// 声明阵元坐标
		SystemVueModelBuilder::Matrix<double> ElementsLoc_H(1, NumOfAnt2D_H);
		SystemVueModelBuilder::Matrix<double> ElementsLoc_V(1, NumOfAnt2D_V);

		switch (SpaceType)
		{
		case RADAR_PhasedArrayRx::Uniform:

			// 均匀线阵求阵元坐标
			for (int i = 0; i < NumOfAnt1D; i++)
			{
				ElementsLoc_H(i) = D_H * i - D_H * (NumOfAnt2D_H - 1) / 2;
				ElementsLoc_V(i) = D_V * i - D_V * (NumOfAnt2D_V - 1) / 2;
			}
			break;
		case RADAR_PhasedArrayRx::NonUniform:

			// 用户定义的阵元坐标数量需要与阵元数量对应
			if (D_H_array.NumElements() != NumOfAnt2D_H)
			{
				POST_ERROR("The size of D_H_array should be same as NumOfAnt2D_H");
				return false;
			}
			if (D_V_array.NumElements() != NumOfAnt2D_V)
			{
				POST_ERROR("The size of D_V_array should be same as NumOfAnt2D_V");
				return false;
			}
			// 非均匀面阵由用户定义阵元坐标
			for (int i = 0; i < NumOfAnt2D_H; i++)
			{
				ElementsLoc_H(i) = D_H_array(i);
			}
			for (int i = 0; i < NumOfAnt2D_V; i++)
			{
				ElementsLoc_V(i) = D_V_array(i);
			}
			break;

		default:
			break;
		}

		break;
	}
	default:
		break;
	}

	// 保底输出

	AntennaGain = 1.0;
	for (int i = 0; i < ArrayInput.GetSize(); i++)
	{
		ArrayOutput[i][0] = ArrayInput[i][0] * AntennaGain;
	}

	return true;
}
