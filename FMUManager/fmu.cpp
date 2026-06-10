#include "fmu.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#define RESOLVE_FUNC(name) \
    this->name##Ptr = reinterpret_cast<name##TYPE*>(lib.resolve(#name)); \
    if (!name##Ptr) { \
        qDebug() << "resolve failed:" << #name; \
        return false; \
    }


FMU::FMU(const fmuCreateInfo& info)
{
    config = info.config;

    for(const auto& var: info.fmuVec)
    {
        varmap.insert(var.varname, var);
    }

    instance = nullptr;

    // fmi接口函数指针初始化
    this->fmi2InstantiatePtr = nullptr;
    this->fmi2SetupExperimentPtr = nullptr;
    this->fmi2EnterInitializationModePtr = nullptr;
    this->fmi2ExitInitializationModePtr = nullptr;
    this->fmi2FreeInstancePtr = nullptr;
    this->fmi2TerminatePtr = nullptr;
    this->fmi2ResetPtr = nullptr;
    this->fmi2DoStepPtr = nullptr;

    this->fmi2SetRealPtr = nullptr;
    this->fmi2GetRealPtr = nullptr;
    this->fmi2SetBooleanPtr = nullptr;
    this->fmi2GetBooleanPtr = nullptr;
    this->fmi2SetStringPtr = nullptr;
    this->fmi2GetStringPtr = nullptr;
    this->fmi2SetIntegerPtr = nullptr;
    this->fmi2GetIntegerPtr = nullptr;

    qDebug() << "[FMU] Constructor - libname:" << config.libname << "path:" << config.path << "guid:" << config.guid;
}


FMU::~FMU()
{
    this->terminate();
    this->free();
    // 依赖库在主库卸载后才释放（shared_ptr 自动管理）
    depLibs.clear();
}


bool FMU::load()
{
    if (lib.isLoaded())
    {
        qDebug() <<"FMU is already loaded" << config.libname;
        return false;
    }

    // 先加载所有依赖库，确保主库的符号依赖可以被解析
    for (const QString& depPath : config.depPaths) {
        auto depLib = std::make_shared<QLibrary>(depPath);
        if (!depLib->load()) {
            qDebug() << "[load] Warning: failed to load dependency:" << depPath
                     << "error:" << depLib->errorString();
        } else {
            qDebug() << "[load] Dependency loaded:" << depPath;
            depLibs.push_back(depLib);  // 保持加载状态
        }
    }

    lib.setFileName(config.path);

    if (!lib.load())
    {
        qDebug() <<"FMU is load failed";
        qDebug() << lib.errorString();
        return false;
    }


    // 解析fmi 接口函数
    RESOLVE_FUNC(fmi2Instantiate);
    RESOLVE_FUNC(fmi2SetupExperiment);
    RESOLVE_FUNC(fmi2EnterInitializationMode);
    RESOLVE_FUNC(fmi2ExitInitializationMode);
    RESOLVE_FUNC(fmi2FreeInstance);
    RESOLVE_FUNC(fmi2Terminate);
    RESOLVE_FUNC(fmi2Reset);
    RESOLVE_FUNC(fmi2DoStep);

    RESOLVE_FUNC(fmi2SetReal);
    RESOLVE_FUNC(fmi2GetReal);
    RESOLVE_FUNC(fmi2SetBoolean);
    RESOLVE_FUNC(fmi2GetBoolean);
    RESOLVE_FUNC(fmi2SetString);
    RESOLVE_FUNC(fmi2GetString);
    RESOLVE_FUNC(fmi2SetInteger);
    RESOLVE_FUNC(fmi2GetInteger);

    qDebug() << "[load] All fmi2 functions resolved - fmi2DoStepPtr:" << (void*)this->fmi2DoStepPtr
             << "fmi2SetRealPtr:" << (void*)this->fmi2SetRealPtr
             << "fmi2InstantiatePtr:" << (void*)this->fmi2InstantiatePtr;

//    this->fmi2InstantiatePtr = (fmi2InstantiateTYPE*) lib.resolve("fmi2Instantiate");
//    this->fmi2SetupExperimentPtr = (fmi2SetupExperimentTYPE*) lib.resolve("fmi2SetupExperiment");
//    this->fmi2EnterInitializationModePtr = (fmi2EnterInitializationModeTYPE*)lib.resolve("fmi2EnterInitializationMode");
//    this->fmi2ExitInitializationModePtr = (fmi2ExitInitializationModeTYPE*) lib.resolve("fmi2ExitInitializationMode");
//    this->fmi2SetRealPtr = (fmi2SetRealTYPE*) lib.resolve("fmi2SetReal");
//    this->fmi2GetRealPtr = (fmi2GetRealTYPE*) lib.resolve("fmi2GetReal");
//    this->fmi2FreeInstancePtr = (fmi2FreeInstanceTYPE*) lib.resolve("fmi2FreeInstance");
//    this->fmi2TerminatePtr = (fmi2TerminateTYPE*) lib.resolve("fmi2Terminate");
//    this->fmi2ResetPtr = (fmi2ResetTYPE*) lib.resolve("fmi2Reset");
//    this->fmi2DoStepPtr = (fmi2DoStepTYPE*) lib.resolve("fmi2DoStep");

    return true;


}


