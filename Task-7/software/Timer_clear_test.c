// Timer_clear_test.c — STATUS Write-1-to-Clear (W1C)
// Demonstrates: timeout flag set, W1C clear, and flag staying cleared
// after the timer is disabled.
#include "io.h"

// ====================================================
// Timer Register Map — Base address 0x00400040
// ====================================================
#define IO_TIMER_CTRL    64
#define IO_TIMER_LOAD    68
#define IO_TIMER_VALUE   72
#define IO_TIMER_STATUS  76

#define TIMER_LOAD_VALUE 10   // 0x0A

// CTRL bit definitions
#define CTRL_EN        (1 << 0)
#define CTRL_MODE_ONE  (0 << 1)   // one-shot mode

int main() {
    printf("=== Timer STATUS Clear (W1C) Test ===\n");

    // Load countdown value
    IO_OUT(IO_TIMER_LOAD, TIMER_LOAD_VALUE);
    printf("LOAD written: 0x%x -> readback: 0x%x\n",
            TIMER_LOAD_VALUE, IO_IN(IO_TIMER_LOAD));

    // Enable timer, one-shot
    IO_OUT(IO_TIMER_CTRL, CTRL_EN | CTRL_MODE_ONE);
    printf("CTRL written -> readback: 0x%x\n", IO_IN(IO_TIMER_CTRL));

    // Wait for timeout
    while (!(IO_IN(IO_TIMER_STATUS) & 0x1)) {
        // poll
    }
    printf("TIMEOUT OCCURRED -> STATUS = 0x%x\n", IO_IN(IO_TIMER_STATUS));

    // Clear the timeout flag (write 1 to clear)
    IO_OUT(IO_TIMER_STATUS, 0x1);
    printf("STATUS after W1C clear = 0x%x (expect 0)\n", IO_IN(IO_TIMER_STATUS));

    // Verify flag stays cleared (timer disabled, no new timeout)
    IO_OUT(IO_TIMER_CTRL, 0x0);   // disable timer
    printf("STATUS after disable = 0x%x (expect 0)\n", IO_IN(IO_TIMER_STATUS));

    printf("=== Test Done ===\n");

    return 0;
}
