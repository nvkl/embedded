/*
 * Tässä tehtynä vaan eka piste, loput teen myöhemmin
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/timing/timing.h>
#include <string.h>
#include <stdint.h>

static const struct gpio_dt_spec red =
        GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct gpio_dt_spec green =
        GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static const struct gpio_dt_spec blue =
        GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

#define STACKSIZE 500
#define PRIORITY 5

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

static const struct device *const uart_dev =
        DEVICE_DT_GET(UART_DEVICE_NODE);

K_FIFO_DEFINE(dispatcher_fifo);

struct data_t {
        void *fifo_reserved;
        char color;
};

K_CONDVAR_DEFINE(red_cond);
K_CONDVAR_DEFINE(yellow_cond);
K_CONDVAR_DEFINE(green_cond);

K_CONDVAR_DEFINE(release_cond);

K_MUTEX_DEFINE(cond_mutex);

uint64_t red_time = 0;
uint64_t yellow_time = 0;
uint64_t green_time = 0;

uint64_t sequence_time = 0;

int sequence_step = 0;

int init_led(void);
int init_uart(void);

void red_led_task(void *, void *, void *);
void yellow_led_task(void *, void *, void *);
void green_led_task(void *, void *, void *);
void uart_task(void *, void *, void *);
void dispatcher_task(void *, void *, void *);

K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_thread, STACKSIZE, uart_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(dispatcher_thread, STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void) {
    int ret;

    timing_init();

    ret = init_led();
    if (ret < 0) {
        //printk("LED init. failed\n");
        return 0;
    }

    ret = init_uart();
    if (ret != 0) {
        //printk("UART init. failed\n");
        return 0;
    }

    //printk("Program started\n");

    return 0;
}

int init_led(void) {
    int ret;

    ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        //printk("ERR: Red LED configure failed\n");
        return ret;
    }
    gpio_pin_set_dt(&red, 0);

    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        //printk("ERR: Green LED configure failed\n");
        return ret;
    }
    gpio_pin_set_dt(&green, 0);

    ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        //printk("ERR: Blue LED configure failed\n");
        return ret;
    }
    gpio_pin_set_dt(&blue, 0);

    //printk("LED init. ok\n");

    return 0;
}

int init_uart(void) {
    if (!device_is_ready(uart_dev)) {
        return 1;
    }

    //printk("UART init. ok\n");

    return 0;
}

void red_led_task(void *a, void *b, void *c) {
    //printk("Red LED thread start\n");

    while (true) {
        k_mutex_lock(&cond_mutex, K_FOREVER);
        k_condvar_wait(&red_cond, &cond_mutex, K_FOREVER);
        k_mutex_unlock(&cond_mutex);

        timing_start();

        timing_t red_start_time = timing_counter_get();

        gpio_pin_set_dt(&red, 1);
        //printk("RED ON\n");

        k_sleep(K_SECONDS(1));

        gpio_pin_set_dt(&red, 0);
        //printk("RED OFF\n");

        timing_t red_end_time = timing_counter_get();

        timing_stop();

        red_time = timing_cycles_to_ns(
                timing_cycles_get(&red_start_time, &red_end_time)
        );

        printk("RED task: %llu us\n", red_time / 1000);

        k_mutex_lock(&cond_mutex, K_FOREVER);
        k_condvar_signal(&release_cond);
        k_mutex_unlock(&cond_mutex);
    }
}

void yellow_led_task(void *a, void *b, void *c) {
    //printk("Yellow LED thread start\n");

    while (true) {
        k_mutex_lock(&cond_mutex, K_FOREVER);
        k_condvar_wait(&yellow_cond, &cond_mutex, K_FOREVER);
        k_mutex_unlock(&cond_mutex);

        timing_start();

        timing_t yellow_start_time = timing_counter_get();

        gpio_pin_set_dt(&red, 1);
        gpio_pin_set_dt(&green, 1);

        //printk("YELLOW ON\n");

        k_sleep(K_SECONDS(1));

        gpio_pin_set_dt(&red, 0);
        gpio_pin_set_dt(&green, 0);

        //printk("YELLOW OFF\n");

        timing_t yellow_end_time = timing_counter_get();

        timing_stop();

        yellow_time = timing_cycles_to_ns(
                timing_cycles_get(&yellow_start_time, &yellow_end_time)
        );

        printk("YELLOW task: %llu us\n", yellow_time / 1000);

        k_mutex_lock(&cond_mutex, K_FOREVER);
        k_condvar_signal(&release_cond);
        k_mutex_unlock(&cond_mutex);
    }
}

void green_led_task(void *a, void *b, void *c) {
    //printk("Green LED thread start\n");

    while (true) {
        k_mutex_lock(&cond_mutex, K_FOREVER);
        k_condvar_wait(&green_cond, &cond_mutex, K_FOREVER);
        k_mutex_unlock(&cond_mutex);

        timing_start();

        timing_t green_start_time = timing_counter_get();

        gpio_pin_set_dt(&green, 1);
        //printk("GREEN ON\n");

        k_sleep(K_SECONDS(1));

        gpio_pin_set_dt(&green, 0);
        //printk("GREEN OFF\n");

        timing_t green_end_time = timing_counter_get();

        timing_stop();

        green_time = timing_cycles_to_ns(
                timing_cycles_get(&green_start_time, &green_end_time)
        );

        printk("GREEN task: %llu us\n", green_time / 1000);

        k_mutex_lock(&cond_mutex, K_FOREVER);
        k_condvar_signal(&release_cond);
        k_mutex_unlock(&cond_mutex);
    }
}

void uart_task(void *a, void *b, void *c) {
    char rc;

    //printk("UART thread start\n");

    while (true) {
        if (uart_poll_in(uart_dev, &rc) == 0) {
            if (rc == 'R' || rc == 'Y' || rc == 'G') {
                //printk("Received: %c\n", rc);

                struct data_t *buf = k_malloc(sizeof(struct data_t));

                if (buf == NULL) {
                    //printk("ERROR: k_malloc failed\n");
                    continue;
                }

                buf->color = rc;

                k_fifo_put(&dispatcher_fifo, buf);
            }
        }

        k_msleep(10);
    }
}

void dispatcher_task(void *a, void *b, void *c) {
    //printk("Dispatcher thread start\n");

    while (true) {
        struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);

        char color = rec_item->color;

        //printk("Dispatcher: %c\n", color);

        k_free(rec_item);

        k_mutex_lock(&cond_mutex, K_FOREVER);

        if (color == 'R') {
            //printk("Dispatcher -> RED\n");
            k_condvar_signal(&red_cond);
        } else if (color == 'Y') {
            //printk("Dispatcher -> YELLOW\n");
            k_condvar_signal(&yellow_cond);
        } else if (color == 'G') {
            //printk("Dispatcher -> GREEN\n");
            k_condvar_signal(&green_cond);
        }

        k_mutex_unlock(&cond_mutex);

        k_mutex_lock(&cond_mutex, K_FOREVER);
        k_condvar_wait(&release_cond, &cond_mutex, K_FOREVER);
        k_mutex_unlock(&cond_mutex);

        //printk("Dispatcher: LED released\n");

        if (sequence_step == 0) {
            if (color == 'R') {
                sequence_step = 1;
            }
        } else if (sequence_step == 1) {
            if (color == 'Y') {
                sequence_step = 2;
            }
        } else if (sequence_step == 2) {
            if (color == 'G') {
                sequence_time = red_time + yellow_time + green_time;

                printk("\n");
                printk("SEQUENCE COMPLETE\n");
                printk("RED:    %llu us\n", red_time / 1000);
                printk("YELLOW: %llu us\n", yellow_time / 1000);
                printk("GREEN:  %llu us\n", green_time / 1000);
                printk("TOTAL:  %llu us\n", sequence_time / 1000);
                printk("\n");

                sequence_step = 0;
            }
        }
    }
}
