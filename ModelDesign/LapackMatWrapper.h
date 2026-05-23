#ifndef LAPACKMATWRAPPER_H
#define LAPACKMATWRAPPER_H

#include <vector>
#include <complex>
#include <string>
#include <memory>

// 前向声明，避免直接依赖 LapackMat.h
struct LapackMatImpl;

class LapackMatWrapper
{
public:
    LapackMatWrapper();
    LapackMatWrapper(int rows, int cols);
    LapackMatWrapper(const LapackMatWrapper& other);
    LapackMatWrapper(LapackMatWrapper&& other) noexcept;
    ~LapackMatWrapper();

    LapackMatWrapper& operator=(const LapackMatWrapper& other);
    LapackMatWrapper& operator=(LapackMatWrapper&& other) noexcept;

    // 初始化矩阵
    void init(int rows, int cols);
    void initZero(int rows, int cols);
    void initIdentity(int rows, int cols);

    // 获取矩阵维度
    int rows() const;
    int cols() const;

    // 设置和获取元素（使用 std::complex<double>）
    void set(int row, int col, const std::complex<double>& value);
    void setReal(int row, int col, double real);
    void setImag(int row, int col, double imag);
    std::complex<double> get(int row, int col) const;
    double getReal(int row, int col) const;
    double getImag(int row, int col) const;

    // 矩阵运算
    void add(const LapackMatWrapper& other);
    void subtract(const LapackMatWrapper& other);
    void multiply(const LapackMatWrapper& other, LapackMatWrapper& result) const;
    void multiplyScalar(const std::complex<double>& scalar);
    void transpose(LapackMatWrapper& result) const;
    void conjugateTranspose(LapackMatWrapper& result) const;
    void invert();
    void solveLinearSystem(const LapackMatWrapper& b, LapackMatWrapper& x) const;

    // 静态方法：线性方程组求解
    static bool solveLinearSystem(const LapackMatWrapper& A, const LapackMatWrapper& b, LapackMatWrapper& x);

    // 打印矩阵
    void print(const std::string& label = "") const;
    void printReal(const std::string& label = "") const;

    // 获取底层数据指针（谨慎使用）
    const std::complex<double>* data() const;
    std::complex<double>* data();

private:
    std::unique_ptr<LapackMatImpl> pImpl;
};

#endif // LAPACKMATWRAPPER_H
