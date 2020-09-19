#include "array.hpp"
#include <catch.hpp>

using namespace dstree_;

TEST_CASE("Increase size", "[array]")
{
  std::vector<uint8_t> parent(array<int16_t>::struct_size +
                              sizeof(int16_t) * 3);

  // Fill parent vector with array<uint16_t>{ 65535, 65535, 65535 } and extra
  // bytes { 108, 109, 110, 111 }
  array<uint16_t>::get(parent).size = 3;
  for (int i = 0; i < 3; ++i)
    array<uint16_t>::get(parent).data()[i] = ~0;
  for (uint8_t x : { 108, 109, 110, 111 })
    parent.push_back(x);

  // Do resize
  array<uint16_t>::get(parent).resize(6, parent);

  REQUIRE(array<uint16_t>::get(parent).size == 6);
  REQUIRE(parent ==
          std::vector<uint8_t>{
            parent[0], parent[1], parent[2], parent[3], parent[4], parent[5],
            parent[6], parent[7], 255,       255,       255,       255,
            255,       255,       0,         0,         0,         0,
            0,         0,         108,       109,       110,       111 });
}

TEST_CASE("Decrease size", "[array]")
{
  std::vector<uint8_t> parent(array<int16_t>::struct_size +
                              sizeof(int16_t) * 3);

  array<uint16_t>::get(parent).size = 3;
  for (int i = 0; i < 3; ++i)
    array<uint16_t>::get(parent).data()[i] = ~0;
  for (uint8_t x : { 108, 109, 110, 111 })
    parent.push_back(x);

  array<uint16_t>::get(parent).resize(1, parent);

  REQUIRE(array<uint16_t>::get(parent).size == 1);
  REQUIRE(parent ==
          std::vector<uint8_t>{ parent[0], parent[1], parent[2], parent[3],
                                parent[4], parent[5], parent[6], parent[7],
                                255, 255, 108, 109, 110, 111 });
}

auto parent_with_array3_and_array4()
{
  std::vector<uint8_t> parent(array<int16_t>::struct_size +
                              sizeof(int16_t) * 3 +
                              array<int8_t>::struct_size + sizeof(int8_t) * 4);
  {
    auto& arr = array<uint16_t>::get(parent);
    arr.size = 3;
    for (int i = 0; i < 3; ++i)
      arr.data()[i] = 256 * 100 + 100;
  }
  {
    auto& arr =
      array<uint8_t>::get(parent, arrays_start(0), array_index(1),
                          arrays_schema().add<uint16_t>().add<uint8_t>());
    arr.size = 4;
    for (int i = 0; i < 4; ++i)
      arr.data()[i] = 101;
  }

  return parent;
}

TEST_CASE("store two arrays", "[array]")
{
  auto parent = parent_with_array3_and_array4();

  union u
  {
    uint64_t integer;
    uint8_t bytes[8];
  };
  u _3 = { 3 };
  u _4 = { 4 };

  REQUIRE(parent ==
          std::vector<uint8_t>{
            _3.bytes[0], _3.bytes[1], _3.bytes[2], _3.bytes[3], _3.bytes[4],
            _3.bytes[5], _3.bytes[6], _3.bytes[7], 100,         100,
            100,         100,         100,         100,         _4.bytes[0],
            _4.bytes[1], _4.bytes[2], _4.bytes[3], _4.bytes[4], _4.bytes[5],
            _4.bytes[6], _4.bytes[7], 101,         101,         101,
            101 });
}

TEST_CASE("resize two arrays", "[array]")
{
  auto parent = parent_with_array3_and_array4();

  auto& arr0 = array<uint16_t>::get(parent);
  arr0.resize(10, parent);
  auto& arr1 =
    array<uint8_t>::get(parent, arrays_start(0), array_index(1),
                        arrays_schema().add<uint16_t>().add<uint8_t>());
  arr1.resize(10, parent);

  REQUIRE(parent.size() == 2 * array<int>::struct_size + 10 * 2 + 10 * 1);
}