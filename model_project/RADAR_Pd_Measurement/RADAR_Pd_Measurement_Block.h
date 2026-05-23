#ifndef RADAR_PD_MEASUREMENT_BLOCK_H
#define RADAR_PD_MEASUREMENT_BLOCK_H
#include "RADAR_Pd_Measurement.h"
#include "Block.h"
#include "SimulationControl.h"

#ifdef __has_include
    #if __has_include(<filesystem>)
        #include <filesystem>
        namespace fs = std::filesystem;
    #elif __has_include(<experimental/filesystem>)
        #include <experimental/filesystem>
        namespace fs = std::experimental::filesystem;
    #else
        #error "Missing filesystem support"
    #endif
#else
    // 对于不支持 __has_include 的编译器
    #if defined(_WIN32) || defined(__cpp_lib_filesystem)
        #include <filesystem>
        namespace fs = std::filesystem;
    #else
        #include <experimental/filesystem>
        namespace fs = std::experimental::filesystem;
    #endif
#endif

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Pd_Measurement_Block : public Block
{
public:
    RADAR_Pd_Measurement_Block(const std::string& name);
    ~RADAR_Pd_Measurement_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;
    bool Done() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    void cleanup();
    bool openFileForAppend();
    bool ModelSetup();

    std::unique_ptr<RADAR_Pd_Measurement> m_radar;

    int Start;
    int PRI_NUM;
    int FFT_Size;
    int DetectionNum;
    int TargetsInPRI;
    double TargetThreshold;
    char* FileName;

    std::string m_UserId;

    QFile m_qfile;
    QTextStream m_stream;
    QString m_fullPath;
    QString m_WritePath;

    bool DetectStatus;
    int DetectCount;

    SinkControl m_control;
};
RegAlgo(RADAR_Pd_Measurement_Block);
#endif // RADAR_PD_MEASUREMENT_BLOCK_H
