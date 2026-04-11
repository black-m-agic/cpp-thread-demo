#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

int main() {
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    std::cerr << "Failed to create socket" << std::endl;
    return -1;
  }

  struct sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(8080);
  bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  listen(listen_fd, 5);
  std::cout << "Server is listening on port 8080..." << std::endl;
  while (true) {
    int client_fd = accept(listen_fd, nullptr, nullptr);
    if (client_fd < 0) {
      std::cerr << "Failed to accept connection" << std::endl;
      continue;
    }
    const char* message = "Hello from the server!";
    send(client_fd, message, strlen(message), 0);
    std::cout << "Sent message to client" << std::endl;
    close(client_fd);
  }
  close(listen_fd);
  return 0;
}