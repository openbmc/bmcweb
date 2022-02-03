/*
// Copyright (c) 2021 NVIDIA Corporation
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
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <map>
#include <string>
#include <vector>

namespace redfish
{
inline std::string translateSeverityDbusToRedfish(const std::string& s)
{
    if ((s == "xyz.openbmc_project.Logging.Entry.Level.Alert") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Critical") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Emergency") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Error"))
    {
        return "Critical";
    }
    if ((s == "xyz.openbmc_project.Logging.Entry.Level.Debug") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Informational") ||
        (s == "xyz.openbmc_project.Logging.Entry.Level.Notice"))
    {
        return "OK";
    }
    if (s == "xyz.openbmc_project.Logging.Entry.Level.Warning")
    {
        return "Warning";
    }
    return "";
}

class AdditionalData
{
    enum SameKeyOp
    {
        overwrite = 0,
        append = 1,
    };

  public:
    // DBus Event Log additionalData format is like,
    // "key1=val1" "key2=val2"...
    AdditionalData(const std::vector<std::string>& additionalData,
                   const SameKeyOp& op = overwrite)
    {
        convertion(additionalData, data, op);
    }

    void convertion(const std::vector<std::string>& additionalData,
                    std::map<std::string, std::string>& data,
                    const SameKeyOp& op)
    {
        for (auto& kv : additionalData)
        {
            std::vector<std::string> fields;
            fields.reserve(2);
            boost::split(fields, kv, boost::is_any_of("="));
            if (data.count(fields[0]) <= 0)
            {
                data[fields[0]] = "";
            }
            if (op == overwrite)
            {
                data[fields[0]] = fields[1];
            }
            else if (op == append)
            {
                // In append mode, all values for the same key will be
                // separated by ';', e.g., "key1=val1_1;val1_2;...;val1_n"
                data[fields[0]] += (!data[fields[0]].empty()) ? ";" : "";
                data[fields[0]] += fields[1];
            }
        }
    }

    std::string& operator[](const std::string& key)
    {
        return data[key];
    }

    std::size_t count(const std::string& key)
    {
        return data.count(key);
    }

  protected:
    std::map<std::string, std::string> data;
};
} // namespace redfish