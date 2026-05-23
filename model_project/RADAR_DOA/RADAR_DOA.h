#pragma once
#include "ModelBuilder.h"
#include "Matrix.h"
#include "MatrixCircularBuffer.h"
#include "EnvelopeSignal.h"

// 添加Eigen库支持 - 用于矩阵运算
#include "Eigen/Dense"
#include <vector>
#include <complex>
#include <cmath>

// RADAR_DOA类定义 - 波达方向估计算法模型
class SYSTEMVUEMODELBUILDER_API RADAR_DOA : public SystemVueModelBuilder::DFModel
{
public:
	// 枚举类型 - 定义DOA算法类型
	enum SelectedMTI_Type {
		MUSIC,      // MUSIC算法
		ESPRIT,     // ESPRIT算法
		MUSIC_2D    // 二维MUSIC算法
	};

public:
	// 这个宏是所有从DFModel派生的类必须的
	DECLARE_MODEL_INTERFACE(RADAR_DOA);

	// 构造函数
	RADAR_DOA();

	//-------- 函数重载 --------
	virtual bool	Run();		// 主运行函数
	virtual bool    Setup();	// 初始化函数

	// 端口定义
	SystemVueModelBuilder::DComplexCircularBufferBus input;	// 输入端口：接收复数数据
	SystemVueModelBuilder::IntCircularBuffer number;			// 输出端口：信号源数量
	SystemVueModelBuilder::DoubleMatrixCircularBuffer elevation;	// 输出端口：俯仰角矩阵
	SystemVueModelBuilder::DoubleMatrixCircularBuffer azimuth;		// 输出端口：方位角矩阵

	// 参数定义
	double Fc;		// 中心频率
	double D;		// 阵元间距
	int NumOfCh;		// 通道数/阵元数
	int SnapShotLen;	// 快拍长度
	SelectedMTI_Type MTI_Type;		// 算法类型

    // DOA结果结构体 - 存储DOA估计结果
    struct DOAResult {
        int number;			// 检测到的信号源数量
        std::vector<double> azimuth;		// 方位角数组
        std::vector<double> elevation;		// 俯仰角数组
    };

    // DOA算法实现函数
    DOAResult DOA_MUSIC_1D(const Eigen::MatrixXcd& X, int M, double d, double lambda, int L);
    //DOAResult DOA_ESPRIT_1D(const Eigen::MatrixXcd& X, int M, double d, double lambda, int L);
    //DOAResult DOA_MUSIC_2D(const Eigen::MatrixXcd& X, int M, int N, double d_h, double d_v, double lambda, int L);
    double lambda_;		// 波长（由Fc计算得出）
private:
	// DOA估计相关的私有成员

	// 辅助函数
	Eigen::VectorXcd steering_vector_1D(double theta, int M, double d, double lambda);	// 生成一维导向矢量
	//Eigen::VectorXcd steering_vector_2D(double az, double el, int M, int N, double d_h, double d_v, double lambda);
	int estimate_num_sources(const Eigen::VectorXd& eigenvalues, int M, int L);		// 估计信号源数量
	std::pair<std::vector<double>, std::vector<double>> find_2d_peaks(
		const Eigen::MatrixXd& P_2d,
		const std::vector<double>& az_scan,
		const std::vector<double>& el_scan,
		int num_peaks);

	// 工具函数
	std::vector<int> findpeaks(const Eigen::VectorXd& signal, int n_peaks, int min_distance);	// 查找一维峰值
	std::vector<double> arange(double start, double end, double step);				// 生成等差数组
};