bool FMU::initstartvalue()
{
    if (!this->instance)
    {
        return false;
    }

    // 设置startValue

    for(auto it = varmap.begin(); it != varmap.end(); ++it)
    {
        const FmuVar& var = it.value();

        if (var.causality != Input && var.causality != Parameter)
        {
            continue;
        }

        fmi2ValueReference vr = var.vr;

        switch (var.type)
        {
        case Real:
        {
            if (!this->fmi2SetRealPtr)
            {
                return false;
            }

            double v = std::get<double>(var.startValue);
            fmi2Real val = v;

            fmi2Status s = this->fmi2SetRealPtr(this->instance, &vr, 1, &val);
            if (s != fmi2OK)
            {
                return false;
            }
            break;

        }

        case Integer:
        {
            if (!this->fmi2SetIntegerPtr)
            {
                return false;
            }

            int v = std::get<double>(var.startValue);
            fmi2Integer val = v;

            fmi2Status s = this->fmi2SetIntegerPtr(this->instance, &vr, 1, &val);
            if (s != fmi2OK)
            {
                return false;
            }
            break;

        }

        case Boolean:
        {
            if (!this->fmi2SetBooleanPtr)
            {
                return false;
            }

            double v = std::get<bool>(var.startValue);
            fmi2Boolean val = v ? fmi2True : fmi2False;

            fmi2Status s = this->fmi2SetBooleanPtr(this->instance, &vr, 1, &val);
            if (s != fmi2OK)
            {
                return false;
            }
            break;

        }

        case String:
        {
           if (!this->fmi2SetStringPtr)
           {
               return false;
           }
            QString v = std::get<QString>(var.startValue);
            QString name = var.varname;

            if (this->setString(name, v))
            {
                return false;
            }
            break;

        }
        }
    }

    return true;
}

void fmuLogger(fmi2ComponentEnvironment env,
               fmi2String instanceName,
               fmi2Status status,
               fmi2String category,
               fmi2String message, ...)
{
    qDebug() << "[FMU]" << instanceName << category << message;
}

bool FMU::instantiate()
{
    if (!fmi2InstantiatePtr)
    {
        return false;
    }

    fmi2CallbackFunctions cbf;
    cbf.logger = fmuLogger;
    cbf.allocateMemory = calloc;
    cbf.freeMemory = ::free;
    cbf.stepFinished = NULL;
    cbf.componentEnvironment = NULL;

    // 从 DLL 路径推导 resourceURI
    // DLL 路径: .../binaries/linux64/xxx.so 或 .../binaries/win64/xxx.dll
    // resources 路径: .../resources/
    QFileInfo dllInfo(config.path);
    QDir binariesDir = dllInfo.absoluteDir(); // binaries/linux64 或 binaries/win64
    binariesDir.cdUp();                        // binaries
    binariesDir.cdUp();                        // FMU 根目录
    QString resourcePath = binariesDir.absolutePath() + "/resources";
    QByteArray resourceUri = ("file:///" + resourcePath).toUtf8();

    qDebug() << "FMU resourceURI:" << resourceUri;

    this->instance = fmi2InstantiatePtr(config.libname.toStdString().c_str(),
                                        (fmi2Type)config.type,
                                        config.guid.toStdString().c_str(),
                                        resourceUri.constData(),
                                        &cbf,
                                        0,
                                        1);  // loggingOn=1，启用FMU内部日志

   if (!this->instance)
   {
       qDebug() << "instantiate failed";
       return false;
   }

   if (!fmi2SetupExperimentPtr)
   {
       return false;
   }

   fmi2SetupExperimentPtr(this->instance, 0, 0, 0, 0, 0);

   if (!fmi2EnterInitializationModePtr)
   {
       return false;
   }
   fmi2EnterInitializationModePtr(this->instance);

   if (!fmi2ExitInitializationModePtr)
   {
       return false;
   }

   fmi2ExitInitializationModePtr(this->instance);

   qDebug() << "[instantiate] FMU instantiated successfully, instance:" << this->instance;

   return true;
}

