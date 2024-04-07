#include "lib/parsing.h"
#include "lib/hamming.h"
#include "lib/archive.h"

int main(int argc, char** argv) {
  Options options;
  options.Parse(argc, argv);
  Archiver archiver;
  if (options.operation == create) {
    archiver.Create(options.arch_name, options.file_names, options.control_bits_count);
  } else if (options.operation == list) {
    archiver.List(options.arch_name);
  } else if (options.operation == extract) {
    archiver.Extract(options.arch_name, options.file_names);
  } else if (options.operation == append) {
    archiver.Append(options.arch_name, options.file_names, options.control_bits_count);
  } else if (options.operation == del) {
    archiver.Delete(options.arch_name, options.file_names);
  } else if (options.operation == concatenate) {
    archiver.Concatenate(options.arch_name, options.file_names);
  }

  return 0;
}