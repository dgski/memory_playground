#pragma once

#include <fstream>
#include <filesystem>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace bip = boost::interprocess;

template<typename T>
class FileMappedVector {
  const std::string _filename;
  bip::file_mapping _file;
  bip::mapped_region _region;
  std::ofstream _appendStream;
  std::span<T> _data;
  size_t _size = 0;

  void resizeFile() {
    const auto pos = _appendStream.tellp();
    _appendStream.close();
    std::filesystem::resize_file(_filename, pos);
    _appendStream.open(_filename, std::ios::binary | std::ios::app);
  }
public:
  FileMappedVector(std::string_view filename) : _filename(filename) {
    _appendStream.open(filename.data(), std::ios::binary | std::ios::app);
    _appendStream.seekp(0, std::ios::end);
    _size = _appendStream.tellp() / sizeof(T);
    remap();
  }
  ~FileMappedVector() { resizeFile(); }
  FileMappedVector(const FileMappedVector&) = delete;
  FileMappedVector& operator=(const FileMappedVector&) = delete;
  FileMappedVector(FileMappedVector&&) = delete;
  FileMappedVector& operator=(FileMappedVector&&) = delete;

  void push_back(const T& value) {
    _appendStream.write(reinterpret_cast<const char*>(&value), sizeof(T));
  }
  void pop_back() {
    _appendStream.seekp(-sizeof(T), std::ios::end);
  }
  void clear() { _appendStream.seekp(0); }
  T& operator[](size_t index) { return _data[index]; }
  const T& operator[](size_t index) const { return _data[index]; }
  size_t size() const { return _data.size(); }
  auto begin() { return _data.begin(); }
  auto end() { return _data.end(); }
  auto begin() const { return _data.begin(); }
  auto end() const { return _data.end(); }
  auto& front() { return _data.front(); }
  auto& back() { return _data.back(); }
  auto& front() const { return _data.front(); }
  auto& back() const { return _data.back(); }
  void remap() {
    resizeFile();
    _file = bip::file_mapping(_filename.data(), bip::read_only);
    _region = bip::mapped_region(_file, bip::read_only);
    _data = std::span<T>(static_cast<T*>(_region.get_address()), _region.get_size() / sizeof(T));
  }
};