bool FMU::terminate()
{  
    if (!fmi2FreeInstancePtr || !fmi2TerminatePtr || !this->instance)
    {
        return false;
    }

    fmi2TerminatePtr(this->instance);

    fmi2FreeInstancePtr(this->instance);
    this->instance = nullptr;

    return true;
}


bool FMU::free()
{
    if (!lib.isLoaded())
    {
        return false;
    }


    lib.unload();
    qDebug() << "lib is unloaded" << config.libname;
    return true;

}


bool FMU::setReal(const QString& name, double value)
{
    if (!fmi2SetRealPtr || !instance)
    {
        return false;
    }

    if (!varmap.contains(name))
    {
        qDebug() << "not found" << name;
        return false;
    }

    const FmuVar& tmpVar = varmap[name];
    if (tmpVar.type != Real)
    {
        qDebug() << "type mismath";
        return false;
    }

    fmiValueReference vr = tmpVar.vr;
    fmi2Real val = value;
    fmi2Status s = fmi2SetRealPtr(this->instance, &vr, 1, &val);

    return (s==fmi2OK);
}

double FMU::getReal(const QString& name)
{
    if (!fmi2GetRealPtr || !instance)
    {
        return 0.0;
    }

    if (!varmap.contains(name))
    {
        qDebug() << "not found" << name;
        return 0.0;
    }

    const FmuVar& tmpVar = varmap[name];
    if (tmpVar.type != Real)
    {
        qDebug() << "type mismath";
        return 0.0;
    }

    fmiValueReference vr = tmpVar.vr;
    fmi2Real val = 0;

    fmi2GetRealPtr(this->instance, &vr, 1, &val);
    return val;

}

bool FMU::setBoolean(const QString& name, bool value)
{
    if (!fmi2SetBooleanPtr || !instance)
    {
        return false;
    }

    if (!varmap.contains(name))
    {
        qDebug() << "not found" << name;
        return false;
    }

    const FmuVar& tmpVar = varmap[name];
    if (tmpVar.type != Boolean)
    {
        qDebug() << "type mismath";
        return false;
    }

    fmiValueReference vr = tmpVar.vr;
    fmi2Boolean val = value;
    fmi2Status s = fmi2SetBooleanPtr(this->instance, &vr, 1, &val);

    return (s==fmi2OK);
}

bool FMU::getBoolean(const QString& name)
{
    if (!fmi2GetBooleanPtr || !instance)
    {
        return false;
    }

    if (!varmap.contains(name))
    {
        qDebug() << "not found" << name;
        return false;
    }

    const FmuVar& tmpVar = varmap[name];
    if (tmpVar.type != Boolean)
    {
        qDebug() << "type mismath";
        return false;
    }
    fmiValueReference vr = tmpVar.vr;
    fmi2Boolean val = 0;
    fmi2GetBooleanPtr(this->instance, &vr, 1, &val);
    return bool(val);
}

bool FMU::setInteger(const QString& name, int value)
{
    if (!fmi2SetIntegerPtr || !instance)
    {
        return false;
    }

    if (!varmap.contains(name))
    {
        qDebug() << "not found" << name;
        return false;
    }

    const FmuVar& tmpVar = varmap[name];
    if (tmpVar.type != Integer)
    {
        qDebug() << "type mismath";
        return false;
    }

    fmiValueReference vr = tmpVar.vr;
    fmi2Integer val = value;
    fmi2Status s = fmi2SetIntegerPtr(this->instance, &vr, 1, &val);

    return (s==fmi2OK);
}

int FMU::getInteger(const QString& name)
{
    if (!fmi2GetIntegerPtr || !instance)
    {
        return false;
    }

    if (!varmap.contains(name))
    {
        qDebug() << "not found" << name;
        return false;
    }

    const FmuVar& tmpVar = varmap[name];
    if (tmpVar.type != Integer)
    {
        qDebug() << "type mismath";
        return false;
    }

    fmiValueReference vr = tmpVar.vr;
    fmi2Integer val = 0;
    fmi2GetIntegerPtr(this->instance, &vr, 1, &val);

    return val;
}


bool FMU::setString(const QString& name, QString value)
{
    if (!fmi2SetStringPtr || !instance)
    {
        return false;
    }

    if (!varmap.contains(name))
    {
        qDebug() << "not found" << name;
        return false;
    }

    const FmuVar& tmpVar = varmap[name];
    if (tmpVar.type != String)
    {
        qDebug() << "type mismath";
        return false;
    }

    fmi2ValueReference vr = tmpVar.vr;

    QByteArray ba = value.toUtf8();
    const char *str = ba.constData();

    const char* values[1];
    values[0] = str;

    fmi2Status s = this->fmi2SetStringPtr(this->instance, &vr, 1, values);

    return (s== fmi2OK);

}


