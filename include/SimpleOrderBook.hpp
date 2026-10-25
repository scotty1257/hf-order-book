#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <string>
// -- Ask ------------------------------
// Price   Volume  Queue
//   105      100  [#1 : 40], [#2, 60]
//   104       40
//   103      200
//
//
// -- Bid ------------------------------
// Price   Volume  Queue
//   100       80
//    99       70
//    98      110

class SimpleOrder {
public:
  std::uint32_t id;
  std::uint32_t volume;
  std::uint64_t price;
};

enum class SimpleSide { Buy, Sell };

class SimpleOrderIterator {
  SimpleSide side;
  std::uint32_t volume;
  std::list<SimpleOrder>::iterator position;
};

class SimpleOrderBook {
public:
  void AddOrder(std::uint32_t id, SimpleSide side, std::uint32_t volume, std::uint64_t price);
  void FillOrder(std::uint32_t restingOrderId,
      std::uint32_t incomingId,
      std::uint64_t price,
      std::uint32_t volume);

private:
  // Want buyers with highest price at the beginning
  std::map<std::uint64_t, std::list<SimpleOrder>, std::greater<>> mBids;
  std::map<std::uint64_t, std::list<SimpleOrder>> mAsks;

  // id, OrderIterator
  std::map<std::uint32_t, SimpleOrderIterator> mIterator;
};
