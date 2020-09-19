#include "tree.hpp"
#include "array.hpp"
#include <algorithm>

namespace {
const dstree_::array_index string_table_id(0), node_table_id(1),
  child_table_id(2);
const auto schema = dstree_::arrays_schema()
                      .add<int8_t>()
                      .add<dstree_::node>()
                      .add<dstree_::child>();
}

namespace {
auto& get_node_array(uint8_t* parent)
{
  return dstree_::array<dstree_::node>::get(
    parent, dstree_::arrays_start(dstree_::header::struct_size), node_table_id,
    schema);
}
auto& get_child_array(uint8_t* parent)
{
  return dstree_::array<dstree_::child>::get(
    parent, dstree_::arrays_start(dstree_::header::struct_size),
    child_table_id, schema);
}
auto& get_header(uint8_t* parent)
{
  return *reinterpret_cast<dstree_::header*>(parent);
}
}

void dstree_::init_empty_tree(std::vector<uint8_t>& parent)
{
  parent.clear();
  parent.resize(
    header::struct_size + 3 * array<int>::struct_size + node::struct_size, 0);
  auto h = reinterpret_cast<header*>(parent.data());
  *h = header();

  /*auto& node_array = get_node_array(parent.data());
  node_array.resize(1, parent);
  auto& n = node_array.data()[0];
  n.valid = true;*/
}

dstree_::node* dstree_::get_node(uint8_t* parent, uint64_t node_id)
{
  auto& node_array = get_node_array(parent);
  if (node_array.size <= node_id)
    return nullptr;
  return &node_array.data()[node_id];
}

uint64_t dstree_::create_node(std::vector<uint8_t>& parent)
{
  auto* node_array = &get_node_array(parent.data());
  auto* header = &get_header(parent.data());
  if (node_array->size <= header->free_node_id) {
    auto prev_size = node_array->size;
    auto new_size = (1 + prev_size) *
      static_cast<uint64_t>(pow(2, header->node_array_growth_factor));
    node_array->resize(new_size, parent);
    node_array = &get_node_array(parent.data());
    header = &get_header(parent.data());
    for (auto i = prev_size; i < new_size; ++i)
      node_array->data()[i] = node();
    ++header->node_array_growth_factor;
  }

  auto res = header->free_node_id;
  header->free_node_id++;
  while (1) {
    if (node_array->size >= header->free_node_id)
      break;
    if (!node_array->data()[header->free_node_id].valid)
      break;
    ++header->free_node_id;
  }
  node_array->data()[res].valid = true;
  return res;
}

void dstree_::destroy_node(std::vector<uint8_t>& parent, uint64_t node_id)
{
  auto [child_begin, child_end] =
    get_valid_childs_range(parent.data(), node_id);
  for (auto it = child_begin; it != child_end; ++it) {
    destroy_node(parent, it->node_id);
  }

  auto* node_array = &get_node_array(parent.data());
  auto* header = &get_header(parent.data());
  auto& n = node_array->data()[node_id];

  if (n.parent_node != node().parent_node) {
    auto parent_node = get_node(parent.data(), n.parent_node);
    auto& child_array = get_child_array(parent.data());
    for (auto i = parent_node->child_nodes_begin,
              end =
                parent_node->child_nodes_begin + parent_node->child_nodes_size;
         i < end; ++i) {
      auto& ch = child_array.data()[i];
      if (ch.node_id == node_id) {
        ch.node_id = ~0;
        break;
      }
    }
    std::sort(&child_array.data()[parent_node->child_nodes_begin],
              &child_array.data()[parent_node->child_nodes_begin +
                                  parent_node->child_nodes_size]);
    parent_node->child_nodes_size--;
  }

  n = node();
  if (node_id < header->free_node_id)
    header->free_node_id = node_id;
}

void insert_sorted(dstree_::child* arr, int64_t n, uint64_t key, int capacity)
{
  if (n >= capacity)
    return;

  int64_t i;
  for (i = n - 1; (i >= 0 && arr[i].node_id > key); i--)
    arr[i + 1] = arr[i];

  arr[i + 1].node_id = key;
}

