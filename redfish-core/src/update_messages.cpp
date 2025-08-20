/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 * DO NOT modify this registry outside of running the
 * parse_registries.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/
#include "update_messages.hpp"

#include "registries.hpp"
#include "registries/update_message_registry.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

// Clang can't seem to decide whether this header needs to be included or not,
// and is inconsistent.  Include it for now
// NOLINTNEXTLINE(misc-include-cleaner)
#include <utility>

namespace redfish
{

namespace messages
{

static nlohmann::json::object_t getLog(redfish::registries::Update::Index name,
                                       std::span<const std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::Update::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(redfish::registries::Update::header,
                              redfish::registries::Update::registry, index,
                              args);
}

/**
 * @internal
 * @brief Formats TargetDetermined message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t targetDetermined(std::string_view arg1,
                                          std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::targetDetermined,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats AllTargetsDetermined message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t allTargetsDetermined()
{
    return getLog(redfish::registries::Update::Index::allTargetsDetermined, {});
}

/**
 * @internal
 * @brief Formats NoTargetsDetermined message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t noTargetsDetermined(std::string_view arg1)
{
    return getLog(redfish::registries::Update::Index::noTargetsDetermined,
                  std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats UpdateInProgress message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t updateInProgress()
{
    return getLog(redfish::registries::Update::Index::updateInProgress, {});
}

/**
 * @internal
 * @brief Formats TransferringToComponent message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t transferringToComponent(std::string_view arg1,
                                                 std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::transferringToComponent,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats VerifyingAtComponent message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t verifyingAtComponent(std::string_view arg1,
                                              std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::verifyingAtComponent,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats InstallingOnComponent message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t installingOnComponent(std::string_view arg1,
                                               std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::installingOnComponent,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ApplyingOnComponent message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t applyingOnComponent(std::string_view arg1,
                                             std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::applyingOnComponent,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats TransferFailed message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t transferFailed(std::string_view arg1,
                                        std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::transferFailed,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats VerificationFailed message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t verificationFailed(std::string_view arg1,
                                            std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::verificationFailed,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ApplyFailed message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t applyFailed(std::string_view arg1,
                                     std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::applyFailed,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ActivateFailed message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t activateFailed(std::string_view arg1,
                                        std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::activateFailed,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats AwaitToUpdate message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t awaitToUpdate(std::string_view arg1,
                                       std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::awaitToUpdate,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats AwaitToActivate message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t awaitToActivate(std::string_view arg1,
                                         std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::awaitToActivate,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats UpdateSuccessful message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t updateSuccessful(std::string_view arg1,
                                          std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::updateSuccessful,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats OperationTransitionedToJob message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t operationTransitionedToJob(std::string_view arg1)
{
    return getLog(
        redfish::registries::Update::Index::operationTransitionedToJob,
        std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats UpdateSkipped message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t updateSkipped(std::string_view arg1,
                                       std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::updateSkipped,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats UpdateSkippedSameVersion message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t updateSkippedSameVersion(std::string_view arg1,
                                                  std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::updateSkippedSameVersion,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats UpdateNotApplicable message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json::object_t updateNotApplicable(std::string_view arg1,
                                             std::string_view arg2)
{
    return getLog(redfish::registries::Update::Index::updateNotApplicable,
                  std::to_array({arg1, arg2}));
}

} // namespace messages
} // namespace redfish
