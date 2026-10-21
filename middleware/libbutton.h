#ifndef LIBBUTTON_H
#define LIBBUTTON_H

typedef enum {
    EVENT_BUTTON_SHORT_PRESS,
    EVENT_BUTTON_LONG_PRESS
} button_event_t;

typedef void (*button_callback_t)(button_event_t event);

int  button_init(void);
void button_listen(button_callback_t callback);
void button_close(void);

#endif /* LIBBUTTON_H */