TEMPLATE = subdirs
CONFIG += c++17

win32 {
    DEFINES += WINDOWS_PLATFORM
    DESTDIR = $$PWD/../bin/models
    QMAKE_CXXFLAGS += /utf-8
    QMAKE_CFLAGS += /utf-8
    # MSVC编译选项（使用MSVC风格的警告控制）
    QMAKE_CXXFLAGS += /wd4996  # 禁用deprecated declarations警告
    QMAKE_CXXFLAGS += /wd4100  # 禁用unused parameter警告
    QMAKE_CXXFLAGS += /wd4514  # 禁用unreferenced inline function警告
    QMAKE_CXXFLAGS += /wd4828      # 禁用无效字符警告
    QMAKE_CXXFLAGS += /wd4819
}

linux {
DEFINES += QT_NO_DEBUG_OUTPUT
    DEFINES += LINUX_PLATFORM
    DESTDIR = $$PWD/../bin/models
    CONFIG += unversioned_libname unversioned_soname
    QMAKE_CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
    # 强制使用 C++17，覆盖所有默认设置
    QMAKE_CXXFLAGS = -std=c++17 -Wno-unused-variable -fPIC
    QMAKE_CFLAGS = -std=c11
    CONFIG += c++17
    CONFIG += c++1z
}

# OpenBLAS配置
win32 {
    LIBS += -L"$$PWD/../ModelDesign/openBlas/lib/x64_release/openblas/bin"
    LIBS += -L"$$PWD/../ModelDesign/openBlas/lib/x64_release/openblas/lib"
    LIBS += -llibopenblas
    INCLUDEPATH += $$PWD/../ModelDesign/openBlas/lib/x64_release/openblas/include
}

unix {
    LIBS += -lopenblas
}

#通过ModelDesignHelper.pro include ModelDesign.pri 专为代码分析创建的辅助项目文件，不参与实际构建
SUBDIRS += \
#$$PWD/../ModelDesign/ModelDesignHelper.pro \
    Abs_M \
    AdaptLinQuant \
Add \
AddCx \
AddEnv \
    AddEnv_M \
    AddGuard \
AddInt \
    AddNDensity \
    AddNoise \
Amplifier \
AsyncCommutator \
AsyncCommutatorCx \
AsyncCommutatorEnv \
AsyncCommutatorInt \
    AsyncDistributor \
    AsyncDistributorCx\
    AsyncDistributorEnv\
    AsyncDistributorInt \
    AtoD \
    AtoD_M \
    AutoCorr \
    AverageCxWOffset \
    Average\
    AverageCx \
    AvgSqrErr_M \
    BCH_Decoder \
    BCH_Encoder \
    BER \
Biquad \
BiquadCascade \
Bits \
    BitDeformatter\
BitFormatter \
BitShiftRegister \
BitsToInt \
BlockAllPole \
BPF_Butterworth \
BPF_ChebyshevI \
BPF_ChebyshevII \
BSF_Butterworth \
BSF_ChebyshevI \
BSF_ChebyshevII \
    CRC_Coder \
    CRC_Decoder \
ChirpGen \
Chop \
ChopCx \
ChopInt \
    ChopVarOffset \
    CoderRS \
    Combiner_M \
Commutator \
CommutatorCx \
CommutatorEnv \
CommutatorInt \
ComplexExpGen \
    Compress\
    Conjugate_M \
Const \
ConstCx \
    ConvolutionalCoder \
CxToEnv \
ConstInt\
    Convolve\
    ConvolveCx \
    CrossCorr\
    CxToEnv_M \
CxToPolar\
    CxToPolar_M \
CxToRect \
    CxToRect_M \
    CyclicShift \
    CyclicShiftCx \
    CyclicShiftInt \
    DB \
    DeScrambler \
    DeadZone \
Delay\
DelayCx\
DelayEnv\
DelayInt\
    Demapper \
Demodulator\
    DeMux \
    DiagonalCx_M \
    Diagonal_M \
    Dirichlet \
    Distributor\
    DistributorCx \
    DistributorEnv \
    DistributorInt \
DownSample \
DownSampleCx\
DownSampleEnv\
    DownSampleVarPhase \
DtoA\
EnvFcChange\
    EnvFcChange_M \
EnvToCx \
    EnvToCx_M \
EnvToData\
    Expand \
    FFT_Shift \
FreqMpyDiv\
FFT_Cx\
Gain \
GainCx\
GainEnv \
GainInt\
GaussianNoiseGen\
    GeometricMean \
    GrayDecoder \
    GrayEncoder \
Hermitian_M\
HPF_Butterworth\
HPF_ChebyshevI\
HPF_ChebyshevII\
    Hysteresis \
IID_Gaussian\
IID_Uniform\
IIR\
Identity_M\
IdentityCx_M \
Impulse\
IntToBits \
    IntToReal \
    Integrator \
    IntegratorCx \
    IntegratorInt \
    InterleaveDeinterleave \
    InterleaveDeinterleaveCx \
    InterleaveDeinterleaveEnv \
    InterleaveDeinterleaveInt \
    InverseCx_M \
Inverse_M \
    Latch \
    Limit \
    Limit_M \
