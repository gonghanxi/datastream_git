#ifndef MATRIX_H
#define MATRIX_H
#pragma once

// Force Modelbuilder Export OLD_MODELBUILDER_API

#include <complex>
//#include <crtdbg.h>
#include <algorithm>
#include "CircularBuffer.h"

#ifdef _WIN32
    #include <crtdbg.h>  // Windows 专用
#elif __linux__
    #include <assert.h>  // Linux 替代：使用标准断言
    #include <stdlib.h>  // 包含 abort() 等函数
    // Linux 下的调试工具
    #define _ASSERT(expr) assert(expr)
    #define _CrtDbgReport(type, file, line, module, msg) \
        fprintf(stderr, "Debug: %s:%d: %s\n", file, line, msg)
#else
    #error "Unsupported platform"
#endif

#ifdef _MSC_VER
    #pragma warning ( push )
    #pragma warning( disable : 4244 )
    #pragma warning( disable : 4800 )
#else
    // GCC/Clang 下可以忽略这些 pragma 或添加对应的诊断忽略
    #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunknown-pragmas"
    #endif
#endif

namespace SystemVueModelBuilder {
    namespace ScalarConversion
    {
    // Utility functions used by Matrix class.

        template <typename T1, typename T2> inline void Copy(const std::complex<T1>& t1, std::complex<T2>& t2)
        {
            t2 = t1;
        };

        template <typename T1, typename T2> inline void Copy(const std::complex<T1>& t1, T2& t2)
        {
            t2 = real(t1);
        };

        template <typename T1, typename T2> inline void Copy(const T1& t1, T2& t2)
        {
            t2 = t1;
        };
    }
    namespace MatrixUtilities
        {
        /// Sets the first iElements elements in the array starting at the address pIn to 0.
        /// <param name="pIn">Array start address.</param>
        /// <param name="iElements">Number of elements that will be set to 0.</param>
        template <typename T> inline void Zero(T* pIn, size_t iElements)
        {
            for ( size_t i = 0 ; i < iElements; i++)
                pIn[i] = 0;
        }

        /// Copies iElements elements of type T from the adress pIn to the adress pOut.
        /// <param name="pIn">Address from which elements will be copied.</param>
        /// <param name="pOut">Address to which elements will be copied.</param>
        /// <param name="iElements">Number of elements that will be copied.</param>
        template <typename T> inline void Copy(const T* pIn, T* pOut, size_t iElements)
        {
            if ( iElements != 0 )
            {
                _ASSERT(pIn != 0 && pOut != 0);
                memcpy(pOut, pIn, iElements*sizeof(T));
            }
        }

