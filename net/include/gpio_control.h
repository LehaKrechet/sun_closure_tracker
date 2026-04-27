#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

void gpio_list(const char *dev_name);
void gpio_write(const char *dev_name, int offset, uint8_t value);
void gpio_read(const char *dev_name, int offset);
void gpio_poll(const char *dev_name, int offset);
void init(void);
void start(void);
void stop(void);

#ifdef __cplusplus
}
#endif

#endif