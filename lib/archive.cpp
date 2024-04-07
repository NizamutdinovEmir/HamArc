#include "archive.h"
#include "math.h"

bool isDecodedCorrectly = true;

//структура архива
// файл хранится в виде закодированных байт
// каждый байт кодируется кодом хэмминга и записываются в архив
// в архиве хранятся заголовки о
// -> количестве байт - 8 байт , закодированная 16 байт
// -> имя файла - 20 байт , закодированное 40 байт
// -> биты контроля (4)
// -> сам файл
//

// создать архив
void Archiver::Create(const std::string &arch_name,
                      const std::vector<std::string> &file_names,
                      unsigned char control_bits_count) {
  Archive archive = ArchiveOpen(arch_name);
  archive.Clear();
  // создали архив
  for (size_t i = 0; i < file_names.size(); i++) {
    archive.AddFile(file_names[i], control_bits_count);
    // добавляем файлы
  }
  CheckErrors();
}

void Archiver::List(const std::string& arch_name) {
  Archive archive = ArchiveOpen(arch_name);
  // открыли архив
  std::vector<std::string> file_names = archive.FileList();
  for (size_t i = 0; i < file_names.size(); i++) {
    std::cout << file_names[i] << '\n';
  }
  CheckErrors();
}

void Archiver::Extract(const std::string& arch_name, const std::vector<std::string>& file_names) {
  Archive archive = ArchiveOpen(arch_name);
  std::vector<std::string> files_extract;
  if (file_names.empty()) {
    files_extract = archive.FileList();
  } else {
    files_extract = file_names;
  }
  std::filesystem::create_directory(NameWithoutExtension(arch_name));
  for (size_t i = 0; i < files_extract.size(); i++) {
    archive.ExtractFile(files_extract[i], NameWithoutExtension(arch_name));
  }
  CheckErrors();
}

void Archiver::Append(const std::string& arch_name, const std::vector<std::string>& file_names, unsigned char control_bits_count) {
  Archive archive = ArchiveOpen(arch_name);
  for (size_t i = 0; i < file_names.size(); i++) {
    archive.AddFile(file_names[i], control_bits_count);
  }
  CheckErrors();
}

void Archiver::Delete(const std::string& arch_name, const std::vector<std::string>& file_names) {
  Archive archive = ArchiveOpen(arch_name);
  for (size_t i = 0; i < file_names.size(); i++) {
    archive.DeleteFile(file_names[i]);
  }
  CheckErrors();
}

void Archiver::Concatenate(const std::string& arch_name, const std::vector<std::string>& file_names) {
  Archive archive = ArchiveOpen(arch_name);
  archive.Clear();
  for (size_t i = 0; i < file_names.size(); i++) {
    archive.AddFilesFrom(file_names[i]);
  }
  CheckErrors();
}

void Archive::AddFile(const std::string& file_name, unsigned char control_bits_count) {
  std::ifstream file;
  std::ofstream arch;
  file.open(file_name, std::ios::binary);
  arch.open(arch_name, std::ios::app | std::ios::binary);
  // в бинарном виде с конца

  uint64_t bytes_count = BytesCount(file);
  AddCountHeaderToArchive(arch, bytes_count);
  // количество байт в файле

  AddNameHeaderToArchive(arch, file_name);
  // кодирует имя файла

  AddEncodedByteToArchive(control_bits_count , arch);
  // колво контрольных битов

  AddEncodedFileToArchive(file, arch, control_bits_count);
  // добавляем файл

  arch.close();
  file.close();
}

