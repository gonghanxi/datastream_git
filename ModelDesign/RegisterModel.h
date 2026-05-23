#ifndef REGISTERMODEL_H
#define REGISTERMODEL_H
#pragma once

#define DFInterfaceVersion 8


// 跨平台调试头文件处理
#ifdef _WIN32
    #include <crtdbg.h>  // Windows 调试头文件
#else
    // Linux 下模拟必要的调试功能
    #ifdef DEBUG
        #include <iostream>
        #define _ASSERT(expr) do { if (!(expr)) std::cerr << "Assertion failed: " #expr << std::endl; } while(0)
        #define _ASSERTE(expr) _ASSERT(expr)
        #define _RPT0(a, b) std::cerr << b << std::endl
        #define _RPT1(a, b, c) std::cerr << b << c << std::endl
        #define _RPT2(a, b, c, d) std::cerr << b << c << d << std::endl
    #else
        #define _ASSERT(expr) ((void)0)
        #define _ASSERTE(expr) ((void)0)
        #define _RPT0(a, b) ((void)0)
        #define _RPT1(a, b, c) ((void)0)
        #define _RPT2(a, b, c, d) ((void)0)
    #endif
#endif
//#include <string>
//#include <memory>
#ifdef _MSC_VER
    #pragma warning(disable:4273)
#endif

namespace SystemVueModelBuilder {
    class DFModel;
    typedef DFModel* (*pfnDFModelCreate) (const char** pccClassName);
    //helper类-前向声明
    class RegisterModelImplementation;
    //前向声明
    class ModelFactory;
    ///CRegisterModel类是下面的RegisterModel模板化类的helper类，不能直接使用这个类。
    class CRegisterModel
    {
    public:
        CRegisterModel(pfnDFModelCreate funcPtr, int iInterfaceVersion, const char* pLibName );
        CRegisterModel(ModelFactory* pModelFactory, const char* pClassName, int iInterfaceVersion, const char* pLibName );
        ~CRegisterModel();
    private:
        RegisterModelImplementation* m_pImplementation;
    };
// The _REGISTER_MODEL_QUOTE_LIBNAME(x) and REGISTER_MODEL_QUOTE_LIBNAME(x) macros are used by the RegisterModel class below
#ifndef SV_CODE_GEN
#if defined(SV_MODEL_BUILDER)
#define _REGISTER_MODEL_QUOTE_LIBNAME(x) #x
#define REGISTER_MODEL_QUOTE_LIBNAME(x) _REGISTER_MODEL_QUOTE_LIBNAME(x)
#else
#define REGISTER_MODEL_QUOTE_LIBNAME(x) ""
#endif
#endif

///RegisterModel模板化类用于将模型注册到SystemVue中。它由DEFINE_MODEL_INTERFACE宏自动定义。
    template <typename T_DFMODEL_CLASS> class RegisterModel
    {
        public:
            RegisterModel( const char* pccClassName = 0)
            {
                const char* pLibName = "";
        #ifndef SV_CODE_GEN
        #ifdef LIBNAME
                pLibName = REGISTER_MODEL_QUOTE_LIBNAME(LIBNAME);
        #endif
        #endif

                // Must define m_pccClassName before CRegisterModel is created below as it is used in the Create method
                m_pccClassName = pccClassName;

                // Now register the model
                m_pCRegisterModel = new CRegisterModel(Create, DFInterfaceVersion, pLibName);
            }
            ~RegisterModel()
            {
                delete m_pCRegisterModel;
            }

            /// Create method is called by SystemVue to create the method - it will declare the DFModel class name to SystemVue.
            /// Create方法由SystemVue调用来创建方法——它将向SystemVue声明DFModel类名
            static DFModel* Create(const char** pccClassName)
            {
                (*pccClassName) = m_pccClassName;
                return new T_DFMODEL_CLASS;
            }

            CRegisterModel* m_pCRegisterModel;
            static const char* m_pccClassName;
    };
    /// Static const char* data member of the RegisterModel class
    template <class T> const char* RegisterModel<T> ::m_pccClassName = 0;
    }
#endif // REGISTERMODEL_H
