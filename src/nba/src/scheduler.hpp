/*
 * Copyright (C) 2021 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <nba/log.hpp>
#include <nba/common/compiler.hpp>
#include <nba/integer.hpp>
#include <functional>
#include <limits>

namespace nba::core {

struct Scheduler {  
  template<class T>
  using EventMethod = void (T::*)(int);

  struct Event {
    std::function<void(int)> callback;
  private:
    friend class Scheduler;
    int handle;
    u64 timestamp;
  };

  Scheduler() {
    for (int i = 0; i < kMaxEvents; i++) {
      heap[i] = new Event();
      heap[i]->handle = i;
    }
    Reset();
  }

 ~Scheduler() {
    for (int i = 0; i < kMaxEvents; i++) {
      delete heap[i];
    }
  }

  void Reset() {
    heap_size = 0;
    timestamp_now = 0;
    Add(std::numeric_limits<u64>::max(), [](int) {
      Assert(false, "Scheduler: reached end of the event queue.");
    });
  }

  auto GetTimestampNow() const -> u64 {
    return timestamp_now;
  }

  auto GetTimestampTarget() const -> u64 {
    return heap[0]->timestamp;
  }

  auto GetRemainingCycleCount() const -> int {
    return int(GetTimestampTarget() - GetTimestampNow());
  }

  void AddCycles(int cycles) {
    auto timestamp_next = timestamp_now + cycles;
    Step(timestamp_next);
    timestamp_now = timestamp_next;
  }

  auto Add(u64 delay, std::function<void(int)> callback) -> Event* {
    int n = heap_size++;
    int p = Parent(n);

    Assert(
      heap_size <= kMaxEvents,
      "Scheduler: reached maximum number of events."
    );

    auto event = heap[n];
    event->timestamp = GetTimestampNow() + delay;
    event->callback = callback;

    while (n != 0 && heap[p]->timestamp > heap[n]->timestamp) {
      Swap(n, p);
      n = p;
      p = Parent(n);
    }

    return event;
  }

  template<class T>
  auto Add(u64 delay, T* object, EventMethod<T> method) -> Event* {
    return Add(delay, [object, method](int cycles_late) {
      (object->*method)(cycles_late);
    });
  }

  void Cancel(Event* event) {
    Remove(event->handle);
  }

private:
  static constexpr int kMaxEvents = 64;

  constexpr int Parent(int n) { return (n - 1) / 2; }
  constexpr int LeftChild(int n) { return n * 2 + 1; }
  constexpr int RightChild(int n) { return n * 2 + 2; }

  void Step(u64 timestamp_next) {
    while (heap[0]->timestamp <= timestamp_next && heap_size > 0) {
      auto event = heap[0];
      timestamp_now = event->timestamp;
      event->callback(0);
      Remove(event->handle);
    }
  }

  void Remove(int n) {
    Swap(n, --heap_size);

    int p = Parent(n);
    if (n != 0 && heap[p]->timestamp > heap[n]->timestamp) {
      do {
        Swap(n, p);
        n = p;
        p = Parent(n);
      } while (n != 0 && heap[p]->timestamp > heap[n]->timestamp);
    } else {
      Heapify(n);
    }
  }

  void Swap(int i, int j) {
    auto tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
    heap[i]->handle = i;
    heap[j]->handle = j;
  }

  void Heapify(int n) {
    int l = LeftChild(n);
    int r = RightChild(n);

    if (l < heap_size && heap[l]->timestamp < heap[n]->timestamp) {
      Swap(l, n);
      Heapify(l);
    }

    if (r < heap_size && heap[r]->timestamp < heap[n]->timestamp) {
      Swap(r, n);
      Heapify(r);
    }
  }

  Event* heap[kMaxEvents];
  int heap_size;
  u64 timestamp_now;
};

} // namespace nba::core
