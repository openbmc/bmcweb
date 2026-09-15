// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/leak_detector.hpp"
#include "generated/enums/resource.hpp"
#include "http_response.hpp"
#include "leak_detector.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* chassisId = "ChassisId";
constexpr const char* chassisPath = "ChassisPath";
constexpr const char* detectorId = "Detector0";
constexpr const char* detectorPath =
    "/xyz/openbmc_project/state/leak/detector/Detector0";
constexpr const char* detectorInterface =
    "xyz.openbmc_project.State.Leak.Detector";

boost::urls::url collectionPath()
{
    return boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/LeakDetection/LeakDetectors",
        chassisId);
}

dbus::utility::MapperGetSubTreeResponse makeSubTree()
{
    dbus::utility::MapperServiceMap services;
    services.emplace_back("xyz.openbmc_project.LeakDetector",
                          std::vector<std::string>{detectorInterface});

    dbus::utility::MapperGetSubTreeResponse subtree;
    subtree.emplace_back(detectorPath, services);
    return subtree;
}

TEST(ToDetectorState, DBusStatesMapToRedfish)
{
    EXPECT_EQ(
        toDetectorState(
            "xyz.openbmc_project.State.Leak.Detector.DetectorState.Normal"),
        leak_detector::DetectorState::OK);
    EXPECT_EQ(
        toDetectorState(
            "xyz.openbmc_project.State.Leak.Detector.DetectorState.Abnormal"),
        leak_detector::DetectorState::Critical);
    EXPECT_EQ(
        toDetectorState(
            "xyz.openbmc_project.State.Leak.Detector.DetectorState.Unknown"),
        leak_detector::DetectorState::Unavailable);
}

TEST(ToDetectorType, OnlyTheCableTypeIsKnown)
{
    EXPECT_EQ(
        toDetectorType(
            "xyz.openbmc_project.State.Leak.Detector.DetectorType.LeakSensingCable"),
        leak_detector::LeakDetectorType::Moisture);
    EXPECT_EQ(
        toDetectorType(
            "xyz.openbmc_project.State.Leak.Detector.DetectorType.Unknown"),
        leak_detector::LeakDetectorType::Invalid);
    EXPECT_EQ(toDetectorType("something.else"),
              leak_detector::LeakDetectorType::Invalid);
}

TEST(FindLeakDetector, MatchesOnTheObjectPathFilename)
{
    dbus::utility::MapperGetSubTreeResponse subtree = makeSubTree();

    EXPECT_NE(findLeakDetector(subtree, detectorId), subtree.end());
    EXPECT_EQ(findLeakDetector(subtree, "Detector1"), subtree.end());
}

TEST(AfterGetLeakDetectorCollection, MembersNameEachDetector)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetLeakDetectorCollection(response, collectionPath(), ec,
                                   {detectorPath});

    nlohmann::json& members = response->res.jsonValue["Members"];
    ASSERT_EQ(members.size(), 1);
    EXPECT_EQ(
        members[0]["@odata.id"],
        "/redfish/v1/Chassis/ChassisId/ThermalSubsystem/LeakDetection/LeakDetectors/Detector0");
    EXPECT_EQ(response->res.jsonValue["Members@odata.count"], 1);
}

TEST(AfterGetLeakDetectorCollection, MembersAreSortedNaturally)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    const std::string base = "/xyz/openbmc_project/state/leak/detector/";

    afterGetLeakDetectorCollection(
        response, collectionPath(), ec,
        {base + "Detector2", base + "Detector10", base + "Detector1"});

    nlohmann::json& members = response->res.jsonValue["Members"];
    ASSERT_EQ(members.size(), 3);
    EXPECT_EQ(
        members[0]["@odata.id"],
        "/redfish/v1/Chassis/ChassisId/ThermalSubsystem/LeakDetection/LeakDetectors/Detector1");
    EXPECT_EQ(
        members[1]["@odata.id"],
        "/redfish/v1/Chassis/ChassisId/ThermalSubsystem/LeakDetection/LeakDetectors/Detector2");
    EXPECT_EQ(
        members[2]["@odata.id"],
        "/redfish/v1/Chassis/ChassisId/ThermalSubsystem/LeakDetection/LeakDetectors/Detector10");
    EXPECT_EQ(response->res.jsonValue["Members@odata.count"], 3);
}

TEST(AfterGetLeakDetectorCollection, NoAssociationIsAnEmptyCollection)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetLeakDetectorCollection(response, collectionPath(), ec, {});

    EXPECT_EQ(response->res.jsonValue["Members"].size(), 0);
    EXPECT_EQ(response->res.jsonValue["Members@odata.count"], 0);
}

TEST(AfterGetLeakDetectorCollection, OtherErrorsAreInternal)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetLeakDetectorCollection(response, collectionPath(), ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetValidChassisForCollection, UnknownChassisIsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetValidChassisForCollection(response, chassisId, std::nullopt);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(
        response->res.jsonValue["error"]["message"],
        "The requested resource of type Chassis named 'ChassisId' was not found.");
}

TEST(AfterGetValidChassisForCollectionHead, ValidChassisNamesTheSchema)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetValidChassisForCollectionHead(
        response, chassisId, std::make_optional<std::string>(chassisPath));

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        response->res.getHeaderValue(boost::beast::http::field::link),
        "</redfish/v1/JsonSchemas/LeakDetectorCollection/LeakDetectorCollection.json>; rel=describedby");
}

TEST(AfterGetValidChassisForCollectionHead, UnknownChassisIsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetValidChassisForCollectionHead(response, chassisId, std::nullopt);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(
        response->res.jsonValue["error"]["message"],
        "The requested resource of type Chassis named 'ChassisId' was not found.");
    EXPECT_TRUE(
        response->res.getHeaderValue(boost::beast::http::field::link).empty());
}

