#pragma once
#include <google/protobuf/struct.pb.h>

#include <nlohmann/json.hpp>

namespace google::protobuf
{
void to_json(nlohmann::json& json, const Struct& message) noexcept;
void from_json(Struct& message, const nlohmann::json& json) noexcept;
void to_json(nlohmann::json& json, const ListValue& message) noexcept;
void from_json(ListValue& message, const nlohmann::json& json) noexcept;
} // namespace google::protobuf
