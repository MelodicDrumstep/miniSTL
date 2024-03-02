#pragma once

#include <optional>
#include <shared_mutex>
#include <utility>
#include <string_view>
#include <mutex>   
#include <memory>

#include "copy-on-write_trie.hpp"

// This class is used to guard the value returned by the trie. It holds a reference to the root so
// that the reference to the value will not be invalidated.

/*
The ValueGuard class is designed to protect the value returned by the trie by holding onto the root of the trie. 
This is important because in some data structures, like tries, 
the validity of a pointer or reference to a value can depend on the lifetime of the entire data structure. 
If the trie is destroyed or modified, a raw pointer or reference to a value in the trie could become invalid.

By returning a ValueGuard object that holds onto the root of the trie, 
you ensure that the trie stays alive at least as long as the ValueGuard object. 
This guarantees that the reference to the value remains valid for the lifetime of the ValueGuard.

*/

template <class T>
class ValueGuard {
 public:
  ValueGuard(Trie root, const T &value) : root_(std::move(root)), value_(value) {}
  auto operator*() const -> const T & { return value_; }

 private:
  Trie root_;
  const T &value_;
};

// This class is a thread-safe wrapper around the Trie class. It provides a simple interface for
// accessing the trie. It should allow concurrent reads and a single write operation at the same
// time.
class TrieStore {
 public:
  // This function returns a ValueGuard object that holds a reference to the value in the trie. If
  // the key does not exist in the trie, it will return std::nullopt.
  template <class T>
  auto Get(std::string_view key) -> std::optional<ValueGuard<T>>
  {
    // Pseudo-code:
    // (1) Take the root lock, get the root, and release the root lock. Don't lookup the value in the
    //     trie while holding the root lock.
    // (2) Lookup the value in the trie.
    // (3) If the value is found, return a ValueGuard object that holds a reference to the value and the
    //     root. Otherwise, return std::nullopt.
    //throw NotImplementedException("TrieStore::Get is not implemented.");
    root_lock_.lock();
    Trie & cur = root_;
    root_lock_.unlock();
    const T * res = cur.Get<T>(key);
    if (res == nullptr) 
    {
        return std::nullopt;
    }
    return ValueGuard<T>(cur, *(res));
  }
  
  // This function will insert the key-value pair into the trie. If the key already exists in the
  // trie, it will overwrite the value.
  template <class T>
  void Put(std::string_view key, T value)
  {
    write_lock_.lock();
    root_lock_.lock();
    root_ = root_.Put<T>(key, std::move(value));
    root_lock_.unlock();
    write_lock_.unlock();
  }

  // This function will remove the key-value pair from the trie.
  void Remove(std::string_view key)
  {
    // You will need to ensure there is only one writer at a time. Think of how you can achieve this.
    // The logic should be somehow similar to `TrieStore::Get`.
    //throw NotImplementedException("TrieStore::Remove is not implemented.");
    write_lock_.lock();
    root_lock_.lock();
    root_ = root_.Remove(key);
    root_lock_.unlock();
    write_lock_.unlock();
  }

 private:
  // This mutex protects the root. Every time you want to access the trie root or modify it, you
  // will need to take this lock.
  std::mutex root_lock_;

  // This mutex sequences all writes operations and allows only one write operation at a time.
  std::mutex write_lock_;

  // Stores the current root for the trie.
  Trie root_;
};