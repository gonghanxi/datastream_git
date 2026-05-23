#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H
#pragma once

#include <algorithm>
#include <complex>
#include <cstring>
#include <cassert>

#ifdef _WIN32
    #include <crtdbg.h>  // Windows 专用
#elif __linux__
    #include <assert.h>  // Linux 替代：使用标准断言
    #include <stdlib.h>  // 包含 abort() 等函数
    // Linux 下的调试工具
#ifndef _ASSERT
    #define _ASSERT(expr) assert(expr)
#endif
    #define _CrtDbgReport(type, file, line, module, msg) \
        fprintf(stderr, "Debug: %s:%d: %s\n", file, line, msg)
#else
    #error "Unsupported platform"
#endif


#ifdef _MSC_VER  // 仅在Windows MSVC编译器下启用
    #pragma warning(disable: 4067)  // 未预料的令牌后的警告
    #pragma warning(disable: 4251)  // 类需要dll-interface警告
#endif

namespace SystemVueModelBuilder {
    //CircularBuffer允许你的模型使用循环缓冲区实现DFPorts
    //CircularBufferBase是一个抽象基类
    class CircularBufferBase
    {
    public:
        CircularBufferBase()
        {
            m_pBuffer = nullptr;
            m_iCurrent = m_iSize = 0;
            m_iHistoryDepth = m_iRate = 1;
            m_bContiguousBuffer = false;
            //m_bIsConnected默认值为true,端口未连接为false
            m_bIsConnected = true;

            m_iSizeOf = 0;
        }
        virtual ~CircularBufferBase() {}
        //调用SetBuffer方法为DFPorts设置循环缓冲区
        //pBuffer:指向循环内存缓冲区
        //iNumberItems:循环缓冲区可容纳的项数
        //iAdvanceRate:调用Advance方法时缓冲区索引增加的量
        //iStartLocation:初始缓冲区索引的位置
        //CircularBufferBase不会释放该内存，调用GetReadPtr(),GetWritePtr(),Write()方法，实际缓冲区大小可能必须大于iNumberItems参数
        void SetBuffer(void* pBuffer,size_t iNumberItems, size_t iAdvanceRate = 1, size_t iStartLocation = 0)
        {
            if( (iStartLocation == 0 && iNumberItems ==0 && pBuffer == nullptr) ||
                ( iStartLocation >= iNumberItems || iAdvanceRate > iNumberItems) )
                return;

            m_pBuffer = pBuffer;
            m_iSize = iNumberItems;
            m_iRate = iAdvanceRate;
            m_iCurrent = iStartLocation;
        }
        //SetRate方法在CircularBuffer上设置multirate属性，只允许在Setup方法中调用。
        void SetRate(size_t iRate)
        {
            m_iRate = iRate;
            if(m_iHistoryDepth < m_iRate) {
                m_iHistoryDepth = m_iRate;
            }
        }
        //获取multirate属性
        size_t GetRate() const
        {
            return m_iRate;
        }
        //用于设置可以被CircularBuffer索引的样本总数
        //[0]将指向最原始样本。默认情况下，HistoryDepth等于multi-rate value，只允许在Setup方法中调用。
        void SetHistoryDepth(size_t iHistoryDepth)
        {
            m_iHistoryDepth = iHistoryDepth;
        }
        //获取样本总数
        size_t GetHistoryDepth() const
        {
            return m_iHistoryDepth;
        }
        /// The Copy method copies iNumberItems from pSource to pDestination.
        //将pSource缓冲区指向的内容复制iNumberItems个项数，到pDestination缓冲区
        virtual void Copy(void* pSource, void* pDestination, size_t iNumberItems) const = 0;
        /// The Copy method copies iNumberItems from this buffer (starting at index iFrom) to pDestination (start at index iTo).
        //将从iFrom索引的位置将iNumberItems个项数，复制到pDestination缓冲区中iTo索引的位置
        virtual void Copy(size_t iFrom, CircularBufferBase* pDestination, size_t iTo, size_t iNumberItems) const = 0;
        /// The Zero method zeros the specified number of items from this buffer starting at the specified index.
        //从iStart索引的位置将缓冲区中iNumberItems指定的项数归零
        //pReference:一个指向引用数据的指针，用于复制定点精度、矩阵维数和载波频率等属性
        virtual bool Zero(size_t iStart, size_t iNumberItems, void* pReference) = 0;
        /// The Initialize method will set all of the items in the buffer to zero.
        //设置所有项为0
        void Initialize()
        {
            Zero(0,m_iSize,nullptr);
        }
        //以指定m_iRate的速率增加缓冲区的m_iccurrent位置中的项数
        //每次DFModel::Run()被调用后，该方法被所有DFPort调用`
        inline void Advance()
        {
            //_ASSERT( m_pBuffer); // Why are we advancing a pointer on a buffer that contains no data?
            if(m_pBuffer) {
                m_iCurrent = IndexAt(m_iRate);
            }
        };
        /// The SetContiguousProperty declares that the circular buffer will support reading/writing contiguous sections in the buffer.
        //声明允许缓冲区读写缓冲区中的连续部分
        void SetContiguousProperty()
        {
            if( m_pBuffer == nullptr) return; // Must be set before the memory is allocated

            m_bContiguousBuffer = true;
        }
        bool GetContiguousProperty() const
        {
            return m_bContiguousBuffer;
        }
        /// The GetReadPtr method is used to get a pointer to read from in a contiguous manner.
        //获取一个连续读取的指针
        void* GetReadPtr()
        {
            if(!m_bContiguousBuffer) return nullptr; // Must have set the contiguous buffer parameter in your Setup method

            // Determine where the the contiguous buffer ends
            size_t iEnd = m_iCurrent + m_iRate;

            // If it ends after the end of the circular buffer, we must copy the data
            if ( iEnd > m_iSize)
                Copy( m_pBuffer, GetPointer(m_iSize-m_iCurrent), iEnd - m_iSize);

            return GetPointer(0);
        }
        /// The GetWritePtr method is used to get a pointer to write to in a contiguous manner.
        //获取一个连续写入的指针
        void* GetWritePtr()
        {
            if(!m_bContiguousBuffer) return nullptr; // Must have set the contiguous buffer parameter in your Setup method
            return GetPointer(0);
        }
        /// The Write method copies data from the contiguous buffer to the circular buffer.
        //将数据从连续缓冲区复制到循环缓冲区
        void Write()
        {
            if(!m_bContiguousBuffer) return; // Must have set the contiguous buffer parameter in your Setup method

            size_t iEnd = m_iCurrent + m_iRate;

            if ( iEnd > m_iSize)
                Copy( GetPointer(m_iSize-m_iCurrent), m_pBuffer, iEnd - m_iSize );
        }
        /// Get a pointer to the buffer at the specified index
        //获取指向iIndex索引处的指针
        void* GetPointer(size_t iIndex)
        {
            if(!m_pBuffer) return nullptr; // The buffer must be allocated before you can call this method
            return static_cast<char*>(m_pBuffer) + (m_iSizeOf * IndexAt(iIndex));
        }
        /// Get the pointer to the start of the circular buffer memory.
        //获取指向循环缓冲区开始的指针
        void* GetBufferMemory()
        {
            return m_pBuffer;
        }
        bool IsConnected()
        {
            return m_bIsConnected;
        }
        /// Declare if the circular buffer is associated with a connected port
        //声明如果循环缓冲区已经连接到一个端口
        void SetConnected( bool bConnected = true)
        {
            m_bIsConnected = bConnected;
        }
        size_t GetCurrentIndex() const { return m_iCurrent; }
        size_t GetSizeOf() const { return m_iSizeOf; }
        size_t GetSize() const
        {
            return m_iSize;
        }
        // Function to return index taking into account the circular nature of the buffer
        //返回索引（根据循环性质）
        inline size_t IndexAt(size_t iIndex) const
        {
            if(m_iSize == 0) return 0; // We don't use modulo to wrap-around for efficiency - your iIndex must be < 2*m_iSize
            if ( iIndex ==0)
            {
                iIndex = m_iCurrent;
            }
            else
            {
                iIndex = m_iCurrent + iIndex;
                if ( iIndex >= m_iSize)
                {
                    iIndex -= m_iSize;

                    //_ASSERT( iIndex < m_iSize); // Circular indexing only works for one wraparound on the circular buffer

                }
            }
            return iIndex;
        };
        /// The ResizeMemory method should only be used if you want to have a buffer that is owned completely by your model (i.e. it is not used as a port).
        //完全由自己定义的模型控制的缓冲区时使用该方法
        bool ResizeMemory(size_t iNumberItems, bool bInitializeBuffer, void* pZeroReference =0)
        {
            if( iNumberItems <= 0) return false;  // If you want to deallocate memory... use the DeallocateMemory method.

            // Only create a new buffer if the size is increasing
            if ( iNumberItems > 0 && iNumberItems > m_iSize)
            {
                // Allocate memory for new buffer memory
                void* pNewBuffer = AllocateMemory( iNumberItems);

                // Copy old memory contents to new buffer memory
                if ( m_iSize > 0)
                {
                    // Copy the beginning part of the buffer memory
                    Copy( m_pBuffer, pNewBuffer, m_iCurrent );

                    if ( m_iCurrent < m_iSize)
                    {
                        // Copy the end of the buffer memory
                        size_t iNumToCopy = m_iSize - m_iCurrent;
                        Copy( ((char*)m_pBuffer)+m_iCurrent, ((char*)pNewBuffer)+iNumberItems-iNumToCopy,iNumToCopy);
                    }

                    // Deallocate old buffer memory
                    DeallocateMemory();
                }

                {
                    size_t iNumToInitialize = iNumberItems - m_iSize;

                    // Set the circular buffer to new buffer memory
                    m_pBuffer = pNewBuffer;
                    m_iSize = iNumberItems;

                    // Zero out the buffer
                    if ( bInitializeBuffer)
                        Zero( m_iCurrent, iNumToInitialize, pZeroReference);
                }
            }

            return true;
        }
        /// The AllocateMemory method is used to
        /// <param name="iNumberItems">The size of the memory.</param>
        /// <remarks>CircularBuffers do not manage their own memory as the memory is typically shared.  If you call AllocateMemory, you must call DeallocateMemory before the CircularBuffer is deleted.</remarks>
        virtual void* AllocateMemory(size_t iNumberItems) = 0;
        /// If the memory is allocated using AllocateMemory, DeallocateMemory needs to be called to deallocate it.
        /// <param name="pBuffer">Pointer to the buffer to deallocate, if 0 the memory of the CircularBuffer will be deleted.</param>
        /// <remarks>To delete the CircularBuffer memory, do not set the pBuffer parameter.</remarks>
        virtual void DeallocateMemory(void* pBuffer = 0)= 0;
        protected:
            size_t m_iRate;
            size_t m_iHistoryDepth;
            bool m_bContiguousBuffer;
            size_t m_iSizeOf;
            size_t m_iCurrent;
            void* m_pBuffer;
            size_t m_iSize;
            bool m_bIsConnected;
    };
    /// The CircularBuffer template class allows your models to make efficient use of circular buffers
    /// for input and output ports of types that can use memcpy (shallow copy).
    /// typedefs for the most commonly used types (bool, int, float, double, std::complex<float>,
    /// std::complex<double>) have been defined at the end of this file.
    /// It is highly recommended to use these typedefs when possible.
    template <typename T> class CircularBuffer : public CircularBufferBase
    {
    public:
        CircularBuffer() : CircularBufferBase()
        {
            m_iSizeOf = sizeof(T);
        }
        /// The Copy method copies iNumberItems from pSource to pDestination.
        virtual void Copy(void* pSource, void* pDestination, size_t iNumberItems) const
        {
            _ASSERT( pSource && pDestination && iNumberItems > 0);
            if (!pSource || !pDestination || iNumberItems == 0) return;
            T* pDst = reinterpret_cast<T*>(pDestination);
            T* pSrc = reinterpret_cast<T*>(pSource);


            //memcpy(pDst,pSrc, iNumberItems* sizeof(T));
            std::copy(pSrc, pSrc + iNumberItems, pDst);
        }
        /// The Copy method copies iNumberItems from this buffer (starting at index iFrom) to a pDestination.
        void Copy(size_t iFrom, T* pDestination, size_t iNumberItems) const
        {
            _ASSERT( pDestination && iNumberItems <= m_iSize);
            if (!pDestination || iNumberItems == 0) return;

            //const T* pSource = (const T*)m_pBuffer;
            const T* pSource = static_cast<const T*>(m_pBuffer);

            iFrom = IndexAt(iFrom);

            size_t itemsCopied = 0;
            while ( itemsCopied < iNumberItems )
            {
                size_t available = m_iSize - iFrom;
                size_t toCopy = std::min(iNumberItems - itemsCopied, available);

                std::copy(&pSource[iFrom],&pSource[iFrom + toCopy],&pDestination[itemsCopied]);

                itemsCopied += toCopy;
                iFrom = (iFrom + toCopy) % m_iSize;
            }
        }
        /// The CopyFrom method copies iNumberItems into this buffer (starting at index iTo) to a pSource.
        void CopyFrom(size_t iTo, const T* pSource, size_t iNumberItems)
        {
            if( !pSource || iNumberItems == 0 || iTo >= m_iSize) return;

            T* pDestination = (T*) m_pBuffer;
            iTo = IndexAt(iTo);

            size_t iFrom = 0;
            if( iNumberItems == 1) {
                pDestination[iTo] = pSource[iFrom];
            }
            else
            {
                while ( iNumberItems>0)
                {
                    size_t iCopy = std::min( iNumberItems, m_iSize - iTo);
                    memcpy( &pDestination[iTo], &(pSource[iFrom]), iCopy*sizeof(T));
                    iFrom += iCopy;
                    iTo += iCopy;
                    if ( iTo == m_iSize) iTo = 0;
                    iNumberItems -= iCopy;
                }
            }
        }
        /// The Copy method copies iNumberItems from this buffer (starting at index iFrom) to pDestination (start at index iTo).
        void Copy(size_t iFrom, CircularBufferBase* pDestination, size_t iTo, size_t iNumberItems) const
        {
            _ASSERT( dynamic_cast<CircularBuffer<T>*>(pDestination));  // Copy can only be done on circular buffers of the same class

            _ASSERT( iNumberItems <= m_iSize && iNumberItems <= pDestination->GetSize());  // Should only copy less items than either buffer can hold

            if (!pDestination || iNumberItems == 0) return;

            CircularBuffer<T>* destBuffer = dynamic_cast<CircularBuffer<T>*>(pDestination);
            if(!destBuffer) return;

            const T* pFrom = static_cast<const T*>(m_pBuffer);
            T* pTo = static_cast<T*>(pDestination->GetBufferMemory());
            iFrom = IndexAt(iFrom);
            iTo = destBuffer->IndexAt(iTo);

            size_t itemsCopied = 0;
            while(itemsCopied < iNumberItems) {
                size_t availFrom = m_iSize - iFrom;
                size_t availTo = destBuffer->GetSize() - iTo;
                size_t toCopy = std::min({iNumberItems - itemsCopied, availFrom, availTo});

                std::copy(&pFrom[iFrom], &pFrom[iFrom + toCopy], &pTo[iTo]);

                itemsCopied += toCopy;
                iFrom = (iFrom + toCopy) % m_iSize;
                iTo = (iTo + toCopy) % destBuffer->GetSize();
            }
        }
        /// The Zero method zeros the specified number of items from this buffer starting at the specified index.
        bool Zero(size_t iStart, size_t iNumberItems = 1, void* pReference = 0)
        {
            (void) pReference;  // unused
            if( iStart > m_iSize || iNumberItems == 0) {
                return false;
            }

            T* buffer = static_cast<T*>(m_pBuffer);
            iStart = IndexAt(iStart);

            size_t itemsZeroed = 0;
            while(itemsZeroed < iNumberItems) {
                size_t available = m_iSize - iStart;
                size_t toZero = std::min(iNumberItems - itemsZeroed, available);

                std::fill(&buffer[iStart], &buffer[iStart + toZero], T(0));

                itemsZeroed += toZero;
                iStart = (iStart + toZero) % m_iSize;
            }
            return true;
        }

        /// Reading and writing to the buffer locations is done in the same way as you would with a <c>double*</c>.
        inline T& operator[](size_t iIndex)
        {
            _ASSERT( m_pBuffer);   // Should have been set by call to SetBuffer
            if(!m_pBuffer) {
                static T dummy;
                return dummy;
            }
            return reinterpret_cast<T*>(m_pBuffer)[IndexAt(iIndex)];
        }
        const T& operator[](size_t iIndex) const
        {
            if (!m_pBuffer)
            {
                static T dummy;
                return dummy;
            }
            return static_cast<const T*>(m_pBuffer)[IndexAt(iIndex)];
        }
        /// The AllocateMemory method is used to
        /// <param name="iNumberItems">The size of the memory.</param>
        /// <remarks>CircularBuffers do not manage their own memory as the memory is typically shared.  If you call AllocateMemory, you must call DeallocateMemory before the CircularBuffer is deleted.</remarks>
        void* AllocateMemory(size_t iNumberItems)
        {
            if(iNumberItems == 0) return nullptr;
            return new T[iNumberItems];
        }

        /// If the memory is allocated using AllocateMemory, DeallocateMemory needs to be called to deallocate it.
        /// <param name="pBuffer">Pointer to the buffer to deallocate, if 0 the memory of the CircularBuffer will be deleted.</param>
        /// <remarks>To delete the CircularBuffer memory, do not set the pBuffer parameter.</remarks>
        virtual void DeallocateMemory(void* pBuffer = 0)
        {
            _ASSERT( pBuffer != m_pBuffer); // If you want to delete circular buffer memory, do not set the pBuffer parameter.

            if ( pBuffer)
            {
                delete [] static_cast<T*>(pBuffer);
            }
            else
            {
                delete [] static_cast<T*>(m_pBuffer);
                m_pBuffer = nullptr;
                m_iSize = m_iCurrent = 0;
            }
        }
        void SetBuffer(void* pBuffer, size_t iNumberItems, size_t iAdvanceRate = 1, size_t iStartLocaiton = 0)
        {
            CircularBufferBase::SetBuffer(pBuffer, iNumberItems, iAdvanceRate, iStartLocaiton);
        }

    };
    template <typename T> class CircularBufferE : public CircularBufferBase
    {
    public:
        CircularBufferE() : CircularBufferBase()
        {
            m_iSizeOf = sizeof(T);
        }
        /// The Copy method copies iNumberItems from pSource to pDestination.
        void Copy(void* pSource, void* pDestination, size_t iNumberItems) const
        {
            if( !pSource || !pDestination || iNumberItems == 0) return;

            size_t i;
            T* pDest = (T*)pDestination;
            T* pSrc = (T*)pSource;
            for( i = 0; i < iNumberItems; i++)
                *(pDest++) = *(pSrc++);
        }
        /// The Copy method copies iNumberItems from this buffer (starting at index iFrom) to a pDestination.
        void Copy(size_t iFrom, T* pDestination, size_t iNumberItems) const
        {
            if( !pDestination || iNumberItems > m_iSize || iFrom >= iNumberItems) return;

            size_t i;
            for( i = iFrom; i < iNumberItems; i++)
                pDestination[i] = (*this)[iFrom + i];
        }
        /// The CopyFrom method copies iNumberItems into this buffer (starting at index iTo) to a pSource.
        void CopyFrom(size_t iTo, const T* pSource, size_t iNumberItems)
        {
            if( !pSource || iNumberItems == 0 || iTo >= m_iSize) return;

            size_t i;
            for(i = iTo; i < iNumberItems; i++)
                (*this)[i] = *(pSource++);
        }
        /// The Copy method copies iNumberItems from this buffer (starting at index iFrom) to pDestination (start at index iTo).
        void Copy(size_t iFrom, CircularBufferBase* pDestination, size_t iTo, size_t iNumberItems) const
        {
            if( !dynamic_cast<CircularBufferE<T>*>(pDestination)) return;  // Only copy between buffers that are of the same type
            if( iNumberItems > m_iSize || iNumberItems > pDestination->GetSize()) return;

            CircularBufferE<T>* pDest = (CircularBufferE<T>*)pDestination;
            const CircularBufferE<T>* pSrc = this;
            while(iNumberItems--) {
                (*pDest)[iTo++] = (*pSrc)[iFrom++];
            }
        }
        /// The Zero method zeros the specified number of items from this buffer starting at the specified index.
        bool Zero(size_t iStart, size_t iNumberItems, void* pReference)
        {
            if( iNumberItems > m_iSize || iStart >= m_iSize ) return false;

            CircularBufferE<T>& buf = *this;
            bool bSuccess = true;
            while( iNumberItems-- && bSuccess) {
                bSuccess = buf[iStart++].Zero((T*)pReference);
            }
            return bSuccess;
        }
        /// Reading and writing to the buffer locations is done in the same way as you would with a <c>double*</c>.
        inline T& operator[](size_t iIndex)
        {
            if( iIndex >= m_iSize || !m_pBuffer) {
                static T dummy;
                return dummy;  // 返回默认值
            }
            _ASSERT( m_pBuffer);				// Should have been set in SetBuffer
            return ((T*)m_pBuffer)[IndexAt(iIndex)];
        }

        /// Reading and writing to the buffer locations is done in the same way as you would with a <c>double*</c>.
        inline const T& operator[](size_t iIndex) const
        {
            if( iIndex >= m_iSize || !m_pBuffer) {
                static T dummy;
                return dummy;  // 返回默认值
            }
            _ASSERT( m_pBuffer);				// Should have been set in SetBuffer
            return ((T*)m_pBuffer)[IndexAt(iIndex)];
        }
        /// The AllocateMemory method is used to
        /// <param name="iNumberItems">The size of the memory.</param>
        /// <remarks>CircularBuffers do not manage their own memory as the memory is typically shared.  If you call AllocateMemory, you must call DeallocateMemory before the CircularBuffer is deleted.</remarks>
        void* AllocateMemory(size_t iNumberItems)
        {
            if( iNumberItems <= 0) return nullptr;
            return new T[iNumberItems];
        }
        /// If the memory is allocated using AllocateMemory, DeallocateMemory needs to be called to deallocate it.
        /// <param name="pBuffer">Pointer to the buffer to deallocate, if 0 the memory of the CircularBuffer will be deleted.</param>
        /// <remarks>To delete the CircularBuffer memory, do not set the pBuffer parameter.</remarks>
        virtual void DeallocateMemory(void* pBuffer = 0)
        {
            if( pBuffer == m_pBuffer) return;
            if( pBuffer){
                delete [] (T*) pBuffer;
            }
            else {
                delete [] (T*) m_pBuffer;
                m_pBuffer = 0;
                m_iSize = m_iCurrent = 0;
            }
        }
    };
    class CircularBufferBus
    {
    public:
        /// The Initialize method resizes the bus.  This is called by SystemVue based on the number of connections.
        virtual void Initialize(size_t iNumberItems) = 0;
        /// Delete all of the bus circular buffers
        virtual ~CircularBufferBus() {}
        size_t GetSize() const {return m_iSize;}
        /// The Get method returns the CircularBuffer at the specified index.
        virtual CircularBufferBase* Get(size_t iIndex) = 0;
    protected:
        size_t m_iSize;
    };

