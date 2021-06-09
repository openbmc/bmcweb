#ifndef ATTRIBUTEREGISTRY_V1
#define ATTRIBUTEREGISTRY_V1

#include "AttributeRegistry_v1.h"
#include "Resource_v1.h"

enum class AttributeRegistry_v1_AttributeType {
    Enumeration,
    String,
    Integer,
    Boolean,
    Password,
};
enum class AttributeRegistry_v1_DependencyType {
    Map,
};
enum class AttributeRegistry_v1_MapFromCondition {
    EQU,
    NEQ,
    GTR,
    GEQ,
    LSS,
    LEQ,
};
enum class AttributeRegistry_v1_MapFromProperty {
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
enum class AttributeRegistry_v1_MapTerms {
    AND,
    OR,
};
enum class AttributeRegistry_v1_MapToProperty {
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
struct AttributeRegistry_v1_Actions
{
    AttributeRegistry_v1_OemActions oem;
};
struct AttributeRegistry_v1_AttributeRegistry
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string language;
    std::string registryVersion;
    std::string owningEntity;
    AttributeRegistry_v1_SupportedSystems supportedSystems;
    AttributeRegistry_v1_RegistryEntries registryEntries;
    AttributeRegistry_v1_Actions actions;
};
struct AttributeRegistry_v1_Attributes
{
    std::string attributeName;
    AttributeRegistry_v1_AttributeType type;
    AttributeRegistry_v1_AttributeValue value;
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
    Resource_v1_Resource oem;
};
struct AttributeRegistry_v1_AttributeValue
{
    std::string valueName;
    std::string valueDisplayName;
};
struct AttributeRegistry_v1_Dependencies
{
    AttributeRegistry_v1_Dependency dependency;
    std::string dependencyFor;
    AttributeRegistry_v1_DependencyType type;
};
struct AttributeRegistry_v1_Dependency
{
    AttributeRegistry_v1_MapFrom mapFrom;
    std::string mapToAttribute;
    AttributeRegistry_v1_MapToProperty mapToProperty;
    std::string mapToValue;
};
struct AttributeRegistry_v1_MapFrom
{
    std::string mapFromAttribute;
    AttributeRegistry_v1_MapFromProperty mapFromProperty;
    AttributeRegistry_v1_MapFromCondition mapFromCondition;
    std::string mapFromValue;
    AttributeRegistry_v1_MapTerms mapTerms;
};
struct AttributeRegistry_v1_Menus
{
    std::string menuName;
    std::string displayName;
    int64_t displayOrder;
    bool readOnly;
    bool grayOut;
    std::string menuPath;
    bool hidden;
    Resource_v1_Resource oem;
};
struct AttributeRegistry_v1_OemActions
{
};
struct AttributeRegistry_v1_RegistryEntries
{
    AttributeRegistry_v1_Attributes attributes;
    AttributeRegistry_v1_Menus menus;
    AttributeRegistry_v1_Dependencies dependencies;
};
struct AttributeRegistry_v1_SupportedSystems
{
    std::string productName;
    std::string systemId;
    std::string firmwareVersion;
};
#endif
