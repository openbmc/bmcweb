#ifndef ORG
#define ORG

#include "Org.OData.Capabilities.V1.h"

enum class Org.OData.Capabilities.V1ConformanceLevelType{
    Minimal,
    Intermediate,
    Advanced,
};
enum class Org.OData.Capabilities.V1FilterExpressionType{
    SingleValue,
    MultiValue,
    SingleInterval,
};
enum class Org.OData.Capabilities.V1IsolationLevel{
    Snapshot,
};
enum class Org.OData.Capabilities.V1NavigationType{
    Recursive,
    Single,
    None,
};
enum class Org.OData.Capabilities.V1SearchExpressions{
    none, AND, OR, NOT, phrase, group,
};
struct Org.OData.Capabilities.V1CallbackProtocol
{
    std::string id;
    std::string urlTemplate;
    std::string documentationUrl;
};
struct Org.OData.Capabilities.V1CallbackType
{
    OrgCallbackProtocol callbackProtocols;
};
struct Org.OData.Capabilities.V1ChangeTrackingType
{
    bool supported;
    std::string filterableProperties;
    std::string expandableProperties;
};
struct Org.OData.Capabilities.V1CountRestrictionsType
{
    bool countable;
    std::string nonCountableProperties;
    std::string nonCountableNavigationProperties;
};
struct Org.OData.Capabilities.V1DeleteRestrictionsType
{
    bool deletable;
    std::string nonDeletableNavigationProperties;
};
struct Org.OData.Capabilities.V1ExpandRestrictionsType
{
    bool expandable;
    std::string nonExpandableProperties;
};
struct Org.OData.Capabilities.V1FilterExpressionRestrictionType
{
    std::string property;
    OrgFilterExpressionType allowedExpressions;
};
struct Org.OData.Capabilities.V1FilterRestrictionsType
{
    bool filterable;
    bool requiresFilter;
    std::string requiredProperties;
    std::string nonFilterableProperties;
    OrgFilterExpressionRestrictionType filterExpressionRestrictions;
};
struct Org.OData.Capabilities.V1InsertRestrictionsType
{
    bool insertable;
    std::string nonInsertableNavigationProperties;
};
struct Org.OData.Capabilities.V1NavigationPropertyRestriction
{
    std::string navigationProperty;
    OrgNavigationType navigability;
};
struct Org.OData.Capabilities.V1NavigationRestrictionsType
{
    OrgNavigationType navigability;
    OrgNavigationPropertyRestriction restrictedProperties;
};
struct Org.OData.Capabilities.V1SearchRestrictionsType
{
    bool searchable;
    OrgSearchExpressions unsupportedExpressions;
};
struct Org.OData.Capabilities.V1SortRestrictionsType
{
    bool sortable;
    std::string ascendingOnlyProperties;
    std::string descendingOnlyProperties;
    std::string nonSortableProperties;
};
struct Org.OData.Capabilities.V1UpdateRestrictionsType
{
    bool updatable;
    std::string nonUpdatableNavigationProperties;
};
#endif
