#include "COMM_AntennaPolarizationTx.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(COMM_AntennaPolarizationTx)
{
	SET_MODEL_DESCRIPTION("Communication antenna radiation pattern, gain and polarization for TX");
	SET_MODEL_CATEGORY("Communications");

	// =====================================================================
	// 输入端口
	// =====================================================================
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(DirectionAzimuth);
		p.SetDescription("Azimuth angle of communication user or propagation path related to the antenna reference coordinate (radian)");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(DirectionElevation);
		p.SetDescription("Elevation angle of communication user or propagation path related to the antenna reference coordinate (radian)");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(BeamAzimuth);
		p.SetDescription("Azimuth angle of beam direction related to the antenna reference coordinate (radian)");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(BeamElevation);
		p.SetDescription("Elevation angle of beam direction related to the antenna reference coordinate (radian)");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input);
		p.SetDescription("Input envelope signal in the communication Tx chain");
	}

	// =====================================================================
	// 输出端口
	// =====================================================================
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output_V);
		p.SetDescription("Vertical polarization output envelope signal");
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output_H);
		p.SetDescription("Horizontal polarization output envelope signal");
	}

	// =====================================================================
	// 方向图来源
	// =====================================================================
	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(PatternDataMode, SelectedPatternDataMode);

		p.AddEnumeration("Parametric Pattern", COMM_AntennaPolarizationTx::ParametricPattern);
		p.AddEnumeration("Import Actual Pattern", COMM_AntennaPolarizationTx::ImportActualPattern);
		p.SetDescription("Select whether the antenna pattern is generated from parameters or imported from actual far-field data");
		p.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(ParametricPatternType, SelectedParametricPatternType);

		p.AddEnumeration("Isotropic", COMM_AntennaPolarizationTx::IsotropicPattern);
		p.AddEnumeration("Cosine Power", COMM_AntennaPolarizationTx::CosinePowerPattern);
		p.AddEnumeration("Gaussian", COMM_AntennaPolarizationTx::GaussianPattern);
		p.AddEnumeration("3GPP", COMM_AntennaPolarizationTx::ThreeGPPPattern);
		p.SetDescription("Parametric antenna radiation pattern type");
		p.SetDefaultValue("2");
		p.SetHideCondition("PatternDataMode ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(PeakGain_dBi);
		p.SetDescription("Peak antenna power gain in dBi");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetHideCondition("PatternDataMode == 1 && ImportedGainMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(AzimuthHPBW);
		p.SetDescription("Azimuth half-power beamwidth");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("65");
		p.SetHideCondition("PatternDataMode ~= 0 || ParametricPatternType == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(ElevationHPBW);
		p.SetDescription("Elevation half-power beamwidth");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("65");
		p.SetHideCondition("PatternDataMode ~= 0 || ParametricPatternType == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(MaxAttenuation_dB);
		p.SetDescription("Maximum attenuation of the normalized parametric pattern");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("30");
		p.SetHideCondition("PatternDataMode ~= 0 || ParametricPatternType == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(VerticalSidelobeAttenuation_dB);
		p.SetDescription("Vertical side-lobe attenuation used by the 3GPP pattern");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("30");
		p.SetHideCondition("PatternDataMode ~= 0 || ParametricPatternType ~= 3");
	}

	// =====================================================================
	// 极化参数
	// =====================================================================
	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(PolarizationType, SelectedPolarizationType);

		p.AddEnumeration("Pattern Components", COMM_AntennaPolarizationTx::PatternComponents);
		p.AddEnumeration("Horizontal", COMM_AntennaPolarizationTx::HorizontalPolarization);
		p.AddEnumeration("Vertical", COMM_AntennaPolarizationTx::VerticalPolarization);
		p.AddEnumeration("Linear Slant", COMM_AntennaPolarizationTx::LinearSlantPolarization);
		p.AddEnumeration("RHCP", COMM_AntennaPolarizationTx::RHCPPolarization);
		p.AddEnumeration("LHCP", COMM_AntennaPolarizationTx::LHCPPolarization);
		p.AddEnumeration("User Defined Jones", COMM_AntennaPolarizationTx::UserDefinedJones);
		p.SetDescription("Polarization mode. Pattern Components directly uses imported theta/phi complex pattern components");
		p.SetDefaultValue("2");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(PolarizationTiltAngle);
		p.SetDescription("Linear polarization tilt angle measured from the horizontal polarization axis");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("45");
		p.SetHideCondition("PolarizationType ~= 3");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(XPD_dB);
		p.SetDescription("Cross-polarization discrimination in dB");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("40");
		p.SetHideCondition("PolarizationType == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(CrossPolarPhaseAngle);
		p.SetDescription("Phase of the cross-polarization leakage component");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetHideCondition("PolarizationType == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(UserJonesHMagnitude);
		p.SetDescription("Magnitude of the horizontal component of the user-defined Jones vector");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("1");
		p.SetHideCondition("PolarizationType ~= 6");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(UserJonesHPhase);
		p.SetDescription("Phase of the horizontal component of the user-defined Jones vector");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetHideCondition("PolarizationType ~= 6");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(UserJonesVMagnitude);
		p.SetDescription("Magnitude of the vertical component of the user-defined Jones vector");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("0");
		p.SetHideCondition("PolarizationType ~= 6");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(UserJonesVPhase);
		p.SetDescription("Phase of the vertical component of the user-defined Jones vector");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetHideCondition("PolarizationType ~= 6");
	}

	// =====================================================================
	// 实际方向图文件参数
	// 仅在 PatternDataMode == ImportActualPattern 时显示。
	// =====================================================================
	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(ElementPatternFileType, SelectedElementPatternFileType);

		p.AddEnumeration("EMPro", COMM_AntennaPolarizationTx::EMPro);
		p.AddEnumeration("HFSS", COMM_AntennaPolarizationTx::HFSS);
		p.AddEnumeration("CST", COMM_AntennaPolarizationTx::CST);
		p.SetDescription("Imported antenna pattern file format: EMPro, HFSS or CST");
		p.SetDefaultValue("0");
		p.SetHideCondition("PatternDataMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ARRAY_PARAMETER(ElementPatternFileScaleFactor,
				ElementPatternFileScaleFactor_Size);

		p.SetDescription("Linear amplitude scale factors for theta/V and phi/H pattern components. One value applies to both components");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("[1]");
		p.SetHideCondition("PatternDataMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(ImportedPatternDimension,
				SelectedImportedPatternDimension);

		p.AddEnumeration("UserDefine2D", COMM_AntennaPolarizationTx::UserDefine2D);
		p.AddEnumeration("UserDefine3D", COMM_AntennaPolarizationTx::UserDefine3D);
		p.SetDescription("Dimension of the imported antenna pattern");
		p.SetDefaultValue("1");
		p.SetHideCondition("PatternDataMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(ImportedGainMode,
				SelectedImportedGainMode);

		p.AddEnumeration("Use File Gain", COMM_AntennaPolarizationTx::UseFileGain);
		p.AddEnumeration("Normalize File To Peak Gain", COMM_AntennaPolarizationTx::NormalizeFileToPeakGain);
		p.SetDescription("Use absolute complex gain from the file or normalize the imported pattern to PeakGain_dBi");
		p.SetDefaultValue("0");
		p.SetHideCondition("PatternDataMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(TxAntennaPatternFileName1);
		p.SetDescription("Communication Tx antenna pattern filename");
		p.SetSchematicDisplay(0);
		p.SetParamAsFile();
		p.SetHideCondition("PatternDataMode ~= 1");
	}

	// =====================================================================
	// 波束控制
	// =====================================================================
	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(BeamControlMode, SelectedBeamControlMode);

		p.AddEnumeration("Fixed Beam", COMM_AntennaPolarizationTx::FixedBeam);
		p.AddEnumeration("Beam Sweep", COMM_AntennaPolarizationTx::BeamSweep);
		p.SetDescription("Communication beam control mode");
		p.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ENUM_PARAMETER(BeamScanPattern, SelectedBeamScanPattern);

		p.AddEnumeration("Circular", COMM_AntennaPolarizationTx::CircularScan);
		p.AddEnumeration("Bidirectional Sector", COMM_AntennaPolarizationTx::BidirectionalSector);
		p.AddEnumeration("Unidirectional Sector", COMM_AntennaPolarizationTx::UnidirectionalSector);
		p.AddEnumeration("Bidirectional Raster", COMM_AntennaPolarizationTx::BidirectionalRaster);
		p.AddEnumeration("Unidirectional Raster", COMM_AntennaPolarizationTx::UnidirectionalRaster);
		p.SetDescription("Communication beam sweep pattern");
		p.SetDefaultValue("0");
		p.SetHideCondition("BeamControlMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(ScanRate);
		p.SetDescription("Beam scan rate in rpm");
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("15");
		p.SetHideCondition("BeamControlMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(ElevationAngle);
		p.SetDescription("Elevation angle used by circular and sector beam scans");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetHideCondition("BeamControlMode ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(SectorScanStartAngle);
		p.SetDescription("Start angle of the beam scan sector");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("-60");
		p.SetHideCondition("BeamControlMode ~= 1 || BeamScanPattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(SectorScanEndAngle);
		p.SetDescription("End angle of the beam scan sector");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("60");
		p.SetHideCondition("BeamControlMode ~= 1 || BeamScanPattern == 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(FlybackTime);
		p.SetDescription("Flyback time from the scan end position to the start position");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDefaultValue("0");
		p.SetHideCondition("BeamControlMode ~= 1 || (BeamScanPattern ~= 2 && BeamScanPattern ~= 4)");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(NumberOfRasterBars);
		p.SetDescription("Number of additional raster bars");
		p.SetDefaultValue("0");
		p.SetHideCondition("BeamControlMode ~= 1 || (BeamScanPattern ~= 3 && BeamScanPattern ~= 4)");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(RasterBarWidth);
		p.SetDescription("Elevation angle between adjacent raster bars");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("5");
		p.SetHideCondition("BeamControlMode ~= 1 || (BeamScanPattern ~= 3 && BeamScanPattern ~= 4)");
	}

	// =====================================================================
	// 默认方向参数
	// =====================================================================
	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ARRAY_PARAMETER(DirectionAzimuthAngle,
				DirectionAzimuthAngle_Size);

		p.SetDescription("Default azimuth angles of communication users or propagation paths");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("[0]");
	}

	{
		SystemVueModelBuilder::DFParam p =
			ADD_MODEL_ARRAY_PARAMETER(DirectionElevationAngle,
				DirectionElevationAngle_Size);

		p.SetDescription("Default elevation angles of communication users or propagation paths");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("[0]");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(BeamAzimuthAngle);
		p.SetDescription("Default fixed beam azimuth angle");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetHideCondition("BeamControlMode ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(BeamElevationAngle);
		p.SetDescription("Default fixed beam elevation angle");
		p.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		p.SetDefaultValue("0");
		p.SetHideCondition("BeamControlMode ~= 0");
	}

	return true;
}
#endif

COMM_AntennaPolarizationTx::COMM_AntennaPolarizationTx() :
	PatternDataMode(ParametricPattern),
	ParametricPatternType(GaussianPattern),
	PeakGain_dBi(0.0),
	AzimuthHPBW(65.0),
	ElevationHPBW(65.0),
	MaxAttenuation_dB(30.0),
	VerticalSidelobeAttenuation_dB(30.0),
	PolarizationType(VerticalPolarization),
	PolarizationTiltAngle(45.0),
	XPD_dB(40.0),
	CrossPolarPhaseAngle(0.0),
	UserJonesHMagnitude(1.0),
	UserJonesHPhase(0.0),
	UserJonesVMagnitude(0.0),
	UserJonesVPhase(0.0),
	ElementPatternFileType(EMPro),
	ElementPatternFileScaleFactor(0),
	ElementPatternFileScaleFactor_Size(0),
	ImportedPatternDimension(UserDefine3D),
	ImportedGainMode(UseFileGain),
	TxAntennaPatternFileName1(0),
	BeamControlMode(FixedBeam),
	BeamScanPattern(CircularScan),
	ScanRate(15.0),
	ElevationAngle(0.0),
	SectorScanStartAngle(-60.0),
	SectorScanEndAngle(60.0),
	FlybackTime(0.0),
	NumberOfRasterBars(0),
	RasterBarWidth(5.0),
	DirectionAzimuthAngle(0),
	DirectionAzimuthAngle_Size(0),
	DirectionElevationAngle(0),
	DirectionElevationAngle_Size(0),
	BeamAzimuthAngle(0.0),
	BeamElevationAngle(0.0),
	patternLoaded_(false),
	patternPeakAmplitude_(0.0)
{
}

bool COMM_AntennaPolarizationTx::Setup()
{
	input.SetRate(1);

	BeamAzimuth.SetRate(1);
	BeamElevation.SetRate(1);

	for (size_t i = 0; i < DirectionAzimuth.GetSize(); ++i)
	{
		DirectionAzimuth[i].SetRate(1);
	}

	for (size_t i = 0; i < DirectionElevation.GetSize(); ++i)
	{
		DirectionElevation[i].SetRate(1);
	}

	for (size_t i = 0; i < output_V.GetSize(); ++i)
	{
		output_V[i].SetRate(1);
	}

	for (size_t i = 0; i < output_H.GetSize(); ++i)
	{
		output_H[i].SetRate(1);
	}

	if (!validateConfiguration())
	{
		return false;
	}

	if (PatternDataMode == ImportActualPattern)
	{
		if (!loadPatternFile())
		{
			POST_ERROR("COMM_AntennaPolarizationTx: failed to load the selected antenna pattern file.");
			return false;
		}
	}
	else
	{
		clearPattern();
	}

	return true;
}

ERESULT COMM_AntennaPolarizationTx::PropagateCharacterizationFrequency()
{
	const double fc = input.GetCharacterizationFrequency();

	for (size_t i = 0; i < output_V.GetSize(); ++i)
	{
		output_V[i].SetCharacterizationFrequency(fc);
	}

	for (size_t i = 0; i < output_H.GetSize(); ++i)
	{
		output_H[i].SetCharacterizationFrequency(fc);
	}

	return static_cast<ERESULT>(0);
}

bool COMM_AntennaPolarizationTx::Run()
{
	const std::complex<double> x = input[0].complex();
	const double timeNow = input.GetTime(0, GetCount());

	double beamAzRad = 0.0;
	double beamElRad = 0.0;
	getBeamAngle(timeNow, beamAzRad, beamElRad);

	const int directionCount = determineDirectionCount();

	for (size_t i = 0; i < output_V.GetSize(); ++i)
	{
		output_V[i][0] = std::complex<double>(0.0, 0.0);
	}

	for (size_t i = 0; i < output_H.GetSize(); ++i)
	{
		output_H[i][0] = std::complex<double>(0.0, 0.0);
	}

	const size_t outVSize = output_V.GetSize();
	const size_t outHSize = output_H.GetSize();
	const int maxWritable = static_cast<int>(std::max(outVSize, outHSize));

	const int nRun =
		(maxWritable > 0)
		? std::min(directionCount, maxWritable)
		: directionCount;

	for (int ch = 0; ch < nRun; ++ch)
	{
		double directionAzRad = 0.0;
		double directionElRad = 0.0;

		if (hasDirectionAzimuthPort(ch))
		{
			directionAzRad = DirectionAzimuth[static_cast<size_t>(ch)][0];
		}
		else
		{
			directionAzRad = deg2rad(getArrayValue(DirectionAzimuthAngle,
				DirectionAzimuthAngle_Size,
				ch,
				0.0));
		}

		if (hasDirectionElevationPort(ch))
		{
			directionElRad = DirectionElevation[static_cast<size_t>(ch)][0];
		}
		else
		{
			directionElRad = deg2rad(getArrayValue(DirectionElevationAngle,
				DirectionElevationAngle_Size,
				ch,
				0.0));
		}

		const double relAzRad = normalizeRad(directionAzRad - beamAzRad);
		const double relElRad = normalizeRad(directionElRad - beamElRad);

		std::complex<double> Gtheta(0.0, 0.0);
		std::complex<double> Gphi(0.0, 0.0);

		if (PatternDataMode == ImportActualPattern)
		{
			double thetaDeg = 90.0;
			double phiDeg = 0.0;

			azelToPatternThetaPhi(relAzRad,
				relElRad,
				thetaDeg,
				phiDeg);

			lookupImportedPolarizationGain(thetaDeg,
				phiDeg,
				Gtheta,
				Gphi);

			const double scaleTheta = getScaleValue(0);
			const double scalePhi =
				(ElementPatternFileScaleFactor_Size >= 2)
				? getScaleValue(1)
				: scaleTheta;

			Gtheta *= scaleTheta;
			Gphi *= scalePhi;

			if (ImportedGainMode == NormalizeFileToPeakGain)
			{
				const double targetPeakAmplitude = linearAmplitudeFromDb(PeakGain_dBi);

				if (patternPeakAmplitude_ > 0.0)
				{
					const double normalizationScale =
						targetPeakAmplitude / patternPeakAmplitude_;

					Gtheta *= normalizationScale;
					Gphi *= normalizationScale;
				}
			}

			if (PolarizationType != PatternComponents)
			{
				const std::complex<double> scalarGain =
					combinePatternComponentsToScalar(Gtheta, Gphi);

				applyConfiguredPolarization(scalarGain,
					Gtheta,
					Gphi);
			}
		}
		else
		{
			const std::complex<double> scalarGain =
				calculateParametricScalarGain(relAzRad, relElRad);

			applyConfiguredPolarization(scalarGain,
				Gtheta,
				Gphi);
		}

		const std::complex<double> yV = x * Gtheta;
		const std::complex<double> yH = x * Gphi;

		if (static_cast<size_t>(ch) < output_V.GetSize())
		{
			output_V[static_cast<size_t>(ch)][0] = yV;
		}

		if (static_cast<size_t>(ch) < output_H.GetSize())
		{
			output_H[static_cast<size_t>(ch)][0] = yH;
		}
	}

	return true;
}

// ============================================================================
// 端口连接判断
// ============================================================================

bool COMM_AntennaPolarizationTx::hasBeamAzimuthPort()
{
	return BeamAzimuth.IsConnected();
}

bool COMM_AntennaPolarizationTx::hasBeamElevationPort()
{
	return BeamElevation.IsConnected();
}

bool COMM_AntennaPolarizationTx::hasDirectionAzimuthPort(int ch)
{
	if (ch < 0)
	{
		return false;
	}

	if (static_cast<size_t>(ch) >= DirectionAzimuth.GetSize())
	{
		return false;
	}

	return DirectionAzimuth[static_cast<size_t>(ch)].IsConnected();
}

bool COMM_AntennaPolarizationTx::hasDirectionElevationPort(int ch)
{
	if (ch < 0)
	{
		return false;
	}

	if (static_cast<size_t>(ch) >= DirectionElevation.GetSize())
	{
		return false;
	}

	return DirectionElevation[static_cast<size_t>(ch)].IsConnected();
}

int COMM_AntennaPolarizationTx::determineDirectionCount()
{
	int n = 0;

	for (size_t i = 0; i < DirectionAzimuth.GetSize(); ++i)
	{
		if (DirectionAzimuth[i].IsConnected())
		{
			n = std::max(n, static_cast<int>(i + 1));
		}
	}

	for (size_t i = 0; i < DirectionElevation.GetSize(); ++i)
	{
		if (DirectionElevation[i].IsConnected())
		{
			n = std::max(n, static_cast<int>(i + 1));
		}
	}

	n = std::max(n, DirectionAzimuthAngle_Size);
	n = std::max(n, DirectionElevationAngle_Size);
	n = std::max(n, static_cast<int>(output_V.GetSize()));
	n = std::max(n, static_cast<int>(output_H.GetSize()));

	if (n <= 0)
	{
		n = 1;
	}

	return n;
}

// ============================================================================
// 参数数组读取
// ============================================================================

double COMM_AntennaPolarizationTx::getArrayValue(const double* data,
	int size,
	int index,
	double defaultValue) const
{
	if (data == 0 || size <= 0)
	{
		return defaultValue;
	}

	if (index < 0)
	{
		return data[0];
	}

	if (index < size)
	{
		return data[index];
	}

	return data[size - 1];
}

double COMM_AntennaPolarizationTx::getScaleValue(int index) const
{
	if (ElementPatternFileScaleFactor == 0 ||
		ElementPatternFileScaleFactor_Size <= 0)
	{
		return 1.0;
	}

	if (index < 0)
	{
		return ElementPatternFileScaleFactor[0];
	}

	if (index < ElementPatternFileScaleFactor_Size)
	{
		return ElementPatternFileScaleFactor[index];
	}

	return ElementPatternFileScaleFactor[ElementPatternFileScaleFactor_Size - 1];
}

// ============================================================================
// Beam 角度计算
// ============================================================================

void COMM_AntennaPolarizationTx::getBeamAngle(double timeNow,
	double& beamAzRad,
	double& beamElRad)
{
	if (hasBeamAzimuthPort())
	{
		beamAzRad = BeamAzimuth[0];
	}
	else if (BeamControlMode == BeamSweep)
	{
		if (BeamScanPattern == CircularScan)
		{
			beamAzRad = deg2rad(getCircularScanAzimuth(timeNow));
		}
		else if (BeamScanPattern == BidirectionalSector)
		{
			beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, true));
		}
		else if (BeamScanPattern == UnidirectionalSector)
		{
			beamAzRad = deg2rad(getSectorScanAzimuth(timeNow, false));
		}
		else if (BeamScanPattern == BidirectionalRaster)
		{
			double azDeg = 0.0;
			double elDeg = 0.0;
			getRasterScanAngle(timeNow, true, azDeg, elDeg);
			beamAzRad = deg2rad(azDeg);
		}
		else if (BeamScanPattern == UnidirectionalRaster)
		{
			double azDeg = 0.0;
			double elDeg = 0.0;
			getRasterScanAngle(timeNow, false, azDeg, elDeg);
			beamAzRad = deg2rad(azDeg);
		}
		else
		{
			beamAzRad = deg2rad(BeamAzimuthAngle);
		}
	}
	else
	{
		beamAzRad = deg2rad(BeamAzimuthAngle);
	}

	if (hasBeamElevationPort())
	{
		beamElRad = BeamElevation[0];
	}
	else if (BeamControlMode == BeamSweep &&
		(BeamScanPattern == BidirectionalRaster ||
			BeamScanPattern == UnidirectionalRaster))
	{
		double azDeg = 0.0;
		double elDeg = 0.0;

		getRasterScanAngle(timeNow,
			BeamScanPattern == BidirectionalRaster,
			azDeg,
			elDeg);

		beamElRad = deg2rad(elDeg);
	}
	else if (BeamControlMode == BeamSweep)
	{
		beamElRad = deg2rad(ElevationAngle);
	}
	else
	{
		beamElRad = deg2rad(BeamElevationAngle);
	}
}

double COMM_AntennaPolarizationTx::getCircularScanAzimuth(double timeNow) const
{
	const double rateDegPerSec = ScanRate * 6.0;

	if (rateDegPerSec == 0.0)
	{
		return 0.0;
	}

	return wrap360(rateDegPerSec * timeNow);
}

double COMM_AntennaPolarizationTx::getSectorScanAzimuth(double timeNow,
	bool bidirectional) const
{
	const double startDeg = SectorScanStartAngle;
	const double endDeg = SectorScanEndAngle;

	double width = endDeg - startDeg;

	if (std::fabs(width) < 1.0e-15)
	{
		return startDeg;
	}

	const double dir = (width >= 0.0) ? 1.0 : -1.0;
	width = std::fabs(width);

	const double rateDegPerSec = std::fabs(ScanRate * 6.0);

	if (rateDegPerSec <= 0.0)
	{
		return startDeg;
	}

	const double forwardTime = width / rateDegPerSec;

	if (bidirectional)
	{
		const double period = 2.0 * forwardTime;

		if (period <= 0.0)
		{
			return startDeg;
		}

		double t = std::fmod(timeNow, period);

		if (t < 0.0)
		{
			t += period;
		}

		if (t <= forwardTime)
		{
			return startDeg + dir * rateDegPerSec * t;
		}

		return endDeg - dir * rateDegPerSec * (t - forwardTime);
	}

	const double flyback = std::max(0.0, FlybackTime);
	const double period = forwardTime + flyback;

	if (period <= 0.0)
	{
		return startDeg;
	}

	double t = std::fmod(timeNow, period);

	if (t < 0.0)
	{
		t += period;
	}

	if (t <= forwardTime)
	{
		return startDeg + dir * rateDegPerSec * t;
	}

	if (flyback > 0.0)
	{
		const double k = (t - forwardTime) / flyback;
		return endDeg + (startDeg - endDeg) * k;
	}

	return startDeg;
}

void COMM_AntennaPolarizationTx::getRasterScanAngle(double timeNow,
	bool bidirectional,
	double& azDeg,
	double& elDeg) const
{
	const int barCount = std::max(1, NumberOfRasterBars + 1);
	const double startDeg = SectorScanStartAngle;
	const double endDeg = SectorScanEndAngle;

	double width = endDeg - startDeg;

	if (std::fabs(width) < 1.0e-15)
	{
		azDeg = startDeg;
		elDeg = ElevationAngle;
		return;
	}

	const double dir = (width >= 0.0) ? 1.0 : -1.0;
	width = std::fabs(width);

	const double rateDegPerSec = std::fabs(ScanRate * 6.0);

	if (rateDegPerSec <= 0.0)
	{
		azDeg = startDeg;
		elDeg = ElevationAngle;
		return;
	}

	const double scanTime = width / rateDegPerSec;
	const double flyback = std::max(0.0, FlybackTime);
	const double rowTime = bidirectional ? scanTime : (scanTime + flyback);
	const double period = rowTime * barCount;

	if (period <= 0.0)
	{
		azDeg = startDeg;
		elDeg = ElevationAngle;
		return;
	}

	double t = std::fmod(timeNow, period);

	if (t < 0.0)
	{
		t += period;
	}

	int row = static_cast<int>(t / rowTime);

	if (row >= barCount)
	{
		row = barCount - 1;
	}

	const double rowLocal = t - row * rowTime;

	elDeg = ElevationAngle + row * RasterBarWidth;

	if (bidirectional)
	{
		const bool reverse = (row % 2) != 0;

		if (!reverse)
		{
			azDeg = startDeg + dir * rateDegPerSec * rowLocal;
		}
		else
		{
			azDeg = endDeg - dir * rateDegPerSec * rowLocal;
		}
	}
	else if (rowLocal <= scanTime)
	{
		azDeg = startDeg + dir * rateDegPerSec * rowLocal;
	}
	else if (flyback > 0.0)
	{
		const double k = (rowLocal - scanTime) / flyback;
		azDeg = endDeg + (startDeg - endDeg) * k;
	}
	else
	{
		azDeg = startDeg;
	}
}

// ============================================================================
// 参数化方向图和极化
// ============================================================================

bool COMM_AntennaPolarizationTx::validateConfiguration() const
{
	if (PatternDataMode == ParametricPattern)
	{
		if (PolarizationType == PatternComponents)
		{
			return false;
		}

		if (ParametricPatternType != IsotropicPattern)
		{
			if (AzimuthHPBW <= 0.0 || ElevationHPBW <= 0.0)
			{
				return false;
			}

			if (ParametricPatternType == CosinePowerPattern &&
				(AzimuthHPBW >= 180.0 || ElevationHPBW >= 180.0))
			{
				return false;
			}

			if (MaxAttenuation_dB < 0.0)
			{
				return false;
			}

			if (VerticalSidelobeAttenuation_dB < 0.0)
			{
				return false;
			}
		}
	}

	if (XPD_dB < 0.0)
	{
		return false;
	}

	if (PolarizationType == UserDefinedJones)
	{
		const double power =
			UserJonesHMagnitude * UserJonesHMagnitude +
			UserJonesVMagnitude * UserJonesVMagnitude;

		if (power <= 0.0)
		{
			return false;
		}
	}

	return true;
}

double COMM_AntennaPolarizationTx::calculateCosineExponent(
	double halfPowerBeamwidthDeg) const
{
	const double halfAngleRad = deg2rad(0.5 * halfPowerBeamwidthDeg);
	const double cosineValue = std::cos(halfAngleRad);

	if (cosineValue <= 0.0 || cosineValue >= 1.0)
	{
		return 0.0;
	}

	return std::log(0.5) / std::log(cosineValue);
}

double COMM_AntennaPolarizationTx::calculateParametricAttenuationDb(
	double relAzRad,
	double relElRad) const
{
	if (ParametricPatternType == IsotropicPattern)
	{
		return 0.0;
	}

	const double azDeg = std::fabs(rad2deg(relAzRad));
	const double elDeg = std::fabs(rad2deg(relElRad));
	const double maxAttenuation = std::max(0.0, MaxAttenuation_dB);

	if (ParametricPatternType == CosinePowerPattern)
	{
		const double exponentAz = calculateCosineExponent(AzimuthHPBW);
		const double exponentEl = calculateCosineExponent(ElevationHPBW);

		double powerAz = 0.0;
		double powerEl = 0.0;

		if (azDeg < 90.0)
		{
			powerAz = std::pow(std::max(0.0, std::cos(deg2rad(azDeg))),
				exponentAz);
		}

		if (elDeg < 90.0)
		{
			powerEl = std::pow(std::max(0.0, std::cos(deg2rad(elDeg))),
				exponentEl);
		}

		const double power = powerAz * powerEl;

		if (power <= 0.0)
		{
			return maxAttenuation;
		}

		const double attenuation = -10.0 * std::log10(power);
		return std::min(maxAttenuation, std::max(0.0, attenuation));
	}

	if (ParametricPatternType == GaussianPattern)
	{
		const double attenuation =
			12.0 * (azDeg / AzimuthHPBW) * (azDeg / AzimuthHPBW) +
			12.0 * (elDeg / ElevationHPBW) * (elDeg / ElevationHPBW);

		return std::min(maxAttenuation, std::max(0.0, attenuation));
	}

	const double horizontalAttenuation = std::min(
		12.0 * (azDeg / AzimuthHPBW) * (azDeg / AzimuthHPBW),
		maxAttenuation);

	const double verticalAttenuation = std::min(
		12.0 * (elDeg / ElevationHPBW) * (elDeg / ElevationHPBW),
		std::max(0.0, VerticalSidelobeAttenuation_dB));

	return std::min(maxAttenuation,
		horizontalAttenuation + verticalAttenuation);
}

std::complex<double> COMM_AntennaPolarizationTx::calculateParametricScalarGain(
	double relAzRad,
	double relElRad) const
{
	const double attenuationDb =
		calculateParametricAttenuationDb(relAzRad, relElRad);

	const double directionGainDb = PeakGain_dBi - attenuationDb;
	const double amplitude = linearAmplitudeFromDb(directionGainDb);

	return std::complex<double>(amplitude, 0.0);
}

void COMM_AntennaPolarizationTx::buildPolarizationJones(
	std::complex<double>& jonesV,
	std::complex<double>& jonesH) const
{
	jonesV = std::complex<double>(0.0, 0.0);
	jonesH = std::complex<double>(0.0, 0.0);

	if (PolarizationType == HorizontalPolarization)
	{
		jonesH = std::complex<double>(1.0, 0.0);
	}
	else if (PolarizationType == VerticalPolarization)
	{
		jonesV = std::complex<double>(1.0, 0.0);
	}
	else if (PolarizationType == LinearSlantPolarization)
	{
		const double tiltRad = deg2rad(PolarizationTiltAngle);
		jonesH = std::complex<double>(std::cos(tiltRad), 0.0);
		jonesV = std::complex<double>(std::sin(tiltRad), 0.0);
	}
	else if (PolarizationType == RHCPPolarization)
	{
		const double invSqrt2 = 1.0 / std::sqrt(2.0);
		jonesH = std::complex<double>(invSqrt2, 0.0);
		jonesV = std::complex<double>(0.0, -invSqrt2);
	}
	else if (PolarizationType == LHCPPolarization)
	{
		const double invSqrt2 = 1.0 / std::sqrt(2.0);
		jonesH = std::complex<double>(invSqrt2, 0.0);
		jonesV = std::complex<double>(0.0, invSqrt2);
	}
	else if (PolarizationType == UserDefinedJones)
	{
		jonesH = magPhaseToComplex(UserJonesHMagnitude,
			UserJonesHPhase,
			false,
			true);

		jonesV = magPhaseToComplex(UserJonesVMagnitude,
			UserJonesVPhase,
			false,
			true);
	}
	else
	{
		jonesV = std::complex<double>(1.0, 0.0);
	}

	normalizeJones(jonesV, jonesH);

	const double crossAmplitude =
		std::pow(10.0, -std::max(0.0, XPD_dB) / 20.0);

	if (crossAmplitude > 0.0)
	{
		const std::complex<double> orthogonalV = -std::conj(jonesH);
		const std::complex<double> orthogonalH = std::conj(jonesV);

		const std::complex<double> crossCoefficient =
			magPhaseToComplex(crossAmplitude,
				CrossPolarPhaseAngle,
				false,
				true);

		jonesV += crossCoefficient * orthogonalV;
		jonesH += crossCoefficient * orthogonalH;

		normalizeJones(jonesV, jonesH);
	}
}

void COMM_AntennaPolarizationTx::applyConfiguredPolarization(
	const std::complex<double>& scalarGain,
	std::complex<double>& Gtheta,
	std::complex<double>& Gphi) const
{
	std::complex<double> jonesV(0.0, 0.0);
	std::complex<double> jonesH(0.0, 0.0);

	buildPolarizationJones(jonesV, jonesH);

	Gtheta = scalarGain * jonesV;
	Gphi = scalarGain * jonesH;
}

std::complex<double> COMM_AntennaPolarizationTx::combinePatternComponentsToScalar(
	const std::complex<double>& Gtheta,
	const std::complex<double>& Gphi) const
{
	const double totalAmplitude =
		std::sqrt(std::norm(Gtheta) + std::norm(Gphi));

	if (totalAmplitude <= 0.0)
	{
		return std::complex<double>(0.0, 0.0);
	}

	const std::complex<double> dominant =
		(std::abs(Gtheta) >= std::abs(Gphi))
		? Gtheta
		: Gphi;

	const double dominantAmplitude = std::abs(dominant);

	if (dominantAmplitude <= 0.0)
	{
		return std::complex<double>(totalAmplitude, 0.0);
	}

	return dominant * (totalAmplitude / dominantAmplitude);
}

// ============================================================================
// 方向图坐标转换
// ============================================================================

void COMM_AntennaPolarizationTx::azelToPatternThetaPhi(double relAzRad,
	double relElRad,
	double& thetaDeg,
	double& phiDeg) const
{
	const double azDeg = rad2deg(relAzRad);
	const double elDeg = rad2deg(relElRad);

	thetaDeg = clamp(90.0 - elDeg, 0.0, 180.0);
	phiDeg = wrap360(azDeg);
}

// ============================================================================
// 实际方向图文件读取
// ============================================================================

void COMM_AntennaPolarizationTx::clearPattern()
{
	patternTable_.clear();
	patternLoaded_ = false;
	patternPeakAmplitude_ = 0.0;
	patternOpt_ = PatternFileOptions();
}

bool COMM_AntennaPolarizationTx::loadPatternFile()
{
	clearPattern();

	if (TxAntennaPatternFileName1 == 0)
	{
		return false;
	}

	const std::string fileName = TxAntennaPatternFileName1;

	if (fileName.empty())
	{
		return false;
	}

	std::ifstream fin(fileName.c_str());

	if (!fin.good())
	{
		return false;
	}

	bool inParameterSection = false;
	bool afterParameterSection = false;
	bool explicitParameterSectionFound = false;

	std::string line;

	while (std::getline(fin, line))
	{
		const std::string t = trim(line);

		if (t.empty())
		{
			continue;
		}

		const std::string low = lowerString(t);

		if (low.find("begin_<parameters>") != std::string::npos ||
			low.find("begin_parameters") != std::string::npos)
		{
			inParameterSection = true;
			afterParameterSection = false;
			explicitParameterSectionFound = true;
			continue;
		}

		if (low.find("end_<parameters>") != std::string::npos ||
			low.find("end_parameters") != std::string::npos)
		{
			inParameterSection = false;
			afterParameterSection = true;
			continue;
		}

		if (inParameterSection)
		{
			parseParameterLine(t);
			continue;
		}

		if (explicitParameterSectionFound && !afterParameterSection)
		{
			continue;
		}

		std::vector<double> nums;

		if (!parseNumericLine(t.c_str(), nums))
		{
			continue;
		}

		// EMPro mag/phase 数据可以只有四列；HFSS/CST 的通用远场数据
		// 通常至少包含六列。这里保持对原模型四列数据的兼容性。
		if (nums.size() < 4)
		{
			continue;
		}

		PatternPoint p;

		double theta = nums[0];
		double phi = nums[1];

		if (!patternOpt_.directionInDegrees)
		{
			theta = rad2deg(theta);
			phi = rad2deg(phi);
		}

		p.thetaDeg = theta;
		p.phiDeg = wrap360(phi);

		if (patternOpt_.useMagPhase)
		{
			const double thetaGain = nums.size() > 2 ? nums[2] : 0.0;
			const double phiGain = nums.size() > 3 ? nums[3] : thetaGain;
			const double thetaPhase = nums.size() > 4 ? nums[4] : 0.0;
			const double phiPhase = nums.size() > 5 ? nums[5] : 0.0;

			p.Gtheta = magPhaseToComplex(thetaGain,
				thetaPhase,
				patternOpt_.magnitudeInDb,
				patternOpt_.phaseInDegrees);

			p.Gphi = magPhaseToComplex(phiGain,
				phiPhase,
				patternOpt_.magnitudeInDb,
				patternOpt_.phaseInDegrees);
		}
		else
		{
			const double thetaRe = nums.size() > 2 ? nums[2] : 1.0;
			const double thetaIm = nums.size() > 3 ? nums[3] : 0.0;
			const double phiRe = nums.size() > 4 ? nums[4] : thetaRe;
			const double phiIm = nums.size() > 5 ? nums[5] : thetaIm;

			p.Gtheta = std::complex<double>(thetaRe, thetaIm);
			p.Gphi = std::complex<double>(phiRe, phiIm);
		}

		const double totalAmplitude =
			std::sqrt(std::norm(p.Gtheta) + std::norm(p.Gphi));

		patternPeakAmplitude_ =
			std::max(patternPeakAmplitude_, totalAmplitude);

		patternTable_.push_back(p);
	}

	patternLoaded_ = !patternTable_.empty();

	return patternLoaded_;
}

bool COMM_AntennaPolarizationTx::parseParameterLine(const std::string& line)
{
	std::string low = lowerString(line);

	const size_t commentPosition = low.find("//");

	if (commentPosition != std::string::npos)
	{
		low = low.substr(0, commentPosition);
	}

	low = trim(low);

	if (low.empty())
	{
		return false;
	}

	if (low.find("mag_phase") != std::string::npos)
	{
		patternOpt_.useMagPhase = true;
		return true;
	}

	if (low.find("real_imag") != std::string::npos)
	{
		patternOpt_.useMagPhase = false;
		return true;
	}

	if (low.find("magnitude") != std::string::npos)
	{
		if (low.find("db") != std::string::npos)
		{
			patternOpt_.magnitudeInDb = true;
		}
		else if (low.find("linear") != std::string::npos)
		{
			patternOpt_.magnitudeInDb = false;
		}

		return true;
	}

	if (low.find("direction") != std::string::npos)
	{
		if (low.find("radian") != std::string::npos)
		{
			patternOpt_.directionInDegrees = false;
		}
		else if (low.find("degree") != std::string::npos)
		{
			patternOpt_.directionInDegrees = true;
		}

		return true;
	}

	if (low.find("phase") != std::string::npos)
	{
		if (low.find("radian") != std::string::npos)
		{
			patternOpt_.phaseInDegrees = false;
		}
		else if (low.find("degree") != std::string::npos)
		{
			patternOpt_.phaseInDegrees = true;
		}

		return true;
	}

	std::stringstream ss(low);
	std::string key;
	double value = 0.0;

	ss >> key >> value;

	if (ss.fail())
	{
		return false;
	}

	if (key == "phi_min")
	{
		patternOpt_.phiMin = value;
		return true;
	}

	if (key == "phi_max")
	{
		patternOpt_.phiMax = value;
		return true;
	}

	if (key == "phi_inc")
	{
		patternOpt_.phiInc = value;
		return true;
	}

	if (key == "theta_min")
	{
		patternOpt_.thetaMin = value;
		return true;
	}

	if (key == "theta_max")
	{
		patternOpt_.thetaMax = value;
		return true;
	}

	if (key == "theta_inc")
	{
		patternOpt_.thetaInc = value;
		return true;
	}

	return false;
}

bool COMM_AntennaPolarizationTx::parseNumericLine(const char* line,
	std::vector<double>& nums) const
{
	nums.clear();

	if (line == 0)
	{
		return false;
	}

	std::string source = line;
	const size_t commentPosition = source.find("//");

	if (commentPosition != std::string::npos)
	{
		source = source.substr(0, commentPosition);
	}

	std::string cleaned;

	for (size_t i = 0; i < source.size(); ++i)
	{
		const char c = source[i];

		if ((c >= '0' && c <= '9') ||
			c == '.' ||
			c == '-' ||
			c == '+' ||
			c == 'e' ||
			c == 'E')
		{
			cleaned.push_back(c);
		}
		else
		{
			cleaned.push_back(' ');
		}
	}

	std::stringstream ss(cleaned);
	double value = 0.0;

	while (ss >> value)
	{
		nums.push_back(value);
	}

	return !nums.empty();
}

void COMM_AntennaPolarizationTx::lookupImportedPolarizationGain(
	double thetaDeg,
	double phiDeg,
	std::complex<double>& Gtheta,
	std::complex<double>& Gphi) const
{
	if (!patternLoaded_ || patternTable_.empty())
	{
		Gtheta = std::complex<double>(0.0, 0.0);
		Gphi = std::complex<double>(0.0, 0.0);
		return;
	}

	int bestIndex = 0;
	double bestScore = 1.0e300;

	for (size_t i = 0; i < patternTable_.size(); ++i)
	{
		const PatternPoint& point = patternTable_[i];
		const double thetaDifference = thetaDeg - point.thetaDeg;
		const double phiDifference = angleDiffDeg(phiDeg, point.phiDeg);

		double score = 0.0;

		if (ImportedPatternDimension == UserDefine2D)
		{
			score = phiDifference * phiDifference;
		}
		else
		{
			score =
				thetaDifference * thetaDifference +
				phiDifference * phiDifference;
		}

		if (score < bestScore)
		{
			bestScore = score;
			bestIndex = static_cast<int>(i);
		}
	}

	const PatternPoint& best = patternTable_[static_cast<size_t>(bestIndex)];

	Gtheta = best.Gtheta;
	Gphi = best.Gphi;
}

// ============================================================================
// 工具函数
// ============================================================================

std::string COMM_AntennaPolarizationTx::trim(const std::string& s)
{
	size_t begin = 0;

	while (begin < s.size() &&
		std::isspace(static_cast<unsigned char>(s[begin])))
	{
		++begin;
	}

	size_t end = s.size();

	while (end > begin &&
		std::isspace(static_cast<unsigned char>(s[end - 1])))
	{
		--end;
	}

	return s.substr(begin, end - begin);
}

std::string COMM_AntennaPolarizationTx::lowerString(const std::string& s)
{
	std::string out = s;

	for (size_t i = 0; i < out.size(); ++i)
	{
		out[i] = static_cast<char>(
			std::tolower(static_cast<unsigned char>(out[i])));
	}

	return out;
}

double COMM_AntennaPolarizationTx::deg2rad(double x)
{
	return x * M_PI / 180.0;
}

double COMM_AntennaPolarizationTx::rad2deg(double x)
{
	return x * 180.0 / M_PI;
}

double COMM_AntennaPolarizationTx::normalizeRad(double x)
{
	while (x > M_PI)
	{
		x -= 2.0 * M_PI;
	}

	while (x < -M_PI)
	{
		x += 2.0 * M_PI;
	}

	return x;
}

double COMM_AntennaPolarizationTx::wrap360(double x)
{
	double y = std::fmod(x, 360.0);

	if (y < 0.0)
	{
		y += 360.0;
	}

	return y;
}

double COMM_AntennaPolarizationTx::clamp(double x,
	double lo,
	double hi)
{
	if (x < lo)
	{
		return lo;
	}

	if (x > hi)
	{
		return hi;
	}

	return x;
}

double COMM_AntennaPolarizationTx::angleDiffDeg(double a,
	double b)
{
	double difference = a - b;

	while (difference > 180.0)
	{
		difference -= 360.0;
	}

	while (difference < -180.0)
	{
		difference += 360.0;
	}

	return difference;
}

double COMM_AntennaPolarizationTx::linearAmplitudeFromDb(double gainDb)
{
	return std::pow(10.0, gainDb / 20.0);
}

std::complex<double> COMM_AntennaPolarizationTx::magPhaseToComplex(
	double magnitude,
	double phase,
	bool magnitudeInDb,
	bool phaseInDegrees)
{
	const double amplitude =
		magnitudeInDb
		? linearAmplitudeFromDb(magnitude)
		: magnitude;

	const double phaseRad =
		phaseInDegrees
		? deg2rad(phase)
		: phase;

	return std::complex<double>(
		amplitude * std::cos(phaseRad),
		amplitude * std::sin(phaseRad));
}

void COMM_AntennaPolarizationTx::normalizeJones(
	std::complex<double>& jonesV,
	std::complex<double>& jonesH)
{
	const double power = std::norm(jonesV) + std::norm(jonesH);

	if (power <= 0.0)
	{
		jonesV = std::complex<double>(1.0, 0.0);
		jonesH = std::complex<double>(0.0, 0.0);
		return;
	}

	const double scale = 1.0 / std::sqrt(power);

	jonesV *= scale;
	jonesH *= scale;
}