QString FMU::getString(const QString& name)
{
    if (!fmi2GetStringPtr || !instance)
    {
        return "";
    }

    if (!varmap.contains(name))
    {
        qDebug() << "not found" << name;
        return "";
    }

    const FmuVar& tmpVar = varmap[name];
    if (tmpVar.type != String)
    {
        qDebug() << "type mismath";
        return "";
    }

    fmi2ValueReference vr = tmpVar.vr;

    fmi2String values[1];

    fmi2Status s = this->fmi2GetStringPtr(this->instance, &vr, 1, values);

    if (s != fmi2OK)
    {
        qDebug() << "fmi2GetString failed";
        return "";
    }

    if (!values[0])
    {
        return "";
    }

    return QString::fromUtf8(values[0]);
}


bool FMU::setReals(const std::vector<QString>& names, const std::vector<double>& values)
{
    if(!fmi2SetRealPtr || !instance)
    {
        return false;
    }

    if (names.size() != values.size())
    {
        return false;
    }

    std::vector<fmiValueReference> vrs;
    std::vector<fmi2Real> vals;

    for (int i = 0; i < names.size(); i++)
    {
        if (!this->varmap.contains(names[i]))
        {
            return false;
        }
        const FmuVar& var = this->varmap[names[i]];
        if (var.type != Real)
        {
            qDebug() << "type mismath";
            return false;
        }
        vrs.push_back(var.vr);
        vals.push_back(values[i]);
    }

    fmi2Status s = this->fmi2SetRealPtr(this->instance, vrs.data(), vrs.size(), vals.data());
    return (s==fmi2OK);
}



std::vector<double> FMU::getReals(const std::vector<QString>& names)
{
    if(!fmi2GetRealPtr || !instance)
    {
        return {};
    }

    std::vector<fmiValueReference> vrs;
    std::vector<fmi2Real> values(names.size(), 0.0);

    for (int i = 0; i < names.size(); i++)
    {
        if (!this->varmap.contains(names[i]))
        {
            return {};
        }
        const FmuVar& var = this->varmap[names[i]];
        if (var.type != Real)
        {
            qDebug() << "type mismath";
            return {};
        }
        vrs.push_back(var.vr);
    }
    qDebug() << "FMU::getReals - vrs data: " << vrs.data();
    qDebug() << "FMU::getReals - vrs size: " << vrs.size();


    fmi2Status s = fmi2GetRealPtr(this->instance, vrs.data(), vrs.size(), values.data());

    qDebug() << "FMU::getReals - values size: " << values.size();

    if(s != fmi2OK)
    {
        qDebug() << "fmi2GetReal failed";
        return {};
    }

    return values;
}

bool FMU::setBooleans(const std::vector<QString>& names, const std::vector<bool>& values)
{
    if(!fmi2SetBooleanPtr || !instance)
    {
        return false;
    }

    if (names.size() != values.size())
    {
        return false;
    }

    std::vector<fmiValueReference> vrs;
    std::vector<fmi2Boolean> vals;

    for (int i = 0; i < names.size(); i++)
    {
        if (!this->varmap.contains(names[i]))
        {
            return false;
        }
        const FmuVar& var = this->varmap[names[i]];
        if (var.type != Boolean)
        {
            qDebug() << "type mismath";
            return false;
        }
        vrs.push_back(var.vr);
        vals.push_back(int(values[i]));
    }

    fmi2Status s = this->fmi2SetBooleanPtr(this->instance, vrs.data(), vrs.size(), vals.data());
    return (s==fmi2OK);
}

std::vector<bool> FMU::getBooleans(const std::vector<QString>& names)
{
    std::vector<bool> result;
    if(!fmi2GetBooleanPtr || !instance)
    {
        return {};
    }

    std::vector<fmiValueReference> vrs;
    std::vector<fmi2Boolean> values(names.size(), 0);

    for (int i = 0; i < names.size(); i++)
    {
        if (!this->varmap.contains(names[i]))
        {
            return {};
        }
        const FmuVar& var = this->varmap[names[i]];
        if (var.type != Boolean)
        {
            qDebug() << "type mismath";
            return {};
        }
        vrs.push_back(var.vr);
    }

    fmi2Status s = fmi2GetBooleanPtr(this->instance, vrs.data(), vrs.size(), values.data());

    if(s != fmi2OK)
    {
        qDebug() << "fmi2GetBoolean failed";
        return {};
    }

    result.assign(values.begin(), values.end());
    return result;
}

