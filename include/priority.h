#ifndef PRIORITY_H
#define PRIORITY_H


#define COMCORE 0 // UART + router
#define SERVOCORE 1 // Pour les servos


#define PRIO_SAFE   10
#define PRIO_CTRL    2
#define PRIO_EST     8
#define PRIO_UART_RX 5
#define PRIO_DEC     4
#define PRIO_ROUTER  2
#define PRIO_UART_TX 3
#define PRIO_TELEM   2
#define PRIO_BG      1

#endif