unsigned char Archive::ExtractFile(const std::string& file_name, const std::string& path) {
  std::ofstream file;
  std::ifstream arch;
  arch.open(arch_name, std::ios::in | std::ios::binary);
  file.open(path + '\\' + file_name, std::ios::binary);

  unsigned char current_byte;
  uint64_t bytes_count;
  std::string current_file_name;
  unsigned char control_bits_count;


  while (arch >> current_byte) {
    bytes_count = DecodeCountHeader(current_byte, arch);
    current_file_name = DecodeNameHeader(arch);

    control_bits_count = DecodeBitsHeader(arch);

    uint64_t hamming_bits_count = pow(2, control_bits_count);
    uint64_t hamming_bytes_count = hamming_bits_count / CHAR_BIT;
    uint64_t information_bits_count = hamming_bits_count - control_bits_count - 1;
    uint64_t encoded_bytes_count =
        ((bytes_count * CHAR_BIT / information_bits_count) + (bytes_count * 8 % information_bits_count != 0)) *
            hamming_bytes_count;

    if (current_file_name == file_name) {

      std::vector<bool> current_hamming_bits(hamming_bits_count, false);
      std::vector<bool> to_byte(CHAR_BIT);
      uint64_t current_bytes_count = 0;
      bool current_bits[CHAR_BIT];
      bool byte[CHAR_BIT];
      uint64_t current_bit = 0;
      uint64_t control_bit = 1;
      uint64_t current_to_byte_bit = 0;
      for (size_t i = 0; i < encoded_bytes_count; i += hamming_bytes_count) {
        current_bit = 0;
        control_bit = 1;
        for (size_t j = 0; j < hamming_bytes_count; j++) {

          current_byte = arch.get();

          for (size_t k = 0; k < CHAR_BIT; k++) {
            byte[CHAR_BIT - k - 1] = (current_byte >> k) & 1;
          }

          for (size_t k = 0; k < CHAR_BIT; k++) {
            current_hamming_bits[current_bit++] = byte[k];
          }
        }

        bool byte_decode = DecodeHamming(current_hamming_bits);

        if (!byte_decode && isDecodedCorrectly) {
          isDecodedCorrectly = false;
        }

        control_bit = 1;
        current_byte = 0;

        for (uint64_t k = 0; k < hamming_bits_count; k++) {
          if (current_bytes_count == bytes_count) {
            break;
          }
          if (k == control_bit - 1) {
            control_bit *= 2;
            continue;
          }
          current_bits[current_to_byte_bit++] = current_hamming_bits[k];
          if (current_to_byte_bit == 8) {
            current_to_byte_bit = 0;
            for (size_t j = 0; j < CHAR_BIT; j++) {
              if (current_bits[j]) {
                current_byte += static_cast<unsigned char>(pow(2, CHAR_BIT - j - 1));
              }
            }
            file << current_byte;
            current_bytes_count++;
            current_byte = 0;
          }
        }
      }
      return control_bits_count;
    } else {
      arch.seekg(encoded_bytes_count, std::ios::cur);
    }
  }
  std::cerr << "Can't find file in archive.";
  return 0;
}

std::vector<std::string> Archive::FileList() {

  std::vector<std::string> file_list;

  std::ifstream arch(arch_name, std::ios::binary | std::ios::in);

  unsigned char current_byte;
  uint64_t bytes_count;
  std::string current_file_name;
  unsigned char control_bits_count;

  while (arch >> current_byte) {

    bytes_count = DecodeCountHeader(current_byte, arch);

    current_file_name = DecodeNameHeader(arch);

    control_bits_count = DecodeBitsHeader(arch);

    file_list.push_back(current_file_name);

    uint64_t hamming_bits_count = pow(2, control_bits_count);
    uint64_t hamming_bytes_count = hamming_bits_count / CHAR_BIT;
    uint64_t information_bits_count = hamming_bits_count - control_bits_count - 1;
    uint64_t encoded_bytes_count =
        ((bytes_count * CHAR_BIT / information_bits_count) + (bytes_count * 8 % information_bits_count != 0)) *
            hamming_bytes_count;

    arch.seekg(encoded_bytes_count, std::ios::cur);
  }
  return file_list;
}


unsigned char DecodeBitsHeader(std::ifstream& arch) {
  unsigned char first_byte;
  unsigned char second_byte;
  arch >> first_byte;
  arch >> second_byte;

  unsigned char result = DecodeHammingToByte(first_byte, second_byte);

  return result;
}

std::string NameWithoutExtension(const std::string& file_name) {
  for (size_t i = 0; i < file_name.size(); i++) {
    if (file_name[i] == '.') {
      return file_name.substr(0, i);
    }
  }
  return file_name;
}
// заполняет массив данными представляющими количество байт в файле
void CountHeaderFill(unsigned char count_header[], uint64_t bytes_count_encoded) {
  for (size_t i = 0; i < kCountHeaderSize; i++) {
    count_header[i] = (bytes_count_encoded >> ((kCountHeaderSize - i - 1) * CHAR_BIT));
  }
}


// добавляет байт в архив
static void AddEncodedByteToArchive(unsigned char current_byte, std::ofstream& arch) {
  bool current_bits[CHAR_BIT];
  size_t control_bit;
  std::vector<bool> hamming_code(CHAR_BIT, false);

  for (size_t j = 0; j < CHAR_BIT; j++) {
    // заполняем массив байта из текущего байта
    current_bits[CHAR_BIT - j - 1] = (current_byte >> j) & 1;
  }
  // применяем код хэмминга
  for (uint16_t bits_count: kBits) {
    control_bit = 1;
    for (size_t i = 0; i < CHAR_BIT; i++) {
      if (control_bit - 1 == i) {
        control_bit *= 2;
      } else {
        hamming_code[i] = current_bits[bits_count];
        bits_count++;
      }
    }
    EncodeHamming(hamming_code);
    current_byte = 0;
    for (int i = 0; i < CHAR_BIT; i++) {
      if (hamming_code[i]) {
        current_byte += static_cast<unsigned char>(pow(2, CHAR_BIT - i - 1));
      }
    }
    arch << current_byte;
  }
}


