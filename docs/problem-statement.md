The project is a Nasdaq ITCH-driven limit order book, a matching engine, and a market-making backtester, built in C++20 on Linux. Each phase ends with a check you can verify before moving on.

Phase 0: Setup

Use Linux. WSL2 works for most phases, but native Linux gives accurate perf counters and core pinning. Build with CMake, GCC or Clang, `-O3 -march=native`, GoogleTest, and Google Benchmark.

```
order-book/
  include/book/ReferenceOrderBook.hpp
  include/book/FastOrderBook.hpp
  include/memory/ObjectPool.hpp
  include/feed/ItchParser.hpp
  include/concurrency/SingleProducerSingleConsumerQueue.hpp
  tests/
  benchmarks/
  tools/ReplayItchFile.cpp
```

Phase 1: Reference book (correctness oracle)

Start with a deliberately slow book: `std::map<price, std::list<Order>>` plus `std::unordered_map<orderIdentifier, iterator>`. Its rules:

- Price-time priority, with partial fills.
- A reduce keeps the order's place in the queue.
- A replace (new price or larger size) loses its place.

```cpp
enum class Side : std::uint8_t { Buy, Sell };

struct Fill {
    std::uint64_t restingOrderIdentifier;
    std::uint64_t incomingOrderIdentifier;
    std::int64_t priceInTicks;
    std::uint32_t quantity;
};

class ReferenceOrderBook {
public:
    std::vector<Fill> addLimitOrder(std::uint64_t orderIdentifier, Side side,
                                    std::int64_t priceInTicks, std::uint32_t quantity);
    void cancelOrder(std::uint64_t orderIdentifier);
    void reduceOrder(std::uint64_t orderIdentifier, std::uint32_t quantityToRemove);
    void replaceOrder(std::uint64_t oldOrderIdentifier, std::uint64_t newOrderIdentifier,
                      std::int64_t newPriceInTicks, std::uint32_t newQuantity);
    std::optional<std::int64_t> bestBidPrice() const;
    std::optional<std::int64_t> bestAskPrice() const;
};
```

```cpp
TEST(ReferenceOrderBookTest, PartialFillRespectsTimePriority) {
    ReferenceOrderBook orderBook;
    orderBook.addLimitOrder(1, Side::Sell, 10050, 100);
    orderBook.addLimitOrder(2, Side::Sell, 10050, 200);
    std::vector<Fill> fills = orderBook.addLimitOrder(3, Side::Buy, 10050, 150);

    ASSERT_EQ(fills.size(), 2u);
    EXPECT_EQ(fills[0].restingOrderIdentifier, 1u);
    EXPECT_EQ(fills[0].quantity, 100u);
    EXPECT_EQ(fills[1].restingOrderIdentifier, 2u);
    EXPECT_EQ(fills[1].quantity, 50u);
}
```

Done when: there are 20+ tests covering crosses, sweeps through several levels, cancel of an unknown id, and a zero-quantity reduce.

Phase 2: Fast book (no allocation on the hot path)

Use the following structures:

- An intrusive doubly linked list of orders per price level.
- Price levels in a flat array indexed by tick.
- An object pool for orders.
- An open-addressing hash map from order id to order.

```cpp
struct PriceLevel;

struct Order {
    std::uint64_t orderIdentifier;
    std::uint32_t priceIndex;
    std::uint32_t remainingQuantity;
    Side side;
    Order* nextOrderAtLevel;
    Order* previousOrderAtLevel;
};

struct PriceLevel {
    Order* headOrder = nullptr;
    Order* tailOrder = nullptr;
    std::uint64_t totalQuantity = 0;
};
```