        /// Copies iElements elements of type T1 from the adress pIn to elements of type T2 at the adress pOut.
        /// <param name="pIn">Address from which elements will be copied.</param>
        /// <param name="pOut">Address to which elements will be copied.</param>
        /// <param name="iElements">Number of elements that will be copied.</param>
        template <typename T1, typename T2> inline void Copy(const T1* pIn, T2* pOut, size_t iElements)
        {
            for ( size_t i = 0 ; i < iElements; i++)
                ScalarConversion::Copy(pIn[i],pOut[i]);
        }

    }

#ifdef _MSC_VER
    #pragma warning ( pop )
#elif defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
    /// Matrix类实现了一个轻量级的矩阵结构（二维数组），它包含了类中使用自定义构建模型所需的所有必要特性。
    template <typename T> class Matrix
    {
    public:
        Matrix()
        {
            Initialize();
        }
        ~Matrix()
        {
            delete [] m_pData;
            delete [] m_piDimension;
            delete [] m_piColumnMajorStride;
        }
        /// 创建未初始化的 nRows x nCols matrix.
        /// <param name="nRows">Number of matrix rows.</param>
        /// <param name="nCols">Number of matrix columns.</param>
        Matrix(size_t nRows, size_t nCols)
        {
            Initialize();
            Resize(nRows,nCols);
        }

        /// Creates uninitialized nDimensions matrix with dimension sizes specified by piDimension.
        /// <param name="nDimensions">Number of dimensions.  This must be >= 2.</param>
        /// <param name="piDimension">Array with dimension sizes.  If null, empty matrix.</param>
        Matrix(size_t nDimensions, const size_t* piDimension)
        {
            Initialize();
            ResizeMultidimensional(nDimensions, piDimension);
        }

        /// Copy constructor
        /// <param name="matrix">Matrix that will be copied.</param>
        Matrix(const Matrix & matrix)
        {
            Initialize();
            *this = matrix;
        }
        /// Resize matrix to nRows x nCols.
        /// <param name="nRows">Number of matrix rows.</param>
        /// <param name="nCols">Number of matrix columns.</param>
        void Resize(size_t nRows, size_t nCols)
        {
            size_t dim[2];
            dim[0] = nRows;
            dim[1] = nCols;
            ResizeMultidimensional( 2, dim);
        }
        /// Resize matrix to a nDimensions matrix with dimension sizes specified by piDimension.
        /// <param name="nDimensions">Number of dimensions.  This must be >= 2.</param>
        /// <param name="piDimension">Array with dimension sizes.  If null, empty matrix.</param>
        void ResizeMultidimensional(size_t nDimensions, const size_t* piDimension)
        {
            _ASSERT( nDimensions >= 2); // Matrix dimensionality must be 2 or greater.
            _ASSERT( piDimension);

            size_t iNumElements;

            if ( piDimension != 0)
            {
                iNumElements = 1;
                for ( size_t i = 0 ; i < nDimensions; i++)
                    iNumElements *= piDimension[i];
            }
            else
            {
                iNumElements = 0;
            }

            if ( nDimensions > m_nDimensions)
            {
                delete [] m_piDimension;
                m_piDimension = new size_t[ nDimensions];
                delete [] m_piColumnMajorStride;
                m_piColumnMajorStride = new size_t[ nDimensions];
            }

            m_nDimensions = nDimensions;
            m_iNumElements = iNumElements;

            if ( iNumElements > m_iMaxElements)
                SetMaxElements( iNumElements);

            if ( nDimensions > 0)
            {
                memcpy( m_piDimension, piDimension, nDimensions * sizeof( size_t));
            }

            // See http://en.wikipedia.org/wiki/Row-major_order - we compute the constant part of the column-major memory offset here.
            for ( size_t k = 0; k < m_nDimensions; k++)
            {
                m_piColumnMajorStride[k] = 1;
                if ( k > 0)
                {
                    for ( size_t l = 0; l <= k-1 ; l++)
                    {
                        m_piColumnMajorStride[k] *= m_piDimension[l];
                    }
                }
            }
        }
        /// Return the number of rows
        inline size_t NumRows() const
        {
            return Size(0);
        }

        /// Return the number of columns
        inline size_t NumColumns() const
        {
            return Size(1);
        }

        /// Return the number of dimensions
        inline size_t NumDimensions() const
        {
            return m_nDimensions;
        }

        /// Return true if the array is a matrix
        inline bool IsMatrix() const
        {
            return NumDimensions() == 2;
        }

        /// Return the sizes of each dimension
        inline const size_t* Dimensions() const
        {
            return m_piDimension;
        }
        /// Return the size of a matrix for a specified dimension.
        /// <param name="iDimension">Dimension index.</param>
        inline size_t Size( size_t iDimension) const
        {
            size_t iSize;

            _ASSERT( iDimension < m_nDimensions);

            if ( NumElements() == 0)
                iSize = 0;
            else
                iSize = m_piDimension[ iDimension];

            return iSize;
        }

        /// Return the number of matrix elements
        inline size_t NumElements() const { return m_iNumElements; }

        /// Set the maximum number of elements the matrix can hold.
        /// <param name="iMaxElements">New maximum number of matrix elements.</param>
        inline void SetMaxElements( size_t iMaxElements)
        {
            if ( m_iMaxElements != iMaxElements)
            {
                m_iMaxElements = iMaxElements;
                delete [] m_pData;
                if ( m_iMaxElements > 0)
                    m_pData = new T[ m_iMaxElements];
                else
                    m_pData = 0;
            }
        }
        /// Set all elements to zero
        bool Zero()
        {
            MatrixUtilities::Zero( m_pData, m_iNumElements);
            return true;
        }

        /// Resize based on dimensions of a reference matrix (if reference matrix is null set to empty matrix).
        /// Set all elements of resized matrix to zero.
        /// <param name="pReference">Reference matrix.</param>
        bool Zero(Matrix* pReference)
        {
            if ( pReference)
            {
                ResizeMultidimensional( pReference->m_nDimensions, pReference->m_piDimension);
                Zero();
            }
            else
            {
                SetEmpty();
            }
            return true;
        }

        /// Return TRUE if this matrix is equal size to another one.
        /// <param name="matrix">Matrix to compare to.</param>
        bool IsSizeEqual( const Matrix& matrix) const
        {
            bool bIsSizeEqual = matrix.NumDimensions() == NumDimensions();
            for ( size_t i = 0; i < NumDimensions() && bIsSizeEqual; i++)
                bIsSizeEqual = ( matrix.Size(i) == Size(i) );
            return bIsSizeEqual;
        }

        /// Return TRUE if this matrix is empty.
        bool IsEmpty() const
        {
            return NumElements() == 0;
        }
        /// Set empty
        void SetEmpty()
        {
            m_iNumElements = 0;
        }

        /// Return TRUE if this matrix is equal to another one.
        /// <param name="matrix">Matrix to compare to.</param>
        bool operator == (const Matrix & matrix) const
        {
            bool bIsEqual = IsSizeEqual( matrix );
            size_t i;

            // The "&& bIsEqual" will break out of the loop as soon as two corresponding matrix entries are different.
            for (i=0; i< NumElements() && bIsEqual; i++)
            {
                bIsEqual = ( operator()(i)==matrix(i) );
            }
            return bIsEqual;
        }

        /// Return TRUE if this matrix is not equal to another one.
        /// <param name="matrix">Matrix to compare to.</param>
        bool operator != (const Matrix & matrix) const
        {
            return ! ( *this == matrix );
        }
        /// Assignment operator (copy contents of right hand side operand to left hand side operand).
        /// <param name="matrix">Matrix whose contents are copied.</param>
        Matrix& operator = (const Matrix& matrix)
        {
            ResizeMultidimensional( matrix.m_nDimensions, matrix.m_piDimension);

            if ( matrix.NumElements() > 0)
            {
                _ASSERT( matrix.NumElements()==NumElements() && m_pData);
                matrix.CopyTo(m_pData, matrix.NumElements());
            }

            return *this;
        };
#ifdef _MSC_VER
    #pragma warning ( push )
    #pragma warning( disable : 4244 )
    #pragma warning( disable : 4800 )
#else
    // GCC/Clang 下可以忽略这些 pragma 或添加对应的诊断忽略
    #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunknown-pragmas"
    #endif
#endif

        /// Copy iSize elements from address pData to this matrix.
        /// If the matrix has M elements where M < iSize then only the
        /// first M elemenents will be copied.
        /// If the matrix has M elements where M > iSize then the last
        /// (M - iSize) elemenents of the matrix are set to zero.
        /// <param name="pData">Address from which elements will be copied.</param>
        /// <param name="iSize">Number of elements to be copied.</param>
        template <typename T2> void CopyFrom(const T2* pData, size_t iSize)
        {
            _ASSERT( pData != 0);
            size_t iCopy = (std::min)(NumElements(),iSize);
            MatrixUtilities::Copy(pData, m_pData, iCopy);
            if ( iCopy < NumElements())
                MatrixUtilities::Zero(&m_pData[iCopy], NumElements() - iCopy);
        }

        /// Copy the first iSize matrix elements to address pData.
        /// If the matrix has M elements where M < iSize then the last
        /// (iSize - M) elemenents in the destination are set to zero.
        /// If the matrix has M elements where M > iSize then only the
        /// first iSisze elemenents of the matrix are copied to the destination.
        /// <param name="pData">Address to which elements will be copied.</param>
        /// <param name="iSize">Number of elements to be copied.</param>
        template <typename T2> void CopyTo(T2* pData, size_t iSize) const
        {
            _ASSERT( pData != 0);
            size_t iCopy = (std::min)(NumElements(),iSize);
            MatrixUtilities::Copy(m_pData, pData, iCopy);
            if ( iCopy < iSize)
                MatrixUtilities::Zero(&pData[iCopy], iSize - iCopy);
        }
#ifdef _MSC_VER
#pragma warning ( pop )
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
        /// Return a reference to the matrix element at row iRow and column iCol.  If this matrix is greater than 2 dimensions, the remaining indicies are assumed to be 0.
        /// <param name="iRow">Row of element to be returned.</param>
        /// <param name="iCol">Column of element to be returned.</param>
        T& operator() ( size_t iRow, size_t iCol)
        {
            _ASSERT( m_pData && NumElements());
            _ASSERT( iRow < NumRows());
            _ASSERT( iCol < NumColumns());
            size_t iIndex = NumRows()*iCol+iRow;
            return m_pData[iIndex];
        }

        /// Return the matrix element at row iRow and column iCol.  If this matrix is greater than 2 dimensions, the remaining indicies are assumed to be 0.
        /// <param name="iRow">Row of element to be returned.</param>
        /// <param name="iCol">Column of element to be returned.</param>
        T operator() ( size_t iRow, size_t iCol) const
        {
            _ASSERT( m_pData && NumElements());
            _ASSERT( iRow < NumRows());
            _ASSERT( iCol < NumColumns());
            size_t iIndex = NumRows()*iCol+iRow;
            return m_pData[iIndex];
        }
        /// Return a slice (subset) of the matrix.  The number of dimensions will be the same as this matrix.
        /// <param name="nDimensions">Number of dimensions.  This must match the number of dimensions of this matrix.</param>
        /// <param name="piIndexTopCorner">The index of the top corner (must have smallest indicies).</param>
        /// <param name="piIndexBottomCorner">The index of the bottom corner (must have largest indicies).</param>
        /// <param name="matrixSlice">The matrix to store the slice in.</param>
        Matrix& GetSlice( size_t nDimensions, const size_t* piIndexTopCorner, const size_t* piIndexBottomCorner, Matrix& matrixSlice) const
        {
            _ASSERT( nDimensions == m_nDimensions);
            size_t* pIndex = new size_t[ nDimensions];
            size_t i;

            // Calculate dimension of matrix slice
            for ( i = 0; i < nDimensions; i++)
            {
                _ASSERT( piIndexTopCorner[ i] <= piIndexBottomCorner[ i]);
                _ASSERT( piIndexBottomCorner[ i] < m_piDimension[i]);
                pIndex[i] = piIndexBottomCorner[ i] - piIndexTopCorner[ i] + 1;
            }
            matrixSlice.ResizeMultidimensional( nDimensions, pIndex);

            // Set index to first element to copy
            for ( i = 0; i < nDimensions; i++)
            {
                pIndex[i] = piIndexTopCorner[ i];
            }

            // Copy the elements
            {
                size_t iElementsCopied = 0;

                while ( iElementsCopied < matrixSlice.NumElements())
                {
                    matrixSlice(iElementsCopied++) = Get( nDimensions, pIndex);

                    // Increment index
                    for ( i = 0; i < nDimensions; i++)
                    {
                        pIndex[i]++;
                        if ( pIndex[i] > piIndexBottomCorner[ i])
                        {
                            pIndex[i] = piIndexTopCorner[ i];
                        }
                        else
                        {
                            break; // Do not need to increment other dimensions
                        }
                    }
                }
            }

            delete [] pIndex;
            return matrixSlice;
        }
        /// Return a reference to the iIndex multidimensional matrix element (elements stored in column major form).
        /// <param name="nDimensions">Number of dimensions.  This must be >= 2.</param>
        /// <param name="piIndex">Element index.</param>
        T& Get(size_t nDimensions, const size_t* piIndex)
        {
            _ASSERT( nDimensions == m_nDimensions);
            _ASSERT( nDimensions >= 2);

            size_t iIndex = piIndex[0];

            for ( size_t k = 1; k < m_nDimensions; k++)
            {
                iIndex += m_piColumnMajorStride[k] * piIndex[k];
            }

            return m_pData[iIndex];
        }

        /// Return the value of the iIndex multidimensional matrix element (elements stored in column major form).
        /// <param name="nDimensions">Number of dimensions.  This must be >= 2.</param>
        /// <param name="piIndex">Element index.</param>
        T Get(size_t nDimensions, const size_t* piIndex) const
        {
            _ASSERT( nDimensions == m_nDimensions);
            _ASSERT( nDimensions >= 2);

            size_t iIndex = piIndex[0];

            for ( size_t k = 1; k < m_nDimensions; k++)
            {
                iIndex += m_piColumnMajorStride[k] * piIndex[k];
            }

            return m_pData[iIndex];
        }
        /// Return a reference to the iIndex matrix element (elements stored in column major form).
        /// <param name="iIndex">Index of element to be returned.</param>
        T& operator() ( size_t iIndex)
        {
            _ASSERT( m_pData && NumElements());
            _ASSERT( iIndex < NumElements());
            return m_pData[iIndex];
        }

        /// Return the iIndex matrix element (elements stored in column major form).
        /// <param name="iIndex">Index of element to be returned.</param>
        T operator() ( size_t iIndex) const
        {
            _ASSERT( m_pData && NumElements());
            _ASSERT( iIndex < NumElements());
            return m_pData[iIndex];
        }

        /// Negate matrix
        Matrix& operator-()
        {
            for ( size_t i = 0 ; i < NumElements() ; i++)
                m_pData[i] = - m_pData[i];
            return *this;
        }

        /// Add scalar to each matrix element.
        /// <param name="scalar">Scalar to be added to each matrix element.</param>
        template<typename S> Matrix& operator+= (S scalar)
        {
            _ASSERT( NumElements() > 0);
            for ( size_t i = 0 ; i < NumElements() ; i++)
                m_pData[i] += scalar;
            return *this;
        }

        /// Matrix addition
        /// <param name="matrix">Matrix to add.</param>
        template<typename M> Matrix& operator+= (const Matrix<M>& matrix)
        {
            _ASSERT( IsSizeEqual( matrix));
            for ( size_t i = 0 ; i < NumElements() ; i++)
                m_pData[i] += matrix(i);
            return *this;
        }
        /// Subtract scalar from each matrix element.
        /// <param name="scalar">Scalar to be subtracted from each matrix element.</param>
        template<typename S> Matrix& operator-= (S scalar)
        {
            _ASSERT( NumElements() > 0);
            for ( size_t i = 0 ; i < NumElements() ; i++)
                m_pData[i] -= scalar;
            return *this;
        }

        /// Matrix subtraction
        /// <param name="matrix">Matrix to subtract.</param>
        template<typename M> Matrix& operator-= (const Matrix<M>& matrix)
        {
            _ASSERT( IsSizeEqual( matrix));
            for ( size_t i = 0 ; i < NumElements() ; i++)
                m_pData[i] -= matrix(i);
            return *this;
        }

        /// Multiply each matrix element with a scalar.
        /// <param name="scalar">Scalar to multiply with.</param>
        template<typename S> Matrix& operator*= (S scalar)
        {
            _ASSERT( NumElements() > 0);
            for ( size_t i = 0 ; i < NumElements() ; i++)
                m_pData[i] *= scalar;
            return *this;
        }

        /// Matrix multiplication
        /// <param name="matrix">Matrix to multiply with.</param>
        template<typename T2> Matrix& operator*= (const Matrix<T2>& matrix)
        {
            _ASSERT( NumDimensions() == 2); // Only first 2 dimensions supported - others are ignored.
            _ASSERT( NumElements() > 0 && matrix.NumElements() > 0);
            _ASSERT( NumColumns() == matrix.NumRows());

            Matrix output( NumRows(), matrix.NumColumns());

            size_t i, j, k;
            for (i=0; i < output.NumRows(); i++)
            {
                for (j=0;j< output.NumColumns(); j++)
                {
                    output(i,j) = 0;
                    for (k=0; k< NumColumns(); k++)
                    {
                        output(i,j) += (*this)(i,k) * matrix(k,j);
                    }
                }
            }

            Swap(&output);

            return *this;
        }
        /// Make this matrix a diagonal one with all diagonal elements set to data.
        /// If the matrix is not square it is left unchanged.
        /// <param name="data">Value of diagonal elements.</param>
        bool diagonal(T data)
        {
            bool bResult = true;

            for( size_t i = 1; i < NumDimensions() && bResult; i++)
                bResult =  ( Size(0) == Size(i));

            if ( bResult)
            {
                size_t i,j,k=0;
                for ( j = 0; j < NumColumns()  ; j++)
                    for ( i = 0; i < NumRows()  ; i++)
                        m_pData[k++] = (i==j? data:(T)0);
            }

            return bResult;
        }

        /// Make this matrix an identity one.
        /// If the matrix is not square it is left unchanged.
        bool identity()
        {
            return diagonal(1);
        }

        /// Get access to the internal storage array
        /// Matrix elements are stored in column major form.
        T* GetBuffer()
        {
            return m_pData;
        }

        /// Get access to the internal storage array (const version)
        /// Matrix elements are stored in column major form.
        const T* GetBuffer() const
        {
            return m_pData;
        }
        /// Swap contents with another matrix.
        /// <param name="pMatrix">Matrix to swap contents with.</param>
        void Swap(Matrix* pMatrix)
        {
            T* pData = m_pData;
            size_t iMaxElements = m_iMaxElements;
            size_t* piDimension = m_piDimension;
            size_t nDimensions = m_nDimensions;
            size_t iNumElements = m_iNumElements;

            m_pData = pMatrix->m_pData;
            m_iMaxElements = pMatrix->m_iMaxElements;
            m_piDimension = pMatrix->m_piDimension;
            m_nDimensions = pMatrix->m_nDimensions;
            m_iNumElements = pMatrix->m_iNumElements;

            pMatrix->m_pData = pData;
            pMatrix->m_iMaxElements = iMaxElements;
            pMatrix->m_piDimension = piDimension;
            pMatrix->m_nDimensions = nDimensions;
            pMatrix->m_iNumElements = iNumElements;
        }
    private:
        T* m_pData;
        size_t m_nDimensions;
        size_t* m_piDimension;
        size_t m_iNumElements;
        size_t m_iMaxElements;
        size_t* m_piColumnMajorStride;

        void Initialize()
        {
            m_pData=0;
            m_iMaxElements = 0;
            m_nDimensions = 2;
            m_piDimension = new size_t[2];
            m_piDimension[0] = m_piDimension[1] = 0;
            m_piColumnMajorStride = new size_t[2];
            m_piColumnMajorStride[0] = m_piColumnMajorStride[1] = 0;
            m_iNumElements = 0;
        }
    };
    /// Return sum of matrices mx1 and mx2.
    template <typename T>
    Matrix<T> operator + (const Matrix<T> &mx1, const Matrix<T> &mx2)
    {
        Matrix<T> result;
        _ASSERT(mx1.NumColumns() == mx2.NumColumns() && mx1.NumRows() == mx2.NumRows());

        result.Resize(mx1.NumRows(),mx1.NumColumns());

        size_t i;
        for (i = 0 ; i < mx1.NumElements(); i++)
            result(i) = mx1(i) + mx2(i);

        return result;
    }

