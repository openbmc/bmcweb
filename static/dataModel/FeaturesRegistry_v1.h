#ifndef FEATURESREGISTRY_V1
#define FEATURESREGISTRY_V1

#include "FeaturesRegistry_v1.h"
#include "Resource_v1.h"

struct FeaturesRegistry_v1_Actions
{
    FeaturesRegistry_v1_OemActions oem;
};
struct FeaturesRegistry_v1_FeaturesRegistry
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string language;
    std::string registryPrefix;
    std::string registryVersion;
    std::string owningEntity;
    FeaturesRegistry_v1_FeaturesRegistryProperty features;
    FeaturesRegistry_v1_Actions actions;
};
struct FeaturesRegistry_v1_FeaturesRegistryProperty
{};
struct FeaturesRegistry_v1_OemActions
{};
struct FeaturesRegistry_v1_SupportedFeature
{
    std::string featureName;
    std::string description;
    std::string version;
    std::string correspondingProfileDefinition;
};
#endif
