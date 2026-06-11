# Why use a price ladder?

A pricing screen needs the highest bid and the lowest ask after each change. The interesting case is a cancellation. If the best bid disappears, the next bid might be several hundred empty ticks away. Checking those ticks one at a time makes the work depend on the gap.

## Walk through a cancellation

Imagine bids at offsets 63 and 128, measured from tick 10000. Each group holds 64 offsets. Bit 63 in group 0 represents the first bid. Bit 0 in group 2 represents the second. The summary word has bits 0 and 2 set.

The highest set summary bit points to group 2. The highest set bit inside that group points to offset 128. Now cancel offset 128. Group 2 becomes empty, so its summary bit clears. The same two lookups now find offset 63, even though the entire middle group is empty. For asks, the lookup uses the lowest set bit at each step.

A separate array holds the size at each offset. Changing an existing size changes one slot and keeps its occupancy bit set. The flags and sizes belong to one thread, which also reads the quote. There are no locks or heap allocations inside the ladder's update and quote functions.

## What the fixed band costs

The ladder reserves 4096 sizes per side, plus 64 occupancy words and one summary word per side. The stored fields use 33,808 bytes per instrument. The benchmark prints the compiler's actual object size. A map only allocates nodes for active levels and can naturally accommodate a wider range. For a fair experiment the reference book enforces the same price band as the ladder.

A slow-moving instrument with known bounds is a good fit. A large price jump outside that band stops replay. For a live feed, the consumer would need to invalidate the quote, obtain a fresh snapshot and choose a new band before publishing again. The offline tool starts each file from an empty book.

## How correctness is checked

The unit tests fill every tick and drain each side best-first. This forces every stored price to appear at the top, including ticks on both sides of each 64-bit boundary. A separate test compares bid price, bid size, ask price and ask size with the map after each of 300,000 seeded updates. The CLI fixture covers a cancellation, replacement, spread change and an empty side.

The next experiment would replay a captured, normalized instrument tape. Its price range, time spent at each level and bursts of cancellations would show whether the synthetic workload resembles the instrument of interest. Wire decoding, network arrival times, snapshot recovery and queueing would each need separate measurements for a live system.
