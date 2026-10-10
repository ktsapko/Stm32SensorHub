# USART2 DMA TX host tests

Run all tests:

```sh
cmake -S tests -B build-tests -G Ninja
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

Run only DMA tests with `ctest --test-dir build-tests -R Usart2Dma --output-on-failure`.
Each test has a five-second timeout to catch accidental unbounded recovery loops.

`usart2_dma_test.cpp` includes the production driver once. This lets the fixture
reset private state and inspect the staging buffer without firmware test hooks.
Do not add `src/drivers/usart2.cpp` as another source of the test target.
Only this target puts `tests/fakes` before the production include directory.
The real GPIO, RCC, NVIC, DMA and USART definitions are still used.

The model implements HIFCR write-one-to-clear, NVIC enable/disable writes,
a stream that refuses to clear EN, flags that refuse to clear, and PRIMASK
save/restore bookkeeping. Test code raises HISR flags directly; this is host
simulation, not a way to inject faults into read-only HISR on hardware.

Coverage includes individual/simultaneous errors, completion priority, known
versus uncertain loss, queue continuation without replay, stop/flag-clear
failures, retry exhaustion, budget renewal after a successful transfer,
unchanged RX IRQ enable, RX processing during TX error, and PRIMASK restoration.

The model does not execute DMA transfers or interrupt preemption and does not
simulate bus timing, NVIC pending/active scheduling, or USART wire delivery.
Tests cannot establish physical DMA fault recovery; that still needs testing
on the board. Firmware implementation files are not modified by this test setup.