bool FMU::setIntegers(const std::vector<QString>& names, const std::vector<int>& values)
{
    if(!fmi2SetIntegerPtr || !instance)
    {
        return false;
    }

    if (names.size() != values.size())
    {
        return false;
    }

    std::vector<fmiValueReference> vrs;
    std::vector<fmi2Integer> vals;

    for (int i = 0; i < names.size(); i++)
    {
        if (!this->varmap.contains(names[i]))
        {
            return false;
        }
        const FmuVar& var = this->varmap[names[i]];
        if (var.type != Integer)
        {
            qDebug() << "type mismath";
            return false;
        }
        vrs.push_back(var.vr);
        vals.push_back(values[i]);
    }

    fmi2Status s = this->fmi2SetIntegerPtr(this->instance, vrs.data(), vrs.size(), vals.data());
    return (s==fmi2OK);
}

std::vector<int> FMU::getIntegers(const std::vector<QString>& names)
{
    std::vector<int> result;
    if(!fmi2GetIntegerPtr || !instance)
    {
        return {};
    }

    std::vector<fmiValueReference> vrs;
    std::vector<fmi2Integer> values(names.size(), 0);

    for (int i = 0; i < names.size(); i++)
    {
        if (!this->varmap.contains(names[i]))
        {
            return {};
        }
        const FmuVar& var = this->varmap[names[i]];
        if (var.type != Integer)
        {
            qDebug() << "type mismath";
            return {};
        }
        vrs.push_back(var.vr);
    }

    fmi2Status s = fmi2GetIntegerPtr(this->instance, vrs.data(), vrs.size(), values.data());

    if(s != fmi2OK)
    {
        qDebug() << "fmi2GetInteger failed";
        return {};
    }

    result.assign(values.begin(), values.end());
    return result;
}


bool FMU::doStep(double currentTime, double stepSize)
{
    qDebug() << "[doStep] fmi2DoStepPtr:" << (void*)this->fmi2DoStepPtr
             << "instance:" << this->instance
             << "fmi2SetRealPtr:" << (void*)this->fmi2SetRealPtr;

    if(!this->fmi2DoStepPtr || !this->instance)
    {
        qDebug() << "[doStep] FAILED - cannot execute";
        return false;
    }

    qDebug() << "[doStep] calling fmi2DoStep - currentTime:" << currentTime
             << "stepSize:" << stepSize;

    fmi2Status s = this->fmi2DoStepPtr(this->instance, currentTime, stepSize, 1);

    qDebug() << "[doStep] fmi2DoStep returned status:" << (int)s
             << "(0=OK,1=Warning,2=Discard,3=Error,4=Fatal,5=Pending)";

    return (s==fmi2OK);
}


bool FMU::setStrings(const std::vector<QString>& names, const std::vector<QString>& values)
{
    std::vector<int> result;
    if(!fmi2GetStringPtr || !instance)
    {
        return false;
    }

    if (names.size() != values.size())
    {
        return false;
    }

    std::vector<fmiValueReference> vrs;
    std::vector<fmi2String> vals;
    std::vector<QByteArray> buffs;

    for (int i = 0; i < names.size(); i++)
    {
        if (!this->varmap.contains(names[i]))
        {
            return false;
        }
        const FmuVar& var = this->varmap[names[i]];
        if (var.type != String)
        {
            qDebug() << "type mismath";
            return false;
        }
        vrs.push_back(var.vr);
        buffs.push_back(values[i].toUtf8());
        vals.push_back(buffs.back().constData());
    }
    fmi2Status s = this->fmi2SetStringPtr(this->instance, vrs.data(), vrs.size(), vals.data());
    return (s==fmi2OK);
}


std::vector<QString> FMU::getStrings(const std::vector<QString>& names)
{
    std::vector<QString> result;
    if(!fmi2GetStringPtr || !instance)
    {
        return {};
    }

    std::vector<fmiValueReference> vrs;
    std::vector<fmi2String> values(names.size());

    for (int i = 0; i < names.size(); i++)
    {
        if (!this->varmap.contains(names[i]))
        {
            return {};
        }
        const FmuVar& var = this->varmap[names[i]];
        if (var.type != String)
        {
            qDebug() << "type mismath";
            return {};
        }
        vrs.push_back(var.vr);
    }

    fmi2Status s = fmi2GetStringPtr(this->instance, vrs.data(), vrs.size(), values.data());

    if(s != fmi2OK)
    {
        qDebug() << "fmi2GetStrings failed";
        return {};
    }

    for(auto it: values)
    {
        result.push_back(QString::fromUtf8(it));
    }

    return result;
}
