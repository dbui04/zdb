#include "database/byte_buffer.h"
#include <cassert>

void ByteBuffer::append(std::span<const std::byte> stream) {
  buffer.append_range(stream);
  return;
}

void ByteBuffer::consume(std::size_t n) {
  assert(n <= size());
  read_pos += n;
}

std::span<const std::byte> ByteBuffer::readable() const {
  return std::span<const std::byte>(buffer);
}

std::size_t ByteBuffer::size() const { return buffer.size(); }

bool ByteBuffer::empty() const { return buffer.empty(); }
