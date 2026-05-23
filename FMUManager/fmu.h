#ifndef FMU_H
#define FMU_H

#include <QString>
#include <vector>
#include <unordered_map>
#include <QLibrary>
#include <variant>
#include <QHash>
#include <optional>

#include "fmi2Functions.h"

/**
 * @brief fmi类型，基本全是CS，没有ME
 */
enum fmiType{
    ME = 0,
    CS
};

using fmiValueReference = unsigned int;

/**
 * @brief 参数类型
 */
enum VarType{
    Real,
    Integer,
    Boolean,
    String
};

/**
 * @brief 参数属性，包含端口类型和参数类型
 */
enum VarcausalityType{
    Input,
    Output,
    Parameter,
    Local
};

/**
 * @brief fmu参数结构体
 * @param varname 参数名字
 * @param vr 参数索引值
 * @param type 参数类型
 * @param causality 参数属性
 * @param startValue 初始值
 */
struct FmuVar{
    QString varname;
    fmiValueReference vr;
    VarType type;
    VarcausalityType causality;
    std::variant<double, bool,QString> startValue;
//    union {
//        double realstart;
//        QString stringstart;
//        bool boolstart;
//    };
};

/**
 * @brief fmu配置结构体
 * @param libname 库名称
 * @param path 库路径
 * @param guid
 * @param type fmi类型
 */
struct fmuConfig
{
    QString libname;
    QString path;
    QString guid;
    fmiType type;

};

/**
 * @brief fmu初始化结构体
 */
struct fmuCreateInfo{
    fmuConfig config;
    std::vector<FmuVar> fmuVec;
};


class FMU
{
public:
    FMU(const fmuCreateInfo& info);
    ~FMU();

    /**
     * @brief 加载库文件并初始化fmi函数
     */
    bool load();
    /**
     * @brief 卸载库文件
     */
    bool free();
    /**
     * @brief 初始化初值
     */
    bool initstartvalue();

    // fmi函数封装
    /**
     * @brief fmi函数指针初始化
     */
    bool instantiate();
    /**
     * @brief fmi函数指针销毁
     */
    bool terminate();

    /**
     * @brief 主执行函数，调度
     * @param currentTime 起始时间
     * @param stepSize 时间步长
     * @return 执行是否成功
     */
    bool doStep(double currentTime, double stepSize);


    /**
     * @brief 参数设置函数，包含单个设置和多个设置的不同重载
     */
    bool setReal(const QString& name, double value);
    double getReal(const QString& name);
    bool setBoolean(const QString& name, bool value);
    bool getBoolean(const QString& name);
    bool setInteger(const QString& name, int value);
    int getInteger(const QString& name);
    bool setString(const QString& name, QString value);
    QString getString(const QString& name);

    bool setReals(const std::vector<QString>& names, const std::vector<double>& values);
    std::vector<double> getReals(const std::vector<QString>& names);
    bool setBooleans(const std::vector<QString>& names, const std::vector<bool>& values);
    std::vector<bool> getBooleans(const std::vector<QString>& names);
    bool setIntegers(const std::vector<QString>& names, const std::vector<int>& values);
    std::vector<int> getIntegers(const std::vector<QString>& names);
    bool setStrings(const std::vector<QString>& names, const std::vector<QString>& values);
    std::vector<QString> getStrings(const std::vector<QString>& names);




private:
    fmuConfig config;
    QHash<QString, FmuVar> varmap;
    QLibrary lib;
    fmi2Component instance;


    // fmi接口函数指针
    fmi2InstantiateTYPE* fmi2InstantiatePtr;
    fmi2SetupExperimentTYPE* fmi2SetupExperimentPtr;
    fmi2EnterInitializationModeTYPE* fmi2EnterInitializationModePtr;
    fmi2ExitInitializationModeTYPE* fmi2ExitInitializationModePtr;
    fmi2FreeInstanceTYPE* fmi2FreeInstancePtr;
    fmi2TerminateTYPE* fmi2TerminatePtr;
    fmi2ResetTYPE* fmi2ResetPtr;
    fmi2DoStepTYPE* fmi2DoStepPtr;

    fmi2SetRealTYPE* fmi2SetRealPtr;
    fmi2GetRealTYPE* fmi2GetRealPtr;
    fmi2SetBooleanTYPE* fmi2SetBooleanPtr;
    fmi2GetBooleanTYPE* fmi2GetBooleanPtr;
    fmi2SetStringTYPE*  fmi2SetStringPtr;
    fmi2GetStringTYPE*  fmi2GetStringPtr;
    fmi2SetIntegerTYPE* fmi2SetIntegerPtr;
    fmi2GetIntegerTYPE* fmi2GetIntegerPtr;


};

#endif // FMU_H
