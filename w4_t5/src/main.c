/*
 * Tavoitteena täydet pisteet, tässä tehtynä ensimmäinen
 * Käytössä pelkästään punainen LED
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

#define STACKSIZE 500
#define PRIORITY 5
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define TIME_LEN_ERROR -1
#define TIME_ARRAY_ERROR -2
#define TIME_VALUE_ERROR -3

K_SEM_DEFINE(red_sem, 0, 1);

void timer_expiry(struct k_timer *timer);

K_TIMER_DEFINE(red_timer, timer_expiry, NULL);

int init_led(void);
int init_uart(void);
int time_parse(char *time);
void red_led_task(void *, void *, void *);
void uart_task(void *, void *, void *);

K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_thread, STACKSIZE, uart_task, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void) {
    int ret = init_led();

    if (ret < 0) {
        printk("LED init. failed\n");
        return 0;
    }

    ret = init_uart();

    if (ret != 0) {
        printk("UART init. failed\n");
        return 0;
    }

    printk("Program started\n");
    printk("Enter HHMMSS:\n");

    return 0;
}

int init_led(void) {
    int ret;

    ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);

    if (ret < 0) {
        return ret;
    }

    gpio_pin_set_dt(&red, 0);

    return 0;
}

int init_uart(void) {
    if (!device_is_ready(uart_dev)) {
        return 1;
    }

    return 0;
}

int time_parse(char *time) {
    int seconds = TIME_LEN_ERROR;
    int values[3];

    if (time == NULL) {
        return TIME_ARRAY_ERROR;
    }

    if (strlen(time) != 6) {
        return TIME_LEN_ERROR;
    }

    for (int i = 0; i < 6; i++) {
        if (!isdigit((unsigned char)time[i])) {
            return TIME_VALUE_ERROR;
        }
    }

    values[2] = atoi(time + 4);
    time[4] = 0;

    values[1] = atoi(time + 2);
    time[2] = 0;

    values[0] = atoi(time);

    if (values[0] > 23 || values[1] > 59 || values[2] > 59) {
        return TIME_VALUE_ERROR;
    }

    seconds = values[1] * 60 + values[2];

    return seconds;
}

void timer_expiry(struct k_timer *timer) {
    ARG_UNUSED(timer);

    printk("Timer expired\n");

    k_sem_give(&red_sem);
}

void red_led_task(void *a, void *b, void *c) {
    while (true) {
        k_sem_take(&red_sem, K_FOREVER);

        gpio_pin_set_dt(&red, 1);
        printk("RED ON\n");

        k_sleep(K_SECONDS(1));

        gpio_pin_set_dt(&red, 0);
        printk("RED OFF\n");
    }
}

void uart_task(void *a, void *b, void *c) {
    char rc;
    char buffer[7];
    int cnt = 0;

    while (true) {
        if (uart_poll_in(uart_dev, &rc) == 0) {
            if (rc == '\n' || rc == '\r') {

                if (cnt > 0) {
                    buffer[cnt] = '\0';

                    int seconds = time_parse(buffer);

                    if (seconds >= 0) {
                        printk("Timer: %d seconds\n", seconds);

                        k_timer_start(&red_timer, K_SECONDS(seconds), K_NO_WAIT);
                    } else {
                        printk("Invalid time: %d\n", seconds);
                    }

                    cnt = 0;
                    memset(buffer, 0, sizeof(buffer));
                }

            } else if (cnt < 6) {
                buffer[cnt] = rc;
                cnt++;
            }
        }

        k_msleep(10);
    }
}
