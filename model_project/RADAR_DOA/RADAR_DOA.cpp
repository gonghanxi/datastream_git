#include "RADAR_DOA.h"
#include <iostream>
#include <algorithm>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const double SPEED_OF_LIGHT = 3e8;  // 光速 m/s

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_DOA)
{
	SET_MODEL_DESCRIPTION("RADAR direction of arrival (DOA) estimation");
	SET_MODEL_SYMBOL("SYM_RADAR_DOA@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Processing");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(number);
	SystemVueModelBuilder::DFPort F1 = ADD_MODEL_OUTPUT(elevation);
	F1.SetOptional(true);
	ADD_MODEL_OUTPUT(azimuth);

	SystemVueModelBuilder::DFParam P1 = ADD_MODEL_PARAMETER(Fc);
	P1.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
	P1.SetDefaultValue("10e9");

	SystemVueModelBuilder::DFParam P5 = ADD_MODEL_PARAMETER(NumOfCh);
	P5.SetDefaultValue("16");

	SystemVueModelBuilder::DFParam P4 = ADD_MODEL_ENUM_PARAMETER(MTI_Type, SelectedMTI_Type);
	P4.AddEnumeration("MUSIC", MUSIC);
	P4.AddEnumeration("ESPRIT", ESPRIT);
	P4.AddEnumeration("MUSIC_2D", MUSIC_2D);
	P4.SetDefaultValue(0);

	SystemVueModelBuilder::DFParam P2 = ADD_MODEL_PARAMETER(D);
	P2.SetDefaultValue("0.5");

	SystemVueModelBuilder::DFParam P3 = ADD_MODEL_PARAMETER(SnapShotLen);
	P3.SetDefaultValue("100");

	return true;
}
#endif

RADAR_DOA::RADAR_DOA()
{
	lambda_ = 0.0;  // 初始化波长为0
}

