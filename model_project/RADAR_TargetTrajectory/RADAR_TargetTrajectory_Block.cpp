#include "RADAR_TargetTrajectory_Block.h"
namespace {
std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}
RADAR_TargetTrajectory_Block::RADAR_TargetTrajectory_Block(const std::string &name)
    :Block(name)
{

}

bool RADAR_TargetTrajectory_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_TargetTrajectory_Block::Run()
{
    if (!m_radar->Run()) {
        return false;
    }

    //可选输出端口
    Buffer* Target_Pos_Z_port = GetOutputPort("Target_Pos_Z");
    Buffer* Target_Pos_Y_port = GetOutputPort("Target_Pos_Y");
    Buffer* Target_Pos_X_port = GetOutputPort("Target_Pos_X");
    Buffer* Target_Az_Angle_port = GetOutputPort("Target_Az_Angle");
    Buffer* Target_El_Angle_port = GetOutputPort("Target_El_Angle");

    //必选输出端口
    Buffer* Delay_Output_port = GetOutputPort("Delay_Output");
    Buffer* Range_Output_port = GetOutputPort("Range_Output");

    //可选端口输出
    if(Target_Pos_Z_port->GetReaderCount() != 0) {
        std::vector<double> Target_Pos_Z_Data;
        Target_Pos_Z_Data.push_back(m_radar->Target_Pos_Z[0U]);
        WriteOutputData(Target_Pos_Z_port->GetName(), Target_Pos_Z_Data);
    }
    if(Target_Pos_Y_port->GetReaderCount() != 0) {
        std::vector<double> Target_Pos_Y_Data;
        Target_Pos_Y_Data.push_back(m_radar->Target_Pos_Y[0U]);
        WriteOutputData(Target_Pos_Y_port->GetName(), Target_Pos_Y_Data);
    }
    if(Target_Pos_X_port->GetReaderCount() != 0) {
        std::vector<double> Target_Pos_X_Data;
        Target_Pos_X_Data.push_back(m_radar->Target_Pos_X[0U]);
        WriteOutputData(Target_Pos_X_port->GetName(), Target_Pos_X_Data);
    }
    if(Target_Az_Angle_port->GetReaderCount() != 0) {
        std::vector<double> Target_Az_Angle_Data;
        Target_Az_Angle_Data.push_back(m_radar->Target_Az_Angle[0U]);
        WriteOutputData(Target_Az_Angle_port->GetName(), Target_Az_Angle_Data);
    }
    if(Target_El_Angle_port->GetReaderCount() != 0) {
        std::vector<double> Target_El_Angle_Data;
        Target_El_Angle_Data.push_back(m_radar->Target_El_Angle[0U]);
        WriteOutputData(Target_El_Angle_port->GetName(), Target_El_Angle_Data);
    }


    //必选端口输出
    std::vector<double> Delay_OutputData;
    std::vector<double> Range_OutputData;
    Delay_OutputData.push_back(m_radar->Delay_Output[0U]);
    Range_OutputData.push_back(m_radar->Range_Output[0U]);

    WriteOutputData(Delay_Output_port->GetName(), Delay_OutputData);
    WriteOutputData(Range_Output_port->GetName(), Range_OutputData);

    return true;
}

