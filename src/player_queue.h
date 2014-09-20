// ----------------------------------------------------------------------------
//
//     Filename   : player_queue.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __player_queue_h__
#define __player_queue_h__

// ----------------------------------------------------------------------------
#include <string>
#include <algorithm>

// ----------------------------------------------------------------------------
template<typename T>
struct player_queue_element
{
  T        value;
  unsigned priority;
public:
  bool operator< (const player_queue_element<T>& rhs) const
  {
    return priority > rhs.priority;
  }
};

// ----------------------------------------------------------------------------
template<typename T>
class player_queue
{
  using element = player_queue_element<T>;
public:
  player_queue()
  {
  }
public:
  bool empty()
  {
    return q_.empty();
  }
public:
  std::size_t size()
  {
    return q_.size();
  }
public:
  size_t push(T value, unsigned priority=1)
  {
    q_.emplace(begin(q_), element{value, priority});
    std::stable_sort(q_.begin(), q_.end());

    auto pos = std::count_if(q_.begin(), q_.end(), [&](const element& elm) { return priority >= elm.priority; });

    return pos;
  }
public:
  T pop()
  {
    auto v = q_.back().value;

    q_.pop_back();

    return std::move(v);
  }
public:
  const T& front()
  {
    return q_.back().value;
  }
public:
  void erase_priority(unsigned priority)
  {
    auto predicate = [&](const element& elm) {
      return priority == elm.priority;
    };
    q_.erase(std::remove_if(q_.begin(), q_.end(), predicate), q_.end());
  }
private:
  std::vector<element> q_;
};

// ----------------------------------------------------------------------------
#endif // __player_queue_h__
