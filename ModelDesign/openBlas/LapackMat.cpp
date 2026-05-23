#include "LapackMat.h"
#include <assert.h>
#include <string.h>
#include <complex>
#include <QVector>
#include <qmath.h>

#include <complex>
#include <iostream>
#define PI 3.141592653589793238462643

using namespace std;

//LapackMat::LapackMat()
//{
//    nr = 0;
//    nc = 0;
//    layout = LAPACK_ROW_MAJOR;
//    data = nullptr;
//}

void LapackMat::riToMA(_lapack_complex_double &ma)
{
    // 计算模（振幅）
    double real = ma.real;
    double imag = ma.imag;
    ma.real = sqrt(real * real + imag * imag);

    // 计算相位（以度表示）
    ma.imag = atan2(imag, real)*PI/180;;
}

//double LapackMat::abs(const _lapack_complex_double &ri)
//{
//    return sqrt(ri.real * ri.real + ri.imag * ri.imag);
//}

double LapackMat::totalAmp() const
{
    _lapack_complex_double result = {0,0};

    for (int j = 0; j < nc; j++) {
        _lapack_complex_double temp = data[j*nc+0];
        result = result+temp*conj(temp);
    }
    double r = sqrt(result.real);
    printf( "totalAmp %0.12f  %0.12f\n", r, result.imag);
    return  r;
}

//void LapackMat::setDataPtr(_lapack_complex_double *value)
//{
//    data = value;
//}

//_lapack_complex_double LapackMat::conj(const _lapack_complex_double &ri)
//{
//    _lapack_complex_double result = {ri.real, -ri.imag};
//    return result;
//}

void LapackMat::riToMA(const _lapack_complex_double &ri, _lapack_complex_double &ma)
{
    // 计算模（振幅）
//    double real = ma.real;
//    double imag = ma.imag;
    ma.real = sqrt(ri.real * ri.real + ri.imag * ri.imag);

    // 计算相位（以度表示）
    ma.imag = atan2(ri.imag, ri.real)*180/PI;
}

void LapackMat::frqInter(const QVector<LapackMat> &data, const QVector<double> &freqList, double userFrqRio, double frq, LapackMat &mat)
{
    double f = userFrqRio*frq;
    double step = freqList[1] - freqList[0];

    double rowDouble = (f-freqList[0])/step+step/1e100; //为了使qFloor稳定
    int row = qFloor(rowDouble);//有可能为负

    //频率扩展
    if (row<0)
    {
        mat.setMat(data[0]);
        return;
    }

    if (row>=freqList.size()-1)
    {
        mat.setMat(data[freqList.size()-1]);
        return;
    }

    if (qAbs(rowDouble - row)<step/1e99)
    {
        mat.setMat(data[row]);
        return;
    }

    double x_x1 = f-freqList[row];
    double x2_x = freqList[row+1]-f;
    double x2_x1 = freqList[row+1]-freqList[row];
    mat.zaxpy((x2_x/x2_x1), data[row], (x_x1/x2_x1), data[row+1]);
}


LapackMat &LapackMat::operator=(const LapackMat &other)
{

    if (this != &other) { // 检查自我赋值
        // 在这里实现赋值逻辑
        cpyData(other.getDataConst(), other.getNr(), other.getNc());

    }
    return *this;

}

LapackMat::LapackMat(const LapackMat &other)
{
    if (this != &other) { // 检查自我赋值
        // 在这里实现赋值逻辑
        cpyData(other.getDataConst(), other.getNr(), other.getNc());

    }
}

LapackMat::LapackMat( LapackMat &&other)
{
    if (this != &other) {
        data = other.getData();
        nr = other.getNr();
        nc = other.getNc();
        other.setDataPtr(nullptr);
    }
}

LapackMat &LapackMat::operator=(LapackMat &&other)
{
    if (this != &other) {
        if (data!=nullptr)
            free(data);          // 释放当前对象的资源
        data = other.getData();      // 获取其他对象的指针
        nr = other.getNr();
        nc = other.getNc();
        other.setDataPtr(nullptr);  // 防止其他对象销毁时重复释放指针
    }
    return *this;
}