TEST(AfterGetLeakDetectorSubTreeForDetectorHead, KnownDetectorNamesTheSchema)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetLeakDetectorSubTreeForDetectorHead(response, detectorId,
                                               makeSubTree());

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        response->res.getHeaderValue(boost::beast::http::field::link),
        "</redfish/v1/JsonSchemas/LeakDetector/LeakDetector.json>; rel=describedby");
}

TEST(AfterGetLeakDetectorSubTreeForDetectorHead, UnknownDetectorIsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetLeakDetectorSubTreeForDetectorHead(response, "Detector1",
                                               makeSubTree());

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(
        response->res.jsonValue["error"]["message"],
        "The requested resource of type LeakDetector named 'Detector1' was not found.");
}

TEST(AfterGetLeakDetectorSubTreeForDetector, UnknownDetectorIsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetLeakDetectorSubTreeForDetector(response, chassisId, "Detector1",
                                           makeSubTree());

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(
        response->res.jsonValue["error"]["message"],
        "The requested resource of type LeakDetector named 'Detector1' was not found.");
}

TEST(AfterGetValidChassisForDetector, UnknownChassisIsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetValidChassisForDetector(response, chassisId, detectorId,
                                    std::nullopt);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(
        response->res.jsonValue["error"]["message"],
        "The requested resource of type Chassis named 'ChassisId' was not found.");
}

TEST(AfterGetValidChassisForDetectorHead, UnknownChassisIsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetValidChassisForDetectorHead(response, chassisId, detectorId,
                                        std::nullopt);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(
        response->res.jsonValue["error"]["message"],
        "The requested resource of type Chassis named 'ChassisId' was not found.");
}

TEST(AfterGetLeakDetector, EbadrIsNotFound)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetLeakDetector(response, chassisId, detectorId, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(
        response->res.jsonValue["error"]["message"],
        "The requested resource of type LeakDetector named 'Detector0' was not found.");
}

TEST(AfterGetLeakDetector, OtherErrorsAreInternal)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetLeakDetector(response, chassisId, detectorId, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetLeakDetector, PropertiesMapToRedfish)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    dbus::utility::DBusPropertiesMap properties;
    properties.emplace_back("PrettyName", std::string("Leak Detector 0"));
    properties.emplace_back(
        "State",
        std::string(
            "xyz.openbmc_project.State.Leak.Detector.DetectorState.Abnormal"));
    properties.emplace_back(
        "Type",
        std::string(
            "xyz.openbmc_project.State.Leak.Detector.DetectorType.LeakSensingCable"));

    afterGetLeakDetector(response, chassisId, detectorId, ec, properties);

    nlohmann::json& json = response->res.jsonValue;
    EXPECT_EQ(json["@odata.type"], "#LeakDetector.v1_6_0.LeakDetector");
    EXPECT_EQ(
        json["@odata.id"],
        "/redfish/v1/Chassis/ChassisId/ThermalSubsystem/LeakDetection/LeakDetectors/Detector0");
    EXPECT_EQ(json["Id"], detectorId);
    EXPECT_EQ(json["Name"], "Leak Detector 0");
    EXPECT_EQ(json["DetectorState"], leak_detector::DetectorState::Critical);
    EXPECT_EQ(json["LeakDetectorType"],
              leak_detector::LeakDetectorType::Moisture);
    EXPECT_EQ(json["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(json["Status"]["Health"], resource::Health::Critical);
    EXPECT_EQ(
        response->res.getHeaderValue(boost::beast::http::field::link),
        "</redfish/v1/JsonSchemas/LeakDetector/LeakDetector.json>; rel=describedby");
}

TEST(AfterGetLeakDetector, AnUnknownTypeIsOmitted)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    dbus::utility::DBusPropertiesMap properties;
    properties.emplace_back("PrettyName", std::string("Leak Detector 0"));
    properties.emplace_back(
        "State",
        std::string(
            "xyz.openbmc_project.State.Leak.Detector.DetectorState.Normal"));
    properties.emplace_back("Type", std::string("something.else"));

    afterGetLeakDetector(response, chassisId, detectorId, ec, properties);

    EXPECT_EQ(response->res.jsonValue["DetectorState"],
              leak_detector::DetectorState::OK);
    EXPECT_EQ(response->res.jsonValue["Status"]["Health"],
              resource::Health::OK);
    EXPECT_FALSE(response->res.jsonValue.contains("LeakDetectorType"));
}

TEST(AfterGetLeakDetector, AnUnavailableDetectorIsOffline)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    dbus::utility::DBusPropertiesMap properties;
    properties.emplace_back("PrettyName", std::string("Leak Detector 0"));
    properties.emplace_back(
        "State",
        std::string(
            "xyz.openbmc_project.State.Leak.Detector.DetectorState.Unknown"));
    properties.emplace_back(
        "Type",
        std::string(
            "xyz.openbmc_project.State.Leak.Detector.DetectorType.LeakSensingCable"));

    afterGetLeakDetector(response, chassisId, detectorId, ec, properties);

    nlohmann::json& json = response->res.jsonValue;
    EXPECT_EQ(json["DetectorState"], leak_detector::DetectorState::Unavailable);
    EXPECT_EQ(json["Status"]["State"], resource::State::UnavailableOffline);
    EXPECT_FALSE(json["Status"].contains("Health"));
}

TEST(ToDetectorHealth, DetectorStatesMapToHealth)
{
    EXPECT_EQ(toDetectorHealth(leak_detector::DetectorState::OK),
              resource::Health::OK);
    EXPECT_EQ(toDetectorHealth(leak_detector::DetectorState::Critical),
              resource::Health::Critical);
}

} // namespace
} // namespace redfish
