#include "gpio_control.h"
#include <linux/gpio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/poll.h>
 
typedef enum
{
    APP_OPT_GPIO_WRITE,
    APP_OPT_UNKNOWN
} app_mode_t;
 
typedef struct
{
    char *dev;
    int offset;
    uint8_t val;
    app_mode_t mode;
} app_opt_t;
 
 
void gpio_write(const char *dev_name, int offset, uint8_t value)
{
    struct gpiohandle_request rq;
    struct gpiohandle_data data;
    int fd, ret;
    printf("Write value %d to GPIO at offset %d (OUTPUT mode) on chip %s\n", value, offset, dev_name);
    fd = open(dev_name, O_RDWR);
    if (fd < 0)
    {
        printf("Unabled to open %s: %s", dev_name, strerror(errno));
        return;
    }
    rq.lineoffsets[0] = offset;
    rq.flags = GPIOHANDLE_REQUEST_OUTPUT;
    rq.lines = 1;
    ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &rq);
    close(fd);
    if (ret == -1)
    {
        printf("Unable to line handle from ioctl : %s", strerror(errno));
        return;
    }
    data.values[0] = value;
    ret = ioctl(rq.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    if (ret == -1)
    {
        printf("Unable to set line value using ioctl : %s", strerror(errno));
    }else{
        usleep(1000);
    }
    close(rq.fd);
}
 


void init() {

}

void start() {
  gpio_write("/dev/gpiochip0", 12, 1);
  gpio_write("/dev/gpiochip0", 13, 0);
  gpio_write("/dev/gpiochip0", 20, 0);
  gpio_write("/dev/gpiochip0", 21, 1);
}



void stop() {
  gpio_write("/dev/gpiochip0", 12, 0);
  gpio_write("/dev/gpiochip0", 13, 0);
  gpio_write("/dev/gpiochip0", 20, 0);
  gpio_write("/dev/gpiochip0", 21, 0);
}


