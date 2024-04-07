#include "parsing.h"

void Options::Parse(int argc, char** argv) {
  check_create = false;
  check_list = false;
  check_extract = false;
  check_append = false;
  check_del = false;
  check_concatenate = false;
  check_file = false;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i], "--create") == 0 || strcmp(argv[i], "-c") == 0) {
        operation = create;
        check_create = true;
      } else if (strcmp(argv[i], "--list") == 0 || strcmp(argv[i], "-l") == 0) {
        operation = list;
        check_list = true;
      } else if (strcmp(argv[i], "--extract") == 0 || strcmp(argv[i], "-x") == 0) {
        operation = extract;
        check_extract = true;
      } else if (strcmp(argv[i], "--append") == 0 || strcmp(argv[i], "-a") == 0) {
        operation = append;
        check_append = true;
      } else if (strcmp(argv[i], "--delete") == 0 || strcmp(argv[i], "-d") == 0) {
        operation = del;
        check_del = true;
      } else if (strcmp(argv[i], "--concatenate") == 0 || strcmp(argv[i], "-A") == 0) {
        operation = concatenate;
        check_concatenate = true;
      } else if (strcmp(argv[i], "-f") == 0) {
        if (i + 1 < argc) {
          arch_name = argv[i + 1];
          check_file = true;
          ++i;
        } else {
          std::cerr << "Error: Missing file name after -f option.\n";
          exit(1);
        }
      } else {
        std::string arg = argv[i];
        if (arg.find("--file=") == 0) {
          arch_name = arg.substr(7);
          check_file = true;
        }
      }
    } else {
      file_names.emplace_back(argv[i]);
    }
  }

  if (!check_file || (!check_create && !check_list && !check_extract && !check_append && !check_del && !check_concatenate)) {
    std::cerr << "Error: Missing required arguments. Usage: ./your_program --create -f <archive_path> [file1 file2 ...]\n";
    exit(1);
  }
}
