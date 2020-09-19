#include <cstdint>
#include <functional>
#include <memory>
#include <variant>

class dstree
{
public:
  enum class owning_mode
  {
    owning,
    non_owning,
  };
  using key = std::variant<int64_t, double>;
  using for_each_callback = std::function<void(dstree&)>;

  dstree();
  explicit dstree(const key& data);

  static dstree deserialize(const uint8_t* binary, size_t length,
                            owning_mode m = owning_mode::owning);
  size_t serialize(uint8_t* buf, size_t buf_size);

  dstree insert(const key& k);
  void erase(const dstree& node);
  key data() const;
  void for_each_child(const for_each_callback& callback);
  void for_each_matching_child(const key& k,
                               const for_each_callback& callback);
  dstree find(const key& k);

  void set_data(key k);

private:
  explicit dstree(const key& data, dstree* root, uint64_t node_id);

  struct impl;
  std::unique_ptr<impl, void (*)(impl*)> pimpl;
};