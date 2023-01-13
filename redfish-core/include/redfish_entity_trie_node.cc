#include "redfish_entity_trie_node.h"

namespace milotic::authz {

void RedfishEntityTrieNode::AddChild(
    const std::string& path, std::shared_ptr<RedfishEntityTrieNode> child) {
  children_.emplace(path, child);
}

std::shared_ptr<RedfishEntityTrieNode>  RedfishEntityTrieNode::GetChild(
    const std::string& path) const {
  if (!children_.contains(path)) {
    return nullptr;
  }

  return children_.at(path);
}

std::string RedfishEntityTrieNode::GetEntityTag() const { return entity_tag_; }
void RedfishEntityTrieNode::SetEntityTag(const std::string& tag) {
  entity_tag_ = tag;
}
bool RedfishEntityTrieNode::IsWildcard() const { return wildcard_; }

}  // namespace milotic::authz
