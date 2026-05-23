#ifndef MODELBUILDER_H
#define MODELBUILDER_H

#include <vector>
#include "RegisterModel.h"
#include "Matrix.h"
#include "DFModel.h"
#include "FixedPointEnums.h"
#include "DFInterface.h"
#include "DFErrorHandler.h"
#include "MatrixCircularBuffer.h"

//------------------------------------------------------------------------------
// The macro DECLARE_MODEL_INTERFACE( ModelClass ) must be included in the
// header file for C++ Data Flow model class under public methods.
// This will declare methods needed to create interface between
// the simulator and Data Flow model class
//
// For example in C++ Data Flow model class named MyClass add
//		public:
//			DECLARE_MODEL_INTERFACE(MyClass)
//------------------------------------------------------------------------------
#ifndef SV_CODE_GEN
#define DECLARE_MODEL_INTERFACE( ModelClass ) \
virtual bool DefineInterface( SystemVueModelBuilder::DFInterface &model ) override; \
virtual const char* GetModelClassName() const override { return #ModelClass; }
#else
#define DECLARE_MODEL_INTERFACE( ModelClass )
#endif
//------------------------------------------------------------------------------
// The macro DEFINE_MODEL_INTERFACE( ModelClass ) must be used in the
// cpp file for C++ Data Flow model class to specify/define the interface
// between C++ Data Flow model and the simulator.
//
// For example in C++ Data Flow model class named MyClass
//			DEFINE_MODEL_INTERFACE(MyClass)
//			{
//			  // add the interface code here
//			}
//------------------------------------------------------------------------------
#ifndef SV_CODE_GEN
#ifndef SV_NO_MODEL_REGISTRATION
#define DEFINE_MODEL_INTERFACE( ModelClass ) \
\
    const char* __DFModel_Class_Name__##ModelClass = #ModelClass; \
    \
    SystemVueModelBuilder::DFModel* __DFModel_Class_Create__##ModelClass(const char** pccClassName) \
{ \
        SystemVueModelBuilder::DFModel* pModel = new ModelClass; \
        *pccClassName = __DFModel_Class_Name__##ModelClass; \
        return pModel; \
} \
    \
    SystemVueModelBuilder::CRegisterModel _register_##ModelClass( __DFModel_Class_Create__##ModelClass, DFInterfaceVersion, REGISTER_MODEL_QUOTE_LIBNAME(LIBNAME)); \
    \
    static void __SaveConfig_##ModelClass(SystemVueModelBuilder::DFInterface &model)\
    { \
        if(model.GetImplementation() && model.GetImplementation()->HasConfigManager()) { \
            auto mgr = model.GetImplementation()->GetConfigManager();\
            std::string filename = "E:/project/archermind/ModelDesign/" #ModelClass "_config.json"; \
            if(mgr) { \
                mgr->SaveToFile(filename);\
            } \
        } \
    } \
    \
    bool	ModelClass::DefineInterface(SystemVueModelBuilder::DFInterface &model)
#endif
#else
#define DEFINE_MODEL_INTERFACE( ) \
The_DEFINE_MODEL_INTERFACE_macro_is_not_supported_for_code_generation_see_documentation_for_more_details
#endif

//------------------------------------------------------------------------------------------
// ADD_PARENT_MODEL_INTERFACE( ParentClass ) is used inside DEFINE_MODEL_INTERFACE to add
// the interface defined in in the model from which current model is inherited.
// Optionally you could redfine all the interface by yourself, if you don't call this
// macro. For example if a Subtractor model is derived from a calss Adder, which is also
// a DFModel then in cpp of Subtractor you could write the following.
//
//	DEFINE_MODEL_INTERFACE(Subtractor)
//	{
//		ADD_PARENT_MODEL_INTERFACE(Adder);
//
//		return true;
//	}
//------------------------------------------------------------------------------------------
#define ADD_PARENT_MODEL_INTERFACE( ParentModelClass )  ParentModelClass::DefineInterface(model)

//------------------------------------------------------------------------------------------
// SET_MODEL_NAMESPACE( model_namespace ) is used inside DEFINE_MODEL_INTERFACE to set
// the namespace of the model class for code generation.
//
// For example:
//		namespace UserNamespace
//		{
//			class Foo : public DFModel
//			{
//				...
//			};
//		}
// can be added using
//		DEFINE_MODEL_INTERFACE( Foo )
//		{
//			SET_MODEL_NAMESPACE( UserNamespace );
//		}
//
//------------------------------------------------------------------------------------------
#define SET_MODEL_NAMESPACE( model_namespace )	\
model.SetModelNamespace( model_namespace )

//------------------------------------------------------------------------------------------
// ADD_MODEL_HEADER_FILE( header_file ) is used inside DEFINE_MODEL_INTERFACE to set
// the header file of the model class for code generation.
// This macro should be used when the header file of the model is different than the model's
// class name appended w/ ".h".  Note, this can be called multiple times if the model requires
// more than one header file.
//
// For example, suppose a model class Foo is defined in CFoo.h, then
// ADD_MODEL_HEADER_FILE( "\"CFoo.h\"" )
// can be used to set model's header file.
//------------------------------------------------------------------------------------------
#define ADD_MODEL_HEADER_FILE( header_file )	\
model.AddModelHeaderFile( header_file )

//------------------------------------------------------------------------------------------
// ADD_MODEL_SOURCE_FILE( header_file ) is used inside DEFINE_MODEL_INTERFACE to set
// the source file of the model class for code generation.
// This macro should be used when the source file of the model is different than the model's
// class name appended w/ ".cpp".  Note, this can be called multiple times if the model requires
// more than one source file.
//
// For example, suppose a model class Foo is defined in CFoo.cpp, then
// ADD_MODEL_SOURCE_FILE( "\"CFoo.cpp\"" )
// can be used to set model's source file.
//------------------------------------------------------------------------------------------
#define ADD_MODEL_SOURCE_FILE( source_file )	\
model.AddModelSourceFile( source_file )

//------------------------------------------------------------------------------------------
// ADD_MODEL_INPUT( user_variable ) is used inside DEFINE_MODEL_INTERFACE to add
// a public member of C++ Data Flow model class as input port.
//
// For example public member variable
//		int m_iFoo;
// can be added using
//		ADD_MODEL_INPUT(m_iFoo);
// format.
//
// For supported data types consult the C++ Models section of SystemVue documentation at
// User's Guide -> Using SystemVue -> User Defined Models -> C++ Models -> Supported Data
// Types -> Data Types Used as Inputs/Outputs
//
// ADD_MODEL_INPUT returns an object of type DFPort. Which can be further used to change
// name, and description, and to add a rate-variable (for non-CircularBuffer data types).
//
// Please read "C++ Models" documentation for further detail.
//------------------------------------------------------------------------------------------
#define ADD_MODEL_INPUT( user_variable ) \
    ([&](){ \
        static bool __inputexecuted_##user_variable = false; \
        static SystemVueModelBuilder::DFPort __port_##user_variable; \
        if (!__inputexecuted_##user_variable) { \
            __inputexecuted_##user_variable = true; \
            __port_##user_variable = model.AddInput(user_variable, #user_variable); \
            __port_##user_variable.SetName(#user_variable); \
            __port_##user_variable.SetPutType("in"); \
            \
            { \
                auto currentModel = dynamic_cast<SystemVueModelBuilder::DFModel*>(this); \
                if (currentModel) { \
                    SystemVueModelBuilder::Block::DataType dataType = GetBlockDataType(user_variable); \
                    qDebug() << "DataType value:" << static_cast<int>(dataType); \
                    size_t bufferSize = 1; \
                    currentModel->AddInputPort(#user_variable, user_variable, bufferSize, dataType); \
                    qDebug() << "[AutoRegister] Registered input port:" << #user_variable; \
                } \
            } \
            \
            if (auto impl = model.GetImplementation()) { \
                auto buffer = impl->GetPortSystemVueBuffer(#user_variable); \
                if (buffer) { \
                    qDebug() << "Created SystemVue buffer for port" << #user_variable \
                             << "size:" << buffer->GetSize(); \
                } \
            } \
            \
            if(model.GetImplementation() && model.GetImplementation()->HasConfigManager()) { \
                SystemVueModelBuilder::PortConfig portConfig; \
                portConfig.name = __port_##user_variable.GetImplementation()->GetName(); \
                portConfig.dataType = GetDataTypeString(user_variable); \
                portConfig.putType = __port_##user_variable.GetImplementation()->GetPutType(); \
                model.GetImplementation()->SavePortConfig(#user_variable, portConfig); \
            } \
        } \
        return __port_##user_variable; \
    })()


//------------------------------------------------------------------------------------------
// ADD_MODEL_OUTPUT( user_variable ) is used inside DEFINE_MODEL_INTERFACE to add
// a public member of C++ Data Flow model class as output port.
//
// For example public member variable
//		int m_iFoo;
// can be added using
//		ADD_MODEL_OUTPUT(m_iFoo);
// format.
//
// For supported data types consult the C++ Models section of SystemVue documentation at
// User's Guide -> Using SystemVue -> User Defined Models -> C++ Models -> Supported Data
// Types -> Data Types Used as Inputs/Outputs
//
// ADD_MODEL_OUTPUT returns an object of type DFPort. Which can be further used to change
// name, and description, and to add a rate-variable (for non-CircularBuffer data types).
//
// Please read "C++ Models" documentation for further detail.
//------------------------------------------------------------------------------------------
#define ADD_MODEL_OUTPUT( user_variable )	\
    ([&](){ \
        static bool __executed_##user_variable = false; \
        static SystemVueModelBuilder::DFPort __port_##user_variable; \
        if (!__executed_##user_variable) { \
            __executed_##user_variable = true; \
            \
            __port_##user_variable = model.AddOutput(user_variable, #user_variable); \
            __port_##user_variable.SetName(#user_variable); \
            __port_##user_variable.SetPutType("out"); \
            \
            { \
                auto currentModel = dynamic_cast<SystemVueModelBuilder::DFModel*>(this); \
                if (currentModel) { \
                    SystemVueModelBuilder::Block::DataType dataType = GetBlockDataType(user_variable); \
                    qDebug() << "DataType value:" << static_cast<int>(dataType); \
                    size_t bufferSize = 1; \
                    currentModel->AddOutputPort(#user_variable, user_variable, bufferSize, dataType); \
                    qDebug() << "[AutoRegister] Registered output port:" << #user_variable; \
                } \
            } \
            \
            if (auto impl = model.GetImplementation()) { \
                auto buffer = impl->GetPortSystemVueBuffer(#user_variable); \
                if (buffer) { \
                    qDebug() << "Created SystemVue buffer for port" << #user_variable \
                             << "size:" << buffer->GetSize(); \
                } \
            } \
            \
            if(model.GetImplementation() && model.GetImplementation()->HasConfigManager()) { \
                SystemVueModelBuilder::PortConfig portConfig; \
                portConfig.name = __port_##user_variable.GetImplementation()->GetName(); \
                portConfig.dataType = GetDataTypeString(user_variable); \
                portConfig.putType = __port_##user_variable.GetImplementation()->GetPutType(); \
                model.GetImplementation()->SavePortConfig(#user_variable, portConfig); \
            } \
        } \
        return __port_##user_variable; \
    })()


//------------------------------------------------------------------------------
// ADD_MODEL_PARAM( user_param_variable ) is used inside DEFINE_MODEL_INTERFACE
// to add a public member of C++ Data Flow model class as a non-array parameter.
//
// For example public member variable
//		int m_iFoo;
// can be added using
//		ADD_MODEL_PARAM(m_iFoo);
// format.
//
// Following data types are supported as non-array parameter
// "int", "double", "float", "std::complex<doube>", "std::complex<float>",
// "char *" , and SystemVue's built in enumerations
//
// Please read User's Guide -> Using SystemVue -> User Defined Models -> C++ Models
// -> Supported Data Types -> Data Types Used as Parameters for list of Built in
// enumerations.
//
// ADD_MODEL_PARAM returns an object of type DFParam. which can be further used to
// change name, description, default value and to change string type parameter as
// File type with Browse button.
//
// Please read "C++ Models" documentation for further detail.
//------------------------------------------------------------------------------
#define ADD_MODEL_PARAM( user_param_variable )	 \
    model.AddParam(user_param_variable, #user_param_variable); \
    { \
        SystemVueModelBuilder::DFParam param = model.AddParam(user_param_variable, #user_param_variable); \
        param.SetName(#user_param_variable); \
        if(model.GetImplementation() && model.GetImplementation()->HasConfigManager()) { \
            SystemVueModelBuilder::ParamConfig paramConfig; \
            paramConfig.name = #user_param_variable; \
            paramConfig.dataType = GetDataTypeString(user_param_variable); \
            model.GetImplementation()->SaveParamConfig(#user_param_variable,paramConfig); \
        }\
    } \


#define ADD_MODEL_PARAMETER(x) ADD_MODEL_PARAM(x)

//------------------------------------------------------------------------------
// ADD_MODEL_ENUM_PARAM( user_param_variable, enum_type_name ) is used inside
// DEFINE_MODEL_INTERFACE to add a public enum type member of C++ Data Flow model
// class as a non-array parameter.
//
// For example public member variable
//		MyEnum {A=1, B=5, C};
//		MyEnum m_eFoo;
// can be added using
//		DFParam cEnum = ADD_MODEL_ENUM_PARAM(m_eFoo, MyEnum);
// format.
//
// Only "user defined enums" should be added in this manner. Use ADD_MODEL_PARAM
// for SystemVue's built in enumerations
//
// ADD_MODEL_ENUM_PARAM returns an object of type DFParam.
// ADD_MODEL_ENUM_PARAM macro must be be followed by sequence of
// AddEnumeration(char * name, int value) method of returned object of type DFParam.
//
// In the above example we will need to do
//		cEnum.AddEnumeration("A",A);
//		cEnum.AddEnumeration("B",B);
// 		cEnum.AddEnumeration("C",C);
//
//  The returned DFParam object can be further used to change name, description,
// default value.
//
// Please read "C++ Models" documentation for further detail.
//------------------------------------------------------------------------------
#define ADD_MODEL_ENUM_PARAM( user_param_variable, enum_type_name )	 \
model.AddParamEnum<enum_type_name>(user_param_variable,#user_param_variable,#enum_type_name);\
{ \
    SystemVueModelBuilder::DFParam param = model.AddParamEnum<enum_type_name>(user_param_variable,#user_param_variable,#enum_type_name);\
    param.SetName(#user_param_variable);\
    if(model.GetImplementation() && model.GetImplementation()->HasConfigManager()) { \
        SystemVueModelBuilder::ParamConfig paramConfig; \
        paramConfig.name = #user_param_variable; \
        paramConfig.dataType = #enum_type_name; \
        model.GetImplementation()->SaveParamConfig(#user_param_variable, paramConfig); \
    }\
} \


#define ADD_MODEL_ENUM_PARAMETER(user_param_variable, enum_type_name) ADD_MODEL_ENUM_PARAM(user_param_variable, enum_type_name)

//------------------------------------------------------------------------------
// ADD_MODEL_ARRAY_PARAM(user_param_variable, user_array_size_variable)
// is used inside DEFINE_MODEL_INTERFACE to add a public member of C++ Data Flow
// model class as a an array parameter.
//
// For example public member variable
//		int *m_iFoo;
//		unsigned m_iFooSize;
// can be added using
//		ADD_MODEL_ARRAY_PARAM(m_iFoo,m_iFooSize);
// format.
//
// Following data types are supported as an array parameter
// "int *", "double *", "std::complex<doube> *"
//
//  The user_array_size_variable should be of unsigned type which will be set
// by simulator to the number of elements in the array parameter.
// Simulator will take care of all memory management for pointer data
//
// ADD_MODEL_ARRAY_PARAM returns an object of type DFParam. which can be further
// used to change name, description, and default value.
//
// Please read "C++ Models" documentation for further detail.
//------------------------------------------------------------------------------
#define ADD_MODEL_ARRAY_PARAM(user_param_variable, user_array_size_variable)	\
model.AddParamArray(user_param_variable,user_array_size_variable, #user_param_variable, #user_array_size_variable)

#define ADD_MODEL_ARRAY_PARAMETER(x,y) ADD_MODEL_ARRAY_PARAM(x,y)

//------------------------------------------------------------------------------
// SET_MODEL_NAME( model_name ) can be used inside DEFINE_MODEL_INTERFACE to
// change the model name to some value other than the default class name.
// For example
//		SET_MODEL_NAME("MyAdder");
//
// Please read "C++ Models" documentation for further detail.
//------------------------------------------------------------------------------
#define SET_MODEL_NAME( model_name )	\
model.SetModelName(model_name)


//-------------------------------------------------------------------------------------
// SET_MODEL_DESCRIPTION( model_description ) can be used inside DEFINE_MODEL_INTERFACE
// to change the model description.
// For example
//		SET_MODEL_DESCRIPTION("My first Adder");
//
// Please read "C++ Models" documentation for further detail.
//-------------------------------------------------------------------------------------
#define SET_MODEL_DESCRIPTION( model_description )	\
model.SetModelDescription(model_description);\
{ \
    if(model.GetImplementation() && model.GetImplementation()->HasConfigManager()) { \
        auto modelConfig = model.GetImplementation()->LoadModelConfig();\
        modelConfig.modeldescription = model_description; \
        model.GetImplementation()->SaveModelConfig(modelConfig);\
    } \
} \


//--------------------------------------------------------------------------------------
// SET_MODEL_SYMBOL( model_symbol_name ) can be used inside DEFINE_MODEL_INTERFACE
// to select a pre-built symbol instead of default auto-generated symbol.
// For example
//		SET_MODEL_SYMBOL("SYM_IntToBits");
//
// Note that if you are selecting a symbol then port names in symbol and model
// must match.
//
// Please read "C++ Models" documentation for further detail.
//----------------------------------------------------------------------------------------
#define SET_MODEL_SYMBOL( model_symbol_name )	\
model.SetModelSymbol( model_symbol_name);\
{ \
    if(model.GetImplementation() && model.GetImplementation()->HasConfigManager()) { \
            auto modelConfig = model.GetImplementation()->LoadModelConfig(); \
            modelConfig.modelsymbol = model_symbol_name; \
            model.GetImplementation()->SaveModelConfig(modelConfig);\
    } \
} \

//------------------------------------------------------------------------------
// SET_MODEL_CATEGORY( model_category ) can be used inside DEFINE_MODEL_INTERFACE
// to change the model category.
// For example
//		SET_MODEL_CATEGORY("Math, numeric");
//
// Please read "C++ Models" documentation for further detail.
//------------------------------------------------------------------------------
#define SET_MODEL_CATEGORY( model_category )	\
model.SetModelCategory( model_category );\
{ \
        if(model.GetImplementation() && model.GetImplementation()->HasConfigManager()) { \
            auto modelConfig = model.GetImplementation()->LoadModelConfig(); \
            modelConfig.modelcategory = model_category; \
            model.GetImplementation()->SaveModelConfig(modelConfig);\
    } \
} \

//--------------------------------------------------------------------------------------
// REGISTER_SIMULATION_CONTROL( sim_control_variable ) can be used inside DEFINE_MODEL_INTERFACE
// to register simulation control object of the model if the model uses it to control the simulation.
//
// For example public member variable
//		SinkControl m_sinkControl;
// can be added using
//		REGISTER_SIMULATION_CONTROL(m_sinkControl);
//
// Please read "C++ Models" documentation for further detail.
//----------------------------------------------------------------------------------------
#define REGISTER_SIMULATION_CONTROL( sim_control_variable )	\
    model.RegisterSimulationControl( sim_control_variable, #sim_control_variable )


//------------------------------------------------------------------------------
// DISABLE_PART_GENERATION() can be used inside DEFINE_MODEL_INTERFACE
// to disable automatic part generation for this model.  This is called when you
// create your own part.
//
// Please read "C++ Models" documentation for further detail.
//------------------------------------------------------------------------------
#define DISABLE_PART_GENERATION()	\
        model.DisablePartGeneration()

//------------------------------------------------------------------------------
// ADD_MODEL(model_name) can be used inside DEFINE_MODEL_INTERFACE
// to additional models to the part generated for this model.
//
// Please read "C++ Models" documentation for further detail.
//------------------------------------------------------------------------------
#define ADD_MODEL(model_name) \
model.AddModel(#model_name)


//------------------------------------------------------------------------------
// SET_DEFAULT_MODEL(model_name) can be used inside DEFINE_MODEL_INTERFACE
// to set the default model for the part.
//
// Please read "C++ Models" documentation for further detail.
//------------------------------------------------------------------------------
#define SET_DEFAULT_MODEL(model_name) \
    model.SetDefaultModel(#model_name)

//------------------------------------------------------------------------------
// SET_CUSTOM_UI( UIname ) can be used inside DEFINE_MODEL_INTERFACE
// to define which custom UI this model uses as an interface for parameter setting.
//
// Please read "C++ Models" documentation for further detail.
//------------------------------------------------------------------------------
#define SET_CUSTOM_UI( custom_ui_name )	\
    model.SetCustomUI( custom_ui_name )

/// The POST_ERROR macro posts an error to the error pane and the simulation log.  Additionally this method causes the simulation to be exited.
/// <param name="const_char_error">The message to post, must be a const char*.</param>
/// <remarks>This method can be used anywhere except inside DEFINE_MODEL_INTERFACE macro and the constructors/destructor methods.</remarks>
#define POST_ERROR(const_char_error) \
    SystemVueModelBuilder::DFErrorHandler::PostError(this, const_char_error)

/// The POST_WARNING macro posts a warning to the error pane and the simulation log.
/// <param name="const_char_warning">The message to post, must be a const char*.</param>
/// <remarks>This method can be used anywhere except inside DEFINE_MODEL_INTERFACE macro and the constructors/destructor methods.</remarks>
#define POST_WARNING(const_char_warning) \
    SystemVueModelBuilder::DFErrorHandler::PostWarning(this, const_char_warning)

/// The POST_INFO macro posts a informational message to the error pane and the the simulation log.
/// <param name="const_char_message">The message to post, must be a const char*.</param>
/// <remarks>This method can be used anywhere except inside DEFINE_MODEL_INTERFACE macro and the constructors/destructor methods.</remarks>
#define POST_INFO(const_char_message)	\
    SystemVueModelBuilder::DFErrorHandler::PostInfo(this, const_char_message)

/// The POST_LOG macro posts to the simulation log.
/// <param name="const_char_message">The message to post, must be a const char*.</param>
/// <remarks>This method can be used anywhere except inside DEFINE_MODEL_INTERFACE macro and the constructors/destructor methods.</remarks>
#define POST_LOG(const_char_message)	\
    SystemVueModelBuilder::DFErrorHandler::PostLog(this, const_char_message)

/// The POST_PROGRESS macro posts to the simulation status window.
/// <param name="const_char_message">The message to post, must be a const char*.</param>
/// <remarks>Each call to this macro will replace the message from a previous call.  To clear the progress message, use the CLEAR_PROGRESS macro.  This method can be used anywhere except inside DEFINE_MODEL_INTERFACE macro and the constructors/destructor methods.</remarks>
#define POST_PROGRESS(const_char_message)	\
    SystemVueModelBuilder::DFErrorHandler::PostProgress(this, const_char_message)

/// The CLEAR_PROGRESS macro clears a post to the simulation status window.
/// <remarks>Clears a the progress message posted from the POST_PROGRESS macro.  This method can be used anywhere except inside DEFINE_MODEL_INTERFACE macro and the constructors/destructor methods.</remarks>
#define CLEAR_PROGRESS()	\
    SystemVueModelBuilder::DFErrorHandler::ClearProgress(this)

/// The STOP_REQUESTED macro returns true if model should stop - typically user initiated by hitting stop button.
#define STOP_REQUESTED()	\
    SystemVueModelBuilder::DFErrorHandler::StopRequested()

/// Returns true if model should stop due to an error.
#define ERROR_OCCURRED()	\
    SystemVueModelBuilder::DFErrorHandler::ErrorOccurred()
#endif // MODELBUILDER_H

// 首先添加类型推断辅助模板
template<typename T>
SystemVueModelBuilder::Block::DataType GetBlockDataType(T& var) {
    // 实现类型到 DataType 的映射
    //基础类型
    std::ignore = var;
    if constexpr (std::is_same_v<T, int>) {
        return SystemVueModelBuilder::Block::DataType::INT;
    }
    if constexpr (std::is_same_v<T, double>) {
        return SystemVueModelBuilder::Block::DataType::DOUBLE;
    }
    if constexpr (std::is_same_v<T, float>) {
        return SystemVueModelBuilder::Block::DataType::FLOAT;
    }
    if constexpr (std::is_same_v<T, bool>) {
        return SystemVueModelBuilder::Block::DataType::BOOL;
    }
    if constexpr (std::is_same_v<T, std::complex<float>>) {
        return SystemVueModelBuilder::Block::DataType::COMPLEX_FLOAT;
    }
    if constexpr (std::is_same_v<T, std::complex<double>>) {
        return SystemVueModelBuilder::Block::DataType::COMPLEX_DOUBLE;
    }
    //普通类型
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::IntCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::CIRCULAR_BUFFER_INT;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::DoubleCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::CIRCULAR_BUFFER_DOUBLE;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::FloatCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::CIRCULAR_BUFFER_FLOAT;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::BoolCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::CIRCULAR_BUFFER_BOOL;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::FComplexCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::CIRCULAR_BUFFER_FCOMPLEX;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::DComplexCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::CIRCULAR_BUFFER_DCOMPLEX;
    }
    //时域类型
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<int>>) {
        return SystemVueModelBuilder::Block::DataType::TIMED_INT;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<double>>) {
        return SystemVueModelBuilder::Block::DataType::TIMED_DOUBLE;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<float>>) {
        return SystemVueModelBuilder::Block::DataType::TIMED_FLOAT;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<bool>>) {
        return SystemVueModelBuilder::Block::DataType::TIMED_BOOL;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>>) {
        return SystemVueModelBuilder::Block::DataType::TIMED_FCOMPLEX;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>>) {
        return SystemVueModelBuilder::Block::DataType::TIMED_DCOMPLEX;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::EnvelopeCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::ENVELOPE_SIGNAL;
    }
    //总线类型
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::IntCircularBufferBus>) {
        return SystemVueModelBuilder::Block::DataType::INT_BUS;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::DoubleCircularBufferBus>) {
        return SystemVueModelBuilder::Block::DataType::DOUBLE_BUS;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::FloatCircularBufferBus>) {
        return SystemVueModelBuilder::Block::DataType::FLOAT_BUS;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::BoolCircularBufferBus>) {
        return SystemVueModelBuilder::Block::DataType::BOOL_BUS;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::FComplexCircularBufferBus>) {
        return SystemVueModelBuilder::Block::DataType::FCOMPLEX_BUS;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::DComplexCircularBufferBus>) {
        return SystemVueModelBuilder::Block::DataType::DCOMPLEX_BUS;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::EnvelopeCircularBufferBus>) {
        return SystemVueModelBuilder::Block::DataType::DCOMPLEX_BUS;
    }
    //矩阵类型
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::IntMatrixCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_INT;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::DoubleMatrixCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_DOUBLE;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::FloatMatrixCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_FLOAT;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::BoolMatrixCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_BOOL;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::FComplexMatrixCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_FCOMPLEX;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::DComplexMatrixCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_DCOMPLEX;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::EnvelopeMatrixCircularBuffer>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_DCOMPLEX;
    }
    //时域矩阵
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::Matrix<int>>>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_TIME_INT;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::Matrix<double>>>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_TIME_DOUBLE;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::Matrix<float>>>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_TIME_FLOAT;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::Matrix<bool>>>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_TIME_BOOL;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::Matrix<std::complex<float>>>>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_TIME_FCOMPLEX;
    }
    if constexpr (std::is_same_v<T, SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::Matrix<std::complex<double>>>>) {
        return SystemVueModelBuilder::Block::DataType::MATRIX_TIME_DCOMPLEX;
    }
    return DataType::INT;
}

#ifndef GET_DATA_TYPE_STRING_DEFINED
#define GET_DATA_TYPE_STRING_DEFINED
template<typename T>
    const char* GetDataTypeString(T&) {
        if constexpr (std::is_same_v<T, double>) return "double";
        else if constexpr (std::is_same_v<T, float>) return "float";
        else if constexpr (std::is_same_v<T, int>) return "int";
        else if constexpr (std::is_same_v<T, bool>) return "bool";
        else if constexpr (std::is_same_v<T, char*>) return "string";
        else if constexpr (std::is_same_v<T, std::complex<float>>) return "std::complex<float>";
        else if constexpr (std::is_same_v<T, std::complex<double>>) return "std::complex<double>";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::QueryEnum>) return "QueryEnum";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::BooleanEnum>) return "BooleanEnum";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::Matrix<std::complex<float>>>) return "Matrix<std::complex<float>>";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::Matrix<std::complex<double>>>) return "Matrix<std::complex<double>>";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::Matrix<int>>) return "Matrix<int>";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::Matrix<float>>) return "Matrix<float>";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::Matrix<double>>) return "Matrix<double>";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::Matrix<bool>>) return "Matrix<bool>";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::FixedPointEnums::Sign>) return "FixedPointEnums::Sign";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::FixedPointEnums::QuantizationMode>) return "FixedPointEnums::QuantizationMode";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::FixedPointEnums::OverflowMode>) return "FixedPointEnums::OverflowMode";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::EnvelopeSignal>) return "EnvelopeSignal";
        else if constexpr (std::is_same_v<T, SystemVueModelBuilder::EnvelopeMatrix>) return "EnvelopeMatrix";
        else return "unknown";
    }
#endif // GET_DATA_TYPE_STRING_DEFINED

