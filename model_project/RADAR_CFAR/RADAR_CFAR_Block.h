#ifndef RADAR_CFAR_BLOCK_H
#define RADAR_CFAR_BLOCK_H

#include "Block.h"
#include "RADAR_CFAR.h"
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_CFAR_Block : public SystemVueModelBuilder::Block
{
public:
	RADAR_CFAR_Block(const std::string& name);
	~RADAR_CFAR_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters();
	bool ValidateParameters();
	void UpdateThresholdFactor();

	RADAR_CFAR::SelectedCFARType ConvertStringToCFARType(const std::string& value);
	RADAR_CFAR::SelectedDetectorType ConvertStringToDetectorType(const std::string& value);

	std::unique_ptr<RADAR_CFAR> m_radarCfar;

	RADAR_CFAR::SelectedCFARType m_cfarType;
	int m_cellSize;
	int m_referenceCell;
	int m_guardCell;
	int m_kOrder;
	double m_thresholdScaleFactor;
	RADAR_CFAR::SelectedDetectorType m_detectorType;
	double m_pf;
	double m_alpha;
	double m_beta;
	double m_thresholdFactor;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    std::queue<double> m_thresholdQueue;
    double m_lastOutput;                 // 上次输出值（用于保持）
    double m_lastThreshold;
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(RADAR_CFAR_Block);

#endif // RADAR_CFAR_BLOCK_H
