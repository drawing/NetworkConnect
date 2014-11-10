#ifndef CONFIG_H
#define CONFIG_H

// 被所有文件包含，工程的配置信息
// 如通用常量信息，配置宏

#include <assert.h>

#define WORK_THREAD_NUMS 10

// 登陆重试时间 s
#define LOGIN_TIMEOUT 3
// 登陆重试次数
#define LOGIN_MAX_TIMEOUT 5

// 用户名和密码最大字符数值
#define MAX_USERNAME_LEN 255

// Session Destroyed 多久后释放内存 (s)
#define FREE_AFTER_DESTROYED	10

// 指令队列多久扫描一次(s)
#define INST_SCAN_INTERVAL	1

// 最大登陆失败重试次数
#define MAX_LOGIN_RETRY 5

// 最大保护应用策略
#define MAX_PROTECT_COUNT 100

// 最大网关策略
#define MAX_GATEWAY_COUNT 10

// 心跳包间隔
#define HEARTBEAT_INTERVAL		5

// 最大 EPOLL 客户端连接
#define MAX_EPOLL_SIZE	10000

// 最大监听连接
#define MAX_LISTEN_SIZE 100


#endif // CONFIG_H
