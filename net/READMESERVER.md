#Компиляция
arm-linux-gnueabi-g++ \
  -static \
  -pthread \
  -o arm_server \
  src/server.cpp \
  src/gpio_control.c \
  -I./library -I./include

#Проверка на пк
qemu-arm-static ./arm_server