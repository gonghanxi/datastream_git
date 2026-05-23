#ifndef ILOGWRITER_H
#define ILOGWRITER_H

#include <string>
class ILogWriter{
public:
    virtual void write(std::string msg)=0;
};

#endif // ILOGWRITER_H
