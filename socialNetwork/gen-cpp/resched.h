#ifndef RESCHED_H
#define RESCHED_H

#include <cstddef>  // 用于 size_t
#include <fcntl.h>  // 用于文件控制选项
#include <sys/mman.h>  // 用于内存映射
#include <unistd.h>  // 用于 close, ftruncate 等
#include <atomic>  // 用于原子操作
#include <string>
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <unistd.h>
#include <syscall.h>  // 用于 gettid
#define gettid() syscall(SYS_gettid)

#define SHM_NAME "/thread_priority"
#define BUFFER_SIZE 2048

class RingBuffer {
  public:
  struct SchedMessage {
    int tid;
    int queue;
    long prio;
  };
  struct Buffer {
    std::atomic<size_t> head;
    std::atomic<size_t> tail;
    std::atomic<size_t> write_index;
    SchedMessage buffer[];
  };
  ~RingBuffer() {};
  RingBuffer(const char *topic, size_t buffer_len) : topic_(topic), buffer_len_(buffer_len), is_attached_(false) {}
  bool init() {
    shm_fd_ = shm_open(topic_.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
    if (shm_fd_ < 0 ) {
      if (errno == EEXIST) {
        shm_fd_ = shm_open(topic_.c_str(), O_RDWR, 0666);
        if (shm_fd_ < 0 && errno == ENOENT) {
          std::cerr << "shm_open failed" << std::endl;
          return false;
        }
      } else {
        std::cerr << errno << std::endl;
        std::cerr << "shm_open failed" << std::endl;
        return false;
      }
    }

    auto total_size = sizeof(Buffer) + buffer_len_*sizeof(SchedMessage);
    if (ftruncate(shm_fd_, total_size) == EINVAL) {
      std::cerr << "ftruncate failed" << std::endl;
      close(shm_fd_);
      return false;
    }

    auto *ptr = mmap(nullptr, total_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, shm_fd_, 0);
    if (ptr == MAP_FAILED) {
      std::cerr << "mmap failed" << std::endl;
      close(shm_fd_);
      return false;
    }
    
    buffer_ = static_cast<Buffer *>(ptr);
    buffer_->head = 0;
    buffer_->tail = 0;
    buffer_->write_index = 0;
    is_attached_ = true;

    return true;
  }
  bool attach() {
    shm_fd_ = shm_open(topic_.c_str(), O_RDWR, 0666);
    if (shm_fd_ < 0) {
      std::cerr << "shm_open failed" << std::endl;
      return false;
    }
    auto total_size = sizeof(Buffer) + buffer_len_*sizeof(SchedMessage);
    auto *ptr = mmap(nullptr, total_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, shm_fd_, 0);
    if (ptr == MAP_FAILED) {
      std::cerr << "mmap failed" << std::endl;
      close(shm_fd_);
      return false;
    }
    buffer_ = static_cast<Buffer *>(ptr);
    is_attached_ = true;

    return true;
  }

  bool push_message(const SchedMessage &message) {
    size_t current_write_index;
    size_t next_write_index;
    bool success = false;

    if (!is_attached_) {
      std::cerr << "RingBuffer not attached, abort push message.\n";
      return false;
    }

    do {
      current_write_index = buffer_->write_index.load(std::memory_order_acquire);
      next_write_index = (current_write_index + 1) % buffer_len_;
      if (next_write_index == buffer_->tail.load(std::memory_order_acquire)) {
        std::cerr << "Buffer full, abort push message.\n";
        return false;
      }
      success = buffer_->write_index.compare_exchange_weak(
          current_write_index, next_write_index,
          std::memory_order_release, std::memory_order_acquire
      );
    } while (!success);
    buffer_->buffer[current_write_index] = message;
    size_t current_head = buffer_->head.load(std::memory_order_acquire);
    while (!buffer_->head.compare_exchange_weak(
      current_head, next_write_index,
      std::memory_order_release, std::memory_order_acquire
    ));
    return true;
  }
  bool pop_message(SchedMessage &message) {
    if (!is_attached_) {
      std::cerr << "RingBuffer not attached, abort pop message.\n";
      return false;
    }

    size_t tail = buffer_->tail.load(std::memory_order_acquire);
    if (tail == buffer_->head.load(std::memory_order_acquire)) {
      return false;
    }
    message = buffer_->buffer[tail];
    buffer_->tail.store((tail + 1) % buffer_len_, std::memory_order_release);
    return true;
  }

  bool detach() {
    if (!is_attached_) {
      return true;
    }

    size_t total_size = sizeof(Buffer) + buffer_len_*sizeof(SchedMessage);
    if (munmap(buffer_, total_size) == -1) {
      std::cerr << "munmap failed" << std::endl;
      return false;
    }
    close(shm_fd_);
    return true;
  }

  bool destroy() {
    if (!is_attached_) {
      std::cerr << "RingBuffer not attached, abort destroy.\n";
      return false;
    }

    detach();
    if (shm_unlink(topic_.c_str()) == -1) {
      std::cerr << "shm_unlink failed" << std::endl;
      return false;
    }
    return true;
  }
  private:
  std::string topic_;
  int shm_fd_;
  Buffer *buffer_;
  size_t buffer_len_;
  bool is_attached_;
};

static inline long stringRefToInt(const std::string & str) {
  long result = 0;
  for (char ch : str) {
      if (ch >= '0' && ch <= '9') {
          result = result * 10 + (ch - '0');
      } else {
          throw std::invalid_argument("Invalid character in number");
      }
  }
  return result;
}

inline void reschedule(std::map<std::string, std::string> context) {
  int enabled = 0;
  auto it = context.find("sched-enable");
  if (it != context.end()) {
    try {
      enabled = stringRefToInt(it->second);  // 将字符串转换为 int
#ifdef DEBUG_SCHED
      std::cout << "Found enable: " << enabled << std::endl;
#endif
    } catch (const std::invalid_argument& e) {
      std::cerr << "Invalid argument: value is not a number" << std::endl;
    } catch (const std::out_of_range& e) {
      std::cerr << "Out of range: value is too large for int" << std::endl;
    }

    if (enabled == 0) {
      return;
    }

  } else {
      std::cerr << "Key 'sched-enable' not found" << std::endl;
      return;
  }

  it = context.find("sched-sla");
  int sla = -1;
  if (it != context.end()) {
    try {
      sla = stringRefToInt(it->second);  // 将字符串转换为 int
#ifdef DEBUG_SCHED
      std::cout << "Found SLA: " << sla << std::endl;
#endif
    } catch (const std::invalid_argument& e) {
      std::cerr << "Invalid argument: value is not a number" << std::endl;
      return;
    } catch (const std::out_of_range& e) {
      std::cerr << "Out of range: value is too large for int" << std::endl;
      return;
    }
  } else {
      // 如果没有找到键 "SLA"，输出 "not found"
      std::cerr << "Key 'sched-sla' not found" << std::endl;
      return;
  }

  it = context.find("sched-time-start");
  long time_start_ms = -1;
  if (it != context.end()) {
    try {
      time_start_ms = stringRefToInt(it->second);  // 将字符串转换为 int
#ifdef DEBUG_SCHED
      std::cout << "Found start time: " << time_start_ms << std::endl;
#endif
    } catch (const std::invalid_argument& e) {
      std::cerr << "Invalid argument: value is not a number" << std::endl;
      return;
    } catch (const std::out_of_range& e) {
      std::cerr << "Out of range: value is too large for int" << std::endl;
      return;
    }
  } else {
      std::cerr << "Key 'sched-time-start' not found" << std::endl;
      return;
  }

  it = context.find("sched-time-next");
  int time_next_ms = -1;
  if (it != context.end()) {
    try {
      time_next_ms = stringRefToInt(it->second);  // 将字符串转换为 int
#ifdef DEBUG_SCHED
      std::cout << "Found time of next service: " << time_next_ms << std::endl;
#endif
    } catch (const std::invalid_argument& e) {
      std::cerr << "Invalid argument: value is not a number" << std::endl;
      return;
    } catch (const std::out_of_range& e) {
      std::cerr << "Out of range: value is too large for int" << std::endl;
      return;
    }
  } else {
      std::cerr << "Key 'sched-time-next' not found" << std::endl;
      return;
  }

  it = context.find("sched-time-remaining");
  int time_remaining_ms = -1;
  if (it != context.end()) {
    try {
      time_remaining_ms = stringRefToInt(it->second);  // 将字符串转换为 int
#ifdef DEBUG_SCHED
      std::cout << "Found time of remaining services: " << time_remaining_ms << std::endl;
#endif
    } catch (const std::invalid_argument& e) {
      std::cerr << "Invalid argument: value is not a number" << std::endl;
      return;
    } catch (const std::out_of_range& e) {
      std::cerr << "Out of range: value is too large for int" << std::endl;
      return;
    }
  } else {
      std::cerr << "Key 'sched-time-remaining' not found" << std::endl;
      return;
  }

  auto time_now = std::chrono::system_clock::now();
  auto time_now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch()).count();
  long time_available_ms = sla - (time_now_ms - time_start_ms) - time_remaining_ms;
  std::cout << "SLA: " << sla << "time now: " << time_now_ms << " time start: " << time_start_ms << " time elapsed: " << time_now_ms - time_start_ms << std::endl;
  RingBuffer ringBuffer(SHM_NAME, BUFFER_SIZE);
  if (!ringBuffer.attach()) {
    std::cerr << "Failed to attach to ring buffer\n";
    ringBuffer.detach();
    return;
  }
  int tid = gettid();
  int cpu_id = sched_getcpu();
  RingBuffer::SchedMessage msg;
  if (time_available_ms > 0) {
    msg = {tid, cpu_id+256, time_available_ms};
  } else {
    msg = {tid, cpu_id+128, time_start_ms};
    //std::cout << "tid: " << tid << " time_available_ms: " << time_available_ms << std::endl;
  }
	ringBuffer.push_message(msg);
	ringBuffer.detach();
	int duration = 100; // us
	auto start = std::chrono::steady_clock::now();
	auto end = start + std::chrono::microseconds(duration);
	volatile int dummy = 0;
	while (std::chrono::steady_clock::now() < end) {
		dummy++;
	}
	sched_yield();
}
#endif // RESCHED_H