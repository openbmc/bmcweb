// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "logging.hpp"

#include <json.h>
#include <libcper/cper-parse.h>
#include <unistd.h>

#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <format>
#include <memory>
#include <optional>
#include <string>

namespace redfish::arm
{

inline bool isCperRecord(int fd)
{
    constexpr std::array<char, 4> cperSignature = {'C', 'P', 'E', 'R'};
    std::array<char, cperSignature.size()> signature = {};

    ssize_t bytesRead = pread(fd, signature.data(), signature.size(), 0);
    if (bytesRead != static_cast<ssize_t>(signature.size()))
    {
        return false;
    }

    return signature == cperSignature;
}

inline std::optional<nlohmann::json> decodeCperFromFileDescriptor(
    const sdbusplus::message::unix_fd& unixfd)
{
    int fd = dup(unixfd);
    if (fd < 0)
    {
        BMCWEB_LOG_WARNING("Failed to open FaultLog CPER file");
        return std::nullopt;
    }

    if (!isCperRecord(fd))
    {
        close(fd);
        return std::nullopt;
    }

    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        close(fd);
        BMCWEB_LOG_WARNING("Failed to reset FaultLog CPER file offset to 0");
        return std::nullopt;
    }

    FILE* rawCperFile = fdopen(fd, "rb");
    if (rawCperFile == nullptr)
    {
        close(fd);
        BMCWEB_LOG_WARNING("Failed to open FaultLog CPER file");
        return std::nullopt;
    }

    json_object* decodedJson = cper_to_ir(rawCperFile);
    fclose(rawCperFile);
    if (decodedJson == nullptr)
    {
        BMCWEB_LOG_WARNING("Failed to decode FaultLog CPER file");
        return std::nullopt;
    }

    const char* decodedStr = json_object_to_json_string(decodedJson);
    nlohmann::json cperData = nlohmann::json::parse(decodedStr, nullptr, false);
    json_object_put(decodedJson);
    if (cperData.is_discarded())
    {
        BMCWEB_LOG_WARNING("Failed to parse decoded FaultLog CPER JSON");
        return std::nullopt;
    }

    return cperData;
}

inline void populateFaultLogCperDataFromJson(nlohmann::json& logEntry,
                                             const nlohmann::json& cperData)
{
    logEntry["DiagnosticDataType"] = "CPER";

    nlohmann::json cperSummary;
    if (cperData.contains("header") && cperData["header"].is_object())
    {
        const nlohmann::json& header = cperData["header"];
        if (header.contains("severity") && header["severity"].is_object())
        {
            const std::string severityName =
                header["severity"].value("name", std::string{});
            if (!severityName.empty())
            {
                cperSummary["Severity"] = severityName;
            }
        }

        cperSummary["SectionCount"] = header.value("sectionCount", 0);
    }

    if (cperData.contains("sectionDescriptors") &&
        cperData["sectionDescriptors"].is_array() &&
        cperData.contains("sections") && cperData["sections"].is_array())
    {
        const nlohmann::json& sectionDescriptors =
            cperData["sectionDescriptors"];
        const nlohmann::json& sections = cperData["sections"];
        const size_t sectionCount =
            std::min(sectionDescriptors.size(), sections.size());

        nlohmann::json::array_t sectionArray;
        for (size_t index = 0; index < sectionCount; ++index)
        {
            const nlohmann::json& descriptor = sectionDescriptors[index];
            const nlohmann::json& section = sections[index];
            nlohmann::json sectionEntry;

            if (descriptor.is_object())
            {
                if (descriptor.contains("sectionType") &&
                    descriptor["sectionType"].is_object())
                {
                    const std::string typeName =
                        descriptor["sectionType"].value("type", std::string{});
                    sectionEntry["SectionType"] = typeName;
                    if (typeName == "ARM RAS" && section.is_object() &&
                        section.contains("ArmRas") &&
                        section["ArmRas"].is_object())
                    {
                        const nlohmann::json& armRas = section["ArmRas"];
                        if (armRas.contains("userData") &&
                            armRas["userData"].is_string())
                        {
                            sectionEntry["userData"] = armRas["userData"];
                        }
                    }
                }
            }

            if (!sectionEntry.empty())
            {
                sectionArray.emplace_back(std::move(sectionEntry));
            }
        }

        if (!sectionArray.empty())
        {
            cperSummary["Sections"] = std::move(sectionArray);
        }
    }

    if (!cperSummary.empty())
    {
        logEntry["Oem"]["CPER"] = std::move(cperSummary);
    }
}

inline void populateFaultLogCperData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& entryID)
{
    std::string dumpEntryPath =
        std::format("/xyz/openbmc_project/dump/faultlog/entry/{}", entryID);

    auto cperDataHandler =
        // ast-grep-ignore: long-lambda
        [asyncResp](const boost::system::error_code& ec,
                    const sdbusplus::message::unix_fd& unixfd) {
            if (ec)
            {
                return;
            }

            std::optional<nlohmann::json> cperData =
                decodeCperFromFileDescriptor(unixfd);
            if (cperData)
            {
                populateFaultLogCperDataFromJson(asyncResp->res.jsonValue,
                                                 *cperData);
            }
        };

    dbus::utility::async_method_call(
        asyncResp, std::move(cperDataHandler),
        "xyz.openbmc_project.Dump.Manager", dumpEntryPath,
        "xyz.openbmc_project.Dump.Entry", "GetFileHandle");
}

} // namespace redfish::arm