```cpp
template <typename ElementType, std::size_t Capacity>
class ObjectPool {
public:
    ObjectPool() {
        for (std::uint32_t slotIndex = 0; slotIndex < Capacity; ++slotIndex)
            freeSlotIndices[slotIndex] = slotIndex;
    }
    ElementType* acquire() {
        return freeSlotCount == 0 ? nullptr : &storage[freeSlotIndices[--freeSlotCount]];
    }
    void release(ElementType* element) {
        freeSlotIndices[freeSlotCount++] = static_cast<std::uint32_t>(element - storage.data());
    }
private:
    std::array<ElementType, Capacity> storage;
    std::array<std::uint32_t, Capacity> freeSlotIndices;
    std::size_t freeSlotCount = Capacity;
};
// Allocate it once at startup: auto orderPool = std::make_unique<ObjectPool<Order, 1 << 22>>();
```

Matching, buy side. When the ask side is empty, `bestAskIndex` holds a sentinel equal to the level count, so the loop exits.

```cpp
template <typename FillHandler>
void FastOrderBook::matchAgainstAsks(Order& incomingOrder, FillHandler&& onFill) {
    while (incomingOrder.remainingQuantity > 0 && bestAskIndex <= incomingOrder.priceIndex) {
        PriceLevel& askLevel = askLevels[bestAskIndex];
        Order* restingOrder = askLevel.headOrder;
        std::uint32_t fillQuantity = std::min(incomingOrder.remainingQuantity,
                                              restingOrder->remainingQuantity);
        incomingOrder.remainingQuantity -= fillQuantity;
        restingOrder->remainingQuantity -= fillQuantity;
        askLevel.totalQuantity -= fillQuantity;
        onFill(restingOrder->orderIdentifier, incomingOrder.orderIdentifier,
               bestAskIndex, fillQuantity);

        if (restingOrder->remainingQuantity == 0) {
            unlinkFromLevel(askLevel, restingOrder);
            orderLookup.erase(restingOrder->orderIdentifier);
            orderPool->release(restingOrder);
            if (askLevel.headOrder == nullptr) advanceBestAskToNextNonEmptyLevel();
        }
    }
}
```

The fast book needs two add paths:

- `addLimitOrder`, which matches, for engine mode.
- `addRestingOrder`, which never matches, for feed mode. In feed mode the exchange has already done the matching.

Differential fuzz test (the most important test in the project):

```cpp
TEST(FastOrderBookTest, MatchesReferenceUnderRandomOperations) {
    std::mt19937_64 randomEngine(42);
    ReferenceOrderBook referenceBook;
    FastOrderBook fastBook;
    std::vector<std::uint64_t> liveOrderIdentifiers;

    for (int operationNumber = 0; operationNumber < 5'000'000; ++operationNumber) {
        // randomly: add buy/sell near mid, cancel a live id, reduce a live id
        // apply the same operation to both books, then:
        ASSERT_EQ(referenceBook.bestBidPrice(), fastBook.bestBidPrice());
        ASSERT_EQ(referenceBook.bestAskPrice(), fastBook.bestAskPrice());
    }
}
```

Done when the fuzz test passes with 100M operations. Also compare the fill sequences from both books, not just the top of book.

Phase 3: ITCH 5.0 feed handler

1. Download a sample day from Nasdaq; the files are gzipped.
2. mmap the file. Each message is preceded by a 2-byte big-endian length.
3. Route each message to a book with `std::array<FastOrderBook*, 65536> booksByStockLocate`.

