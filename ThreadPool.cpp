#include "ThreadPool.h"

// 构造函数：创建线程
ThreadPool::ThreadPool(size_t thread_num) : m_running(true) {
  if (thread_num == 0) thread_num = 1;

  // 启动指定数量的工作线程
  for (size_t i = 0; i < thread_num; ++i) {
    m_workers.emplace_back(&ThreadPool::worker_loop, this);
  }
}

// 析构函数：停止线程池
ThreadPool::~ThreadPool() {
  m_running = false;
  m_cv.notify_all();

  // 等待所有线程退出
  for (auto& thread : m_workers) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

// 工作线程主循环
void ThreadPool::worker_loop() {
  while (m_running) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(m_mutex);
      // 等待任务或停止信号
      m_cv.wait(lock, [this]() { return !m_running || !m_tasks.empty(); });

      if (!m_running && m_tasks.empty()) return;

      task = std::move(m_tasks.front());
      m_tasks.pop();
    }

    // 执行任务
    task();
  }
}