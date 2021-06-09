#ifndef OEMMANAGER_V1
#define OEMMANAGER_V1

#include "NavigationReference_.h"
#include "OemManager_v1.h"

struct OemManagerV1FanControllers
{};
struct OemManagerV1PidControllers
{};
struct OemManagerV1StepwiseControllers
{};
struct OemManagerV1FanZones
{};
struct OemManagerV1Fan
{
    OemManagerV1FanControllers fanControllers;
    OemManagerV1PidControllers pidControllers;
    OemManagerV1StepwiseControllers stepwiseControllers;
    OemManagerV1FanZones fanZones;
    std::string profile;
};
struct OemManagerV1FanController
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
    NavigationReference_ zones;
};
struct OemManagerV1FanZone
{
    double failSafePercent;
    double minThermalOutput;
    NavigationReference_ chassis;
};
struct OemManagerV1OpenBmc
{
    OemManagerV1Fan fan;
};
struct OemManagerV1Oem
{
    OemManagerV1OpenBmc openBmc;
};
struct OemManagerV1PidController
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
    NavigationReference_ zones;
};
struct OemManagerV1StepwiseController
{
    std::string inputs;
    double negativeHysteresis;
    double positiveHysteresis;
    std::string direction;
    NavigationReference_ zones;
};
struct OemManagerV1StepwiseSteps
{
    double target;
    double output;
};
#endif