    /// The CircularBufferBusT template class allows your models to have bus ports of specific types.
    /// typedefs for the most commonly used types have been defined at the end of this file as well as
    /// at the end of the file MatrixCircularBuffer.h
    /// It is highly recommended to use these typedefs when possible.
    template <typename T> class CircularBufferBusT : public CircularBufferBus
    {
    public:
        CircularBufferBusT()
        {
            m_pCirBufBus = 0;
            m_iSize = 0;
        }
        virtual ~CircularBufferBusT() { delete[] m_pCirBufBus;}
        void Initialize(size_t iSize)
        {
            if(m_iSize != iSize) {
                delete[] m_pCirBufBus;
                m_pCirBufBus = nullptr;

                m_iSize = iSize;
                if(m_iSize) m_pCirBufBus = new T[m_iSize];
            }
        }
        /// The Get method returns the CircularBuffer at the specified index.
        const T& operator[](size_t iIndex) const
        {
            _ASSERT( iIndex < m_iSize);
            if( iIndex >= m_iSize) {
                static T dummy;
                return dummy;  // 返回默认值
            }
            return m_pCirBufBus[iIndex];
        }
        T& operator[](size_t iIndex)
        {
            _ASSERT( iIndex < m_iSize);
            if( iIndex >= m_iSize) {
                static T dummy;
                return dummy;  // 返回默认值
            }
            return m_pCirBufBus[iIndex];
        }
    private:
        /// The GetCircularBufferBase method returns the CircularBuffer at the specified index.  You should use the Get() method instead in your model code.
        CircularBufferBase* Get(size_t iIndex) {
            _ASSERT( iIndex < m_iSize );
            if( iIndex >= m_iSize) {
                return nullptr;  // 返回默认值
            }
            return &(*this)[iIndex];
        }
        T* m_pCirBufBus;
    };
    /* ********************************************************************** */
    // Bool
    /* ********************************************************************** */
    /// Circular buffer for bool data type
    typedef CircularBuffer<bool> BoolCircularBuffer;
    /// Circular buffer bus for bool data type
    typedef CircularBufferBusT<BoolCircularBuffer> BoolCircularBufferBus;


