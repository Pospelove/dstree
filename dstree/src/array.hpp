#include <cstdint>
#include <vector>

namespace dstree_ {
struct arrays_start
{
public:
  explicit arrays_start(size_t _value);
  size_t value;
};

struct array_index
{
public:
  explicit array_index(size_t _value);
  size_t value;
};

class arrays_schema
{
public:
  template <class T>
  arrays_schema& add()
  {
    array_element_sizes.push_back(sizeof(T));
    return *this;
  }

  std::vector<size_t> array_element_sizes;
};

#pragma pack(push, 1)
template <class T>
class array
{
public:
  static constexpr size_t struct_size = 8;

  uint64_t size;

  // Unfortunately 'T data[0]' would be non-standard
  T* data()
  {
    return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) +
                                struct_size);
  }

  static array<T>& get(std::vector<uint8_t>& parent,
                       arrays_start start = arrays_start(0),
                       array_index i = array_index(0),
                       arrays_schema schema = arrays_schema().add<T>())
  {
    return get(parent.data(), start, i, schema);
  }

  static array<T>& get(uint8_t* parent, arrays_start start = arrays_start(0),
                       array_index i = array_index(0),
                       arrays_schema schema = arrays_schema().add<T>())
  {
    uint8_t* arr_ptr = parent + start.value;

    for (size_t k = 0; k < i.value; ++k) {
      auto num_elements = reinterpret_cast<array<T>*>(arr_ptr)->size;
      auto element_size = schema.array_element_sizes[k];
      arr_ptr += struct_size + num_elements * element_size;
    }

    return *reinterpret_cast<array<T>*>(arr_ptr);
  }

  void resize(uint64_t new_size, std::vector<uint8_t>& parent);
};
static_assert(sizeof(array<int>) == array<int>::struct_size);
#pragma pack(pop)

template <class T>
inline void array<T>::resize(uint64_t new_size, std::vector<uint8_t>& parent)
{
  auto offset = reinterpret_cast<uint8_t*>(this) - parent.data();
  if (new_size != size) {
    const int64_t size_delta = new_size - size;
    const uint64_t old_parent_size = parent.size();
    uint64_t j = offset + struct_size + size * sizeof(T);
    size = new_size;

    enum
    {
      resize,
      move
    };

    int todo[] = { resize, move };
    if (size_delta < 0)
      std::swap(todo[0], todo[1]);

    for (int doing : todo) {
      switch (doing) {
        case resize:
          parent.resize(old_parent_size + size_delta * sizeof(T));
          break;
        case move:
          while (j < old_parent_size) {
            std::swap(parent[j], parent[j + size_delta * sizeof(T)]);
            ++j;
          }
          break;
      }
    }
  }
}
}