    /// Return sum of matrix mx1 and scalar mx2.
    template <typename T>
    Matrix<T> operator + (const Matrix<T> &mx1, const T &mx2)
    {
        Matrix<T> result;
        result.Resize(mx1.NumRows(),mx1.NumColumns());

        size_t i;
        for (i = 0 ; i < mx1.NumElements(); i++)
            result(i) = mx1(i) + mx2;

        return result;
    }

    /// Return sum of scalar mx2 and matrix mx1.
    template <typename T>
    Matrix<T> operator + (const T &mx2, const Matrix<T> &mx1)
    {
        Matrix<T> result;
        result.Resize(mx1.NumRows(),mx1.NumColumns());

        size_t i;
        for (i = 0 ; i < mx1.NumElements(); i++)
            result(i) = mx1(i) + mx2;

        return result;
    }
    /// Return difference of matrices mx1 and mx2 (mx1 - mx2)
    /// Note: The template function below is a replacement for a more generic one similar to those
    /// used for the other operators e.g., +. However, replaced by a simpler version to enable the
    ///operator instantiation in AvgSqrErr_M.cpp.

    template <typename T>
    Matrix<T> operator - (const Matrix<T> &mx1, const Matrix<T> &mx2)
    {
        Matrix<T> result;
        _ASSERT(mx1.NumColumns() == mx2.NumColumns() && mx1.NumRows() == mx2.NumRows());

        result.Resize(mx1.NumRows(),mx1.NumColumns());

        size_t i;
        for (i = 0 ; i < mx1.NumElements(); i++)
            result(i) = mx1(i) - mx2(i);

        return result;
    }


