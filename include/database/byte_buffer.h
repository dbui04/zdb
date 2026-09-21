#pragma once

#include <cstddef>
#include <span>
#include <vector>

class ByteBuffer {
public:
  void append(std::span<const std::byte>);
  void consume(std::size_t);
  std::span<const std::byte> readable() const;
  std::size_t size() const;
  bool empty() const;

private:
  std::vector<std::byte> buffer;
  std::size_t read_pos = 0;
};
