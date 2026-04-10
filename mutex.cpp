#include <iostream>
#include <mutex>
#include <thread>

std::mutex mtx;
int counter = 0;

void increment(int id) {
  for (int i = 0; i < 100000; ++i) {
    std::lock_guard<std::mutex> guard(mtx);
    ++counter;
  }
}

int main() {
  std::thread t1(increment, 1);
  std::thread t2(increment, 2);

  t1.join();
  t2.join();

  std::cout << "Final counter value: " << counter << std::endl;
  return 0;
}