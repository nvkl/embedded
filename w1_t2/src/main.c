/*
 * Tavoite: 3p
 * Tällä palautuksella tehtynä 1p
 * Loput pisteet teen myöhemmin
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec red =
        GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct gpio_dt_spec green =
        GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static const struct gpio_dt_spec blue =
        GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

int led_state = 0;

#define STACKSIZE 500
#define PRIORITY 5

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);

K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);

int init_led();

int main(void) {
    int ret = init_led();

    if (ret < 0) {
        printk("LED init. failed\n");
        return 0;
    }

    printk("Program started\n");

    return 0;
}

int init_led(void) {
    int ret;

    ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("ERR: Red LED configure failed\n");
        return ret;
    }

    gpio_pin_set_dt(&red, 0);

    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("ERR: Green LED configure failed\n");
        return ret;
    }

    gpio_pin_set_dt(&green, 0);

    ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("ERR: Blue LED configure failed\n");
        return ret;
    }

    gpio_pin_set_dt(&blue, 0);

    printk("LED init. ok\n");

    return 0;
}

void red_led_task(void *, void *, void *) {
    printk("Red LED thread start\n");

    while (true) {
        if (led_state == 0) {
            gpio_pin_set_dt(&red, 1);
            printk("RED ON\n");

            k_sleep(K_SECONDS(1));

            gpio_pin_set_dt(&red, 0);
            printk("RED OFF\n");

            led_state = 1;
        }
        k_msleep(1);
    }
}

void yellow_led_task(void *, void *, void *) {
    printk("Yellow LED thread start\n");

    while (true) {
        if (led_state == 1) {
            gpio_pin_set_dt(&red, 1);
            gpio_pin_set_dt(&green, 1);

            printk("YELLOW ON\n");

            k_sleep(K_SECONDS(1));

            gpio_pin_set_dt(&red, 0);
            gpio_pin_set_dt(&green, 0);

            printk("YELLOW OFF\n");

            led_state = 2;
        }
        k_msleep(1);
    }
}

void green_led_task(void *, void *, void *) {
    printk("Green LED thread start\n");

    while (true) {
        if (led_state == 2) {
            gpio_pin_set_dt(&green, 1);
            printk("GREEN ON\n");

            k_sleep(K_SECONDS(1));

            gpio_pin_set_dt(&green, 0);
            printk("GREEN OFF\n");

            led_state = 0;
        }
        k_msleep(1);
    }
}
