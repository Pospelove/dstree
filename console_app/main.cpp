#include <dstree/dstree.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

std::string str(const dstree::key& k)
{
  std::stringstream ss;
  std::visit([&ss](const auto& concrete) { ss << concrete; }, k);
  return ss.str();
}

void print_tree(dstree& t, uint32_t depth = 1)
{
  for (uint32_t i = 0; i < depth; ++i)
    std::cout << "-- ";
  std::cout << str(t.data()) << std::endl;
  depth++;
  t.for_each_child([depth](dstree& child) { print_tree(child, depth); });
  depth--;
}

std::vector<uint8_t> read_file(const std::filesystem::path& p)
{
  std::ifstream infile(p, std::ios_base::binary);

  return std::vector<uint8_t>(std::istreambuf_iterator<char>(infile),
                              std::istreambuf_iterator<char>());
}

int main(int argc, char* argv[])
{
  const char *arg_i = "", *arg_o = "";
  bool parsing_i = false, parsing_o = false;
  for (int i = 0; i < argc; ++i) {
    if (parsing_i) {
      parsing_i = false;
      arg_i = argv[i];
    } else if (parsing_o) {
      parsing_o = false;
      arg_o = argv[i];
    } else if (!strcmp(argv[i], "-i"))
      parsing_i = true;
    else if (!strcmp(argv[i], "-o"))
      parsing_o = true;
  }

  std::cout << "Input file: " << arg_i << std::endl;
  std::cout << "Output file: " << arg_o << std::endl;
  std::cout << std::endl;

  std::vector<uint8_t> content;
  if (arg_i[0]) {
    content = read_file(arg_i);
  } else {
    std::cout << "No input file specified, using sample data" << std::endl;
    dstree t;
    t.set_data(8LL);
    t.insert(2.015);
    t.insert(1000LL);
    t.find(1000LL).insert(2000LL);
    t.find(1000LL).insert("abcd");
    content.resize(t.serialize(nullptr, 0));
    t.serialize(content.data(), content.size());
  }

  auto t = content.empty()
    ? dstree()
    : dstree::deserialize(content.data(), content.size());

  std::cout << std::endl;
  std::cout << "Tree contents:" << std::endl;
  print_tree(t);
  std::cout << std::endl;

  if (arg_o[0]) {
    std::ofstream f(arg_o, std::ios::binary);

    std::vector<uint8_t> data(t.serialize(nullptr, 0));
    t.serialize(data.data(), data.size());
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
    std::cout << "Written to " << arg_o << std::endl;
  } else {
    std::cout << "No output file specified" << std::endl;
  }

  try {
    return 0;
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }
}
