#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "libbutton.h"

#define BUTTON_DEV_PATH "/dev/my_button"
#define LONG_PRESS_THRESHOLD_MS 1000

static int btn_fd = -1;
static int is_running = 0;

static long long get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int button_init(void)
{
    btn_fd = open(BUTTON_DEV_PATH, O_RDONLY);
    if (btn_fd < 0) {
        perror("libbutton: Failed to open button device");
        return -1;
    }
    is_running = 1;
    return 0;
}

void button_listen(button_callback_t callback)
{
    char event_char;
    long long press_start_ms = 0;

    if (btn_fd < 0 || !callback) return;

    while (is_running) {
        if (read(btn_fd, &event_char, 1) > 0) {
            if (event_char == 'P') {
                press_start_ms = get_time_ms();
            } else if (event_char == 'R') {
                long long duration = get_time_ms() - press_start_ms;

                if (duration < 50) continue;

                if (duration >= LONG_PRESS_THRESHOLD_MS) {
                    callback(EVENT_BUTTON_LONG_PRESS);
                } else {
                    callback(EVENT_BUTTON_SHORT_PRESS);
                }
            }
        }
    }
}

void button_close(void)
{
    is_running = 0;
    if (btn_fd >= 0) {
        close(btn_fd);
        btn_fd = -1;
    }
}