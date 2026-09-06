#include <cstddef>
#include <cstdint>

extern "C" {

int _close(int) { return -1; }

int _lseek(int, int, int) { return -1; }

int _read(int, char *, int) { return -1; }

int _write(int, const char *, int) { return -1; }

int _getpid() { return 1; }

int _kill(int, int) { return -1; }

void *_sbrk(std::ptrdiff_t) {
  return reinterpret_cast<void *>(static_cast<std::intptr_t>(-1));
}

[[noreturn]] void _exit(int) {
  while (true) {
    asm volatile("wfi");
  }
}

} // extern "C"