void Archive::DeleteFile(const std::string& file_name) {

  Archive arch_tmp = ArchiveOpen(arch_tmp_name);

  std::vector<std::string> file_list = FileList();

  unsigned char control_bits_count;

  for (size_t i = 0; i < file_list.size(); i++) {
    if (file_list[i] != file_name) {
      control_bits_count = ExtractFile(file_list[i], std::filesystem::temp_directory_path().string());
      arch_tmp.AddFile(std::filesystem::temp_directory_path().string() + file_list[i],
                       control_bits_count);
    }
  }
  remove(arch_name.c_str());
  rename(arch_tmp_name.c_str(), arch_name.c_str());
}

void Archive::Clear() {
  std::ofstream arch;
  arch.open(arch_name, std::ios::trunc | std::ios::binary);
}

void Archive::AddFilesFrom(const std::string& arch) {

  Archive arch_from = ArchiveOpen(arch);

  std::vector<std::string> file_list = arch_from.FileList();

  unsigned char control_bits_count;

  for (size_t i = 0; i < file_list.size(); i++) {
    control_bits_count = arch_from.ExtractFile(file_list[i],
                                               std::filesystem::temp_directory_path().string());
    AddFile(std::filesystem::temp_directory_path().string() + file_list[i], control_bits_count);
  }
}

std::string FileName(const std::string& path) {
  for (int i = path.size() - 1; i >= 0; i--) {
    if (path[i] == '\\') {
      return path.substr(i + 1, path.size() - i - 1);
    }
  }
  return path;
}

Archive ArchiveOpen(const std::string& arch_name) {
  Archive archive;
  archive.arch_name = NameWithoutExtension(arch_name) + ".haf";
  return archive;
}

uint64_t BytesCount(std::ifstream& file) {
  char current_byte;
  size_t byte_count = 0;

  while (file.get(current_byte)) {
    byte_count++;
  }

  file.clear();
  file.seekg(0, std::ios::beg);

  return byte_count;
}

void AddCountHeaderToArchive(std::ofstream& arch, uint64_t bytes_count) {
  unsigned char current_byte;
  uint64_t bytes_count_encoded = bytes_count;
  unsigned char count_header[kCountHeaderSize];
  CountHeaderFill(count_header, bytes_count_encoded);
  for (uint8_t k = 0; k < kCountHeaderSize; k++) {
    current_byte = count_header[k];
    AddEncodedByteToArchive(current_byte, arch);
  }
}

void AddNameHeaderToArchive(std::ofstream& arch, const std::string& path) {
  unsigned char current_byte;
  std::string file_name = FileName(path);
  // для хранения имени файла
  unsigned char name[kNameHeaderSize];
  for (size_t i = 0; i < kNameHeaderSize; i++) {
    if (i < file_name.size()) {
      name[i] = file_name[i];
    } else {
      name[i] = '\0';
    }
  }
  for (uint64_t k = 0; k < kNameHeaderSize; k++) {
    current_byte = name[k];
    AddEncodedByteToArchive(current_byte, arch);
  }
}


void AddEncodedFileToArchive(std::ifstream& file, std::ofstream& arch, unsigned char control_bits_count) {
  unsigned char current_byte; // текущий байт из файла
  uint64_t bytes_count = BytesCount(file); // все байты
  uint64_t hamming_bits_count = pow(2, control_bits_count); // количество бит для кода хэмминга
  uint64_t information_bits_count = hamming_bits_count - control_bits_count - 1; // кол-во информационных битов
  std::vector<bool> current_hamming_bits(hamming_bits_count, false); // вектор текущих битов хэмминга
  std::vector<bool> next_hamming_bits(CHAR_BIT, false); //
  uint64_t prev_bits_count = 0;
  uint64_t current_bytes_count = 0;
  uint64_t bytes_add;
  uint64_t control_bit = 1;
  uint64_t current_bit = 0;
  uint64_t next_bits_count = 0;
  bool current_bits[CHAR_BIT];

  while ((current_bytes_count < bytes_count) || (prev_bits_count != 0)) { // пока не будут обработаны все биты
    bytes_add = (information_bits_count - prev_bits_count) / 8 + ((information_bits_count - prev_bits_count) % 8 != 0);
    for (size_t i = 0; (i < bytes_add) && (current_bytes_count < bytes_count); i++) {
      current_bytes_count++;
      current_byte = file.get();
      // считали байт из файла
      // разбиваем их на биты
      // добавляем контрольные
      // кодируем кодом хэмминга
      for (size_t j = 0; j < CHAR_BIT; j++) {
        current_bits[CHAR_BIT - j - 1] = (current_byte >> j) & 1;
      }

      for (size_t j = 0; j < CHAR_BIT; j++) {
        while (current_bit == control_bit - 1) {
          current_bit++;
          control_bit *= 2;
        }
        if (current_bit >= hamming_bits_count) {
          next_hamming_bits[next_bits_count++] = current_bits[j];
        } else {
          current_hamming_bits[current_bit++] = current_bits[j];
        }
      }
    }

    EncodeHamming(current_hamming_bits);

    // добавляем закодированные байты

    for (uint64_t i = 0; i < hamming_bits_count; i += 8) {
      current_byte = 0;
      for (size_t j = 0; j < CHAR_BIT; j++) {
        if (current_hamming_bits[i + j]) {
          current_byte += static_cast<unsigned char>(pow(2, CHAR_BIT - j - 1));
        }
      }
      arch << current_byte;

    }

    for (uint64_t i = 0; i < hamming_bits_count; i++) {
      current_hamming_bits[i] = false;
    }

    current_bit = 0;
    control_bit = 1;
    for (int i = 0; i < next_bits_count; i++) {
      while (current_bit == control_bit - 1) {
        current_bit++;
        control_bit *= 2;
      }
      current_hamming_bits[current_bit++] = next_hamming_bits[i];
    }
    prev_bits_count = next_bits_count;
    next_bits_count = 0;
  }
}


