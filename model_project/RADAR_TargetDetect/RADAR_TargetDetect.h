#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_TargetDetect : public SystemVueModelBuilder::DFModel
{
public:
	enum SelectedDetectType{ DetectRange, Detect2D };


	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_TargetDetect );

	// Constructor to initialize parameters
	RADAR_TargetDetect();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > input;
	SystemVueModelBuilder::CircularBuffer< bool > IsDetect;
	SystemVueModelBuilder::CircularBuffer< std::complex<double> > output;
	SystemVueModelBuilder::CircularBuffer< int > RangeBinIndex;
	SystemVueModelBuilder::CircularBuffer< int > FreqBinIndex;
	
	// Parameter
	double PRI_Or_WaveGate;				// 可由此计算出CellSize
	//int SampleNumForEstimateNoise;	// 理解为CFAR窗长
	SelectedDetectType DetectType;		// 距离维检测/距离维度+速度维检测
	double FalseAlarmProbability;
	int ReferenceCell;
	int GuardCell;
	//double Coef1;						// 理解为保护窗比例
	//double Coef2;						// 理解为参考窗比例
	//double Coef;
	int FreqChannelNum;					// 相参积累数量
	double SampleRate;

private:
	int CellSize;
	int PRINum;
	double Threshold;
	bool DetectStatus;
};
