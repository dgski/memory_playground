#include <iostream>
#include <fstream>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <execution>

#include "utils.hpp"

#include "vector.hpp"

namespace bip = boost::interprocess;

void createFile(const char* fileName, size_t size) {
  std::ofstream file(fileName, std::ios::binary);
  for (size_t i = 0; i < size; ++i) {
    if (rand() % 2) {
      file.put('b');
    } else {
      file.put('a');
    }
  }
}

int numberOfBs(const char* buffer, size_t size) {
  int count = 0;
  for (size_t i = 0; i < size; ++i) {
    if (buffer[i] == 'b') {
      ++count;
    }
  }
  return count;
}

int main2() {
  constexpr auto ITERATIONS = 10;
  constexpr auto FILE_SIZE = 1024 * 1024 * 1024;
  constexpr const char* FILE_NAME = "file.bin";
  utils::RunOnScopeExit removeFile([FILE_NAME] { std::remove(FILE_NAME); });
  createFile(FILE_NAME, FILE_SIZE);

  const auto mappedFileResult = utils::benchmark([&]() {
    bip::file_mapping file(FILE_NAME, bip::read_only);
    bip::mapped_region region(file, bip::read_only);
    return numberOfBs(static_cast<const char*>(region.get_address()), FILE_SIZE);
  }, ITERATIONS);
  std::cout << "M: " << mappedFileResult.second << " ns" << ", " << mappedFileResult.first << std::endl;

  const auto streamResult2 = utils::benchmark([&]() {
    std::ifstream file(FILE_NAME, std::ios::binary);
    std::vector<char> buffer(FILE_SIZE);
    file.read(buffer.data(), FILE_SIZE);
    return numberOfBs(buffer.data(), FILE_SIZE);
  }, ITERATIONS);
  std::cout << "S: " << streamResult2.second << " ns, " << streamResult2.first << std::endl;

  return 0;
}

std::vector<size_t> getRandomIndices(size_t size, size_t count) {
  std::vector<size_t> indices;
  indices.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    indices.push_back(rand() % size);
  }
  return indices;
}

int main() {
  std::vector<size_t> indices;
  {
    FileMappedVector<int> v("vector.bin");
    indices = getRandomIndices(v.size(), 1000);
  }
  FileMappedVector<int> v("vector.bin");
  const auto result = utils::benchmark([&]() {
    auto result = 0;
    for (auto index : indices) {
      result += v[index];
    }
    return result;
  }, 1000);
  std::cout << "M: " << result.second << " ns" << ", " << result.first << std::endl;

  std::ifstream file("vector.bin", std::ios::binary);
  const auto result2 = utils::benchmark([&]() {
    auto result = 0;
    for (auto index : indices) {
      file.seekg(index * sizeof(int));
      int value;
      file.read(reinterpret_cast<char*>(&value), sizeof(int));
      result += value;
    }
    return result;
  }, 1000);
  std::cout << "S: " << result2.second << " ns" << ", " << result2.first << std::endl;
  return 0;
}