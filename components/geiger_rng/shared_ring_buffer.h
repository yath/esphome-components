#pragma once

#include "esphome/core/hal.h"

template <typename T, size_t N> class VolatileBitSet {
public:
  bool get(size_t n) { return (set_[idx(n)] & mask(n)) != 0; }
  void set(size_t n) { set_[idx(n)] |= mask(n); }
  void clear(size_t n) { set_[idx(n)] &= ~mask(n); }

  size_t popcount() {
    size_t ret = 0;
    for (int i = 0; i < N / sizeof(T); i++)
      ret += builtin_popcount(set_[i]);
    return ret;
  }

protected:
  volatile T set_[N / sizeof(T)] = {0};
  constexpr size_t idx(size_t n) { return n / sizeof(T); }
  constexpr T mask(size_t n) { return (T)1 << (n % sizeof(T)); }

  int builtin_popcount(int x) { return __builtin_popcount(x); }
  int builtin_popcount(long int x) { return __builtin_popcountl(x); }
  int builtin_popcount(long long int x) { return __builtin_popcountll(x); }
};

template <typename T, size_t N> class SharedRingBuffer {
public:
  void IRAM_ATTR push(T value) {
    buffer_[inpos_] = value;
    set_.set(inpos_);

    if (++inpos_ == N)
      inpos_ = 0;
  }

  bool pop(T *value) {
    if (!set_.get(outpos_))
      return false;

    *value = buffer_[outpos_];
    set_.clear(outpos_);

    if (++outpos_ == N)
      outpos_ = 0;

    return true;
  }

  size_t size() { return set_.popcount(); }

protected:
  volatile T buffer_[N];
  VolatileBitSet<long, N> set_;

  size_t inpos_ = 0, outpos_ = 0;
};
