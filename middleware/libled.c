#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "libled.h"

#define LED_DEV_PATH "/dev/my_led"

static int led_fd = -1;
static int current_state = LED_OFF;

int led_init(void)
{
    led_fd = open(LED_DEV_PATH, O_WRONLY);
    if (led_fd < 0) {
        perror("libled: Failed to open LED device");
        return -1;
    }
    led_set(LED_OFF);
    return 0;
}

void led_set(int state)
{
    if (led_fd < 0) return;

    char val = (state == LED_ON) ? '1' : '0';
    if (write(led_fd, &val, 1) > 0) {
        current_state = state;
    }
}

void led_toggle(void)
{
    led_set(current_state == LED_ON ? LED_OFF : LED_ON);
}

void led_close(void)
{
    if (led_fd >= 0) {
        led_set(LED_OFF);
        close(led_fd);
        led_fd = -1;
    }
}