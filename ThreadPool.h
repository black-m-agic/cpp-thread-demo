#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

class ThreadPool {
 public:
  // 构造函数，创建指定数量的线程, 默认线程数为CPU核心数
  explicit ThreadPool(size_t thread_num = std::thread::hardware_concurrency());
  // 禁用拷贝和移动（避免线程/锁的浅拷贝问题）
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;
  // 析构函数，停止线程池并清理资源
  ~ThreadPool();
  // 提交任务到线程池
  template <typename Func, typename... Args>
  void submit(Func&& func, Args&&... args);

 private:
  // 工作线程函数，循环等待并执行任务
  void worker_loop();

  // 成员变量
  std::vector<std::thread> m_workers;         // 工作线程列表
  std::queue<std::function<void()>> m_tasks;  // 任务队列
  std::mutex m_mutex;                         // 保护任务队列的互斥锁
  std::condition_variable m_cv;               // 任务到来时通知工作线程的
  std::atomic<bool> m_running;                // 线程池运行状态标志
};

// 模板函数实现必须放在头文件中
template <typename Func, typename... Args>
void ThreadPool::submit(Func&& func, Args&&... args) {
  using TaskType = std::function<void()>;
  TaskType task =
      std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    // 提交任务前检查线程池是否已停止，避免提交后任务无法执行
    if (!m_running) {
      throw std::runtime_error("ThreadPool has been stopped");
    }
    m_tasks.emplace(std::move(task));
  }
  m_cv.notify_one();  // 通知一个工作线程有新任务
}

#endif