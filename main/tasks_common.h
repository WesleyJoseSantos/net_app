/**
 * @file tasks_common.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-09-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __TASKS_COMMON__H__
#define __TASKS_COMMON__H__

#include "freertos/task.h"

#define NET_APP_TASK_STACK_SIZE 4096
#define NET_APP_TASK_PRIORITY (tskIDLE_PRIORITY + 4)
#define NET_APP_TASK_CORE_ID 0

#define HTTP_SERVER_TASK_STACK_SIZE 8192
#define HTTP_SERVER_TASK_PRIORITY (tskIDLE_PRIORITY + 3)
#define HTTP_SERVER_TASK_CORE_ID 0

#define MQTT_CLIENT_TASK_STACK_SIZE 8192
#define MQTT_CLIENT_TASK_PRIORITY (tskIDLE_PRIORITY + 5)

#endif  //!__TASKS_COMMON__H__