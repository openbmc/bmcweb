#ifndef OEMMANAGER_V1
#define OEMMANAGER_V1

#include "NavigationReference__.h"
#include "OemManager_v1.h"

struct OemManager_v1_Fan
{
    OemManager_v1_FanControllers fanControllers;
    OemManager_v1_PidControllers pidControllers;
    OemManager_v1_StepwiseControllers stepwiseControllers;
    OemManager_v1_FanZones fanZones;
    std::string profile;
};
struct OemManager_v1_FanController
{
    double fFGainCoefficient;
    double fFOffCoefficient;
    double iCoefficient;
    double iLimitMax;
    double iLimitMin;
    std::string inputs;
    double outLimitMax;
    double outLimitMin;
    double negativeHysteresis;
    double positiveHysteresis;
    std::string outputs;
    double pCoefficient;
    double slewNeg;
    double slewPos;
    NavigationReference__ zones;
};
struct OemManager_v1_FanControllers
{};
struct OemManager_v1_FanZone
{
    double failSafePercent;
    double minThermalOutput;
    NavigationReference__ chassis;
};
struct OemManager_v1_FanZones
{};
struct OemManager_v1_Oem
{
    OemManager_v1_OpenBmc openBmc;
};
struct OemManager_v1_OpenBmc
{
    OemManager_v1_Fan fan;
};
struct OemManager_v1_PidController
{
    double fFGainCoefficient;
    double fFOffCoefficient;
    double iCoefficient;
    double iLimitMax;
    double iLimitMin;
    std::string inputs;
    double outLimitMax;
    double outLimitMin;
    double negativeHysteresis;
    double positiveHysteresis;
    double pCoefficient;
    double setPoint;
    std::string setPointOffset;
    double slewNeg;
    double slewPos;
    NavigationReference__ zones;
};
struct OemManager_v1_PidControllers
{};
struct OemManager_v1_StepwiseController
{
    std::string inputs;
    double negativeHysteresis;
    double positiveHysteresis;
    std::string direction;
    NavigationReference__ zones;
};
struct OemManager_v1_StepwiseControllers
{};
struct OemManager_v1_StepwiseSteps
{
    double target;
    double output;
};
#endif
