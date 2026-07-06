// Timer_test.c — One-Shot Mode
// Demonstrates: LOAD write, CTRL enable in one-shot mode, VALUE polling,
// STATUS timeout detection.
#include "io.h"

// ====================================================
// Timer Register Map — Base address 0x00400040
// ====================================================
#define IO_TIMER_CTRL    64  // IO_BASE + 0x40
#define IO_TIMER_LOAD    68  // IO_BASE + 0x44
#define IO_TIMER_VALUE   72  // IO_BASE + 0x48
#define IO_TIMER_STATUS  76  // IO_BASE + 0x4C

#define TIMER_LOAD_VALUE 100000   // 0x000186A0

// CTRL bit definitions
#define CTRL_EN        (1 << 0)
#define CTRL_MODE_ONE  (0 << 1)   // one-shot mode

int main() {
    printf("=== Timer One-Shot Test ===\n");

    // Load the countdown value
    IO_OUT(IO_TIMER_LOAD, TIMER_LOAD_VALUE);
    printf("LOAD written: 0x%x -> readback: 0x%x\n",
            TIMER_LOAD_VALUE, IO_IN(IO_TIMER_LOAD));

    // Enable timer in one-shot mode
    IO_OUT(IO_TIMER_CTRL, CTRL_EN | CTRL_MODE_ONE);
    printf("CTRL written -> readback: 0x%x\n", IO_IN(IO_TIMER_CTRL));

    // Poll VALUE register a few times to observe countdown
    printf("VALUE = 0x%x\n", IO_IN(IO_TIMER_VALUE));
    printf("VALUE = 0x%x\n", IO_IN(IO_TIMER_VALUE));
    printf("VALUE = 0x%x\n", IO_IN(IO_TIMER_VALUE));

    // Poll STATUS until timeout occurs
    while (!(IO_IN(IO_TIMER_STATUS) & 0x1)) {
        // wait for timeout flag
    }

    printf("TIMEOUT OCCURRED!\n");
    printf("STATUS = 0x%x\n", IO_IN(IO_TIMER_STATUS));
    printf("VALUE after timeout = 0x%x\n", IO_IN(IO_TIMER_VALUE));

    printf("=== Test Done ===\n");

    return 0;
}