double LapackMat::ratioToDb(double ratio) {
    // 计算分贝值
//    double dbValue = 10.0 * log10(ratio);
    return 10.0 * log10(ratio);
}

void LapackMat::maToRi_Rad(_lapack_complex_double &val, double m, double rad)
{
//    double rad = phase*PI/180;
    val.real = m*cos(rad);
    val.imag = m*sin(rad);
}

void LapackMat::maToRi(_lapack_complex_double &val, double m, double phase)
{
    double rad = phase*PI/180;
    val.real = m*cos(rad);
    val.imag = m*sin(rad);
}

void LapackMat::dBToRi(_lapack_complex_double &val, double mDb, double phase)
{
    double m = pow(10, mDb/20);
    double rad = phase*PI/180;
    val.real = m*cos(rad);
    val.imag = m*sin(rad);
}

LapackMat::LapackMat(int nr_, int nc_)
{
    nr = nr_;
    nc = nc_;
    layout = LAPACK_ROW_MAJOR;
    data = (lapack_complex_double*)malloc(nr*nc * sizeof(lapack_complex_double));
    memset(data, 0, sizeof(lapack_complex_double) * nr*nc);
}

LapackMat::~LapackMat()
{
//    if (data)
//        free(data);
}

//void LapackMat::resetZero()
//{
//    if (data)
//    {
//      memset(data, 0, sizeof(lapack_complex_double) * nr*nc);
//    }
//}

//void LapackMat::init(int nr_, int nc_)
//{
//    if (data != nullptr)
//    {
//        free(data);
//        data = nullptr;
//    }
//    if (data == nullptr){
//        nr = nr_;
//        nc = nc_;
//        layout = LAPACK_ROW_MAJOR;
//        data = (lapack_complex_double*)malloc(nr*nc * sizeof(lapack_complex_double));
//        memset(data, 0, sizeof(lapack_complex_double) * nr*nc);
//    }
//}

void LapackMat::initI(int nr_, int nc_)
{
    init(nr_, nc_);
    for (int i = 0; i < nr; i++) {
        data[i*nc+i] = {1, 0};
    }
}

//void LapackMat::setMat(const lapack_complex_double &diagVal, const lapack_complex_double &nonDiagVal)
//{
////    init(nr_, nc_);
//    for (int r = 0; r < nr; r++) {
//        for (int c = 0; c < nr; c++) {
//            data[r*nc+c] = nonDiagVal;
//            data[c*nc+r] = nonDiagVal;
//        }
//        data[r*nc+r] = diagVal;
//    }


//}

//void LapackMat::conv()
//{
//    x = complex([1, 2, 3], [4, 5, 6]);  % 示例向量 [1+4i, 2+5i, 3+6i]
//    y = complex([4, 5, 6], [7, 8, 9]);  % 示例向量 [4+7i, 5+8i, 6+9i]

//    result = cblas_zdotc_sub(length(x), x, 1, y, 1);
//}

void LapackMat::initI()
{

//    init(nr_, nc_);
    if (data){
        for (int i = 0; i < nr; i++) {
            data[i*nc+i] = {1, 0};
        }
    }

}

//int LapackMat::index(int r, int c) const
//{

//    int i = -1;
//    if (r>=nr||c>=nc)
//    {
//        printf("error, r<nr||c<nc %d  %d \n", r, c);
//        return i;
//    }


//    if (layout==LAPACK_ROW_MAJOR)
//    {
//       i = r*nc+c;
//    }

//    if (layout==LAPACK_COL_MAJOR)
//    {
//       i = c*nr+r;
//    }
//    return i;
//}

//非共轭转置
void LapackMat::trans(LapackMat &result) const
{
    result.init(nc, nr);
//    LAPACKE_zgetrs(LAPACK_COL_MAJOR, nr, nc, data, nc, result.getData(), nr);
    for (int i = 0; i < nr; i++) {
        for (int j = 0; j < nc; j++) {
            result.set(j,i, data[i*nc+j]);
        }
    }

}

