// Timer_periodic_test.c — Periodic / Auto-Reload Mode
// Demonstrates: multiple timeout + reload cycles, STATUS W1C clear
// used inside a polling loop.
#include "io.h"

// ====================================================
// Timer Register Map — Base address 0x00400040
// ====================================================
#define IO_TIMER_CTRL    64
#define IO_TIMER_LOAD    68
#define IO_TIMER_VALUE   72
#define IO_TIMER_STATUS  76

#define TIMER_LOAD_VALUE 20   // small value to see multiple reloads quickly

// CTRL bit definitions
#define CTRL_EN          (1 << 0)
#define CTRL_MODE_PERIOD (1 << 1)   // periodic / auto-reload mode

int main() {
    printf("=== Timer Periodic (Auto-Reload) Test ===\n");

    // Load the reload value
    IO_OUT(IO_TIMER_LOAD, TIMER_LOAD_VALUE);
    printf("LOAD written: 0x%x -> readback: 0x%x\n",
            TIMER_LOAD_VALUE, IO_IN(IO_TIMER_LOAD));

    // Enable timer in periodic mode
    IO_OUT(IO_TIMER_CTRL, CTRL_EN | CTRL_MODE_PERIOD);
    printf("CTRL written -> readback: 0x%x\n", IO_IN(IO_TIMER_CTRL));

    // Observe multiple timeout + reload cycles
    int i;
    for (i = 0; i < 3; i++) {
        // wait for timeout
        while (!(IO_IN(IO_TIMER_STATUS) & 0x1)) {
            // poll
        }
        printf("Timeout #%d occurred\n", i + 1);
        printf("VALUE after reload = 0x%x\n", IO_IN(IO_TIMER_VALUE));

        // Clear timeout flag (write-1-to-clear)
        IO_OUT(IO_TIMER_STATUS, 0x1);
        printf("STATUS after clear = 0x%x\n", IO_IN(IO_TIMER_STATUS));
    }

    printf("=== Test Done ===\n");

    return 0;
}