```cpp
inline std::uint16_t readBigEndian16(const std::uint8_t* bytes) {
    std::uint16_t value; std::memcpy(&value, bytes, sizeof(value)); return __builtin_bswap16(value);
}
inline std::uint32_t readBigEndian32(const std::uint8_t* bytes) {
    std::uint32_t value; std::memcpy(&value, bytes, sizeof(value)); return __builtin_bswap32(value);
}
inline std::uint64_t readBigEndian64(const std::uint8_t* bytes) {
    std::uint64_t value; std::memcpy(&value, bytes, sizeof(value)); return __builtin_bswap64(value);
}
inline std::uint64_t readBigEndian48(const std::uint8_t* bytes) {
    return (std::uint64_t(readBigEndian16(bytes)) << 32) | readBigEndian32(bytes + 2);
}

void ItchHandler::processFile(const std::uint8_t* fileBytes, std::size_t fileSize) {
    std::size_t readOffset = 0;
    while (readOffset + 2 <= fileSize) {
        std::uint16_t messageLength = readBigEndian16(fileBytes + readOffset);
        const std::uint8_t* messageBytes = fileBytes + readOffset + 2;
        switch (messageBytes[0]) {
            case 'A': case 'F': handleAddOrder(messageBytes); break;
            case 'E': handleOrderExecuted(messageBytes); break;
            case 'C': handleOrderExecutedWithPrice(messageBytes); break;
            case 'X': handleOrderCancel(messageBytes); break;
            case 'D': handleOrderDelete(messageBytes); break;
            case 'U': handleOrderReplace(messageBytes); break;
            default: break;
        }
        readOffset += 2 + messageLength;
    }
}

void ItchHandler::handleAddOrder(const std::uint8_t* messageBytes) {
    std::uint16_t stockLocate = readBigEndian16(messageBytes + 1);
    std::uint64_t timestampNanoseconds = readBigEndian48(messageBytes + 5);
    std::uint64_t orderReferenceNumber = readBigEndian64(messageBytes + 11);
    Side side = messageBytes[19] == 'B' ? Side::Buy : Side::Sell;
    std::uint32_t shareQuantity = readBigEndian32(messageBytes + 20);
    std::uint32_t priceInTenThousandths = readBigEndian32(messageBytes + 32);
    booksByStockLocate[stockLocate]->addRestingOrder(
        orderReferenceNumber, side, priceInTenThousandths, shareQuantity);
}
```

Field offsets for the other messages:

- E: reference number at 11, executed shares at 19.
- X: reference number at 11, canceled shares at 19.
- D: reference number at 11.
- U: original reference number at 11, new reference number at 19, shares at 27, price at 31.

Prices range from fractions of a cent to thousands of dollars, so a single tick array won't fit every stock. Implement two designs and benchmark them:

- A per-symbol array windowed around the open, with fallback for out-of-window prices.
- A sorted `std::vector<PriceLevel>` with the best price at the back.

Done when a full day replays with these invariants holding every message:

- Every E/X/D/U references a known order.
- No quantity underflows.
- The bid is always below the ask.

Phase 4: Measure everything

```cpp
#include <x86intrin.h>

std::uint64_t startCycles = __rdtsc();
fastBook.addRestingOrder(orderReferenceNumber, side, priceIndex, shareQuantity);
std::uint64_t elapsedCycles = __rdtsc() - startCycles;
latencyHistogram.record(elapsedCycles);   // report p50, p99, p99.9, max
```

```
perf stat -e cycles,instructions,cache-misses,branch-misses ./ReplayItchFile day.itch
perf record -g ./ReplayItchFile day.itch && perf report
```

Experiments to run, each written up as one row in a README results table:

- `std::map` vs flat array vs sorted vector for price levels.
- `std::unordered_map` vs your open-addressing map.
- Order struct field ordering and size (fit in one 64-byte line).
- `-march=native` on and off.
- Huge pages for the order pool.

Phase 5: Threads

The pipeline is feed thread → SPSC queue → book thread → SPSC queue → strategy thread. Each thread is pinned to a core and busy-spins.

```cpp
template <typename ElementType, std::size_t Capacity>
class SingleProducerSingleConsumerQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
public:
    bool tryPush(const ElementType& element) {
        std::size_t currentWriteIndex = writeIndex.load(std::memory_order_relaxed);
        if (currentWriteIndex - cachedReadIndex == Capacity) {
            cachedReadIndex = readIndex.load(std::memory_order_acquire);
            if (currentWriteIndex - cachedReadIndex == Capacity) return false;
        }
        buffer[currentWriteIndex & (Capacity - 1)] = element;
        writeIndex.store(currentWriteIndex + 1, std::memory_order_release);
        return true;
    }
    bool tryPop(ElementType& element) {
        std::size_t currentReadIndex = readIndex.load(std::memory_order_relaxed);
        if (currentReadIndex == cachedWriteIndex) {
            cachedWriteIndex = writeIndex.load(std::memory_order_acquire);
            if (currentReadIndex == cachedWriteIndex) return false;
        }
        element = buffer[currentReadIndex & (Capacity - 1)];
        readIndex.store(currentReadIndex + 1, std::memory_order_release);
        return true;
    }
private:
    alignas(64) std::atomic<std::size_t> writeIndex{0};
    alignas(64) std::size_t cachedReadIndex = 0;
    alignas(64) std::atomic<std::size_t> readIndex{0};
    alignas(64) std::size_t cachedWriteIndex = 0;
    alignas(64) std::array<ElementType, Capacity> buffer;
};
```

