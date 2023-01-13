#include "redfish_entity_trie.h"

#include <absl/strings/str_split.h>

namespace milotic::authz {

RedfishEntityTrie::RedfishEntityTrie() {
  root_ = std::make_shared<RedfishEntityTrieNode>();
}

void RedfishEntityTrie::InsertUri(std::string_view uri,
                                  std::string_view entity_tag) {
  std::vector<std::string> uri_split = absl::StrSplit(uri, '/');

  std::shared_ptr<RedfishEntityTrieNode> cur = root_;

  // Every URI starts with a / so the first element is always empty
  for (uint i = 1; i < uri_split.size(); ++i) {
    std::shared_ptr<RedfishEntityTrieNode> child = cur->GetChild(uri_split[i]);

    if (child != nullptr) {
      cur = child;
      continue;
    }

    bool wildcard = uri_split[i].front() == '{';
    auto child_node = std::make_shared<RedfishEntityTrieNode>(wildcard);
    cur->AddChild(uri_split[i], child_node);

    cur = child_node;
  }
  cur->SetEntityTag(std::string(entity_tag));
}

std::optional<std::string> RedfishEntityTrie::GetEntityType(
    std::string_view uri) {
  std::vector<std::string> uri_split = absl::StrSplit(uri, '/');

  std::shared_ptr<RedfishEntityTrieNode> cur = root_;

  for (uint i = 1; i < uri_split.size(); ++i) {
    std::shared_ptr<RedfishEntityTrieNode> child = cur->GetChild(uri_split[i]);

    if (child == nullptr) {
      return std::nullopt;
    }

    cur = child;
  }
  return cur->GetEntityTag();
}

}  // namespace milotic::authz