void LapackMat::transConj(LapackMat &result) const
{
    result.init(nc, nr);
//    LAPACKE_zgetrs(LAPACK_COL_MAJOR, nr, nc, data, nc, result.getData(), nr);
    _lapack_complex_double temp;
    for (int i = 0; i < nr; i++) {
        for (int j = 0; j < nc; j++) {
            temp.imag = -data[i*nc+j].imag;
            temp.real = data[i*nc+j].real;
            result.set(j,i, temp);
        }
    }
}

//cX1*X1 + cX2*X2
void LapackMat::zaxpy(double cX1, const LapackMat &X1, double cX2, const LapackMat &X2)
{
    lapack_complex_double cX1_ = {cX1, 0.0};
    lapack_complex_double cX2_ = {cX2, 0.0};
    cblas_zaxpy(nr * nc, &cX1_, X1.getDataConst(), 1, data, 1);
    cblas_zaxpy(nr * nc, &cX2_, X2.getDataConst(), 1, data, 1);
}

//bool LapackMat::set(int r, int c, const lapack_complex_double &val)
//{
//    int i = index(r, c);
//    if (i==-1)
//    {
//        printf("error LapackMat::set\n");
//        return false;
//    }

//    data[i] = val;
//    return true;
//}


//bool LapackMat::setByOne(int r, int c, const lapack_complex_double &val)
//{
//    int i = index(r-1, c-1);
//    if (i==-1)
//    {
//        printf("error LapackMat::setByOne\n");
//        return false;
//    }

//    data[i] = val;
//    return true;
//}

//bool LapackMat::set(int r, int c, double real, double img)
//{
//    int i = index(r, c);
//    if (i==-1)
//    {
//        printf("error LapackMat::set\n");
//        return false;
//    }

//    data[i].imag = img;
//    data[i].real = real;
//    return true;
//}

//const lapack_complex_double&  LapackMat::get(int r, int c) const
//{

//    int i = index(r, c);
//    if (i==-1)
//    {
//        printf("error LapackMat::get\n");
//        return data[0];
//    }
//    return data[i];
//}

void LapackMat::getRealColVector(double *out, int col) const
{
    for (int i = 0; i < nr; i++) {
        out[i] = get(i, col).real;
    }
}

void LapackMat::getRealRowVector(double *out, int row) const
{
    for (int i = 0; i < nc; i++) {
        out[i] = get(row, i).real;
    }
}

//const lapack_complex_double&  LapackMat::getByOne(int r, int c) const
//{

//    int i = index(r-1, c-1);
//    if (i==-1)
//    {
//        printf("error LapackMat::getByOne\n");
//        return data[0];
//    }
//    return data[i];
//}
// 使用OpenBLAS/LAPACK计算复数矩阵的逆
bool LapackMat::invertCmplx(LapackMat & A) {
    // 准备 LAPACK 需要的参数
    int n = A.getNr();
    vector<int> ipiv(n);
    int info;

    // 第一步：进行 LU 分解
    info = LAPACKE_zgetrf(LAPACK_ROW_MAJOR, n, n,
                         A.getData(), n,
                         ipiv.data());

    if (info != 0) {
        cerr << "LU 分解失败，错误代码: " << info << endl;
        return false;
    }

    // 第二步：计算逆矩阵
//    vector<Complex> work(n);
    info = LAPACKE_zgetri(LAPACK_ROW_MAJOR, n,
                         A.getData(), n,
                         ipiv.data());

    if (info != 0) {
        cerr << "矩阵求逆失败，错误代码: " << info << endl;
        return false;
    }

    return true;
}
void LapackMat::inv()
{
    const int n = nr;
    lapack_int* ipiv = (lapack_int*)malloc(n * sizeof(lapack_int)); // 分配堆空间
    lapack_int info = LAPACKE_zgetrf(layout, nr, nc, data, nr, ipiv);//LU分解
    if (info == 0) {
        info = LAPACKE_zgetri(layout, nr, data, nr, ipiv); // 求解矩阵的逆
        free(ipiv);
//        printf("Inverse matrix A^-1:\n");
//        for (int i = 0; i < nr; i++) {
//            for (int j = 0; j < nc; j++) {
//                printf("(%0.8f, %0.8f)\t", data[i*nc + j].real, data[i*nc + j].imag);
//            }
//            printf("\n");
//        }
        return;
    }
    printf("Error in computing inverse matrix.\n");
}
void LapackMat::add(const LapackMat &mat)
{
//    void cblas_zaxpy(const int n, const void* alpha, const void* x, const int incx,
//                     void* y, const int incy);
//    y[i] = y[i] + alpha * x[i], for i = 0, 1, ..., n-1
//    n：要操作的向量的元素数。
//    alpha：复数倍数，指向一个 void* 类型的指针，实际上是一个 lapack_complex_double 类型的指针，表示复数。即使乘以复数倍数，alpha 本身仍然是复数。
//    x：输入向量，复数双精度数组，指向向量中的首元素。
//    incx：x 中相邻元素之间的跨度（步长），通常为 1。
//    y：输出向量，复数双精度数组，将结果累加到这个向量中，指向向量中的首元素。
//    incy：y 中相邻元素之间的跨度（步长），通常为 1。

    lapack_complex_double alpha = {1.0, 0.0};
    cblas_zaxpy(nr * nc, &alpha, mat.getDataConst(), 1, data, 1);
}

