/*
 * gpio.c
 *
 * keys and encoders
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#include "events.h"

#define NUM_PINS 9

int epfd;
int fd[NUM_PINS];
char buf;
struct epoll_event ev[NUM_PINS];
struct epoll_event events;
pthread_t p;


void *gpio_check(void *);

// extern def

void gpio_init() {
  epfd = epoll_create(1);
  fd[0] = open("/sys/class/gpio/gpio31/value", O_RDWR | O_NONBLOCK); // K1
  fd[1] = open("/sys/class/gpio/gpio35/value", O_RDWR | O_NONBLOCK); // K2
  fd[2] = open("/sys/class/gpio/gpio39/value", O_RDWR | O_NONBLOCK); // K3
  fd[3] = open("/sys/class/gpio/gpio28/value", O_RDWR | O_NONBLOCK); // A1
  fd[4] = open("/sys/class/gpio/gpio29/value", O_RDWR | O_NONBLOCK); // B1
  fd[5] = open("/sys/class/gpio/gpio32/value", O_RDWR | O_NONBLOCK); // A2
  fd[6] = open("/sys/class/gpio/gpio33/value", O_RDWR | O_NONBLOCK); // B2
  fd[7] = open("/sys/class/gpio/gpio36/value", O_RDWR | O_NONBLOCK); // A3
  fd[8] = open("/sys/class/gpio/gpio37/value", O_RDWR | O_NONBLOCK); // B3
  if(fd[0] > 0) {
    printf("GPIO: %s\n", strerror(errno)); fflush(stdout);
    buf = 0;
    int n;
    for(int i=0;i<NUM_PINS;i++) {
      ev[i].events = EPOLLET | EPOLLIN;
      ev[i].data.fd = fd[i];
      n = epoll_ctl(epfd, EPOLL_CTL_ADD, fd[i], &ev[i]);
    }
    if(n) {
      printf("GPIO epoll_ctl returned %d: %s\n", n, strerror(errno)); fflush(stdout); 
    } 
    if(pthread_create(&p, NULL, gpio_check, 0)) { 
      printf("GPIO: Error creating thread\n"); fflush(stdout);
    }
  }
  else {
    printf("GPIO: FAIL. check gpio exports!\n"); fflush(stdout);
  } 
}

void gpio_deinit() {
  pthread_cancel(p);
}

void *gpio_check(void *x) {
  (void)x;
  int n;
  while(1) {
    n = epoll_wait(epfd, &events, 1, -1); // wait for 1 event 
    //printf("GPIO epoll returned %d: %s\n", n, strerror(errno)); fflush(stdout);
    //printf("GPIO event: %d\n", events.data.fd); fflush(stdout);

    if(n > 0) {
      n = lseek(events.data.fd, 0, SEEK_SET);
      //printf("GPIO seek %d bytes: %s\n", n, strerror(errno)); fflush(stdout);
      n = read(events.data.fd, &buf, 1);
      //printf("GPIO read %d bytes: %s\n", n, strerror(errno)); fflush(stdout);
      //printf("GPIO buf = 0x%x\n", buf);

      int i;
      for(i=0;i<NUM_PINS&&fd[i]!=events.data.fd;i++); 

      union event_data *ev = event_data_new(EVENT_GPIO);
      ev->gpio.pin = i;
      ev->gpio.val = buf & 0x1;
      event_post(ev);
    }
  }
}

