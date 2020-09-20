#include <cstdint>
#include <vector>

namespace dstree_ {
#pragma pack(push, 1)
class header
{
public:
  static constexpr size_t struct_size = 32;

  uint32_t version = 1;
  uint64_t free_node_id = 0;
  uint32_t node_array_growth_factor = 1;
  uint32_t childs_array_growth_factor = 1;
  uint32_t reserved[3] = { 0, 0, 0 };
};
static_assert(sizeof(header) == header::struct_size);
#pragma pack(pop)

#pragma pack(push, 1)
struct node_value
{
  static constexpr size_t struct_size = 9;

  node_value() noexcept;
  node_value(int64_t value, std::vector<uint8_t>* parent = nullptr) noexcept;
  node_value(double value, std::vector<uint8_t>* parent = nullptr) noexcept;
  node_value(const char* value, std::vector<uint8_t>* parent) noexcept;

  enum class type : uint8_t
  {
    integer,
    floating_point,
    string_index
  };

  type t = type::integer;
  union
  {
    int64_t integer = 0;
    double floating_point;
    uint64_t string_index;
  } data;
};
static_assert(sizeof(node_value) == node_value::struct_size);
#pragma pack(pop)

#pragma pack(push, 1)
struct node
{
  static constexpr size_t struct_size = 48;

  uint64_t child_nodes_begin = ~0;
  uint32_t child_nodes_capacity = 0;
  uint32_t child_nodes_size = 0;
  node_value value;
  uint8_t valid = 0;
  uint64_t parent_node = ~0;
  uint8_t reserved[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
};
static_assert(sizeof(node) == node::struct_size);
#pragma pack(pop)

#pragma pack(push, 1)
struct child
{
  static constexpr size_t struct_size = 12;

  uint64_t node_id = ~0;
  uint8_t allocated = 0;
  uint8_t reserved[3] = { 0, 0, 0 };

  bool valid() const { return node_id != child().node_id; }

  friend bool operator<(const child& lhs, const child& rhs)
  {
    return lhs.node_id < rhs.node_id;
  }
};
static_assert(sizeof(child) == child::struct_size);
#pragma pack(pop)

void init_empty_tree(std::vector<uint8_t>& parent);
node* get_node(uint8_t* parent, uint64_t node_id);
uint64_t create_node(std::vector<uint8_t>& parent);
void destroy_node(std::vector<uint8_t>& parent, uint64_t node_id);
uint64_t insert(std::vector<uint8_t>& parent, uint64_t node_id,
                const node_value& value);
uint64_t allocate_child_range(std::vector<uint8_t>& parent, uint32_t size);
void free_child_range(std::vector<uint8_t>& parent, uint64_t begin,
                      uint32_t size);
std::pair<child*, child*> get_valid_childs_range(uint8_t* parent,
                                                 uint64_t node_id);
void set_value(uint8_t* parent, uint64_t node_id, node_value new_value);
uint64_t create_string(std::vector<uint8_t>& parent, const char* str);
void destroy_string(std::vector<uint8_t>& parent, uint64_t pos);
const char* get_string(uint8_t* parent, uint64_t pos);
}