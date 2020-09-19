#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <dstree/dstree.hpp>

TEST_CASE("serialization", "[dstree]")
{
  dstree t;
  t.set_data(100LL);

  std::cout << std::get<int64_t>(t.data()) << std::endl;

  t.insert(10LL);
  t.insert(-1.1);
  t.insert(2.0);

  t.find(2.0).insert(3.0);
  t.find(2.0).find(3.0).insert(4LL);

  auto n = t.serialize(nullptr, 0);
  std::vector<uint8_t> vec(n);
  t.serialize(vec.data(), vec.size());

  auto t2 = dstree::deserialize(vec.data(), vec.size());

  std::cout << std::get<int64_t>(t.data()) << std::endl;
  std::cout << std::get<int64_t>(t2.data()) << std::endl;

  REQUIRE(std::get<int64_t>(t2.data()) == 100LL);
  REQUIRE(std::get<int64_t>(t2.find(10LL).data()) == 10LL);
  REQUIRE(std::get<double>(t2.find(-1.1).data()) == -1.1);
  REQUIRE(std::get<double>(t2.find(2.0).data()) == 2.0);
  REQUIRE(std::get<double>(t2.find(2.0).find(3.0).data()) == 3.0);
  REQUIRE(std::get<int64_t>(t2.find(2.0).find(3.0).find(4LL).data()) == 4LL);
}