LinearQuantizer\
LPF_Butterworth\
LPF_ChebyshevI\
LPF_ChebyshevII\
    LogAmp \
    LogVDet \
    Logic \
    LookUpTable \
    M_PSK \
    Mapper \
Math\
MathCx \
#MATLAB_Script\
    MaxMin \
Mixer \
    Modulo \
    ModuloInt \
Mpy\
MpyCx\
MpyEnv \
MpyInt\
Modulator \
    Mux \
    MxCom_M \
    MxDecom_M \
    OSF \
    OrderTwoInt \
Oscillator \
    PAM_Demapper \
    PackBus_M \
    PackCx_M \
    Pack_M \
    PattMatch \
    PcwzLinear \
    PeakDetector \
    PhaseComparator \
PhaseShifter \
PolarToCx\
    PolarToCx_M \
    PolarToRect \
Polynomial \
PolynomialCx \
PolynomialInt \
PulseGen\
    Quantizer \
    Quantizer2D \
    Quantizer_M \
    RADAR_ADBF \
    RADAR_AngleTransform \
    RADAR_AntennaPolarizationRx \
    RADAR_AntennaPolarizationTx \
    RADAR_Antenna_Rx \
    RADAR_Antenna_Tx \
    RADAR_Antenna_Tx \
    RADAR_Antenna_Tx2 \
    RADAR_Antenna_Tx2 \
    RADAR_ArrayCouple \
RADAR_BarkerCode\
    RADAR_BinaryDetector \
RADAR_CFAR\
    RADAR_CFAR_M \
    RADAR_CICDecimate \
    RADAR_CICInterp \
    RADAR_Clutter_H \
RADAR_CoIntgr\
RADAR_CW \
    RADAR_CoIntgr_M \
    RADAR_CornerReflectorLocation \
RADAR_Detector\
RADAR_DOA\
    RADAR_Detector_M \
    RADAR_EWChaff \
    RADAR_EWDeceptionJamming \
    RADAR_EWJamming \
    RADAR_EchoGenerator \
RADAR_Equation\
RADAR_FSK\
RADAR_GainCtrl\
    RADAR_Ground_Clutter \
    RADAR_JammerLocation \
    RADAR_JammingEffect \
    RADAR_Kalman \
RADAR_LFM \
    RADAR_LFMRef \
    RADAR_LocInAntennaFrame \
    RADAR_MNDetector \
    RADAR_MTD_M \
    RADAR_MTI_M \
RADAR_MatchedFilter\
RADAR_MTD\
RADAR_MTI\
    RADAR_MultiCH_Rx \
    RADAR_MultiCH_Tx \
RADAR_NLFM\
    RADAR_NonCoIntgr \
    RADAR_NonCoIntgr_M \
RADAR_PULSE \
    RADAR_PdMeasure \
    RADAR_Pd_Measurement \
    RADAR_Pf_Measurement \
    RADAR_PhasedArrayRx \
    RADAR_PhasedArrayTx \
    RADAR_PlotsCentroid \
    RADAR_PropagationLoss \
RADAR_PulseCompression \
    RADAR_PulseCompression_M \
RADAR_RangeMeas\
RADAR_RCS\
    RADAR_Rx_DBS_2D \
    RADAR_SAR_Echo \
    RADAR_Sea_Clutter \
    RADAR_SignalAnalyzer \
    RADAR_SummerBusRF \
RADAR_Switch\
    RADAR_TargetDetect \
    RADAR_TargetTrack \
    RADAR_TargetTrajectory \
    RADAR_Tx_DBS_2D \
RADAR_UnAmbRange\
RADAR_UnAmbVelocity\
RADAR_VelocityMeas\
    RADAR_WaveGate \
Ramp\
RampGen\
RampSweepGen\
    RealToInt \
    Reciprocal \
RectToCx \
    RectToCx_M \
    RectToPolar \
    RepeatCx \
    RepeatEnv \
    RepeatInt \
Reverse \
ReverseCx \
ReverseEnv\
ReverseInt\
    Rotate \
    Repeat\
SDomainIIR \
    SVD_M \
    SampleHold \
    SampleMean_M \
    SchmittTrig \
SetSampleRate\
SetSampleRateCx\
SetSampleRateEnv\
SetSampleRateInt\
    Sinc \
SineGen \
SineSweepGen\
Sink \
    Sink_M \
SinkCx \
    SinkCx_M \
SinkEnv \
    SinkEnv_M \
SinkInt \
    SinkInt_M \
    SlidWinAvg \
    Splitter_M \
SquareGen\
SquareSweepGen\
Sub \
SubCx\
SubEnv \
SubInt\
    SubMxCx_M \
    SubMx_M \
    SwitchSPDT \
    SwitchSPST \
TimeDelay\
TimeDelayCx\
TimeDelayEnv\
TimeDelayInt\
    TimeSynchronizer \
    ToeplitzCx_M \
    Toeplitz_M \
    Trainer \
    Transpose \
    TransposeCx \
    TransposeCx_M \
    TransposeEnv \
    TransposeInt \
    Transpose_M \
    Trig\
    TrigCx \
    UnpackBus_M \
    UnpackCx_M \
    Unpack_M \
    Unwrap \
UpSample\
UpSampleCx\
UpSampleEnv \
    VarDelay \
    VarDelayCx \
    VarDelayEnv \
    VarDelayInt \
  Variance\
Window