    /// Return matrix mx1 minus scalar mx2.
    /// Scalar mx2 is subtracted from all elements of matrix mx1.
    template <typename T>
    Matrix<T> operator - (const Matrix<T> &mx1, const T &mx2)
    {
        Matrix<T> result;
        result.Resize(mx1.NumRows(),mx1.NumColumns());

        size_t i;
        for (i = 0 ; i < mx1.NumElements(); i++)
            result(i) = mx1(i) - mx2;

        return result;
    }

    /// Return scalar mx2 minus matrix mx1.
    /// Scalar mx2 is added to all negated elements of matrix mx1.
    template <typename T>
    Matrix<T> operator - (const T &mx2, const Matrix<T> &mx1)
    {
        Matrix<T> result;
        result.Resize(mx1.NumRows(),mx1.NumColumns());

        size_t i;
        for (i = 0 ; i < mx1.NumElements(); i++)
            result(i) = mx2 - mx1(i);

        return result;
    }
    /// Return product of matrix mx1 and matrix mx2.
    template <typename T>
    Matrix<T> operator * (const Matrix<T> &mx1, const Matrix<T> &mx2)
    {
        _ASSERT( mx1.NumDimensions() == 2 && mx2.NumDimensions() == 2 ); // Only first 2 dimensions supported - others are ignored.
        _ASSERT( mx1.NumElements() > 0 && mx2.NumElements() > 0 );
        _ASSERT( mx1.NumColumns() == mx2.NumRows() );

        Matrix<T> output( mx1.NumRows(), mx2.NumColumns() );

        size_t i, j, k;
        for ( i = 0; i < output.NumRows(); i++ )
        {
            for ( j = 0; j< output.NumColumns(); j++ )
            {
                output( i, j ) = 0;
                for ( k = 0; k< mx1.NumColumns(); k++ )
                {
                    output( i, j ) += mx1(i, k) * mx2( k, j );
                }
            }
        }

        return output;
    }

