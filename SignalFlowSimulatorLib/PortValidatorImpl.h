#ifndef PORTVALIDATORIMPL_H
#define PORTVALIDATORIMPL_H

#include "ConnectionValidator.h"

class PortValidatorImpl
{
private:
    ConnectionValidator* m_validator;
public:
    explicit PortValidatorImpl(ConnectionValidator* validator) :m_validator(validator) {}

    ConnectionValidator::ValidationResult validatePortDirection();     // 端口方向校验
    ConnectionValidator::ValidationResult validatePortDataType();      // 端口数据类型兼容性
    ConnectionValidator::ValidationResult validateInputPortConnections(); // 检查输入端口连接完整性

    ConnectionValidator::ValidationResult validateSubLinkPortDataType(const QString& linkKey);
};

#endif // PORTVALIDATORIMPL_H