//this - mat 保存在this里
void LapackMat::sub(const LapackMat &mat)
{
    lapack_complex_double alpha = {-1.0, 0.0};
    cblas_zaxpy(nr * nc, &alpha, mat.getDataConst(), 1, data, 1);
}

//int LapackMat::getNr() const
//{
//    return nr;
//}

//int LapackMat::getNc() const
//{
//    return nc;
//}

//⽤矩阵a左乘矩阵b得到的是矩阵ab，⽤矩阵c右乘矩阵b得到的是矩阵bc
//this左乘mat
void LapackMat::multLeft(const LapackMat &mat, LapackMat &result) const
{
//    LapackMat result(this->getNr() , mat.getNc());
//    lapack_complex_double matrix_C[rows_A * cols_B];
//    result.init(this->getNr(), mat.getNc());
    if (result.getNc() == 0)
        return;
    int rows_A = this->getNr();
    int cols_A = this->getNc();
    int cols_B = mat.getNc();
    lapack_complex_double alpha = {1.0, 0.0};
    lapack_complex_double beta = {0.0, 0.0};

    cblas_zgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                rows_A, cols_B, cols_A,
                &alpha,
                data, cols_A,
                mat.getDataConst(), cols_B,
                &beta,
                result.getData(), cols_B);
}

//c*A
void LapackMat::multNum(const _lapack_complex_double &c)
{
    cblas_zscal(nr * nc, &c, data, 1);
}

//const _lapack_complex_double *LapackMat::getDataConst() const
//{
//    return data;
//}

//_lapack_complex_double *LapackMat::getData() const
//{
//    return data;
//}

#include <string.h>
//void LapackMat::cpyData(const _lapack_complex_double *value, int nr_, int nc_)
//{
//    if(value==nullptr)
//    {
//        nr = nr_;
//        nc = nc_;
//        layout = LAPACK_ROW_MAJOR;
//        data = nullptr;
//        return;
//    }
//    if (nr == nr_ && nc == nc_)
//    {
//        memcpy(data, value, nr*nc*sizeof(lapack_complex_double));
//        return;
//    }
//    if (data)
//    {
//        free(data);
//        data = nullptr;
//    }


//    // 使用 memcpy 进行内存拷贝
//    nr = nr_;
//    nc = nc_;
//    layout = LAPACK_ROW_MAJOR;
//    data = (lapack_complex_double*)malloc(nr*nc * sizeof(lapack_complex_double));

//    memcpy(data, value, nr*nc*sizeof(lapack_complex_double));
//}

void LapackMat::setMat(const LapackMat &mat)
{
    cpyData(mat.getDataConst(), mat.getNr(), mat.getNc());
}



void LapackMat::print() {
    lapack_int i, j;
//    qDebug()<<desc;
    for( i = 0; i < nr; i++ ) {
        for( j = 0; j < nc; j++ )
            printf( " (%6.3f,  %6.3f)", data[i*nc+j].real, data[i*nc+j].imag );
        printf( "\n" );
    }
    printf( "--------------------------\n" );

}

