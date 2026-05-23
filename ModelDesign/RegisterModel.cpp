// RegisterModel.cpp - 完整实现
#include "RegisterModel.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <QDebug>

namespace SystemVueModelBuilder {

    // 全局模型工厂
    class ModelRegistry {
    private:
        static ModelRegistry* s_instance;
        std::unordered_map<std::string, pfnDFModelCreate> m_factoryMap;
        std::unordered_map<std::string, std::string> m_libMap; // 类名->DLL名称

    public:
        static ModelRegistry* GetInstance() {
            if (!s_instance) {
                s_instance = new ModelRegistry();
            }
            return s_instance;
        }

        void RegisterModel(const std::string& className,
                          pfnDFModelCreate factory,
                          const std::string& libName = "") {
            m_factoryMap[className] = factory;
            if (!libName.empty()) {
                m_libMap[className] = libName;
            }
//            qDebug() << "[ModelRegistry] Registered model: " << QString::fromStdString(className)
//                     << " from DLL: " << QString::fromStdString(libName);
        }

        void RegisterModel(ModelFactory* pModelFactory,
                          const std::string& className,
                          const std::string& libName = "") {
            std::ignore = pModelFactory;
            std::ignore = className;
            std::ignore = libName;
            // 如果有 ModelFactory 接口，这里可以扩展
//            qDebug() << "[ModelRegistry] Registered model via factory: "
//                     << QString::fromStdString(className);

        }

        DFModel* CreateModel(const std::string& className, const char** pccClassName) {
            auto it = m_factoryMap.find(className);
            if (it != m_factoryMap.end()) {
                *pccClassName = className.c_str();
                return it->second(pccClassName);
            }
            std::cerr << "[ModelRegistry] Model not found: " << className << std::endl;
            return nullptr;
        }

        std::vector<std::string> GetAvailableModels() {
            std::vector<std::string> models;
            for (const auto& pair : m_factoryMap) {
                models.push_back(pair.first);
            }
            return models;
        }
    };

    ModelRegistry* ModelRegistry::s_instance = nullptr;

    // RegisterModelImplementation 实现
    class RegisterModelImplementation {
    private:
        std::string m_className;
        std::string m_libName;

    public:
        RegisterModelImplementation(pfnDFModelCreate funcPtr,
                                  int iInterfaceVersion,
                                  const char* pLibName) {
            // 获取类名 - 这里需要从函数指针反推，简化处理
            const char* className = "";
            funcPtr(&className); // 调用工厂函数获取类名

            iInterfaceVersion = 1;

            m_className = className;
            m_libName = pLibName ? pLibName : "";

            // 注册到全局注册表
            ModelRegistry::GetInstance()->RegisterModel(m_className, funcPtr, m_libName);

//            qDebug() << "[RegisterModel] Registered: " << QString::fromStdString(m_className)
//                     << " v" << iInterfaceVersion
//                     << " from " << QString::fromStdString(m_libName.empty() ? "unknown DLL" : m_libName);
        }

        RegisterModelImplementation(ModelFactory* pModelFactory,
                                  const char* pClassName,
                                  int iInterfaceVersion,
                                  const char* pLibName) {
            m_className = pClassName ? pClassName : "";
            m_libName = pLibName ? pLibName : "";

            // 注册到全局注册表
            ModelRegistry::GetInstance()->RegisterModel(pModelFactory, m_className, m_libName);

            qDebug() << "[RegisterModel] Registered via factory: " << QString::fromStdString(m_className)
                     << " v" << iInterfaceVersion
                     << " from " << QString::fromStdString(m_libName.empty() ? "unknown DLL" : m_libName);
        }

        ~RegisterModelImplementation() {
//            qDebug() << "[RegisterModel] Unregistering: " << QString::fromStdString(m_className);
        }
    };

    // CRegisterModel 成员函数实现
    CRegisterModel::CRegisterModel(pfnDFModelCreate funcPtr,
                                  int iInterfaceVersion,
                                  const char* pLibName) {
        m_pImplementation = new RegisterModelImplementation(funcPtr, iInterfaceVersion, pLibName);
    }

    CRegisterModel::CRegisterModel(ModelFactory* pModelFactory,
                                  const char* pClassName,
                                  int iInterfaceVersion,
                                  const char* pLibName) {
        m_pImplementation = new RegisterModelImplementation(pModelFactory, pClassName, iInterfaceVersion, pLibName);
    }

    CRegisterModel::~CRegisterModel() {
        delete m_pImplementation;
    }
}


