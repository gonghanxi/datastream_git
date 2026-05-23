#include "LapackMatWrapper.h"
#include "openBlas/LapackMat.h"
#include <iostream>
#include <cstring>
#include <stdexcept>

// 内部实现类，封装 LapackMat
struct LapackMatImpl
{
    LapackMat mat;

    LapackMatImpl() = default;
    LapackMatImpl(int rows, int cols) : mat(rows, cols) {}
    LapackMatImpl(const LapackMatImpl& other) : mat(other.mat) {}
};

// 转换辅助函数
static lapack_complex_double toLapackComplex(const std::complex<double>& c)
{
    return {c.real(), c.imag()};
}

static std::complex<double> fromLapackComplex(const lapack_complex_double& c)
{
    return {c.real, c.imag};
}

// LapackMatWrapper 实现
LapackMatWrapper::LapackMatWrapper() : pImpl(std::make_unique<LapackMatImpl>()) {}

LapackMatWrapper::LapackMatWrapper(int rows, int cols) : pImpl(std::make_unique<LapackMatImpl>(rows, cols)) {}

LapackMatWrapper::LapackMatWrapper(const LapackMatWrapper& other) : pImpl(std::make_unique<LapackMatImpl>(*other.pImpl)) {}

LapackMatWrapper::LapackMatWrapper(LapackMatWrapper&& other) noexcept : pImpl(std::move(other.pImpl)) {}

LapackMatWrapper::~LapackMatWrapper() = default;

LapackMatWrapper& LapackMatWrapper::operator=(const LapackMatWrapper& other)
{
    if (this != &other) {
        pImpl = std::make_unique<LapackMatImpl>(*other.pImpl);
    }
    return *this;
}

LapackMatWrapper& LapackMatWrapper::operator=(LapackMatWrapper&& other) noexcept
{
    if (this != &other) {
        pImpl = std::move(other.pImpl);
    }
    return *this;
}

void LapackMatWrapper::init(int rows, int cols)
{
    pImpl->mat.init(rows, cols);
}

void LapackMatWrapper::initZero(int rows, int cols)
{
    pImpl->mat.init(rows, cols);
    pImpl->mat.resetZero();
}

void LapackMatWrapper::initIdentity(int rows, int cols)
{
    pImpl->mat.initI(rows, cols);
}

int LapackMatWrapper::rows() const
{
    return pImpl->mat.getNr();
}

int LapackMatWrapper::cols() const
{
    return pImpl->mat.getNc();
}

void LapackMatWrapper::set(int row, int col, const std::complex<double>& value)
{
    pImpl->mat.set(row, col, toLapackComplex(value));
}

void LapackMatWrapper::setReal(int row, int col, double real)
{
    auto val = get(row, col);
    val.real(real);
    set(row, col, val);
}

void LapackMatWrapper::setImag(int row, int col, double imag)
{
    auto val = get(row, col);
    val.imag(imag);
    set(row, col, val);
}

std::complex<double> LapackMatWrapper::get(int row, int col) const
{
    return fromLapackComplex(pImpl->mat.get(row, col));
}

double LapackMatWrapper::getReal(int row, int col) const
{
    return pImpl->mat.get(row, col).real;
}

double LapackMatWrapper::getImag(int row, int col) const
{
    return pImpl->mat.get(row, col).imag;
}

void LapackMatWrapper::add(const LapackMatWrapper& other)
{
    pImpl->mat.add(other.pImpl->mat);
}

void LapackMatWrapper::subtract(const LapackMatWrapper& other)
{
    pImpl->mat.sub(other.pImpl->mat);
}

void LapackMatWrapper::multiply(const LapackMatWrapper& other, LapackMatWrapper& result) const
{
    pImpl->mat.multLeft(other.pImpl->mat, result.pImpl->mat);
}

void LapackMatWrapper::multiplyScalar(const std::complex<double>& scalar)
{
    pImpl->mat.multNum(toLapackComplex(scalar));
}

void LapackMatWrapper::transpose(LapackMatWrapper& result) const
{
    pImpl->mat.trans(result.pImpl->mat);
}

void LapackMatWrapper::conjugateTranspose(LapackMatWrapper& result) const
{
    pImpl->mat.transConj(result.pImpl->mat);
}

void LapackMatWrapper::invert()
{
    pImpl->mat.inv();
}

void LapackMatWrapper::solveLinearSystem(const LapackMatWrapper& b, LapackMatWrapper& x) const
{
    if (!LapackMat::linearEquationSolverCpy(pImpl->mat, b.pImpl->mat, x.pImpl->mat)) {
        throw std::runtime_error("Failed to solve linear system");
    }
}

bool LapackMatWrapper::solveLinearSystem(const LapackMatWrapper& A, const LapackMatWrapper& b, LapackMatWrapper& x)
{
    return LapackMat::linearEquationSolverCpy(A.pImpl->mat, b.pImpl->mat, x.pImpl->mat);
}

void LapackMatWrapper::print(const std::string& label) const
{
    if (!label.empty()) {
        std::cout << label << ":" << std::endl;
    }
    for (int i = 0; i < rows(); ++i) {
        for (int j = 0; j < cols(); ++j) {
            auto val = get(i, j);
            std::cout << "(" << val.real() << ", " << val.imag() << ") ";
        }
        std::cout << std::endl;
    }
    std::cout << "------------------------" << std::endl;
}

void LapackMatWrapper::printReal(const std::string& label) const
{
    if (!label.empty()) {
        std::cout << label << ":" << std::endl;
    }
    for (int i = 0; i < rows(); ++i) {
        for (int j = 0; j < cols(); ++j) {
            std::cout << getReal(i, j) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "------------------------" << std::endl;
}

const std::complex<double>* LapackMatWrapper::data() const
{
    return reinterpret_cast<const std::complex<double>*>(pImpl->mat.getDataConst());
}

std::complex<double>* LapackMatWrapper::data()
{
    return reinterpret_cast<std::complex<double>*>(pImpl->mat.getData());
}
