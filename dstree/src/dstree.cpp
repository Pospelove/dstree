#include "tree.hpp"
#include <algorithm>
#include <dstree/dstree.hpp>
#include <optional>
#include <stdexcept>
#include <vector>

namespace {
struct root_node
{
  uint8_t* data = nullptr;
  size_t size = 0;
};
struct root_node_owning
{
  std::vector<uint8_t> holder;
};
struct child_node
{
  dstree* parent = nullptr;
  uint64_t node_id = ~0;
};

dstree_::node_value key_to_internal_format(const dstree::key& key)
{
  return std::visit([](const auto& v) { return dstree_::node_value(v); }, key);
}

dstree::key key_to_interface_format(const dstree_::node_value& value)
{
  if (value.t == dstree_::node_value::type::integer) {
    return value.data.integer;
  } else if (value.t == dstree_::node_value::type::floating_point) {
    return value.data.floating_point;
  } else {
    throw std::runtime_error("anomaly, unknown node_value type");
  }
}

}

struct dstree::impl
{
  std::optional<root_node> root;
  std::optional<root_node_owning> root_owning;
  std::optional<child_node> child;

  auto get_root()
  {
    auto pimpl_ = this;
    while (pimpl_->child)
      pimpl_ = pimpl_->child->parent->pimpl.get();
    return pimpl_;
  }

  uint8_t* get_data()
  {
    auto pimpl_ = get_root();
    return pimpl_->root_owning ? pimpl_->root_owning->holder.data()
                               : pimpl_->root->data;
  }
};

dstree::dstree()
  : pimpl(new impl, [](impl* p) { delete p; })
{
  pimpl->root_owning = root_node_owning();
  dstree_::init_empty_tree(pimpl->root_owning->holder);
  dstree_::create_node(pimpl->root_owning->holder);
}

dstree::dstree(const key& data)
  : dstree()
{
  dstree_::set_value(pimpl->root_owning->holder.data(), 0,
                     key_to_internal_format(data));
}

dstree::dstree(const key& data, dstree* parent, uint64_t node_id)
  : pimpl(new impl, [](impl* p) { delete p; })
{
  pimpl->child = { parent, node_id };
}

dstree dstree::deserialize(const uint8_t* binary, size_t length, owning_mode m)
{
  dstree res;

  if (m == owning_mode::owning)
    res.pimpl->root_owning->holder =
      std::vector<uint8_t>(binary, binary + length);
  else {
    res.pimpl->root_owning.reset();
    res.pimpl->root = root_node{ const_cast<uint8_t*>(binary), length };
  }

  return res;
}

size_t dstree::serialize(uint8_t* buf, size_t buf_size)
{
  uint8_t* data;
  size_t size;

  if (pimpl->root) {
    data = pimpl->root->data;
    size = pimpl->root->size;
  } else if (pimpl->root_owning) {
    data = pimpl->root_owning->holder.data();
    size = pimpl->root_owning->holder.size();
  } else if (pimpl->child) {
    throw std::runtime_error("serialization is only for root nodes");
  } else
    return 0;

  if (buf) {
    auto n = std::min(size, buf_size);
    memcpy(buf, data, n);
  }

  return size;
}

dstree dstree::insert(const key& k)
{
  const uint64_t my_node_id = pimpl->child ? pimpl->child->node_id : 0;

  auto root = pimpl->get_root();

  if (!root->root_owning)
    throw std::runtime_error("insert is only available in owning mode");

  const auto child_node_id = dstree_::insert(
    root->root_owning->holder, my_node_id, key_to_internal_format(k));

  return dstree(k, this, child_node_id);
}

void dstree::erase(const dstree& node)
{
  if (!node.pimpl->child || node.pimpl->child->parent != this)
    throw std::runtime_error("erase can only remove child nodes of this node");

  const dstree* root = &node;
  while (root->pimpl->child)
    root = root->pimpl->child->parent;

  if (!root->pimpl->root_owning)
    throw std::runtime_error("erase is only available for owning root nodes");

  dstree_::destroy_node(root->pimpl->root_owning->holder,
                        node.pimpl->child->node_id);
}

dstree::key dstree::data() const
{
  const uint64_t my_node_id = pimpl->child ? pimpl->child->node_id : 0;
  return key_to_interface_format(
    dstree_::get_node(pimpl->get_data(), my_node_id)->value);
}

void dstree::for_each_child(const for_each_callback& callback)
{
  const uint64_t my_node_id = pimpl->child ? pimpl->child->node_id : 0;
  auto [begin, end] =
    dstree_::get_valid_childs_range(pimpl->get_data(), my_node_id);
  for (auto it = begin; it != end; ++it) {
    if (auto child_node = dstree_::get_node(pimpl->get_data(), it->node_id)) {
      dstree child{ key_to_interface_format(child_node->value), this,
                    it->node_id };
      callback(child);
    }
  }
}

void dstree::for_each_matching_child(const key& k,
                                     const for_each_callback& callback)
{
  for_each_child([&](dstree& child) {
    if (child.data() == k)
      callback(child);
  });
}

dstree dstree::find(const key& k)
{
  std::optional<dstree> res;
  for_each_matching_child(k, [&](dstree& child) {
    if (!res)
      res = dstree(child.data(), child.pimpl->child->parent,
                   child.pimpl->child->node_id);
  });
  if (!res)
    throw std::runtime_error("bad lookup");
  return std::move(*res);
}

void dstree::set_data(key k)
{
  const uint64_t my_node_id = pimpl->child ? pimpl->child->node_id : 0;
  dstree_::set_value(pimpl->get_data(), my_node_id, key_to_internal_format(k));
}