bool RADAR_TargetTrajectory_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);
    m_radar = std::make_unique<RADAR_TargetTrajectory>();
    SetDefaultParameters();

    try {
        Coordinate_Mode = ConvertStringToCoordinate_ModeEnum(getParameter("Coordinate_Mode").Value);

        Range_Initial = std::stod(getParameter("Range_Initial").Value);
        ElevationAngle = std::stod(getParameter("ElevationAngle").Value);
        AzimuthAngle = std::stod(getParameter("AzimuthAngle").Value);
        Velocity_Initial = std::stod(getParameter("Velocity_Initial").Value);
        Accelerate = std::stod(getParameter("Accelerate").Value);
        Jerk = std::stod(getParameter("Jerk").Value);
        TimeStep = std::stod(getParameter("TimeStep").Value);

        std::string PositionString = getParameter("Position_Initial_XYZ").Value;
        parseArrayString(PositionString, Position_Initial_XYZ_data);

        std::string VelocityString = getParameter("Velocity_Initial_XYZ").Value;
        parseArrayString(VelocityString, Velocity_Initial_XYZ_data);

        std::string AccelerateString = getParameter("Accelerate_XYZ").Value;
        parseArrayString(AccelerateString, Accelerate_XYZ_data);

        std::string JerkString = getParameter("Jerk_XYZ").Value;
        parseArrayString(JerkString, Jerk_XYZ_data);


    } catch (...) {

    }

    SetParameters();

    //可选输出
    AddOutputPort("Target_Pos_Z", m_radar->Target_Pos_Z, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Target_Pos_Y", m_radar->Target_Pos_Y, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Target_Pos_X", m_radar->Target_Pos_X, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Target_Az_Angle", m_radar->Target_Az_Angle, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Target_El_Angle", m_radar->Target_El_Angle, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    //必选输出
    AddOutputPort("Delay_Output", m_radar->Delay_Output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Range_Output", m_radar->Range_Output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void RADAR_TargetTrajectory_Block::SetDefaultParameters()
{
    Coordinate_Mode = RADAR_TargetTrajectory::Spherical;

    Range_Initial = 20e3;
    ElevationAngle = 0;
    AzimuthAngle = 0;
    Velocity_Initial = 0;
    Accelerate = 0;
    Jerk = 0;

    Position_Initial_XYZ_data.clear();
    Position_Initial_XYZ_data.push_back(0);
    Position_Initial_XYZ_data.push_back(0);
    Position_Initial_XYZ_data.push_back(0);
    Position_Initial_XYZ = Position_Initial_XYZ_data.data();
    Position_Initial_XYZSize = 3;

    Velocity_Initial_XYZ_data.clear();
    Velocity_Initial_XYZ_data.push_back(0);
    Velocity_Initial_XYZ_data.push_back(0);
    Velocity_Initial_XYZ_data.push_back(0);
    Velocity_Initial_XYZ = Velocity_Initial_XYZ_data.data();
    Velocity_Initial_XYZSize = 3;

    Accelerate_XYZ_data.clear();
    Accelerate_XYZ_data.push_back(0);
    Accelerate_XYZ_data.push_back(0);
    Accelerate_XYZ_data.push_back(0);
    Accelerate_XYZ = Accelerate_XYZ_data.data();
    Accelerate_XYZSize = 3;

    Jerk_XYZ_data.clear();
    Jerk_XYZ_data.push_back(0);
    Jerk_XYZ_data.push_back(0);
    Jerk_XYZ_data.push_back(0);
    Jerk_XYZ = Jerk_XYZ_data.data();
    Jerk_XYZSize = 3;

    TimeStep = 1e-9;
}

RADAR_TargetTrajectory::Coordinate_ModeEnum RADAR_TargetTrajectory_Block::ConvertStringToCoordinate_ModeEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "spherical" || lower == "0") return RADAR_TargetTrajectory::Spherical;
    if(lower == "cartesian" || lower == "1") return RADAR_TargetTrajectory::Cartesian;
    return RADAR_TargetTrajectory::Spherical;
}

bool RADAR_TargetTrajectory_Block::parseArrayString(const std::string &arrayStr, std::vector<double> &outArray)
{
    outArray.clear();

    std::string str = arrayStr;
    // 去除首尾空格
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    size_t end = str.find_last_not_of(" \t\n\r");
    str = str.substr(start, end - start + 1);

    // 检查是否是数组格式
    if (str.empty() || str.front() != '[' || str.back() != ']') {
        return false;
    }

    // 去除外层括号
    std::string content = str.substr(1, str.length() - 2);

    // 去除首尾空格
    start = content.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        // 空数组
        return true;
    }
    end = content.find_last_not_of(" \t\n\r");
    content = content.substr(start, end - start + 1);

    // 按逗号分割
    std::stringstream ss(content);
    std::string item;

    while (std::getline(ss, item, ',')) {
        // 去除空格
        start = item.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        end = item.find_last_not_of(" \t\n\r");
        item = item.substr(start, end - start + 1);

        if (!item.empty()) {
            try {
                int value = std::stoi(item);
                outArray.push_back(value);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to parse array element: ", item, " - ", e.what());
                return false;
            }
        }
    }

    return true;
}

void RADAR_TargetTrajectory_Block::SetParameters()
{
    Position_Initial_XYZ = Position_Initial_XYZ_data.data();
    Position_Initial_XYZSize = static_cast<int>(Position_Initial_XYZ_data.size());

    Velocity_Initial_XYZ = Velocity_Initial_XYZ_data.data();
    Velocity_Initial_XYZSize = static_cast<int>(Velocity_Initial_XYZ_data.size());

    Accelerate_XYZ = Accelerate_XYZ_data.data();
    Accelerate_XYZSize = static_cast<int>(Accelerate_XYZ_data.size());

    Jerk_XYZ = Jerk_XYZ_data.data();
    Jerk_XYZSize = static_cast<int>(Jerk_XYZ_data.size());


    if(!m_radar) return;
    m_radar->Coordinate_Mode = Coordinate_Mode;
    m_radar->Range_Initial = Range_Initial;
    m_radar->ElevationAngle = ElevationAngle;
    m_radar->AzimuthAngle = AzimuthAngle;
    m_radar->Velocity_Initial = Velocity_Initial;
    m_radar->Accelerate = Accelerate;
    m_radar->Jerk = Jerk;

    m_radar->Position_Initial_XYZ = Position_Initial_XYZ;
    m_radar->Position_Initial_XYZSize = Position_Initial_XYZSize;

    m_radar->Velocity_Initial_XYZ = Velocity_Initial_XYZ;
    m_radar->Velocity_Initial_XYZSize = Velocity_Initial_XYZSize;

    m_radar->Accelerate_XYZ = Accelerate_XYZ;
    m_radar->Accelerate_XYZSize = Accelerate_XYZSize;

    m_radar->Jerk_XYZ = Jerk_XYZ;
    m_radar->Jerk_XYZSize = Jerk_XYZSize;

    m_radar->TimeStep = TimeStep;
}

