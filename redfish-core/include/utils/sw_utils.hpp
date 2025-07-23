// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/resource.hpp"

#include <asm-generic/errno.h>

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace redfish
{
namespace sw_util
{
/* @brief String that indicates a bios software instance */
constexpr const char* biosPurpose =
    "xyz.openbmc_project.Software.Version.VersionPurpose.Host";

/* @brief String that indicates a BMC software instance */
constexpr const char* bmcPurpose =
    "xyz.openbmc_project.Software.Version.VersionPurpose.BMC";

/* @brief String that indicates a System software instance */
constexpr const char* systemPurpose =
    "xyz.openbmc_project.Software.Version.VersionPurpose.System";

std::optional<sdbusplus::message::object_path> getFunctionalSoftwarePath(
    const std::string& swType);

void afterGetProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& swId, bool runningImage,
    const std::string& swVersionPurpose,
    const std::string& activeVersionPropName, bool populateLinkToImages,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& propertiesList);

void afterGetSubtree(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& swVersionPurpose,
                     const std::string& activeVersionPropName,
                     bool populateLinkToImages,
                     const std::vector<std::string>& functionalSwIds,
                     const boost::system::error_code& ec,
                     const dbus::utility::MapperGetSubTreeResponse& subtree);

void afterAssociatedEndpoints(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& swVersionPurpose,
    const std::string& activeVersionPropName, bool populateLinkToImages,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& functionalSw);

/**
 * @brief Populate the running software version and image links
 *
 * @param[i,o] asyncResp             Async response object
 * @param[i]   swVersionPurpose  Indicates what target to look for
 * @param[i]   activeVersionPropName  Index in asyncResp->res.jsonValue to write
 * the running software version to
 * @param[i]   populateLinkToImages  Populate asyncResp->res "Links"
 * "ActiveSoftwareImage" with a link to the running software image and
 * "SoftwareImages" with a link to the all its software images
 *
 * @return void
 */
void populateSoftwareInformation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& swVersionPurpose,
    const std::string& activeVersionPropName, bool populateLinkToImages);

/**
 * @brief Translate input swState to Redfish state
 *
 * This function will return the corresponding Redfish state
 *
 * @param[i]   swState  The OpenBMC software state
 *
 * @return The corresponding Redfish state
 */
resource::State getRedfishSwState(const std::string& swState);

/**
 * @brief Translate input swState to Redfish health state
 *
 * This function will return the corresponding Redfish health state
 *
 * @param[i]   swState  The OpenBMC software state
 *
 * @return The corresponding Redfish health state
 */
std::string getRedfishSwHealth(const std::string& swState);

/**
 * @brief Put LowestSupportedVersion of input swId into json response
 *
 * This function will put the MinimumVersion from D-Bus of the input
 * software id to ["LowestSupportedVersion"].
 *
 * @param[i,o] asyncResp    Async response object
 * @param[i] swId The software ID to get Minimum Version for
 * @param[i]   dbusSvc  The dbus service implementing the software object
 *
 * @return void
 */
void getSwMinimumVersion(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::shared_ptr<std::string>& swId,
                         const std::string& dbusSvc);

/**
 * @brief Put status of input swId into json response
 *
 * This function will put the appropriate Redfish state of the input
 * software id to ["Status"]["State"] within the json response
 *
 * @param[i,o] asyncResp    Async response object
 * @param[i]   swId     The software ID to get status for
 * @param[i]   dbusSvc  The dbus service implementing the software object
 *
 * @return void
 */
void getSwStatus(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                 const std::shared_ptr<std::string>& swId,
                 const std::string& dbusSvc);

void handleUpdateableEndpoints(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<std::string>& swId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& objPaths);

void handleUpdateableObject(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const boost::system::error_code& ec,
                            const dbus::utility::MapperGetObject& objectInfo);

/**
 * @brief Updates programmable status of input swId into json response
 *
 * This function checks whether software inventory component
 * can be programmable or not and fill's the "Updatable"
 * Property.
 *
 * @param[i,o] asyncResp  Async response object
 * @param[i]   swId       The software ID
 */
void getSwUpdatableStatus(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::shared_ptr<std::string>& swId);

} // namespace sw_util
} // namespace redfish
