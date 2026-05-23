#ifndef LAPACKMATBASE_H
#define LAPACKMATBASE_H
#include "../openBlas/lib/x64_release/openblas/include/cblas.h"
#include "../openBlas/lib/x64_release/openblas/include/lapacke.h"
#include "../openBlas/lib/x64_release/openblas/include/lapacke_config.h"
#include <vector>
#include <QString>
#include <iostream>
template<typename Data>
class LapackMatBase
{
public:
    LapackMatBase()
    {
        nr = 0;
        nc = 0;
        layout = LAPACK_ROW_MAJOR;
        data = nullptr;
    }

    virtual ~LapackMatBase()
    {
        if (data)
            free(data);
    }


    LapackMatBase(int nr_, int nc_)
    {
        nr = nr_;
        nc = nc_;
        layout = LAPACK_ROW_MAJOR;
        data = (Data*)malloc(nr*nc * sizeof(Data));
        memset(data, 0, sizeof(Data) * nr*nc);
    }

    LapackMatBase(const LapackMatBase &other)
    {
        if (this != &other) { // 检查自我赋值
            // 在这里实现赋值逻辑
            cpyData(other.getDataConst(), other.getNr(), other.getNc());
        }
    }

    LapackMatBase &operator=(LapackMatBase &&other)
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

    LapackMatBase &operator=(const LapackMatBase &other)
    {
        if (this != &other) { // 检查自我赋值
            // 在这里实现赋值逻辑
            cpyData(other.getDataConst(), other.getNr(), other.getNc());

        }
        return *this;
    }

    int size() const
    {
        return nr*nc;
    }

    void setRowVector(int row, std::vector<Data> &rowVector)
    {
        for (int col=0; col<nc; col++)
        {
            set(row, col, rowVector[col]);
        }
    }

    void setColVector(int col, std::vector<Data> &colVector)
    {
        for (int row=0; row<nr; row++)
        {
            set(row, col, colVector[row]);
        }
    }

    void getRowVector(int row, std::vector<Data> &rowVector) const
    {
//        rowVector.resize(nc);
        for (int col=0; col<nc; col++)
        {
           Data v =  get(row,col);
           rowVector[col] = v;
        }
    }
    void getColVector(int col, std::vector<Data> &colVector) const
    {
//        colVector.resize(nr);
        for (int row=0; row<nr; row++)
        {
           Data v =  get(row,col);
           colVector[row] = v;
        }
    }


    void getRowVectorOffset(int row, int offid, int offset, std::vector<Data> &rowVector) const
    {
//        rowVector.resize(offset);
        for (int col=0; col<offset; col++)
        {
           Data v =  get(row,offset*offid+col);
           rowVector[col] = v;
        }
    }

    void getColVectorOffset(int col, int offid, int offset, std::vector<Data> &colVector) const
    {
//        colVector.resize(offset);
        for (int row=0; row<offset; row++)
        {
           Data v =  get(offset*offid+row,col);
           colVector[col] = v;
        }
    }



    void resetZero()
    {
        if (data)
        {
          memset(data, 0, sizeof(Data) * nr*nc);
        }
    }


    void init(int nr_, int nc_, const QString &name)
    {
        info = name;
        init(nr_, nc_);
    }

    void init(int nr_, int nc_)
    {
        if (data != nullptr)
        {
            free(data);
            data = nullptr;
        }
        if (data == nullptr){
            nr = nr_;
            nc = nc_;
            layout = LAPACK_ROW_MAJOR;
            data = (Data*)malloc(nr*nc * sizeof(Data));
            memset(data, 0, sizeof(Data) * nr*nc);
        }
    }

//     bool set(int r, int c, double real, double img);

    void setDataPtr(Data *value)
    {
        data = value;
    }

    bool set(int r, int c, const Data &val)
    {
        int i = index(r, c);
        if (i==-1)
        {
            std::cout<<info.toStdString()<<" error LapackMat::set"<<std::endl;
//            printf("error LapackMat::set\n");
            return false;
        }

        data[i] = val;
        return true;
    }

    const Data&  get(int r, int c) const
    {
        int i = index(r, c);
        if (i==-1)
        {
            std::cout<<info.toStdString()<<" error LapackMat::get"<<std::endl;
            return data[0];
        }
        return data[i];
    }

    const Data *getDataConst() const
    {
        return data;
    }

    Data *getData() const
    {
        return data;
    }

    int getNr() const
    {
        return nr;
    }

    int getNc() const
    {
        return nc;
    }


    void setMat(const Data &diagVal, const Data &nonDiagVal)
    {
    //    init(nr_, nc_);
        for (int r = 0; r < nr; r++) {
            for (int c = 0; c < nr; c++) {
                data[r*nc+c] = nonDiagVal;
                data[c*nc+r] = nonDiagVal;
            }
            data[r*nc+r] = diagVal;
        }


    }

    const Data&  getByOne(int r, int c) const
    {

        int i = index(r-1, c-1);
        if (i==-1)
        {
            printf("error LapackMat::getByOne\n");
            return data[0];
        }
        return data[i];
    }

    bool setByOne(int r, int c, const Data &val)
    {
        int i = index(r-1, c-1);
        if (i==-1)
        {
            printf("error LapackMat::setByOne\n");
            return false;
        }

        data[i] = val;
        return true;
    }


protected:
    int nr;
    int nc;
    int layout;
    Data *data;
    QString info;
protected:
    void cpyData(const Data *value, int nr_, int nc_)
    {
        if(value==nullptr)
        {
            nr = nr_;
            nc = nc_;
            layout = LAPACK_ROW_MAJOR;
            data = nullptr;
            return;
        }
        if (nr == nr_ && nc == nc_)
        {
            memcpy(data, value, nr*nc*sizeof(Data));
            return;
        }
        if (data)
        {
            free(data);
            data = nullptr;
        }


        // 使用 memcpy 进行内存拷贝
        nr = nr_;
        nc = nc_;
        layout = LAPACK_ROW_MAJOR;
        data = (Data*)malloc(nr*nc * sizeof(Data));

        memcpy(data, value, nr*nc*sizeof(Data));
    }

    inline int index(int r, int c) const
    {

        int i = -1;
        if (r>=nr||c>=nc)
        {
            std::cout<<info.toStdString()<<"  error, r<nr||c<nc "<<r<<"---"<<c<<std::endl;
//            printf("error, r<nr||c<nc %d  %d \n", r, c);
            return i;
        }


        if (layout==LAPACK_ROW_MAJOR)
        {
           i = r*nc+c;
        }

        if (layout==LAPACK_COL_MAJOR)
        {
           i = c*nr+r;
        }
        return i;
    }};

#endif // LAPACKMATBASE_H
