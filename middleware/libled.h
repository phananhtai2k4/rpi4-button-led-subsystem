#ifndef LIBLED_H
#define LIBLED_H

#include <stdbool.h>

#define LED_OFF 0
#define LED_ON  1

int  led_init(void);
void led_set(int state);
void led_toggle(void);
void led_close(void);

#endif /* LIBLED_H */