    /* ********************************************************************** */
    // Char
    /* ********************************************************************** */
    /// Circular buffer for char data type
    typedef CircularBuffer<char> CharCircularBuffer;
    /// Circular buffer bus for char data type
    typedef CircularBufferBusT<CharCircularBuffer> CharCircularBufferBus;


    /* ********************************************************************** */
    // Integer
    /* ********************************************************************** */
    /// Circular buffer for integer data type
    typedef CircularBuffer<int> IntCircularBuffer;
    /// Circular buffer bus for integer data type
    typedef CircularBufferBusT<IntCircularBuffer> IntCircularBufferBus;


    /* ********************************************************************** */
    // Double
    /* ********************************************************************** */
    /// Circular buffer for double data type
    typedef CircularBuffer<double> DoubleCircularBuffer;
    /// Circular buffer bus for integer data type
    typedef CircularBufferBusT<DoubleCircularBuffer> DoubleCircularBufferBus;


    /* ********************************************************************** */
    // Complex double
    /* ********************************************************************** */
    /// Circular buffer for complex double data type
    typedef CircularBuffer<std::complex<double > > DComplexCircularBuffer;
    /// Circular buffer bus for integer data type
    typedef CircularBufferBusT<DComplexCircularBuffer> DComplexCircularBufferBus;

    /* ********************************************************************** */
    // Float
    /* ********************************************************************** */
    /// Circular buffer for float data type
    typedef CircularBuffer<float> FloatCircularBuffer;
    /// Circular buffer bus for integer data type
    typedef CircularBufferBusT<FloatCircularBuffer> FloatCircularBufferBus;

    /* ********************************************************************** */
    // Complex float
    /* ********************************************************************** */
    /// Circular buffer for complex float data type
    typedef CircularBuffer<std::complex<float > > FComplexCircularBuffer;
    /// Circular buffer bus for integer data type
    typedef CircularBufferBusT<FComplexCircularBuffer> FComplexCircularBufferBus;

}
#endif // CIRCULARBUFFER_H