```cpp
void pinCurrentThreadToCore(int coreNumber) {
    cpu_set_t coreSet;
    CPU_ZERO(&coreSet);
    CPU_SET(coreNumber, &coreSet);
    pthread_setaffinity_np(pthread_self(), sizeof(coreSet), &coreSet);
}
```

Done when you can report the hop latency through the queue (p50 and p99), and see it get worse when you remove the `alignas(64)` padding (false sharing).

Phase 6 (optional): Network

1. Replay the file as UDP multicast wrapped in MoldUDP64 packets.
2. Receive it with a separate process.
3. Detect sequence gaps and recover from them.

Phase 7: Quant layer

Signals:

```cpp
double computeMicroprice(double bidPrice, double askPrice,
                         double bidQuantity, double askQuantity) {
    return (bidPrice * askQuantity + askPrice * bidQuantity) / (bidQuantity + askQuantity);
}

double computeOrderBookImbalance(double bidQuantity, double askQuantity) {
    return (bidQuantity - askQuantity) / (bidQuantity + askQuantity);
}
```

Research steps:

1. Log imbalance at every top-of-book change.
2. Bucket it into deciles.
3. Measure the average mid-price change over the next 10, 100, and 1000 events.
4. Do this on day 1 and validate on day 2, to avoid look-ahead bias.

Then build an Avellaneda-Stoikov market maker:

```cpp
double reservationPrice = midPrice
    - inventoryShares * riskAversion * volatilitySquared * timeRemaining;
double optimalTotalSpread = riskAversion * volatilitySquared * timeRemaining
    + (2.0 / riskAversion) * std::log(1.0 + riskAversion / orderArrivalDecay);
double bidQuote = reservationPrice - optimalTotalSpread / 2.0;
double askQuote = reservationPrice + optimalTotalSpread / 2.0;
```

The backtest needs realistic fills:

- When you join a level, your queue-ahead equals that level's quantity at that moment.
- Queue-ahead shrinks on executions at that level. For cancels, assume they come from behind you (the pessimistic case).
- You fill only once queue-ahead reaches zero.
- Add 5–50 µs of latency between your decision and your order arriving.
- Model fees and rebates.

Metrics to report:

- PnL, Sharpe, and max drawdown.
- Inventory over time and fill rate.
- Adverse selection: the mid-price move 1 second after each of your fills.

Resources:

- [Nasdaq TotalView-ITCH 5.0 spec (PDF)](https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf)
- [Nasdaq sample ITCH files](https://emi.nasdaq.com/ITCH/)
- [David Gross, "When Nanoseconds Matter" (Optiver page with video)](https://www.optiver.com/insights/technology-blog/designing-low-latency-cpp-systems/). The talk covers optimizing order books for speed, common C++ low-latency patterns, and designing systems that are both scalable and robust.
- [Carl Cook, "When a Microsecond Is an Eternity" (CppCon 2017)](https://www.youtube.com/watch?v=NH1Tta7purM)
- [Avellaneda & Stoikov, High-frequency trading in a limit order book (PDF)](https://www.math.nyu.edu/~avellane/HighFrequencyTrading.pdf)
- [Google Benchmark](https://github.com/google/benchmark)
- [GoogleTest](https://github.com/google/googletest)
- [rigtorp/SPSCQueue (compare against yours)](https://github.com/rigtorp/SPSCQueue)
- [Brendan Gregg's perf examples](https://www.brendangregg.com/perf.html)

I can scaffold the Phase 0–1 repo (CMake, the reference book, tests, a changelog) if you want to start building now.
