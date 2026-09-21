#include <netinet/in.h>
#include <sys/socket.h>

int main() {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr{.sin_family = AF_INET,
                   .sin_port = htons(2004),
                   .sin_addr = {.s_addr = htonl(INADDR_LOOPBACK)}};
  connect(client_fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
}
