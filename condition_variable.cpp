#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

std::queue<int> g_queue;
std::mutex g_mtx;
std::condition_variable g_cv;
bool g_is_finished = false;

void producer() {
  for (int i = 1; i <= 5; ++i) {
    std::unique_lock<std::mutex> lock(g_mtx);
    g_queue.push(i);
    std::cout << "producer produced: " << i << std::endl;
    g_cv.notify_one();
    lock.unlock();
    // 如果不加入睡眠消费者太慢了根本抢不到锁，生产者一直占着锁，消费者一直在等待
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  {
    std::lock_guard<std::mutex> lock(g_mtx);
    g_is_finished = true;
    g_cv.notify_all();
  }
}
void consumer() {
  while (true) {
    std::unique_lock<std::mutex> lock(g_mtx);
    g_cv.wait(lock, []() { return !g_queue.empty() || g_is_finished; });
    if (g_is_finished && g_queue.empty()) break;

    int val = g_queue.front();
    g_queue.pop();
    std::cout << " consumer consumed: " << val << std::endl;
  }
}

int main() {
  std::thread prod(producer);
  std::thread cons(consumer);

  prod.join();
  cons.join();

  return 0;
}