#include <array>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr{.sin_family = AF_INET,
                   .sin_port = htons(2004),
                   .sin_addr = {.s_addr = htonl(INADDR_LOOPBACK)}};
  if (connect(client_fd, reinterpret_cast<const sockaddr *>(&addr),
              sizeof(addr)) == -1) {
    // failed
    close(client_fd);
    return -1;
  };
  uint8_t value = 24;
  auto buf = std::bit_cast<std::array<std::byte, sizeof(uint8_t)>>(value);
  write(client_fd, buf.data(), buf.size());
}
