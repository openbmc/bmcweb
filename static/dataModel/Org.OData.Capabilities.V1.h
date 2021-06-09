#ifndef ORG
#define ORG

#include "Org.OData.Capabilities.V1.h"

enum class Org.OData.Capabilities.V1_ConformanceLevelType {
    Minimal,
    Intermediate,
    Advanced,
};
enum class Org.OData.Capabilities.V1_FilterExpressionType {
    SingleValue,
    MultiValue,
    SingleInterval,
};
enum class Org.OData.Capabilities.V1_IsolationLevel {
    Snapshot,
};
enum class Org.OData.Capabilities.V1_NavigationType {
    Recursive,
    Single,
    None,
};
enum class Org.OData.Capabilities.V1_SearchExpressions {
    none,
    AND,
    OR,
    NOT,
    phrase,
    group,
};
struct Org.OData.Capabilities.V1_CallbackProtocol
{
    std::string id;
    std::string urlTemplate;
    std::string documentationUrl;
};
struct Org.OData.Capabilities.V1_CallbackType
{
    Org_CallbackProtocol callbackProtocols;
};
struct Org.OData.Capabilities.V1_ChangeTrackingType
{
    bool supported;
    std::string filterableProperties;
    std::string expandableProperties;
};
struct Org.OData.Capabilities.V1_CountRestrictionsType
{
    bool countable;
    std::string nonCountableProperties;
    std::string nonCountableNavigationProperties;
};
struct Org.OData.Capabilities.V1_DeleteRestrictionsType
{
    bool deletable;
    std::string nonDeletableNavigationProperties;
};
struct Org.OData.Capabilities.V1_ExpandRestrictionsType
{
    bool expandable;
    std::string nonExpandableProperties;
};
struct Org.OData.Capabilities.V1_FilterExpressionRestrictionType
{
    std::string property;
    Org_FilterExpressionType allowedExpressions;
};
struct Org.OData.Capabilities.V1_FilterRestrictionsType
{
    bool filterable;
    bool requiresFilter;
    std::string requiredProperties;
    std::string nonFilterableProperties;
    Org_FilterExpressionRestrictionType filterExpressionRestrictions;
};
struct Org.OData.Capabilities.V1_InsertRestrictionsType
{
    bool insertable;
    std::string nonInsertableNavigationProperties;
};
struct Org.OData.Capabilities.V1_NavigationPropertyRestriction
{
    std::string navigationProperty;
    Org_NavigationType navigability;
};
struct Org.OData.Capabilities.V1_NavigationRestrictionsType
{
    Org_NavigationType navigability;
    Org_NavigationPropertyRestriction restrictedProperties;
};
struct Org.OData.Capabilities.V1_SearchRestrictionsType
{
    bool searchable;
    Org_SearchExpressions unsupportedExpressions;
};
struct Org.OData.Capabilities.V1_SortRestrictionsType
{
    bool sortable;
    std::string ascendingOnlyProperties;
    std::string descendingOnlyProperties;
    std::string nonSortableProperties;
};
struct Org.OData.Capabilities.V1_UpdateRestrictionsType
{
    bool updatable;
    std::string nonUpdatableNavigationProperties;
};
#endif
