#include "hamming.cpp"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstdio>

const uint16_t kCountHeaderSize = 8;
const uint16_t kEncodedCountHeaderSize = kCountHeaderSize * 2;
const uint16_t kNameHeaderSize = 20;
const uint16_t kEncodedNameHeaderSize = kNameHeaderSize * 2;
const uint16_t kBits[] = {0, 4};
const std::string arch_tmp_name = "arch.tmp";

class Archiver {
 public:
  void Create(const std::string& arch_name, const std::vector<std::string>& file_names, unsigned char control_bits_count);

  void List(const std::string& arch_name);

  void Extract(const std::string& arch_name, const std::vector<std::string>& file_names);

  void Append(const std::string& arch_name, const std::vector<std::string>& file_names, unsigned char control_bits_count);

  void Delete(const std::string& arch_name, const std::vector<std::string>& file_names);

  void Concatenate(const std::string& arch_name, const std::vector<std::string>& file_names);
};

struct Archive {
  std::string arch_name;

  // Очищает архив.
  void Clear();

  // Добавляет файл в архив с указанным количеством битов контроля.
  void AddFile(const std::string& file_name, unsigned char control_bits_count);

  // Извлекает файл из архива и сохраняет его по указанному пути.
  unsigned char ExtractFile(const std::string& file_name, const std::string& extraction_path);

  // Возвращает список файлов в архиве.
  std::vector<std::string> FileList();

  // Удаляет файл из архива.
  void DeleteFile(const std::string& file_name);

  // Добавляет файлы из другого архива в текущий.
  void AddFilesFrom(const std::string& source_archive);
};

void CheckErrors();
std::string NameWithoutExtension(const std::string& file_name);
void CountHeaderFill(unsigned char count_header[], uint64_t bytes_count_encoded);
static void AddEncodedByteToArchive(unsigned char current_byte, std::ofstream& arch);
std::string FileName(const std::string& path);
Archive ArchiveOpen(const std::string& arch_name);
uint64_t BytesCount(std::ifstream& file);
void AddCountHeaderToArchive(std::ofstream& arch, uint64_t bytes_count);
void AddNameHeaderToArchive(std::ofstream& arch, const std::string& path);
void AddEncodedFileToArchive(std::ifstream& file, std::ofstream& arch, unsigned char control_bits_count);
unsigned char DecodeHammingToByte(unsigned char first_byte, unsigned char second_byte);
uint64_t DecodeCountHeader(unsigned char current_byte, std::ifstream& arch);
std::string DecodeNameHeader(std::ifstream& arch);
unsigned char DecodeBitsHeader(std::ifstream& arch);

