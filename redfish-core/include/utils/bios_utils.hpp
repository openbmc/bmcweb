// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <string>
namespace redfish
{
namespace bios_utils
{
inline std::string getBiosAttrType(const std::string& attrType)
{
    std::string type;
    if (attrType ==
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Enumeration")
    {
        type = "Enumeration";
    }
    else if (attrType ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String")
    {
        type = "String";
    }
    else if (attrType ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Password")
    {
        type = "Password";
    }
    else if (attrType ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer")
    {
        type = "Integer";
    }
    else if (attrType ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Boolean")
    {
        type = "Boolean";
    }
    else
    {
        type = "UNKNOWN";
    }
    return type;
}
} // namespace bios_utils
} // namespace redfish
