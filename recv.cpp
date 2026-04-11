#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

int main() {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd < 0) {
    std::cerr << "Failed to create socket" << std::endl;
    return -1;
  }
  struct sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8080);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
      0) {
    std::cerr << "Failed to connect to server" << std::endl;
    return -1;
  }
  std::cout << "Connected to server" << std::endl;
  char buffer[1024];
  ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received < 0) {
    std::cerr << "Failed to receive message" << std::endl;
    return -1;
  }
  buffer[bytes_received] = '\0';
  std::cout << "Received message from server: " << buffer << std::endl;
  close(client_fd);
  return 0;
}