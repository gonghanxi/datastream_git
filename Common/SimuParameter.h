#ifndef SIMUPARAMETER_H
#define SIMUPARAMETER_H

#include <string>
#include <utility>

namespace SystemVueModelBuilder {

struct SimuParameter
{
    std::string simuName;
    double startTime;
    double stopTime;
    double samplingRate;
    double time_Interval;
    size_t num_Samples;
    std::string linkName;
    std::string subsystemName;
    std::string User_Id;


    SimuParameter()
        : startTime(0.0), stopTime(0.0), samplingRate(0.0),
          time_Interval(0.0), num_Samples(0), subsystemName("") {}


    SimuParameter(const SimuParameter& other)
        : simuName(other.simuName),
          startTime(other.startTime),
          stopTime(other.stopTime),
          samplingRate(other.samplingRate),
          time_Interval(other.time_Interval),
          num_Samples(other.num_Samples),
          linkName(other.linkName),
          subsystemName(other.subsystemName) {}


    SimuParameter& operator=(const SimuParameter& other) {
        if (this != &other) {
            simuName = other.simuName;
            startTime = other.startTime;
            stopTime = other.stopTime;
            samplingRate = other.samplingRate;
            time_Interval = other.time_Interval;
            num_Samples = other.num_Samples;
            linkName = other.linkName;
            subsystemName = other.subsystemName;
        }
        return *this;
    }


    SimuParameter(SimuParameter&& other) noexcept
        : simuName(std::move(other.simuName)),
          startTime(other.startTime),
          stopTime(other.stopTime),
          samplingRate(other.samplingRate),
          time_Interval(other.time_Interval),
          num_Samples(other.num_Samples),
          linkName(std::move(other.linkName)),
          subsystemName(std::move(other.subsystemName)) {}


    SimuParameter& operator=(SimuParameter&& other) noexcept {
        if (this != &other) {
            simuName = std::move(other.simuName);
            startTime = other.startTime;
            stopTime = other.stopTime;
            samplingRate = other.samplingRate;
            time_Interval = other.time_Interval;
            num_Samples = other.num_Samples;
            linkName = std::move(other.linkName);
            subsystemName = std::move(other.subsystemName);
        }
        return *this;
    }


    ~SimuParameter() = default;
};

} // namespace SystemVueModelBuilder

#endif // SIMUPARAMETER_H
