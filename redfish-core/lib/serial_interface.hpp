/*
// Copyright (c) 2021 AMI
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "app.hpp"

#include <sys/stat.h>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>
#include <error_messages.hpp>
#include <utils/json_utils.hpp>

#include <optional>
#include <variant>

namespace redfish
{

std::string exec(std::string cmd)
{
    char buf[128];
    std::string result = "";
    FILE* pipe;
    if ((pipe = popen(cmd.c_str(), "r")) == NULL)
    {
        BMCWEB_LOG_ERROR << "Error opening pipe!\n";
        return "popen failed!";
    }

    while (fgets(buf, 128, pipe) != NULL)
    {
        result += buf;
    }
    pclose(pipe);

    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    return result;
}

inline void requestRoutesSerialInterfaceCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/SerialInterface/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#SerialInterfaceCollection.SerialInterfaceCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/SerialInterface";
                asyncResp->res.jsonValue["Name"] =
                    "Serial Interface Collection";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of SerialInterface";
                nlohmann::json& serialMemberArray =
                    asyncResp->res.jsonValue["Members"];
                serialMemberArray = nlohmann::json::array();

                struct stat info;
                std::string line;
                if (system("ls /sys/class/tty/ > /tmp/tty.file"))
                {
                    BMCWEB_LOG_DEBUG << "File created Sucessfully ";
                }

                std::ifstream ttyFile("/tmp/tty.file");

                if (ttyFile.is_open())
                {
                    std::size_t cnt = 0;
                    while (getline(ttyFile, line))
                    {
                        if (line == "ttyS0")
                        {
                            continue;
                        }
                        std::string ttyDriverPath =
                            "/sys/class/tty/" + line + "/device/driver";
                        std::string stty =
                            "stty -a -F /dev/" + line + " 2>/dev/null ";
                        if (stat(ttyDriverPath.c_str(), &info) == 0)
                        {
                            if (info.st_mode & S_IFDIR)
                            {
                                BMCWEB_LOG_DEBUG << "exit" << ttyDriverPath
                                                 << "\n";
                                if (system(stty.c_str()))
                                {
                                    BMCWEB_LOG_ERROR << "system command Failed"
                                                     << "\n";
                                }
                                else
                                {
                                    serialMemberArray.push_back(
                                        {{"@odata.id",
                                          "/redfish/v1/Managers/bmc/"
                                          "SerialInterface/" +
                                              line}});
                                    cnt++;
                                }
                            }
                        }
                        else
                        {
                            BMCWEB_LOG_ERROR
                                << "Driver path not exit:" << ttyDriverPath
                                << "\n";
                        }
                    }
                    ttyFile.close();
                    if (system("rm -rf /tmp/tty.file"))
                    {
                        BMCWEB_LOG_DEBUG << "File removed Sucessfully \n";
                    }
                    asyncResp->res.jsonValue["Members@odata.count"] = cnt;
                }

                else
                {
                    BMCWEB_LOG_ERROR << "Unable to open file";
                }
            });
}

/**
 * SerialInterface Instance derived class for delivering SerialInterface Schema
 */
inline void requestRoutesSerialInterfaceInstance(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/SerialInterface/<str>/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& serialInstance) {
                std::string bitRate = "stty -a -F /dev/" + serialInstance +
                                      "  2>/dev/null | gawk 'match($0, /speed "
                                      "([[:digit:]]+) baud;/, a) {print a[1]}'";
                std::string dataBits = "stty -a -F /dev/" + serialInstance +
                                       "  2>/dev/null | gawk 'match($0, "
                                       "/cs([[:digit:]]?)/, a) {print a[1]}'";
                std::string stopBits =
                    "stty -a -F /dev/" + serialInstance +
                    "  2>/dev/null | gawk '{if (match($0, /-cstopb/)) {print "
                    "1} else if(match($0, /cstopb/)) {print 2}}'";
                std::string ixon =
                    "stty -a -F /dev/" + serialInstance +
                    "  2>/dev/null | gawk '{if (match($0, /-ixon/)) {print 1} "
                    "else if(match($0, /ixon/)) {print 0}}'";
                std::string crtscts =
                    "stty -a -F /dev/" + serialInstance +
                    "  2>/dev/null | gawk '{if (match($0, /-crtscts/)) {print "
                    "0} else if(match($0, /crtscts/)) {print 1}}'";
                std::string cdtrdsr =
                    "stty -a -F /dev/" + serialInstance +
                    "  2>/dev/null | gawk '{if (match($0, /-cdtrdsr/)) {print "
                    "0} else if(match($0, /cdtrdsr/)) {print 1}}'";
                std::string parenb =
                    "stty -a -F /dev/" + serialInstance +
                    "  2>/dev/null | gawk '{if (match($0, /-parenb/)) {print "
                    "0} else if(match($0, /parenb/)) {print 1}}'";
                std::string parodd =
                    "stty -a -F /dev/" + serialInstance +
                    "  2>/dev/null | gawk '{if (match($0, /-parodd/)) {print "
                    "0} else if(match($0, /parodd/)) {print 1}}'";

                asyncResp->res.jsonValue["@odata.type"] =
                    "#SerialInterface.v1_1_5.SerialInterface";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/SerialInterface/" +
                    serialInstance;

                asyncResp->res.jsonValue["BitRate"] = exec(bitRate);
                asyncResp->res.jsonValue["DataBits"] = exec(dataBits);
                asyncResp->res.jsonValue["Description"] =
                    "Serial device @ /dev/" + serialInstance;

                std::string ixonVal = exec(ixon);
                std::string crtsctsVal = exec(crtscts);
                std::string cdtrdsrVal = exec(cdtrdsr);

                if (ixonVal == "1")
                {
                    asyncResp->res.jsonValue["FlowControl"] = "Software";
                }
                else if (crtsctsVal == "1")
                {
                    asyncResp->res.jsonValue["FlowControl"] = "Hardware";
                }
                else if (cdtrdsrVal == "1")
                {
                    asyncResp->res.jsonValue["FlowControl"] = "Hardware";
                }
                else
                {
                    asyncResp->res.jsonValue["FlowControl"] = "None";
                }

                asyncResp->res.jsonValue["Id"] = serialInstance;
                asyncResp->res.jsonValue["InterfaceEnabled"] = true;
                asyncResp->res.jsonValue["Name"] = serialInstance;

                std::string parenbVal = exec(parenb);
                std::string paroddVal = exec(parodd);
                if (parenbVal == "1")
                {
                    if (paroddVal == "1")
                    {
                        asyncResp->res.jsonValue["Parity"] = "Odd";
                    }
                    else
                    {
                        asyncResp->res.jsonValue["Parity"] = "Even";
                    }
                }
                else
                {
                    asyncResp->res.jsonValue["Parity"] = "None";
                }
                asyncResp->res.jsonValue["StopBits"] = exec(stopBits);
            });
}

} // namespace redfish