    /// Return product of matrix mx1 and scalar mx2.
    template <typename T>
    Matrix<T> operator * (const Matrix<T> &mx1, const T &mx2)
    {
        Matrix<T> result;
        result.Resize( mx1.NumRows(), mx1.NumColumns() );

        size_t i;
        for ( i = 0; i < mx1.NumElements(); i++ )
            result( i ) = mx1( i ) * mx2;

        return result;
    }

    /// Return product of scalar mx2 and matrix mx1.
    template <typename T>
    Matrix<T> operator * (const T &mx2, const Matrix<T> &mx1)
    {
        Matrix<T> result;
        result.Resize( mx1.NumRows(), mx1.NumColumns() );

        size_t i;
        for ( i = 0; i < mx1.NumElements(); i++ )
            result( i ) = mx1( i ) * mx2;

        return result;
    }
    /// CircularBuffer template specialization for boolean matrix
    template < > class CircularBuffer < Matrix < bool > > : public CircularBufferE < Matrix < bool > >
    {
    };

    /// CircularBuffer template specialization integer matrix
    template < > class CircularBuffer < Matrix < int > > : public CircularBufferE < Matrix < int > >
    {
    };

    /// CircularBuffer template specialization double matrix
    template < > class CircularBuffer < Matrix < double > > : public CircularBufferE < Matrix < double > >
    {
    };

    /// CircularBuffer template specialization float matrix
    template < > class CircularBuffer < Matrix < float > > : public CircularBufferE < Matrix < float > >
    {
    };

    /// CircularBuffer template specialization complex double matrix
    template < > class CircularBuffer < Matrix < std::complex < double > > > : public CircularBufferE < Matrix < std::complex< double > > >
    {
    };

    /// CircularBuffer template specialization complex float matrix
    template < > class CircularBuffer < Matrix < std::complex < float > > > : public CircularBufferE < Matrix < std::complex< float > > >
    {
    };

    /// Commonly used instances of Matrix template class
    typedef Matrix<bool> BoolMatrix;
    typedef Matrix<char> CharMatrix;
    typedef Matrix<int> IntMatrix;
    typedef Matrix<float> FloatMatrix;
    typedef Matrix<double> DoubleMatrix;
    typedef Matrix<std::complex<float> > FComplexMatrix;
    typedef Matrix<std::complex<double> > DComplexMatrix;
    }
#endif // MATRIX_H
