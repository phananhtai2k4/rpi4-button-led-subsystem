#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "libled.h"
#include "libbutton.h"

typedef enum {
    MODE_OFF,
    MODE_ON,
    MODE_BLINKING
} app_mode_t;

static app_mode_t current_mode = MODE_OFF;
static pthread_t blink_thread;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int is_running = 1;

/* Luồng xử lý việc nhấp nháy đèn */
void* blink_worker(void* arg)
{
    while (1) {
        pthread_mutex_lock(&lock);
        if (current_mode != MODE_BLINKING) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);

        led_toggle();
        usleep(200000);
    }
    return NULL;
}

void on_button_event(button_event_t event)
{
    pthread_mutex_lock(&lock);

    if (event == EVENT_BUTTON_SHORT_PRESS) {
        printf("\n[EVENT] Bấm nhả (Short Press) -> ");

        if (current_mode == MODE_BLINKING) {
            printf("Dừng nhấp nháy, TẮT ĐÈN.\n");
            current_mode = MODE_OFF;
            led_set(LED_OFF);
        } else if (current_mode == MODE_OFF) {
            printf("BẬT ĐÈN.\n");
            current_mode = MODE_ON;
            led_set(LED_ON);
        } else {
            printf("TẮT ĐÈN.\n");
            current_mode = MODE_OFF;
            led_set(LED_OFF);
        }
    } 
    else if (event == EVENT_BUTTON_LONG_PRESS) {
        printf("\n[EVENT] Bấm giữ >= 1s (Long Press) -> KÍCH HOẠT CHẾ ĐỘ NHẤP NHÁY (BLINK)!\n");

        if (current_mode != MODE_BLINKING) {
            current_mode = MODE_BLINKING;
            pthread_create(&blink_thread, NULL, blink_worker, NULL);
            pthread_detach(blink_thread);
        }
    }

    pthread_mutex_unlock(&lock);
}

void sigint_handler(int sig)
{
    printf("\nĐang tắt ứng dụng và dọn dẹp tài nguyên...\n");
    is_running = 0;
    button_close();
    led_close();
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);

    printf("====================================================\n");
    printf(" - Bấm nhả nhanh: Bật / Tắt đèn                      \n");
    printf(" - Bấm giữ >= 1s: Chế độ nhấp nháy                  \n");
    printf(" - Nhấn Ctrl + C để thoát                           \n");
    printf("====================================================\n");

    if (led_init() < 0) {
        fprintf(stderr, "Lỗi khởi tạo LED!\n");
        return -1;
    }

    if (button_init() < 0) {
        fprintf(stderr, "Lỗi khởi tạo Nút bấm!\n");
        led_close();
        return -1;
    }

    printf("[Hệ thống] Đang lắng nghe sự kiện từ nút bấm...\n");

    button_listen(on_button_event);

    button_close();
    led_close();
    return 0;
}