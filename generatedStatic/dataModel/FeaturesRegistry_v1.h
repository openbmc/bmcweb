#ifndef FEATURESREGISTRY_V1
#define FEATURESREGISTRY_V1

#include "FeaturesRegistry_v1.h"
#include "Resource_v1.h"

struct FeaturesRegistryV1OemActions
{};
struct FeaturesRegistryV1Actions
{
    FeaturesRegistryV1OemActions oem;
};
struct FeaturesRegistryV1FeaturesRegistryProperty
{};
struct FeaturesRegistryV1FeaturesRegistry
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string language;
    std::string registryPrefix;
    std::string registryVersion;
    std::string owningEntity;
    FeaturesRegistryV1FeaturesRegistryProperty features;
    FeaturesRegistryV1Actions actions;
};
struct FeaturesRegistryV1SupportedFeature
{
    std::string featureName;
    std::string description;
    std::string version;
    std::string correspondingProfileDefinition;
};
#endif
