#ifndef LAPACKMAT_H
#define LAPACKMAT_H

#include "../openBlas/lib/x64_release/openblas/include/cblas.h"
#include "../openBlas/lib/x64_release/openblas/include/lapacke.h"
#include "../openBlas/lib/x64_release/openblas/include/lapacke_config.h"

#include <QString>
#include <math.h>
#include "LapackMatBase.h"
class LapackMat : public LapackMatBase<_lapack_complex_double>
{
public:

    LapackMat& operator=(const LapackMat& other);
    LapackMat(const LapackMat &other);

    LapackMat(LapackMat &&other);
    LapackMat& operator=(LapackMat&&other);// 移动赋值函数

    static double ratioToDb(double ratio);
    static _lapack_complex_double polar(double mag, double ang)
    {
        return {mag * cos (ang), mag * sin (ang)};
    }
    static void maToRi_Rad(_lapack_complex_double &val, double m, double rad);

    static void maToRi(_lapack_complex_double &val, double m, double phase);
    static void dBToRi(_lapack_complex_double &val, double mDb, double phase);
    static void riToMA(_lapack_complex_double &ma) ;
    static void riToMA(const _lapack_complex_double &ri, _lapack_complex_double &ma);

//    static double abs(const _lapack_complex_double &ri);
//    static _lapack_complex_double conj(const _lapack_complex_double &ri);

    static void frqInter(const QVector<LapackMat> &data,
                         const QVector<double> &freqList,
                         double userFrqRio, double frq, LapackMat &mat);

    static bool linearEquationSolverPtrCpy(int n, const lapack_complex_double *A, const lapack_complex_double *b, lapack_complex_double *x);
    static bool linearEquationSolverCpy(const LapackMat& A, const LapackMat& b, LapackMat& x);
    static void test();
    LapackMat()
    {

    }
    LapackMat(int nr_, int nc_);
    ~LapackMat();
//    void resetZero();
//    bool set(int r, int c, const lapack_complex_double &val);
//    const lapack_complex_double& get(int r, int c) const;

    void getRealColVector(double *out, int c) const;

//    const _lapack_complex_double *getDataConst() const;
//    _lapack_complex_double *getData() const;
//    void cpyData(const _lapack_complex_double *value, int nr, int nc);

    void add(const LapackMat& mat);
    void sub(const LapackMat &mat);
    void multLeft(const LapackMat &mat, LapackMat &result) const;
    void multNum(const _lapack_complex_double &num);
    void inv();
    void trans(LapackMat &result) const;
    void zaxpy();

//    void init(int nr_, int nc_);
//    int getNr() const;
//    int getNc() const;

    void print();
    void printReal();
    void setMat(const LapackMat &mat);
    void zaxpy(double cX1, const LapackMat &X1, double cX2, const LapackMat &X2);
    void initI(int nr_, int nc_); //初始化为单位矩阵
    void print(const QString &log) const;
    void initI();

//    void setPort50();
    //diagVal矩阵对角线的值，nonDiagVal矩阵非对角线的值
//    void setMat(const _lapack_complex_double &diagVal, const _lapack_complex_double &nonDiagVal);
//    void transConj(LapackMat &result);
    void transConj(LapackMat &result) const;

//    bool set(int r, int c, double real, double img);
    double totalAmp() const;
//    void setDataPtr(_lapack_complex_double *value);

//    const _lapack_complex_double &getByOne(int r, int c) const;
//    bool setByOne(int r, int c, const _lapack_complex_double &val);
    void getRealRowVector(double *out, int row) const;
//private:
//    int nr;
//    int nc;
//    int layout;
//    lapack_complex_double *data;

//    inline int index(int r, int c) const;

//    LapackMat(const LapackMat &) = delete; // 拷贝构造函数被删除
//    LapackMat(const LapackMat &&) = delete;// 移动构造函数被删除
//    LapackMat &operator=(const LapackMat &); // 拷贝赋值函数被删除
    //    LapackMat& operator=(LapackMat&&);// 移动赋值函数被删除,注意没有const
    static bool invertCmplx(LapackMat &A);
    void printRealLog(const QString &log);
    static bool linearEquationSolver(LapackMat &A, const LapackMat &b, LapackMat &x);
};
double abs(const lapack_complex_double& a);
lapack_complex_double conj(const lapack_complex_double& a);
lapack_complex_double operator+(const lapack_complex_double& a, const lapack_complex_double& b);

lapack_complex_double operator-(const lapack_complex_double& a, const lapack_complex_double& b);

lapack_complex_double operator*(const lapack_complex_double& a,const lapack_complex_double& b);

lapack_complex_double operator*(const lapack_complex_double& a, double d)
;

lapack_complex_double operator*(double d, const lapack_complex_double& a)
;

lapack_complex_double operator/(const lapack_complex_double& a, double d)
;

lapack_complex_double operator/(const lapack_complex_double& a, const lapack_complex_double& other)
;


_lapack_complex_double operator/(double d, const _lapack_complex_double& c);

_lapack_complex_double operator+(double d, const _lapack_complex_double& other);

_lapack_complex_double operator+(const _lapack_complex_double& other, double d);

_lapack_complex_double operator-(double d, const _lapack_complex_double& other);

_lapack_complex_double operator-( const _lapack_complex_double& other, double d);

#endif // LAPACKMAT_H
