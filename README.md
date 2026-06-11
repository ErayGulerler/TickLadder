# TickLadder

A C++20 market-data replay tool for one instrument. Feed it a sequence of price-level updates and it prints the best bid and ask after every change. The project asks a practical question for a pricing desk: **how much processing time can a fixed price range buy, and what happens when the range is wrong?**

The repository includes two books that do the same job. One uses `std::map`. The other stores sizes in a fixed array and uses a two-level bitmap to find the next occupied price. A replay stops on a sequence gap so downstream users can identify the last trustworthy quote.

## Run the demo

You need CMake 3.20 or newer and a C++20 compiler. On Windows, use a recent Visual Studio Developer PowerShell with the C++ workload installed. On Linux or macOS, use a terminal with GCC or Clang. The project has no third-party library dependencies. Run from this directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Windows:

```powershell
.\build\bin\replay.exe examples/session.csv
.\build\bin\book_bench.exe
```

On Linux or macOS:

```sh
./build/bin/replay examples/session.csv
./build/bin/book_bench
```

In the sample, the best bid at tick 12000 shrinks from 75 lots to 25. At update 6 its size falls to zero and the next bid becomes visible at 11998. The ask stays at 12002. You can compare every row with [the expected output](examples/quotes.csv).

Try the same command with [gap.csv](examples/gap.csv). Update 4 would remove the best bid, but update 3 never arrived. The last published quote is therefore sequence 2. The tool exits with the diagnostic `line 4: non-contiguous sequence after 2`.

## Input contract

Each file starts with the header `sequence,side,tick,size` and an empty book. Sequences start at 1 and increase by one per row. Side is `B` for bid or `A` for ask. Prices are integer ticks from 10000 through 14095 inclusive. Size is an unsigned 32-bit count of lots. It replaces the quantity at that level. Zero removes the level and removing an absent level is harmless.

Missing prices and sizes appear as empty output fields. Malformed fields, duplicates, gaps and out-of-band ticks stop replay with a line number and a nonzero exit code. Earlier output remains a valid prefix. The tool publishes what it observes, including a locked or crossed quote if the tape contains one.

The CSV format and sequence policy belong to this project. The price-level replacement rule follows a real exchange convention. [Coinbase Exchange Level2 channel](https://docs.cdp.coinbase.com/exchange/websocket-feed/channels#level2-channel)

## Where to look

| File | What to look for |
| --- | --- |
| [book.hpp](include/tickladder/book.hpp) | The complete fixed ladder, including deletion of the best price |
| [map_book.hpp](support/map_book.hpp) | The ordered-map baseline used to cross-check quotes |
| [replay.cpp](src/replay.cpp) | CSV validation and sequence checks before publication |
| [benchmark.cpp](bench/benchmark.cpp) | Warm-up, two workloads and separate throughput and latency passes |
| [book_tests.cpp](tests/book_tests.cpp) | Full-band boundary tests, random updates and failure cases |
| [Design notes](docs/design.md) | A worked bitmap example and the cost of the fixed band |
| [Benchmark notebook](docs/benchmark.md) | Workloads, timing boundaries, verification record and Linux workflow |
