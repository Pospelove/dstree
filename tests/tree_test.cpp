#include "tree.hpp"
#include <catch.hpp>

TEST_CASE("", "[tree]")
{
  std::vector<uint8_t> parent;
  dstree_::init_empty_tree(parent);

  REQUIRE(dstree_::create_node(parent) == 0);
  REQUIRE(dstree_::create_node(parent) == 1);

  dstree_::insert(parent, 0, dstree_::node_value(1LL));
  dstree_::insert(parent, 0, dstree_::node_value(2LL));
  dstree_::insert(parent, 0, dstree_::node_value(3LL));

  {
    auto [begin, end] = dstree_::get_valid_childs_range(parent.data(), 0);
    REQUIRE(end - begin == 3);
    REQUIRE(begin[0].node_id == 2);
    REQUIRE(begin[1].node_id == 3);
    REQUIRE(begin[2].node_id == 4);
  }

  dstree_::insert(parent, 0, dstree_::node_value(4LL));

  {
    auto [begin, end] = dstree_::get_valid_childs_range(parent.data(), 0);
    REQUIRE(end - begin == 4);
    REQUIRE(begin[0].node_id == 2);
    REQUIRE(begin[1].node_id == 3);
    REQUIRE(begin[2].node_id == 4);
    REQUIRE(begin[3].node_id == 5);
  }

  dstree_::destroy_node(parent, 3);

  {
    auto [begin, end] = dstree_::get_valid_childs_range(parent.data(), 0);
    REQUIRE(end - begin == 3);
    REQUIRE(begin[0].node_id == 2);
    REQUIRE(begin[1].node_id == 4);
    REQUIRE(begin[2].node_id == 5);
  }
}