unsigned char DecodeHammingToByte(unsigned char first_byte, unsigned char second_byte) {
  unsigned char result_byte = 0;
  std::vector<bool> hamming_code1(CHAR_BIT);
  std::vector<bool> hamming_code2(CHAR_BIT);
  std::vector<bool> hamming_decoded(CHAR_BIT);
  for (size_t j = 0; j < CHAR_BIT; j++) {
    hamming_code1[CHAR_BIT - j - 1] = (first_byte >> j) & 1;
  }
  for (size_t j = 0; j < CHAR_BIT; j++) {
    hamming_code2[CHAR_BIT - j - 1] = (second_byte >> j) & 1;
  }

  bool isFirstPartDecoded = DecodeHamming(hamming_code1);
  bool isSecondPartDecoded = DecodeHamming(hamming_code2);

  if (!isFirstPartDecoded || !isSecondPartDecoded) {
    isDecodedCorrectly = false;
  }

  hamming_decoded[0] = hamming_code1[2];
  hamming_decoded[1] = hamming_code1[4];
  hamming_decoded[2] = hamming_code1[5];
  hamming_decoded[3] = hamming_code1[6];

  hamming_decoded[4] = hamming_code2[2];
  hamming_decoded[5] = hamming_code2[4];
  hamming_decoded[6] = hamming_code2[5];
  hamming_decoded[7] = hamming_code2[6];

  for (size_t j = 0; j < CHAR_BIT; j++) {
    if (hamming_decoded[j]) {
      result_byte += static_cast<unsigned char>(std::pow(2, CHAR_BIT - j - 1));
    }
  }

  return result_byte;
}

uint64_t DecodeCountHeader(unsigned char current_byte, std::ifstream& arch) {
  uint64_t bytes_count = 0;
  unsigned char count_header_encoded[kEncodedCountHeaderSize];
  count_header_encoded[0] = current_byte;
  unsigned char count_header[kCountHeaderSize];

  for (size_t i = 1; i < kEncodedCountHeaderSize; i++) {
    arch >> current_byte;
    count_header_encoded[i] = current_byte;
  }

  for (size_t i = 0; i < kCountHeaderSize; i++) {
    count_header[i] = DecodeHammingToByte(count_header_encoded[i * 2], count_header_encoded[i * 2 + 1]);
  }

  for (size_t i = 0; i < kCountHeaderSize; i++) {
    bytes_count += (count_header[i] << ((kCountHeaderSize - i - 1) * CHAR_BIT));
  }

  return bytes_count;
}

std::string DecodeNameHeader(std::ifstream& arch) {
  unsigned char current_byte;
  unsigned char name_header_encoded[kEncodedNameHeaderSize];
  unsigned char name_header[kNameHeaderSize];

  for (size_t i = 0; i < kEncodedNameHeaderSize; i++) {
    arch >> current_byte;
    name_header_encoded[i] = current_byte;
  }

  for (size_t i = 0; i < kNameHeaderSize; i++) {
    name_header[i] = DecodeHammingToByte(name_header_encoded[i * 2], name_header_encoded[i * 2 + 1]);
  }
  std::string file_name(reinterpret_cast<const char* const>(name_header));

  return file_name;
}


void CheckErrors() {
  if (!isDecodedCorrectly) {
    std::cerr << "Archived files can't be decoded correctly!";
  }
}
