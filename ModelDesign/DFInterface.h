#ifndef DFINTERFACE_H
#define DFINTERFACE_H
#pragma once



#include "DFPort.h"
#include "DFParam.h"
#include "DFEnumerations.h"
#include "SimulationControl.h"

//#include "CircularBuffer.h"
#include "FixedPointEnums.h"
#include "Matrix.h"

#include "CDFInterfaceImplementation.h"

#include <complex>
#ifndef SV_CODE_GEN


namespace SystemVueModelBuilder {
    class CDFInterfaceImplementation;
    class DFInterface
    {
    private:
        //实现类指针，隐藏细节
        CDFInterfaceImplementation* m_cImplementation;
        DFParam AddParamEnum(int* pidata, const char* pcCodeGenName, const char* pcEnumType);
    public:


        DFInterface();
        explicit DFInterface(const std::string& configKey);
        ~DFInterface();

        ///用ADD_MODEL( model_name)宏添加模型
        void AddModel(const char* pcModelName);
        ///用SET_DEFAULT_MODEL( model_name)宏添加默认模型
        void SetDefaultModel(const char *pcModelName);
        ///用SET_MODEL_DESCRIPTION( model_description )宏添加模型描述
        void SetModelDescription(const char *pcDescription);
        ///用SET_MODEL_CATEGORY( model_category )宏添加模型类别
        void SetModelCategory(const char *pcCategory);
        ///用SET_MODEL_SYMBOL( model_symbol_name )宏设置模型符号
        void SetModelSymbol(const char *pcSymbolName);
        ///用SET_MODEL_NAME( model_name )宏设置模型名称
        void SetModelName(const char *pcName);
        ///禁用模型的自动零件生成
        void DisablePartGeneration();
        ///禁用模型和零件生成
        void DisablePartAndModelGeneration();
        ///允许模型和零件生成
        void EnablePartAndModelGeneration();
        ///禁止C++代码生成
        void DisableCppCodeGeneration();
        ///启用自定义UI来隐藏I/O端口
        void EnableHidingIO();
        ///启用动态I/O端口
        void EnableHidingIOFromParam();
        ///用SET_CUSTOM_UI( custom_ui_name )宏设置自定义UI
        void SetCustomUI( const char* pcCustomUIName );
        ///用SET_MODEL_NAMESPACE( model_namespace )宏设置模型的命名空间
        void SetModelNamespace( const char* pcNamespace );
        ///设置用于在代码生成期间实例化此模型的名称
        void SetModelCodeGenName( const char *pcName );

        ///REGISTER_SIMULATION_CONTROL( sim_control_variable )宏注册仿真控制对象的模型
        void RegisterSimulationControl( const SinkControl& sinkControl, const char *pcCodeGenName );
        void RegisterSimulationControl( const DynamicControl& dynamicControl, const char *pcCodeGenName );

        ///ADD_MODEL_HEADER_FILE( header_file )设置头文件
        void AddModelHeaderFile( const char* pcHeaderFile );
        ///宏设置源文件
        void AddModelSourceFile( const char* pcSourceFile );

        ///添加double类型的参数
        DFParam AddParam(double &dData, const char *pcCodeGenName);
        DFParam AddParam(float &dData, const char *pcCodeGenName);
        DFParam AddParamArray(double *&pdData, int &iSize,const char *pcCodeGenName, const char* pcSizeName);
        ///添加int类型的参数
        DFParam AddParam(int &iData,const char *pcCodeGenName);
        DFParam AddParamArray(int *&piData, int &iSize,const char *pcCodeGenName, const char* pcSizeName);

        DFParam AddParam(std::complex<float> &cComplexData,const char *pcCodeGenName);
        DFParam AddParam(std::complex<double> &cComplexData,const char *pcCodeGenName);
        DFParam AddParamArray(std::complex<double> *&pcComplexData, int &iSize,const char *pcCodeGenName, const char* pcSizeName);
        DFParam AddParam(char  *&sData,const char *pcCodeGenName);

        template <typename T> DFParam AddParamEnum(T &eEnumData, const char *pcCodeGenName, const char *pcEnumType)
        {
            // Only Enum of size int are supported
            bool enumHasCorrectSize = ( sizeof(T) == sizeof(int) );
            _ASSERT( enumHasCorrectSize );

            if(enumHasCorrectSize)
            {
                int *data;
                data = (int *)&eEnumData;
                return AddParamEnum(data,pcCodeGenName,pcEnumType);
            }
            return DFParam(NULL);
        }

        /// * QUERY_NO
        /// * QUERY_YES
        DFParam AddParam(QueryEnum  &qData,const char *pcCodeGenName);
        /// * BOOLEAN_FALSE
        /// * BOOLEAN_TRUE
        DFParam AddParam(BooleanEnum &bData,const char *pcCodeGenName);
        /// * SWITCH_OFF
        /// * SWITCH_ON
        DFParam AddParam(SwitchEnum &sData,const char *pcCodeGenName);

