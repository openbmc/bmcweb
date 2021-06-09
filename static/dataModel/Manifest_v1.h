#ifndef MANIFEST_V1
#define MANIFEST_V1

#include "Manifest_v1.h"

#include <chrono>

enum class ManifestV1Expand
{
    None,
    All,
    Relevant,
};
enum class ManifestV1StanzaType
{
    ComposeSystem,
    DecomposeSystem,
    ComposeResource,
    DecomposeResource,
    OEM,
};
struct ManifestV1Request
{};
struct ManifestV1Response
{};
struct ManifestV1Stanza
{
    ManifestV1StanzaType stanzaType;
    std::string oEMStanzaType;
    std::string stanzaId;
    ManifestV1Request request;
    ManifestV1Response response;
};
struct ManifestV1Manifest
{
    std::string description;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    ManifestV1Expand expand;
    ManifestV1Stanza stanzas;
};
#endif