bool RADAR_DOA::Setup()
{
	// 计算波长
	lambda_ = SPEED_OF_LIGHT / Fc;  // 根据中心频率计算波长 λ = c/f
	for (int i = 0; i < NumOfCh; i++) {
		input[i].SetRate(SnapShotLen);  // 设置每个输入通道的采样率
	}
	return true;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_DOA::Run()
{
	// 1. 从输入端口读取数据并转换为Eigen矩阵
	int M = NumOfCh;  // 天线阵元数量
	int L = SnapShotLen;  // 快拍数

	Eigen::MatrixXcd X(M, L);  // 创建M×L复数矩阵存储输入数据

	// 读取输入数据
	for (int i = 0; i < M; i++) {
		for (int j = 0; j < L; j++) {
			// 从CircularBuffer读取复数数据
			std::complex<double> sample = input[i][j];
			X(i, j) = sample;  // 将数据存入矩阵
		}
	}

	// 2. 执行DOA估计
	DOAResult result;  // 存储DOA估计结果的结构体

	switch (MTI_Type) {
	case MUSIC:
		result = DOA_MUSIC_1D(X, M, D, lambda_, L);  // 调用MUSIC算法
		break;

	case ESPRIT:
		//	result = DOA_ESPRIT_1D(X, M, D, lambda_, L);
		break;

	case MUSIC_2D:
		// 假设是方阵
	//	int N = (int)sqrt((double)M);
	//	result = DOA_MUSIC_2D(X, N, N, D, D, lambda_, L);
		break;
	}

	// 3. 输出结果到端口
	number[0] = result.number;  // 输出信号源数量

	// 输出方位角
	if (!result.azimuth.empty()) {
		SystemVueModelBuilder::DoubleMatrix azMatrix(1, result.azimuth.size());  // 创建1行×N列矩阵
		for (size_t i = 0; i < result.azimuth.size(); i++) {
			azMatrix(0, i) = result.azimuth[i];  // 填充方位角数据
		}
		azimuth[0] = azMatrix;  // 输出到方位角端口
	}

	// 输出俯仰角（如果有）
	if (!result.elevation.empty()) {
		SystemVueModelBuilder::DoubleMatrix elMatrix(1, result.elevation.size());
		for (size_t i = 0; i < result.elevation.size(); i++) {
			elMatrix(0, i) = result.elevation[i];
		}
		elevation[0] = elMatrix;  // 输出到俯仰角端口
	}

	return true;  // 运行成功
}

//-----------------------------------------------------------------------------------
// DOA算法实现
//-----------------------------------------------------------------------------------

RADAR_DOA::DOAResult RADAR_DOA::DOA_MUSIC_1D(const Eigen::MatrixXcd& X, int M, double d,
	double lambda, int L) {
	DOAResult result;  // 创建结果结构体

	// 计算协方差矩阵
	Eigen::MatrixXcd Rxx = (X * X.adjoint()) / (double)L;  // Rxx = (X*X^H)/L

	// 特征值分解
	Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> es(Rxx);  // 对协方差矩阵进行特征值分解
	Eigen::VectorXd eigenvalues = es.eigenvalues().real();  // 获取实部特征值
	Eigen::MatrixXcd V = es.eigenvectors();  // 获取特征向量

	// 按降序排列特征值
	std::vector<std::pair<double, int>> eigen_pairs;
	for (int i = 0; i < eigenvalues.size(); i++) {
		eigen_pairs.push_back(std::make_pair(eigenvalues(i), i));  // 存储(特征值, 索引)对
	}
	std::sort(eigen_pairs.begin(), eigen_pairs.end(),
		[](const std::pair<double, int>& a, const std::pair<double, int>& b) {
			return a.first > b.first;  // 按特征值降序排列
		});

	Eigen::VectorXd sorted_eigenvalues(M);
	Eigen::MatrixXcd sorted_V(M, M);
	for (int i = 0; i < M; i++) {
		sorted_eigenvalues(i) = eigen_pairs[i].first;  // 存储排序后的特征值
		sorted_V.col(i) = V.col(eigen_pairs[i].second);  // 存储排序后的特征向量
	}

	// 估计信号源数量
	int number = estimate_num_sources(sorted_eigenvalues, M, L);  // 估计信号源数量

	// 噪声子空间
	Eigen::MatrixXcd Un = sorted_V.rightCols(M - number);  // 取后(M-number)个特征向量作为噪声子空间
	Eigen::MatrixXcd UnUn_H = Un * Un.adjoint();  // 计算噪声子空间投影矩阵

	// MUSIC谱计算
	std::vector<double> theta_scan = arange(-90.0, 90.5, 0.5);  // 生成扫描角度 -90°到90°，步长0.5°
	Eigen::VectorXd P_music(theta_scan.size());  // 存储MUSIC谱

	for (size_t i = 0; i < theta_scan.size(); i++) {
		Eigen::VectorXcd a = steering_vector_1D(theta_scan[i], M, d, lambda);  // 计算导向矢量
		std::complex<double> denominator = (a.adjoint() * UnUn_H * a)(0, 0);  // 计算分母 a^H * Un * Un^H * a
		P_music(i) = 1.0 / std::abs(denominator);  // MUSIC谱：P(θ) = 1/|a^H * Un * Un^H * a|
	}

	// 转换为dB
	Eigen::VectorXd P_music_db(P_music.size());
	for (int i = 0; i < P_music.size(); i++) {
		P_music_db(i) = 10.0 * log10(std::abs(P_music(i)));  // 转换为分贝单位
	}

	// 查找峰值
	std::vector<int> locs = findpeaks(P_music_db, number, 3);  // 查找number个峰值，最小间隔3个点

	// 如果没有找到足够的峰值，选择最小距离约束下的最高值
	if ((int)locs.size() < number) {
		std::vector<std::pair<double, int>> peak_pairs;
		for (int i = 0; i < P_music_db.size(); i++) {
			peak_pairs.push_back(std::make_pair(P_music_db(i), i));  // 存储(谱值, 索引)对
		}
		std::sort(peak_pairs.begin(), peak_pairs.end(),
			[](const std::pair<double, int>& a, const std::pair<double, int>& b) {
				return a.first > b.first;  // 按谱值降序排列
			});

		std::vector<int> selected_locs;
		selected_locs.push_back(peak_pairs[0].second);  // 添加最高峰值

		for (size_t i = 1; i < peak_pairs.size() && (int)selected_locs.size() < number; i++) {
			int current_loc = peak_pairs[i].second;
			bool valid = true;

			for (int sel_loc : selected_locs) {
				if (std::abs(theta_scan[current_loc] - theta_scan[sel_loc]) <= 1.5) {  // 检查角度间隔是否大于1.5°
					valid = false;  // 间隔太小，排除
					break;
				}
			}

			if (valid) {
				selected_locs.push_back(current_loc);  // 满足间隔要求，添加到选中列表
			}
		}

		locs = selected_locs;  // 更新峰值位置
		number = locs.size();  // 更新信号源数量
	}

	// 提取方位角并排序
	result.number = number;  // 设置信号源数量
	for (int i = 0; i < number && i < (int)locs.size(); i++) {
		result.azimuth.push_back(theta_scan[locs[i]]);  // 从扫描角度中提取方位角
	}
	std::sort(result.azimuth.begin(), result.azimuth.end());  // 升序排列方位角

	return result;  // 返回DOA估计结果
}



//-----------------------------------------------------------------------------------
// 辅助函数实现
//-----------------------------------------------------------------------------------

Eigen::VectorXcd RADAR_DOA::steering_vector_1D(double theta, int M, double d, double lambda) {
	double k = 2.0 * M_PI / lambda;  // 波数 k = 2π/λ
	Eigen::VectorXcd a(M);

	for (int i = 0; i < M; i++) {
		double phase = k * d * lambda * i * sin(theta * M_PI / 180.0);  // 计算第i个阵元的相位
		a(i) = std::complex<double>(cos(phase), sin(phase));  // 导向矢量元素：exp(j*phase)
	}

	return a;  // 返回导向矢量
}



int RADAR_DOA::estimate_num_sources(const Eigen::VectorXd& eigenvalues, int M, int L) {
	int K_max = std::min(M - 1, M / 2);  // 最大可能信号源数量

	// 方法1: 特征值间隙法（最可靠）
	int K_gap = 1;
	if (M > 1) {
		Eigen::VectorXd eigen_ratios(M - 1);
		for (int i = 0; i < M - 1; i++) {
			eigen_ratios(i) = eigenvalues(i) / (eigenvalues(i + 1) + 1e-10);  // 计算相邻特征值比值
		}

		int max_idx = 0;
		double max_ratio = eigen_ratios(0);
		for (int i = 1; i < eigen_ratios.size(); i++) {
			if (eigen_ratios(i) > max_ratio) {
				max_ratio = eigen_ratios(i);
				max_idx = i;
			}
		}

		if (max_ratio > 10.0) {
			K_gap = max_idx + 1;  // 如果最大比值>10，信号源数为最大比值位置+1
		}
		else {
			for (int i = M - 2; i >= 0; i--) {
				if (eigen_ratios(i) > 3.0) {  // 从后向前找第一个比值>3的位置
					K_gap = i + 1;
					break;
				}
			}
		}
	}

	// 方法2: 噪声水平法（GDE）
	int start_idx = (int)ceil(M * 0.7);  // 假设后30%的特征值代表噪声
	double noise_sum = 0.0;
	int noise_count = M - start_idx;
	for (int i = start_idx; i < M; i++) {
		noise_sum += eigenvalues(i);  // 计算噪声特征值之和
	}
	double noise_level = noise_sum / noise_count;  // 计算噪声水平

	int K_gde = 0;
	for (int i = 0; i < M; i++) {
		if (eigenvalues(i) > 5.0 * noise_level) {  // 特征值>5倍噪声水平，认为是信号
			K_gde++;
		}
	}

	// 方法3: MDL准则
	Eigen::VectorXd mdl(K_max + 1);
	for (int k = 0; k <= K_max; k++) {
		if (k < M && M - k > 0) {
			double lambda_noise = 0.0;
			for (int i = k; i < M; i++) {
				lambda_noise += eigenvalues(i);
			}
			lambda_noise /= (M - k);  // 噪声特征值算术平均

			double lambda_geo_log = 0.0;
			for (int i = k; i < M; i++) {
				lambda_geo_log += log(eigenvalues(i) + 1e-10);
			}
			lambda_geo_log /= (M - k);
			double lambda_geo = exp(lambda_geo_log);  // 噪声特征值几何平均

			mdl(k) = -(double)L * (M - k) * log(lambda_noise / (lambda_geo + 1e-10) + 1e-10) +
				0.5 * k * (2 * M - k) * log((double)L);  // MDL准则公式
		}
		else {
			mdl(k) = 1e10;  // 无效值
		}
	}

	int K_mdl = 0;
	double min_mdl = mdl(0);
	for (int k = 1; k <= K_max; k++) {
		if (mdl(k) < min_mdl) {
			min_mdl = mdl(k);
			K_mdl = k;  // 找到使MDL最小的k
		}
	}

	// 方法4: 简单阈值
	double min_eigen = eigenvalues.minCoeff();  // 最小特征值
	int K_simple = 0;
	for (int i = 0; i < M; i++) {
		if (eigenvalues(i) > 100.0 * min_eigen) {  // 特征值>100倍最小特征值
			K_simple++;
		}
	}

	// 综合多种方法
	int K;
	if (K_gap >= 1 && K_gap <= K_max) {
		K = K_gap;  // 优先使用特征值间隙法
	}
	else {
		std::vector<int> K_estimates = { K_gde, K_mdl, K_simple };
		std::vector<int> valid_estimates;
		for (int est : K_estimates) {
			if (est >= 1 && est <= K_max) {
				valid_estimates.push_back(est);
			}
		}

		if (!valid_estimates.empty()) {
			std::sort(valid_estimates.begin(), valid_estimates.end());
			K = valid_estimates[valid_estimates.size() / 2];  // 取中位数
		}
		else {
			K = 1;  // 默认值
		}
	}

	K = std::max(1, std::min(K, M / 2));  // 确保K在[1, M/2]范围内

	return K;  // 返回估计的信号源数量
}


std::vector<int> RADAR_DOA::findpeaks(const Eigen::VectorXd& signal, int n_peaks, int min_distance) {
	std::vector<std::pair<double, int>> peaks;  // 存储(峰值高度, 位置索引)对

	for (int i = 1; i < signal.size() - 1; i++) {
		if (signal(i) > signal(i - 1) && signal(i) > signal(i + 1)) {  // 检测局部极大值
			peaks.push_back(std::make_pair(signal(i), i));
		}
	}

	// 按峰值高度降序排序
	std::sort(peaks.begin(), peaks.end(),
		[](const std::pair<double, int>& a, const std::pair<double, int>& b) {
			return a.first > b.first;  // 降序排列
		});

	std::vector<int> selected_locs;  // 选中的峰值位置
	for (const auto& peak : peaks) {
		bool valid = true;
		for (int loc : selected_locs) {
			if (std::abs(peak.second - loc) < min_distance) {  // 检查与已有峰值的最小距离
				valid = false;  // 距离太小，排除
				break;
			}
		}

		if (valid) {
			selected_locs.push_back(peak.second);  // 添加满足距离约束的峰值
			if ((int)selected_locs.size() >= n_peaks) {  // 达到所需峰值数量
				break;
			}
		}
	}

	return selected_locs;  // 返回峰值位置索引
}

std::vector<double> RADAR_DOA::arange(double start, double end, double step) {
	std::vector<double> result;  // 结果向量

	// 生成从start到end的序列，步长为step
	for (double val = start; val < end; val += step) {  // 注意：不包含end值
		result.push_back(val);
	}

	return result;  // 返回等差数列
}