uint64_t dstree_::insert(std::vector<uint8_t>& parent, uint64_t node_id,
                         const node_value& value)
{
  auto child_node_id = create_node(parent);
  {
    auto child = get_node(parent.data(), child_node_id);
    child->value = value;
    child->parent_node = node_id;
  }

  {
    auto& child_array = get_child_array(parent.data());
    auto node = get_node(parent.data(), node_id);
    const auto last = node->child_nodes_begin + node->child_nodes_capacity - 1;
    if (node->child_nodes_capacity == 0 || child_array.data()[last].valid()) {

      std::vector<child> childs_data;
      {
        auto& child_array = get_child_array(parent.data());
        childs_data = { child_array.data() + node->child_nodes_begin,
                        child_array.data() + node->child_nodes_begin +
                          node->child_nodes_capacity };
      }

      free_child_range(parent, node->child_nodes_begin,
                       node->child_nodes_capacity);

      auto& header = get_header(parent.data());

      const auto new_capacity = (1 + node->child_nodes_capacity) *
        static_cast<uint32_t>(pow(2, header.node_array_growth_factor));
      const auto new_range_begin = allocate_child_range(parent, new_capacity);
      header.node_array_growth_factor++;

      node = get_node(parent.data(), node_id);

      auto& child_array = get_child_array(parent.data());
      for (size_t i = 0; i < childs_data.size(); ++i)
        child_array.data()[new_range_begin + i] = childs_data[i];

      node->child_nodes_begin = new_range_begin;
      node->child_nodes_capacity = new_capacity;
    }
  }

  auto& child_array = get_child_array(parent.data());
  auto node = get_node(parent.data(), node_id);

  node->child_nodes_size = 0;
  for (size_t i = 0; i < node->child_nodes_capacity; ++i) {
    auto& ch = child_array.data()[node->child_nodes_begin + i];
    if (ch.valid())
      ++node->child_nodes_size;
  }

  insert_sorted(&child_array.data()[node->child_nodes_begin],
                node->child_nodes_size, child_node_id,
                node->child_nodes_capacity);

  node->child_nodes_size++;

  return child_node_id;
}

uint64_t dstree_::allocate_child_range(std::vector<uint8_t>& parent,
                                       uint32_t size)
{
  auto& child_array = get_child_array(parent.data());
  const auto n = child_array.size;

  uint64_t pos = ~0;

  if (n >= size)
    for (uint64_t i = 0; i < n - size; ++i) {
      bool region_in_use = false;
      for (uint32_t j = 0; j < size; ++j) {
        if (child_array.data()[i + j].allocated) {
          region_in_use = true;
          break;
        }
      }
      if (region_in_use)
        continue;
      pos = i;

      for (uint32_t j = 0; j < size; ++j) {
        child_array.data()[i + j] = child();
        child_array.data()[i + j].allocated = 1;
      }
    }

  if (pos == static_cast<uint64_t>(~0)) {
    auto prev_size = child_array.size;
    child_array.resize(prev_size + size, parent);

    auto& arr = get_child_array(parent.data());
    for (auto j = prev_size; j < arr.size; ++j) {
      arr.data()[j] = child();
      arr.data()[j].allocated = 1;
    }
    return prev_size;
  }

  return pos;
}

void dstree_::free_child_range(std::vector<uint8_t>& parent, uint64_t begin,
                               uint32_t size)
{
  if (size) {
    auto& child_array = get_child_array(parent.data());
    for (auto i = begin, n = begin + size; i < n; ++i)
      child_array.data()[i] = child();
  }
}

std::pair<dstree_::child*, dstree_::child*> dstree_::get_valid_childs_range(
  uint8_t* parent, uint64_t node_id)
{
  auto node = get_node(parent, node_id);
  auto& child_array = get_child_array(parent);

  auto begin = &child_array.data()[node->child_nodes_begin];
  auto end = begin + node->child_nodes_size;
  return { begin, end };
}

void dstree_::set_value(uint8_t* parent, uint64_t node_id,
                        node_value new_value)
{
  if (auto n = get_node(parent, node_id))
    n->value = new_value;
}