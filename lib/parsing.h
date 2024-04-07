#include <vector>
#include <string>
#include "cstring"
#include <iostream>

enum operations {create, list, extract, append, del, concatenate};

struct Options {
  operations operation;

  std::string arch_name;
  std::vector<std::string> file_names;
  unsigned char control_bits_count = 4;

  bool check_create;
  bool check_list;
  bool check_extract;
  bool check_append;
  bool check_del;
  bool check_concatenate;
  bool check_file;

  void Parse(int argc, char** argv);
};