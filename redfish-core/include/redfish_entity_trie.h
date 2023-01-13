#ifndef THIRD_PARTY_MILOTIC_EXTERNAL_CC_AUTHZ_REDFISH_ENTITY_TRIE_H_
#define THIRD_PARTY_MILOTIC_EXTERNAL_CC_AUTHZ_REDFISH_ENTITY_TRIE_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "redfish_entity_trie_node.h"

namespace milotic::authz {

class RedfishEntityTrie {
 public:
  RedfishEntityTrie();

  void InsertUri(std::string_view uri, std::string_view entity_tag);

  std::optional<std::string> GetEntityType(std::string_view uri);

 private:
  std::shared_ptr<RedfishEntityTrieNode> root_;
};
}  // namespace milotic::authz

#endif  // THIRD_PARTY_MILOTIC_EXTERNAL_CC_AUTHZ_REDFISH_ENTITY_TRIE_H_