        DFParam AddParamFile(const char* pcName, const char *pcDescription, char  *&sData,const char* pcValue="");

        DFParam AddParam(bool &bData,const char *pcCodeGenName);
        //矩阵类参数
        DFParam AddParam( Matrix < std::complex<float> > &cComplexMatrixData,const char *pcCodeGenName);
        DFParam AddParam( Matrix < std::complex<double> > &cComplexMatrixData,const char *pcCodeGenName);
        DFParam AddParam( Matrix < int > &cIntMatrixData,const char *pcCodeGenName);
        DFParam AddParam( Matrix < float > &cFloatMatrixData,const char *pcCodeGenName);
        DFParam AddParam( Matrix < double> &cDoubleMatrixData,const char *pcCodeGenName);
        DFParam AddParam( Matrix < bool> &cBoolMatrixData,const char *pcCodeGenName);

        //模式枚举类参数
        DFParam AddParam(FixedPointEnums::Sign &sData,const char *pcCodeGenName);
        DFParam AddParam(FixedPointEnums::QuantizationMode &sData,const char *pcCodeGenName);
        DFParam AddParam(FixedPointEnums::OverflowMode &sData,const char *pcCodeGenName);

        //基础类型端口添加
        DFPort AddInput(int &iData,const char *pcCodeGenName);
        DFPort AddOutput(int &iData,const char *pcCodeGenName);
        DFPort AddInput(double &dData,const char *pcCodeGenName);
        DFPort AddOutput(double &dData,const char *pcCodeGenName);
        DFPort AddInput(std::complex<double> &ComplexData,const char *pcCodeGenName);
        DFPort AddOutput(std::complex<double> &ComplexData,const char *pcCodeGenName);
        /// \name Array I/O
        /// 数组类型端口添加
        DFPort AddInput(int *&iData,const char *pcCodeGenName);
        DFPort AddOutput(int *&iData,const char *pcCodeGenName);
        DFPort AddInput(double *&dData,const char *pcCodeGenName);
        DFPort AddOutput(double *&dData,const char *pcCodeGenName);
        DFPort AddInput(std::complex<double> *&ComplexData,const char *pcCodeGenName);
        DFPort AddOutput(std::complex<double> *&ComplexData,const char *pcCodeGenName);
        /// \name Circular Buffer I/O
//        DFPort AddInput( CircularBufferBase& circularBuffer, const char *pcCodeGenName);
//        DFPort AddOutput( CircularBufferBase& circularBuffer, const char *pcCodeGenName);
        DFPort AddInput( CircularBuffer<int>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput( CircularBuffer<int>& circularBuffer, const char *pcCodeGenName);
        DFPort AddInput( CircularBuffer<double>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput( CircularBuffer<double>& circularBuffer, const char *pcCodeGenName);
        DFPort AddInput( CircularBuffer<bool>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput( CircularBuffer<bool>& circularBuffer, const char *pcCodeGenName);
        DFPort AddInput( CircularBuffer<float>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput( CircularBuffer<float>& circularBuffer, const char *pcCodeGenName);
        DFPort AddInput( CircularBuffer<std::complex<float>>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput( CircularBuffer<std::complex<float>>& circularBuffer, const char *pcCodeGenName);
        DFPort AddInput( CircularBuffer<std::complex<double>>& circularBuffer, const char *pcCodeGenName);
        DFPort AddOutput( CircularBuffer<std::complex<double>>& circularBuffer, const char *pcCodeGenName);
        DFPort AddInput( CircularBufferBus& circularBufferBus, const char *pcCodeGenName);
        DFPort AddOutput( CircularBufferBus& circularBufferBus, const char *pcCodeGenName);
        /// \name Envelope Signal I/O
        DFPort AddInput( EnvelopeCircularBuffer& envelopeData, const char *pcCodeGenName);
        DFPort AddOutput( EnvelopeCircularBuffer& envelopeData, const char *pcCodeGenName);

        /// \name Matrix I/O
        DFPort AddInput(IntMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(IntMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddInput(DoubleMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(DoubleMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddInput(FloatMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(FloatMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddInput(BoolMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(BoolMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddInput(FComplexMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(FComplexMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddInput(DComplexMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddOutput(DComplexMatrixCircularBuffer& circularBuffer, const char* pcCodeGenName);
        DFPort AddInput(EnvelopeMatrixCircularBuffer& envelopeData, const char* pcCodeGenName);
        DFPort AddOutput(EnvelopeMatrixCircularBuffer& envelopeData, const char* pcCodeGenName);


        /// For internal use only. A C++ Data Flow model must not use this at all
        const char * GetLastError();

        /// For internal use only. A C++ Data Flow model must not use this at all
        CDFInterfaceImplementation* GetImplementation() const;

        //配置管理方法
        bool SaveToConfig(ConfigManager& externalManager);
        bool LoadFromConfig();
        bool RemoveFromConfig();
        bool HasConfig();
    };

#endif
    }
#endif // DFINTERFACE_H
