# Benchmark notebook

## The experiment

Does a bounded array of price levels help enough to justify reserving 33 KB for one instrument? The baseline uses a `std::map` on each side. Both implementations validate the same prices, apply the same updates and return the best bid and ask after every update.

| Tape | Candidate prices per side | What it exercises |
| --- | ---: | --- |
| Near | 32 | Frequent changes close to the spread, with a small tree |
| Wide | 2048 | Prices spread across the full band, with a larger tree |

The generator uses MT19937 with seed 3367. Each update picks a side and a price within that side's band. Roughly one in four requests a deletion. Other updates replace the size with 1 through 500 lots. A deletion can hit a price that's already empty.

## Run it

Use the Release build in the [README](../README.md). On Linux or macOS, save both outputs with:

```sh
./build/bin/book_bench > trials.csv 2> clock.txt
```

On Windows, run `.\build\bin\book_bench.exe` in PowerShell to see the results. If you put this repository on GitHub, its **Benchmark the order book** workflow can be started manually from Actions. It runs the tests on Linux and saves the raw CSV, clock readings, compiler, CPU, OS and commit as a downloadable artifact. A hosted runner is a shared machine, so compare books within the same run.

## Read the numbers

Each trial starts two copies of one book from the same 8,192 warm-up updates. Both receive the next 250,000 updates. The first pass times a whole loop for throughput. The second times each individual step and reports nearest-rank p50, p99 and maximum service time in nanoseconds. A step applies the new size, reads the quote and folds both prices and sizes into a digest. Samples are stored in reserved memory and sorted after timing.

The benchmark runs five trials on each tape and reverses book order on alternate trials. Each tape should have ten rows with the same digest. The executable returns an error if books or timing passes disagree. The tests separately compare individual quotes, which catches errors that an aggregate digest might miss.

Compare median throughput across the five trials for each book and tape. For tail latency, inspect all five p99 values alongside the clock probe in `clock.txt`. Clock reads contribute to short samples. A zero can mean two reads landed on the same clock tick. A single large maximum can reflect a scheduler interruption. Record CPU, OS, compiler, Release flags, power settings and competing load before quoting a result.

## Verification record

The optimized C++20 book and replay tests passed on Windows x64 with Clang 18.1.6 via Zig 0.13.0 (`-O3 -DNDEBUG`). The tests checked every tick on both sides and matched both fields of each bid and ask across 300,000 random updates. The CLI produced the expected sample CSV and stopped on the deliberate sequence gap.

All three CMake targets built from a clean source tree. The book tests passed under CTest. Windows application control blocked the CMake-built replay executable and the standalone benchmark. The sample comparison passed separately using the directly compiled replay executable. There is no local timing result to report. The Linux workflow is provided for collecting the first measurements and has not been dispatched.

## Measurement boundary

This is a single-threaded service-time test using synthetic updates. Disk I/O, parsing, network transit and queueing are outside the timed loop. A useful follow-up would replay a captured tape at its original arrival times and measure how long updates wait behind earlier work during bursts.
