#include <chrono>
#include <iostream>
#include <string>
#include <thread>

void task(const std::string& name, int time) {
  std::cout << "Task " << name << " is starting." << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(time));
  std::cout << "Task " << name << " is completed." << std::endl;
}

int main() {
  std::cout << "Starting tasks..." << std::endl;

  std::thread t1(task, "A", 1000);
  std::thread t2(task, "B", 1500);
  std::thread t3(task, "C", 500);

  t1.join();
  t2.join();
  t3.join();

  std::cout << "All tasks are completed." << std::endl;
  return 0;
}