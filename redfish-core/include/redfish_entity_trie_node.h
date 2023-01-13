#ifndef THIRD_PARTY_MILOTIC_EXTERNAL_CC_AUTHZ_REDFISH_ENTITY_TRIE_NODE_H_
#define THIRD_PARTY_MILOTIC_EXTERNAL_CC_AUTHZ_REDFISH_ENTITY_TRIE_NODE_H_

#include <optional>
#include <string>

#include <absl/container/flat_hash_map.h>

namespace milotic::authz {

class RedfishEntityTrieNode {
 public:
  // Have to create a default constructor for the root
  RedfishEntityTrieNode() = default;
  RedfishEntityTrieNode(bool wildcard) : wildcard_(wildcard) {}

  void AddChild(const std::string& path,
                std::shared_ptr<RedfishEntityTrieNode> child);
  std::shared_ptr<RedfishEntityTrieNode> GetChild(
      const std::string& path) const;

  std::string GetEntityTag() const;
  void SetEntityTag(const std::string& tag);
  bool IsWildcard() const;

  absl::flat_hash_map<std::string, std::shared_ptr<RedfishEntityTrieNode>>
      children_;
  std::string entity_tag_;
  bool wildcard_;
};

}  // namespace milotic::authz

#endif  // THIRD_PARTY_MILOTIC_EXTERNAL_CC_AUTHZ_REDFISH_ENTITY_TRIE_NODE_H_
