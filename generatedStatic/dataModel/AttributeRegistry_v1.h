#ifndef ATTRIBUTEREGISTRY_V1
#define ATTRIBUTEREGISTRY_V1

#include "AttributeRegistry_v1.h"
#include "Resource_v1.h"

enum class AttributeRegistryV1AttributeType
{
    Enumeration,
    String,
    Integer,
    Boolean,
    Password,
};
enum class AttributeRegistryV1DependencyType
{
    Map,
};
enum class AttributeRegistryV1MapFromCondition
{
    EQU,
    NEQ,
    GTR,
    GEQ,
    LSS,
    LEQ,
};
enum class AttributeRegistryV1MapFromProperty
{
    CurrentValue,
    DefaultValue,
    ReadOnly,
    WriteOnly,
    GrayOut,
    Hidden,
    LowerBound,
    UpperBound,
    MinLength,
    MaxLength,
    ScalarIncrement,
};
enum class AttributeRegistryV1MapTerms
{
    AND,
    OR,
};
enum class AttributeRegistryV1MapToProperty
{
    CurrentValue,
    DefaultValue,
    ReadOnly,
    WriteOnly,
    GrayOut,
    Hidden,
    Immutable,
    HelpText,
    WarningText,
    DisplayName,
    DisplayOrder,
    LowerBound,
    UpperBound,
    MinLength,
    MaxLength,
    ScalarIncrement,
    ValueExpression,
};
struct AttributeRegistryV1OemActions
{};
struct AttributeRegistryV1Actions
{
    AttributeRegistryV1OemActions oem;
};
struct AttributeRegistryV1SupportedSystems
{
    std::string productName;
    std::string systemId;
    std::string firmwareVersion;
};
struct AttributeRegistryV1AttributeValue
{
    std::string valueName;
    std::string valueDisplayName;
};
struct AttributeRegistryV1Attributes
{
    std::string attributeName;
    AttributeRegistryV1AttributeType type;
    AttributeRegistryV1AttributeValue value;
    std::string displayName;
    std::string helpText;
    std::string warningText;
    std::string currentValue;
    std::string defaultValue;
    int64_t displayOrder;
    std::string menuPath;
    bool readOnly;
    bool writeOnly;
    bool grayOut;
    bool hidden;
    bool immutable;
    bool isSystemUniqueProperty;
    int64_t maxLength;
    int64_t minLength;
    int64_t scalarIncrement;
    int64_t upperBound;
    int64_t lowerBound;
    std::string valueExpression;
    bool resetRequired;
    std::string uefiDevicePath;
    std::string uefiKeywordName;
    std::string uefiNamespaceId;
    ResourceV1Resource oem;
};
struct AttributeRegistryV1Menus
{
    std::string menuName;
    std::string displayName;
    int64_t displayOrder;
    bool readOnly;
    bool grayOut;
    std::string menuPath;
    bool hidden;
    ResourceV1Resource oem;
};
struct AttributeRegistryV1MapFrom
{
    std::string mapFromAttribute;
    AttributeRegistryV1MapFromProperty mapFromProperty;
    AttributeRegistryV1MapFromCondition mapFromCondition;
    std::string mapFromValue;
    AttributeRegistryV1MapTerms mapTerms;
};
struct AttributeRegistryV1Dependency
{
    AttributeRegistryV1MapFrom mapFrom;
    std::string mapToAttribute;
    AttributeRegistryV1MapToProperty mapToProperty;
    std::string mapToValue;
};
struct AttributeRegistryV1Dependencies
{
    AttributeRegistryV1Dependency dependency;
    std::string dependencyFor;
    AttributeRegistryV1DependencyType type;
};
struct AttributeRegistryV1RegistryEntries
{
    AttributeRegistryV1Attributes attributes;
    AttributeRegistryV1Menus menus;
    AttributeRegistryV1Dependencies dependencies;
};
struct AttributeRegistryV1AttributeRegistry
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string language;
    std::string registryVersion;
    std::string owningEntity;
    AttributeRegistryV1SupportedSystems supportedSystems;
    AttributeRegistryV1RegistryEntries registryEntries;
    AttributeRegistryV1Actions actions;
};
#endif
