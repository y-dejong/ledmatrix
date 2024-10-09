#ifndef _LWIPOPTS_H_
#define _LWIPOPTS_H_

#include "lwipopts_examples_common.h"

// From pico-examples tcpserver
#if !NO_SYS
#define TCPIP_THREAD_STACKSIZE 1024
#define DEFAULT_THREAD_STACKSIZE 2048
#define DEFAULT_RAW_RECVMBOX_SIZE 8
#define DEFAULT_TCP_RECVMBOX_SIZE 8
#define DEFAULT_UDP_RECVMBOX_SIZE 8
#define TCPIP_MBOX_SIZE 8
#define LWIP_TIMEVAL_PRIVATE 0

// not necessary, can be done either way
#define LWIP_TCPIP_CORE_LOCKING_INPUT 1
#endif

// Customized by me
#define LWIP_TCPIP_CORE_LOCKING   1
#define DEFAULT_ACCEPTMBOX_SIZE 4
#define LWIP_NETCONN 1
#define MEMP_NUM_NETBUF 4
#endif