void LapackMat::printReal()
{
    lapack_int i, j;
    for( i = 0; i < nr; i++ ) {
        for( j = 0; j < nc; j++ )
            printf( " %6.32f,", data[i*nc+j].real);
        printf( "\n" );
    }
    printf( "--------------------------\n" );
}

void LapackMat::printRealLog(const QString & log)
{
    std::cout<<(log.toStdString())<<std::endl;
    lapack_int i, j;
    for( i = 0; i < nr; i++ ) {
        for( j = 0; j < nc; j++ )
            printf( " %6.3f,", data[i*nc+j].real);
        printf( "\n" );
    }
    printf( "--------------------------\n" );
}

void LapackMat::print(const QString & log) const{
    lapack_int i, j;
//    qDebug()<<desc;
    printf(log.toStdString().c_str());
    printf( "\n" );
    for( i = 0; i < nr; i++ ) {
        for( j = 0; j < nc; j++ )
            printf( " (%0.32f,  %0.32f)", data[i*nc+j].real, data[i*nc+j].imag );
        printf( "\n" );
    }
    printf( "--------------------------\n" );

}

_lapack_complex_double operator+(const _lapack_complex_double &a, const _lapack_complex_double &b) {
    lapack_complex_double result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

_lapack_complex_double operator-(const _lapack_complex_double &a, const _lapack_complex_double &b) {
    lapack_complex_double result;
    result.real = a.real - b.real;
    result.imag = a.imag - b.imag;
    return result;
}

_lapack_complex_double operator*(const _lapack_complex_double &a, const _lapack_complex_double &b) {
    lapack_complex_double result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

_lapack_complex_double operator*(const _lapack_complex_double &a, double d)  {
    lapack_complex_double result;
    result.real = a.real*d;
    result.imag = a.imag*d;
    return result;
}

_lapack_complex_double operator*(double d, const _lapack_complex_double &a)
{
    lapack_complex_double result;
    result.real = a.real*d;
    result.imag = a.imag*d;
    return result;
}

_lapack_complex_double operator/(const _lapack_complex_double &a, double d)
{
    lapack_complex_double result;
    result.real = a.real/d;
    result.imag = a.imag/d;
    return result;
}

_lapack_complex_double operator/( const _lapack_complex_double &a, const _lapack_complex_double &other)  {
//    double denominator = other.real * other.real + other.imag * other.imag;
//    lapack_complex_double result;
//    result.real = (a.real * other.real + a.imag * other.imag) / denominator;
//    result.imag = (a.imag * other.real - a.real * other.imag) / denominator;
//    lapack_complex_double_real()coml
    std::complex<double> t;
    t = (*reinterpret_cast<const std::complex<double>*>(&a))/(*reinterpret_cast<const std::complex<double>*>(&other));
    return *reinterpret_cast<_lapack_complex_double*>(&t);
//    return result;
}

_lapack_complex_double operator/(double d, const _lapack_complex_double &c)
{
    _lapack_complex_double result;
    if (c.real == 0.0 && c.imag == 0.0) {
        //        print ("Error: Division by zero is not allowed." ) ;
        //        exit(1); // 或者采取其他错误处理措施
        result.real = 111111;
        result.imag = 0;
        return result;
    }

    result.real = d / c.real;
    result.imag = -d * c.imag / (c.real * c.real);
    return result;
}

_lapack_complex_double operator+(double d, const _lapack_complex_double &other)
{
    _lapack_complex_double result;
    result.real = d + other.real;
    result.imag = other.imag;
    return result;
}

_lapack_complex_double operator+(const _lapack_complex_double &other, double d)
{
    _lapack_complex_double result;
    result.real = d + other.real;
    result.imag = other.imag;
    return result;
}

_lapack_complex_double operator-(double d, const _lapack_complex_double &other)
{
    lapack_complex_double result;
    result.real = d - other.real;
    result.imag = -other.imag;
    return result;
}

_lapack_complex_double operator-(const _lapack_complex_double &other, double d)
{
    _lapack_complex_double result;
    result.real = other.real - d;
    result.imag = other.imag;
    return result;
}



bool LapackMat::linearEquationSolverPtrCpy(int n, const lapack_complex_double *A, const lapack_complex_double *b, lapack_complex_double *x)
{
//    QVector<lapack_complex_double> A_copy(A, A + n * n);
    QVector<lapack_complex_double> A_copy;
    A_copy.reserve(n * n);
    for (int i = 0; i < n * n; ++i) {
        A_copy.append(A[i]);
    }

    for (int i = 0; i < n; i++)
    {
        x[i] = b[i];
    }

   QVector<lapack_int> ipiv(n);
   lapack_int info;


   // LU分解
   info = LAPACKE_zgetrf(LAPACK_ROW_MAJOR, n, n,A_copy.data(),n,ipiv.data());
   if (info != 0)
   {
       return false;
   }


   // 线性方程求解
   info = LAPACKE_zgetrs(LAPACK_ROW_MAJOR,'N',n,1,A_copy.data(),n,ipiv.data(),x,1);

   if (info != 0)
   {
       return false;
   }

   return true;

}

bool LapackMat::linearEquationSolver( LapackMat& A, const LapackMat& b, LapackMat& x)
{
    if(A.getNc() != A.getNr()  || A.getNc() != b.getNr()  || A.getNc() != x.getNr())
    {
        return false;
    }

    const int n= A.getNc();
//    QVector<lapack_complex_double> A_copy(A.data, A.data+n*n);

//    for (int i = 0; i < n; i++)
//    {
//        x.data[i] = b.data[i];
//    }
    x = b;

    QVector<lapack_int> ipiv(n);
    lapack_int info;

    // LU分解
    info = LAPACKE_zgetrf(LAPACK_ROW_MAJOR, n, n,A.getData(),n,ipiv.data());
    if (info != 0)
    {
        return false;
    }

    // 线性方程求解
    info = LAPACKE_zgetrs(LAPACK_ROW_MAJOR,'N',n,1,A.getData(),n,ipiv.data(),x.data,1);


    if (info != 0)
    {
        return false;
    }

    return true;
}

bool LapackMat::linearEquationSolverCpy(const LapackMat& A, const LapackMat& b, LapackMat& x)
{
    if(A.getNc() != A.getNr()  || A.getNc() != b.getNr()  || A.getNc() != x.getNr())
    {
        return false;
    }

    const int n= A.getNc();
//    QVector<lapack_complex_double> A_copy(A.data, A.data+n*n);
    QVector<lapack_complex_double> A_copy;
    A_copy.reserve(n * n);
    for (int i = 0; i < n * n; ++i) {
        A_copy.append(A.data[i]);
    }

//    for (int i = 0; i < n; i++)
//    {
//        x.data[i] = b.data[i];
//    }
    x = b;

    QVector<lapack_int> ipiv(n);
    lapack_int info;

    // LU分解
    info = LAPACKE_zgetrf(LAPACK_ROW_MAJOR, n, n,A_copy.data(),n,ipiv.data());
    if (info != 0)
    {
        return false;
    }

    // 线性方程求解
    info = LAPACKE_zgetrs(LAPACK_ROW_MAJOR,'N',n,1,A_copy.data(),n,ipiv.data(),x.data,1);


    if (info != 0)
    {
        return false;
    }

    return true;
}


void LapackMat::test()
{
    const int n = 3;

    // 测试 复数双精度 线性方程求解 测试数据
    lapack_complex_double CB1[9] = {
        {1.0,1.0}, {2.0,0.0}, {0.0, 0.0},
        {0.0, 0.0}, {3.0, -1.0}, {1.0, 0.0},
        {2.0,0.0}, {0.0,0.0}, {1.0, -1.0}
    };


    lapack_complex_double CB2[9] = {
        {2.0,0.0}, {0.0,1.0}, {1.0, 0.0},
        {1.0, 0.0}, {2.0, 0.0}, {0.0, -1.0},
        {0.0,1.0}, {1.0,0.0}, {1.0, 0.0}
    };


    lapack_complex_double CB3[9] = {
        {3.0,0.0}, {1.0,1.0}, {0.0,0.0},
        {0.0, 0.0}, {4.0, 0.0}, {2.0,-1.0},
        {0.0,1.0}, {0.0,0.0}, {2.0,0.0}
    };


    lapack_complex_double CB4[9] = {
        {1.0,0.0}, {0.0,1.0}, {1.0,0.0},
        {2.0, 0.0}, {1.0, -1.0}, {0.0,0.0},
        {0.0,0.0}, {1.0,0.0}, {2.0,1.0},
    };


    lapack_complex_double CB5[9] = {
        {2.0,1.0}, {1.0,0.0}, {0.0,0.0},
        {1.0, 0.0}, {3.0, 0.0}, {0.0,1.0},
        {0.0,0.0}, {0.0,1.0}, {1.0,0.0},
    };


    lapack_complex_double CB6[9] = {
        {1.0,0.0}, {1.0,0.0}, {0.0, 1.0},
        {0.0, 1.0}, {2.0, 0.0}, {1.0, 0.0},
        {0.0,0.0}, {1.0,0.0}, {2.0, 0.0}
    };


    lapack_complex_double CB7[9] = {
        {1.0,0.0}, {0.0,0.0}, {0.0, 1.0},
        {2.0, 0.0}, {1.0, 0.0}, {-1.0, 0.0},
        {0.0,1.0}, {1.0,0.0}, {2.0, 0.0}
    };


    lapack_complex_double CB8[9] = {
        {2.0,0.0}, {0.0,1.0}, {0.0, 0.0},
        {0.0, 1.0}, {2.0, 0.0}, {1.0, 0.0},
        {0.0,0.0}, {1.0,0.0}, {2.0, 0.0}
    };


    lapack_complex_double CB9[9] = {
        {1.0,1.0}, {1.0,0.0}, {0.0, 0.0},
        {0.0, 0.0}, {2.0, 1.0}, {1.0, 0.0},
        {1.0,0.0}, {0.0,0.0}, {1.0, 0.0}
    };


    lapack_complex_double CB10[9] = {
        {2.0,0.0}, {1.0,0.0}, {0.0, 1.0},
        {1.0, 0.0}, {2.0, 0.0}, {1.0, 0.0},
        {0.0,1.0}, {1.0,0.0}, {2.0, 0.0}
    };


    lapack_complex_double Cb1[3] ={
        {1.0,0.0}, {2.0,1.0}, {3.0, 0.0},
    };


    lapack_complex_double Cb2[3] ={
        {1.0,1.0}, {0.0,0.0}, {2.0, 0.0},
    };


    lapack_complex_double Cb3[3] ={
        {2.0,0.0}, {1.0,0.0}, {0.0, 1.0},
    };


    lapack_complex_double Cb4[3] ={
        {3.0,0.0}, {2.0,1.0}, {1.0, 0.0},
    };


    lapack_complex_double Cb5[3] ={
        {1.0,0.0}, {4.0,0.0}, {0.0, 1.0},
    };


    lapack_complex_double Cb6[3] ={
        {2.0,0.0}, {1.0,1.0}, {3.0, 0.0},
    };


    lapack_complex_double Cb7[3] ={
        {0.0,1.0}, {2.0,0.0}, {3.0, 1.0},
    };


    lapack_complex_double Cb8[3] ={
        {2.0,0.0}, {0.0,1.0}, {1.0, 0.0},
    };


    lapack_complex_double Cb9[3] ={
        {1.0,0.0}, {1.0,0.0}, {0.0, 1.0},
    };


    lapack_complex_double Cb10[3] ={
        {3.0,0.0}, {2.0,1.0}, {1.0, 0.0},
    };




    lapack_complex_double *Matrix_CB[10] = {CB1, CB2, CB3, CB4, CB5, CB6, CB7, CB8, CB9, CB10};
    lapack_complex_double *Matrix_Cb[10] = {Cb1, Cb2, Cb3, Cb4, Cb5, Cb6, Cb7, Cb8, Cb9, Cb10};



    // 复数 线性方程 求解 测试数据
    lapack_complex_double Cx1[3] ={
        {1.0,0.0}, {2.0,1.0}, {3.0, 0.0},
    };


    lapack_complex_double Cx2[3] ={
        {1.0,1.0}, {0.0,0.0}, {2.0, 0.0},
    };


    lapack_complex_double Cx3[3] ={
        {2.0,0.0}, {1.0,0.0}, {0.0, 1.0},
    };


    lapack_complex_double Cx4[3] ={
        {3.0,0.0}, {2.0,1.0}, {1.0, 0.0},
    };


    lapack_complex_double Cx5[3] ={
        {1.0,0.0}, {4.0,0.0}, {0.0, 1.0},
    };


    lapack_complex_double Cx6[3] ={
        {2.0,0.0}, {1.0,1.0}, {3.0, 0.0},
    };


    lapack_complex_double Cx7[3] ={
        {0.0,1.0}, {2.0,0.0}, {3.0, 1.0},
    };


    lapack_complex_double Cx8[3] ={
        {2.0,0.0}, {0.0,1.0}, {1.0, 0.0},
    };


    lapack_complex_double Cx9[3] ={
        {1.0,0.0}, {1.0,0.0}, {0.0, 1.0},
    };


   lapack_complex_double Cx10[3] ={
        {3.0,0.0}, {2.0,1.0}, {1.0, 0.0},
    };

   lapack_complex_double *Matrix_Cx[10] = {Cx1, Cx2, Cx3, Cx4, Cx5, Cx6, Cx7, Cx8, Cx9, Cx10};

   // 测试 10组双精度复数线性方程求解

    // 测试封装接口一 线性方程求解

    cout << "Test interface 1 for solve complex number liner quation " << endl;

    for (int i = 0; i < 10; i++)
    {
        LapackMat::linearEquationSolverPtrCpy(3,Matrix_CB[i],Matrix_Cb[i],Matrix_Cx[i]);
    }


    // 测试数据 打印 验证计算结果

    for (int i = 0; i < 10; i++)
    {
        cout << "test " << i+1 <<":" << endl;

        cout << "Matrix X:"<<endl;
        for(int j =0; j < 3; j++)
        {
            cout << "("<<Matrix_Cx[i][j].real << ","<< Matrix_Cx[i][j].imag<<")"<< " ";
        }
        cout << endl;

        cout << "Matrix b:"<<endl;
        for(int j =0; j < 3; j++)
        {
            cout << "("<<Matrix_Cb[i][j].real << ","<< Matrix_Cb[i][j].imag<<")"<< " ";
        }
        cout << endl;


        std::vector<std::complex<double>>AX(n*1, {0.0,0.0});

        std::complex<double> alpha = {1.0,0.0};
        std::complex<double> beta = {0.0,0.0};

        cblas_zgemv(CblasRowMajor, CblasNoTrans,
                    n, n,
                    &alpha, Matrix_CB[i], n,
                    Matrix_Cx[i], 1,
                    &beta, AX.data(), 1);

        cout << "Matirx AX:" << endl;
        for (int i = 0; i < n; i++)
        {
            cout << AX[i] << " ";
        }
        cout << endl;
    }


    // 测试封装接口二 线性方程求解
    LapackMat LapackA;
    LapackMat Lapackb;
    LapackMat Lapackx;

    cout << "Test interface 2 for solve complex number liner quation " << endl;
    for (int i = 0; i < 10; i++)
    {
        cout << "test " << i+1 <<":" << endl;
        LapackA.cpyData(Matrix_CB[i],3,3);
        Lapackb.cpyData(Matrix_Cb[i],3,1);
        Lapackx.cpyData(Matrix_Cx[i],3,1);
        LapackMat::linearEquationSolverCpy(LapackA,Lapackb,Lapackx);

        cout << "Matrix X:" <<endl;
        for (int i = 0; i < 3; i++)
        {
            cout << "(" << Lapackx.getDataConst()[i].real << ","<<Lapackx.getDataConst()[i].imag <<")"<< " " ;
        }
        cout << endl;
    }
}

_lapack_complex_double conj(const _lapack_complex_double &a)
{
    if (abs(a.imag)<1e-28)
        return {a.real, 0};

    return {a.real, -a.imag};
}

double abs(const _lapack_complex_double &ri)
{
    return sqrt(ri.real * ri.real + ri.imag * ri.imag);
}
