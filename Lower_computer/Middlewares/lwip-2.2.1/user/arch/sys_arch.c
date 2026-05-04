/*
 * Copyright (c) 2017 Simon Goldschmidt
 * All rights reserved.
 *
 * 本文件是 lwIP 适配 FreeRTOS 的系统层移植接口
 * 作用：把 lwIP 操作系统底层依赖 映射到 FreeRTOS 信号量/队列/任务/互斥锁
 */

/* lwIP 头文件包含 */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"

/* FreeRTOS 底层头文件 */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/**
 * LWIP_FREERTOS_THREAD_STACKSIZE_IS_STACKWORDS
 * 置1：sys_thread_new 传入的栈大小 以「字」为单位（FreeRTOS 原生风格）
 * 置0：以「字节」为单位（lwIP 原生风格）
 */
#ifndef LWIP_FREERTOS_THREAD_STACKSIZE_IS_STACKWORDS
#define LWIP_FREERTOS_THREAD_STACKSIZE_IS_STACKWORDS  0
#endif

/**
 * LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
 * 置1：用递归互斥锁实现 lwIP 临界区保护
 * 置0：用关闭调度器/中断方式实现（默认）
 */
#ifndef LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
#define LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX     0
#endif

/**
 * LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
 * 置1：开启临界区嵌套匹配合法性检查，用于调试
 */
#ifndef LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
#define LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK   0
#endif

/**
 * LWIP_FREERTOS_CHECK_QUEUE_EMPTY_ON_FREE
 * 置1：销毁消息邮箱时检查队列是否为空，防止内存泄漏
 */
#ifndef LWIP_FREERTOS_CHECK_QUEUE_EMPTY_ON_FREE
#define LWIP_FREERTOS_CHECK_QUEUE_EMPTY_ON_FREE       0
#endif

/**
 * LWIP_FREERTOS_CHECK_CORE_LOCKING
 * 置1：开启 lwIP 内核锁合法性检查，必须配合 lwipopts.h 宏定义
 */
#ifndef LWIP_FREERTOS_CHECK_CORE_LOCKING
#define LWIP_FREERTOS_CHECK_CORE_LOCKING              0
#endif

/**
 * LWIP_FREERTOS_SYS_NOW_FROM_FREERTOS
 * 置1：用 FreeRTOS 系统滴答作为 lwIP 系统时间 sys_now()
 * 置0：用户自己实现硬件定时器获取时间
 */
#ifndef LWIP_FREERTOS_SYS_NOW_FROM_FREERTOS
#define LWIP_FREERTOS_SYS_NOW_FROM_FREERTOS           1
#endif

/* 编译条件检查：lwIP FreeRTOS 移植必须开启动态内存分配 */
#if !configSUPPORT_DYNAMIC_ALLOCATION
# error "lwIP FreeRTOS port requires configSUPPORT_DYNAMIC_ALLOCATION"
#endif
/* 必须支持任务延时 */
#if !INCLUDE_vTaskDelay
# error "lwIP FreeRTOS port requires INCLUDE_vTaskDelay"
#endif
/* 必须支持任务挂起 */
#if !INCLUDE_vTaskSuspend
# error "lwIP FreeRTOS port requires INCLUDE_vTaskSuspend"
#endif
/* 使用互斥锁或兼容互斥锁时，必须开启 FreeRTOS 互斥锁功能 */
#if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX || !LWIP_COMPAT_MUTEX
#if !configUSE_MUTEXES
# error "lwIP FreeRTOS port requires configUSE_MUTEXES"
#endif
#endif

/* 轻量级临界区且用互斥锁方式：定义全局递归互斥锁 */
#if SYS_LIGHTWEIGHT_PROT && LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
static SemaphoreHandle_t sys_arch_protect_mutex;
#endif
/* 临界区嵌套计数，用于合法性校验 */
#if SYS_LIGHTWEIGHT_PROT && LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
static sys_prot_t sys_arch_protect_nesting;
#endif

/**
 * @brief  系统层初始化
 * @note   创建临界区所用全局递归互斥锁
 */
void
sys_init(void)
{
#if SYS_LIGHTWEIGHT_PROT && LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
  /* 创建递归互斥锁 */
  sys_arch_protect_mutex = xSemaphoreCreateRecursiveMutex();
  /* 断言：创建失败则死机提示 */
  LWIP_ASSERT("failed to create sys_arch_protect mutex",
    sys_arch_protect_mutex != NULL);
#endif
}

/* 禁止16位系统滴答，防止计时溢出 */
#if configUSE_16_BIT_TICKS == 1
#error This port requires 32 bit ticks or timer overflow will fail
#endif

#if LWIP_FREERTOS_SYS_NOW_FROM_FREERTOS
/**
 * @brief  获取系统当前时间（毫秒）
 * @retval 系统运行毫秒数
 */
u32_t
sys_now(void)
{
  /* FreeRTOS 滴答数 × 每滴答毫秒数 转成毫秒 */
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}
#endif

/**
 * @brief  获取系统滴答节拍数
 * @retval FreeRTOS 原生滴答计数值
 */
u32_t
sys_jiffies(void)
{
  return xTaskGetTickCount();
}

#if SYS_LIGHTWEIGHT_PROT
/**
 * @brief  进入 lwIP 临界区保护
 * @retval 保护嵌套层级标记
 */
sys_prot_t
sys_arch_protect(void)
{
#if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
  BaseType_t ret;
  LWIP_ASSERT("sys_arch_protect_mutex != NULL", sys_arch_protect_mutex != NULL);
  /* 获取递归互斥锁，永久等待 */
  ret = xSemaphoreTakeRecursive(sys_arch_protect_mutex, portMAX_DELAY);
  LWIP_ASSERT("sys_arch_protect failed to take the mutex", ret == pdTRUE);
#else 
  /* 不使用互斥锁：进入临界区（关闭调度器） */
  taskENTER_CRITICAL();
#endif

#if LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
  {
    /* 嵌套计数自增，返回当前层级 */
    sys_prot_t ret = sys_arch_protect_nesting;
    sys_arch_protect_nesting++;
    LWIP_ASSERT("sys_arch_protect overflow", sys_arch_protect_nesting > ret);
    return ret;
  }
#else
  return 1;
#endif
}

/**
 * @brief  退出 lwIP 临界区保护
 * @param  pval 进入时返回的嵌套标记
 */
void
sys_arch_unprotect(sys_prot_t pval)
{
#if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
  BaseType_t ret;
#endif

#if LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
  /* 校验嵌套层级匹配 */
  LWIP_ASSERT("unexpected sys_arch_protect_nesting", sys_arch_protect_nesting > 0);
  sys_arch_protect_nesting--;
  LWIP_ASSERT("unexpected sys_arch_protect_nesting", sys_arch_protect_nesting == pval);
#endif

#if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
  LWIP_ASSERT("sys_arch_protect_mutex != NULL", sys_arch_protect_mutex != NULL);
  /* 释放递归互斥锁 */
  ret = xSemaphoreGiveRecursive(sys_arch_protect_mutex);
  LWIP_ASSERT("sys_arch_unprotect failed to give the mutex", ret == pdTRUE);
#else 
  /* 退出临界区，恢复调度器 */
  taskEXIT_CRITICAL();
#endif
  /* 消除未使用参数警告 */
  LWIP_UNUSED_ARG(pval);
}

#endif /* SYS_LIGHTWEIGHT_PROT */

/**
 * @brief  毫秒级延时
 * @param  delay_ms 延时毫秒数
 */
void
sys_arch_msleep(u32_t delay_ms)
{
  /* 毫秒转 FreeRTOS 滴答数 */
  TickType_t delay_ticks = delay_ms / portTICK_RATE_MS;
  vTaskDelay(delay_ticks);
}

#if !LWIP_COMPAT_MUTEX
/**
 * @brief  创建 lwIP 互斥锁
 * @param  mutex 互斥锁句柄指针
 * @retval ERR_OK 成功 / ERR_MEM 内存不足
 */
err_t
sys_mutex_new(sys_mutex_t *mutex)
{
  LWIP_ASSERT("mutex != NULL", mutex != NULL);
  /* 创建 FreeRTOS 递归互斥锁 */
  mutex->mut = xSemaphoreCreateRecursiveMutex();
  if(mutex->mut == NULL) {
    SYS_STATS_INC(mutex.err);
    return ERR_MEM;
  }
  SYS_STATS_INC_USED(mutex);
  return ERR_OK;
}

/**
 * @brief  获取/加锁互斥锁
 * @param  mutex 互斥锁句柄指针
 */
void
sys_mutex_lock(sys_mutex_t *mutex)
{
  BaseType_t ret;
  LWIP_ASSERT("mutex != NULL", mutex != NULL);
  LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);
  /* 永久等待获取锁 */
  ret = xSemaphoreTakeRecursive(mutex->mut, portMAX_DELAY);
  LWIP_ASSERT("failed to take the mutex", ret == pdTRUE);
}

/**
 * @brief  释放/解锁互斥锁
 * @param  mutex 互斥锁句柄指针
 */
void
sys_mutex_unlock(sys_mutex_t *mutex)
{
  BaseType_t ret;
  LWIP_ASSERT("mutex != NULL", mutex != NULL);
  LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);
  /* 释放递归互斥锁 */
  ret = xSemaphoreGiveRecursive(mutex->mut);
  LWIP_ASSERT("failed to give the mutex", ret == pdTRUE);
}

/**
 * @brief  销毁互斥锁
 * @param  mutex 互斥锁句柄指针
 */
void
sys_mutex_free(sys_mutex_t *mutex)
{
  LWIP_ASSERT("mutex != NULL", mutex != NULL);
  LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);

  SYS_STATS_DEC(mutex.used);
  vSemaphoreDelete(mutex->mut);
  mutex->mut = NULL;
}

#endif /* !LWIP_COMPAT_MUTEX */

/**
 * @brief  创建 lwIP 二值信号量
 * @param  sem 信号量句柄指针
 * @param  initial_count 初始值 0/1
 * @retval ERR_OK 成功 / ERR_MEM 失败
 */
err_t
sys_sem_new(sys_sem_t *sem, u8_t initial_count)
{
  LWIP_ASSERT("sem != NULL", sem != NULL);
  LWIP_ASSERT("initial_count invalid (not 0 or 1)",
    (initial_count == 0) || (initial_count == 1));
  /* 创建二值信号量 */
  sem->sem = xSemaphoreCreateBinary();
  if(sem->sem == NULL) {
    SYS_STATS_INC(sem.err);
    return ERR_MEM;
  }
  SYS_STATS_INC_USED(sem);
  /* 初始值为1则立即释放一次 */
  if(initial_count == 1) {
    BaseType_t ret = xSemaphoreGive(sem->sem);
    LWIP_ASSERT("sys_sem_new: initial give failed", ret == pdTRUE);
  }
  return ERR_OK;
}

/**
 * @brief  发送信号量信号（释放信号量）
 * @param  sem 信号量句柄指针
 */
void
sys_sem_signal(sys_sem_t *sem)
{
  BaseType_t ret;
  LWIP_ASSERT("sem != NULL", sem != NULL);
  LWIP_ASSERT("sem->sem != NULL", sem->sem != NULL);

  ret = xSemaphoreGive(sem->sem);
  /* 队列满属于正常情况，仅断言合法返回值 */
  LWIP_ASSERT("sys_sem_signal: sane return value",
    (ret == pdTRUE) || (ret == errQUEUE_FULL));
}

/**
 * @brief  等待信号量
 * @param  sem 信号量句柄
 * @param  timeout_ms 超时毫秒，0为永久等待
 * @retval 超时返回 SYS_ARCH_TIMEOUT，成功返回1
 */
u32_t
sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout_ms)
{
  BaseType_t ret;
  LWIP_ASSERT("sem != NULL", sem != NULL);
  LWIP_ASSERT("sem->sem != NULL", sem->sem != NULL);

  if(!timeout_ms) {
    /* 永久阻塞等待 */
    ret = xSemaphoreTake(sem->sem, portMAX_DELAY);
    LWIP_ASSERT("taking semaphore failed", ret == pdTRUE);
  } else {
    /* 限时等待 */
    TickType_t timeout_ticks = timeout_ms / portTICK_RATE_MS;
    ret = xSemaphoreTake(sem->sem, timeout_ticks);
    if (ret == errQUEUE_EMPTY) {
      return SYS_ARCH_TIMEOUT;
    }
    LWIP_ASSERT("taking semaphore failed", ret == pdTRUE);
  }

  return 1;
}

/**
 * @brief  销毁信号量
 * @param  sem 信号量句柄
 */
void
sys_sem_free(sys_sem_t *sem)
{
  LWIP_ASSERT("sem != NULL", sem != NULL);
  LWIP_ASSERT("sem->sem != NULL", sem->sem != NULL);

  SYS_STATS_DEC(sem.used);
  vSemaphoreDelete(sem->sem);
  sem->sem = NULL;
}

/**
 * @brief  创建 lwIP 消息邮箱（基于FreeRTOS队列）
 * @param  mbox 邮箱句柄
 * @param  size 队列最大长度
 * @retval ERR_OK 成功 / ERR_MEM 失败
 */
err_t
sys_mbox_new(sys_mbox_t *mbox, int size)
{
  LWIP_ASSERT("mbox != NULL", mbox != NULL);
  LWIP_ASSERT("size > 0", size > 0);
  /* 创建队列：长度size，每个元素大小为指针宽度 */
  mbox->mbx = xQueueCreate((UBaseType_t)size, sizeof(void *));
  if(mbox->mbx == NULL) {
    SYS_STATS_INC(mbox.err);
    return ERR_MEM;
  }
  SYS_STATS_INC_USED(mbox);
  return ERR_OK;
}

/**
 * @brief  阻塞方式向邮箱发送消息
 * @param  mbox 邮箱句柄
 * @param  msg  要发送的消息指针
 */
void
sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
  BaseType_t ret;
  LWIP_ASSERT("mbox != NULL", mbox != NULL);
  LWIP_ASSERT("mbox->mbx != NULL", mbox->mbx != NULL);
  /* 队尾发送，永久阻塞 */
  ret = xQueueSendToBack(mbox->mbx, &msg, portMAX_DELAY);
  LWIP_ASSERT("mbox post failed", ret == pdTRUE);
}

/**
 * @brief  非阻塞尝试向邮箱发消息
 * @param  mbox 邮箱句柄
 * @param  msg  消息指针
 * @retval ERR_OK成功 / ERR_MEM队列满
 */
err_t
sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
  BaseType_t ret;
  LWIP_ASSERT("mbox != NULL", mbox != NULL);
  LWIP_ASSERT("mbox->mbx != NULL", mbox->mbx != NULL);
  /* 非阻塞发送 */
  ret = xQueueSendToBack(mbox->mbx, &msg, 0);
  if (ret == pdTRUE) {
    return ERR_OK;
  } else {
    LWIP_ASSERT("mbox trypost failed", ret == errQUEUE_FULL);
    SYS_STATS_INC(mbox.err);
    return ERR_MEM;
  }
}

/**
 * @brief  中断里尝试发送邮箱消息
 * @param  mbox 邮箱句柄
 * @param  msg  消息指针
 * @retval 状态码
 */
err_t
sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
  BaseType_t ret;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  LWIP_ASSERT("mbox != NULL", mbox != NULL);
  LWIP_ASSERT("mbox->mbx != NULL", mbox->mbx != NULL);

  ret = xQueueSendToBackFromISR(mbox->mbx, &msg, &xHigherPriorityTaskWoken);
  if (ret == pdTRUE) {
    if (xHigherPriorityTaskWoken == pdTRUE) {
      return ERR_NEED_SCHED;
    }
    return ERR_OK;
  } else {
    LWIP_ASSERT("mbox trypost failed", ret == errQUEUE_FULL);
    SYS_STATS_INC(mbox.err);
    return ERR_MEM;
  }
}

/**
 * @brief  从邮箱获取消息，可超时阻塞
 * @param  mbox 邮箱句柄
 * @param  msg  接收消息指针
 * @param  timeout_ms 超时毫秒
 * @retval 超时返回SYS_ARCH_TIMEOUT，成功返回1
 */
u32_t
sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout_ms)
{
  BaseType_t ret;
  void *msg_dummy;
  LWIP_ASSERT("mbox != NULL", mbox != NULL);
  LWIP_ASSERT("mbox->mbx != NULL", mbox->mbx != NULL);

  if (!msg) {
    msg = &msg_dummy;
  }

  if (!timeout_ms) {
    /* 永久阻塞接收 */
    ret = xQueueReceive(mbox->mbx, &(*msg), portMAX_DELAY);
    LWIP_ASSERT("mbox fetch failed", ret == pdTRUE);
  } else {
    /* 限时接收 */
    TickType_t timeout_ticks = timeout_ms / portTICK_RATE_MS;
    ret = xQueueReceive(mbox->mbx, &(*msg), timeout_ticks);
    if (ret == errQUEUE_EMPTY) {
      *msg = NULL;
      return SYS_ARCH_TIMEOUT;
    }
    LWIP_ASSERT("mbox fetch failed", ret == pdTRUE);
  }

  return 1;
}

/**
 * @brief  非阻塞尝试从邮箱取消息
 * @param  mbox 邮箱句柄
 * @param  msg  接收消息指针
 * @retval 无消息返回SYS_MBOX_EMPTY，成功返回1
 */
u32_t
sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
  BaseType_t ret;
  void *msg_dummy;
  LWIP_ASSERT("mbox != NULL", mbox != NULL);
  LWIP_ASSERT("mbox->mbx != NULL", mbox->mbx != NULL);

  if (!msg) {
    msg = &msg_dummy;
  }

  ret = xQueueReceive(mbox->mbx, &(*msg), 0);
  if (ret == errQUEUE_EMPTY) {
    *msg = NULL;
    return SYS_MBOX_EMPTY;
  }
  LWIP_ASSERT("mbox fetch failed", ret == pdTRUE);

  return 1;
}

/**
 * @brief  销毁消息邮箱
 * @param  mbox 邮箱句柄
 */
void
sys_mbox_free(sys_mbox_t *mbox)
{
  LWIP_ASSERT("mbox != NULL", mbox != NULL);
  LWIP_ASSERT("mbox->mbx != NULL", mbox->mbx != NULL);

#if LWIP_FREERTOS_CHECK_QUEUE_EMPTY_ON_FREE
  /* 检查队列是否还有未处理消息 */
  {
    UBaseType_t msgs_waiting = uxQueueMessagesWaiting(mbox->mbx);
    LWIP_ASSERT("mbox quence not empty", msgs_waiting == 0);

    if (msgs_waiting != 0) {
      SYS_STATS_INC(mbox.err);
    }
  }
#endif
  /* 删除FreeRTOS队列 */
  vQueueDelete(mbox->mbx);
  SYS_STATS_DEC(mbox.used);
}

/**
 * @brief  创建 lwIP 任务线程
 * @param  name     任务名
 * @param  thread   任务入口函数
 * @param  arg      任务传入参数
 * @param  stacksize 栈大小
 * @param  prio     任务优先级
 * @retval lwip 任务句柄
 */
sys_thread_t
sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
  TaskHandle_t rtos_task;
  BaseType_t ret;
  sys_thread_t lwip_thread;
  size_t rtos_stacksize;

  LWIP_ASSERT("invalid stacksize", stacksize > 0);
#if LWIP_FREERTOS_THREAD_STACKSIZE_IS_STACKWORDS
  rtos_stacksize = (size_t)stacksize;
#else
  /* 字节转 FreeRTOS 栈字大小 */
  rtos_stacksize = (size_t)stacksize / sizeof(StackType_t);
#endif

  /* 创建FreeRTOS任务 */
  ret = xTaskCreate(thread, name, (configSTACK_DEPTH_TYPE)rtos_stacksize, arg, prio, &rtos_task);
  LWIP_ASSERT("task creation failed", ret == pdTRUE);

  lwip_thread.thread_handle = rtos_task;
  return lwip_thread;
}

#if LWIP_NETCONN_SEM_PER_THREAD
#if configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0
/**
 * @brief  获取线程局部存储的网络连接信号量
 */
sys_sem_t *
sys_arch_netconn_sem_get(void)
{
  void* ret;
  TaskHandle_t task = xTaskGetCurrentTaskHandle();
  LWIP_ASSERT("task != NULL", task != NULL);

  ret = pvTaskGetThreadLocalStoragePointer(task, 0);
  return ret;
}

/**
 * @brief  分配当前线程专属网络连接信号量
 */
void
sys_arch_netconn_sem_alloc(void)
{
  void *ret;
  TaskHandle_t task = xTaskGetCurrentTaskHandle();
  LWIP_ASSERT("task != NULL", task != NULL);

  ret = pvTaskGetThreadLocalStoragePointer(task, 0);
  if(ret == NULL) {
    sys_sem_t *sem;
    err_t err;
    /* 申请内存并创建信号量 */
    sem = mem_malloc(sizeof(sys_sem_t));
    LWIP_ASSERT("sem != NULL", sem != NULL);
    err = sys_sem_new(sem, 0);
    LWIP_ASSERT("err == ERR_OK", err == ERR_OK);
    LWIP_ASSERT("sem invalid", sys_sem_valid(sem));
    /* 存入线程局部存储 */
    vTaskSetThreadLocalStoragePointer(task, 0, sem);
  }
}

/**
 * @brief  释放当前线程专属网络连接信号量
 */
void sys_arch_netconn_sem_free(void)
{
  void* ret;
  TaskHandle_t task = xTaskGetCurrentTaskHandle();
  LWIP_ASSERT("task != NULL", task != NULL);

  ret = pvTaskGetThreadLocalStoragePointer(task, 0);
  if(ret != NULL) {
    sys_sem_t *sem = ret;
    sys_sem_free(sem);
    mem_free(sem);
    vTaskSetThreadLocalStoragePointer(task, 0, NULL);
  }
}

#else 
#error LWIP_NETCONN_SEM_PER_THREAD needs configNUM_THREAD_LOCAL_STORAGE_POINTERS
#endif
#endif

#if LWIP_FREERTOS_CHECK_CORE_LOCKING
#if LWIP_TCPIP_CORE_LOCKING
/* lwIP 内核锁计数 & 持有任务句柄 */
static u8_t lwip_core_lock_count;
static TaskHandle_t lwip_core_lock_holder_thread;

/**
 * @brief  加锁 TCPIP 内核
 */
void
sys_lock_tcpip_core(void)
{
   sys_mutex_lock(&lock_tcpip_core);
   if (lwip_core_lock_count == 0) {
     lwip_core_lock_holder_thread = xTaskGetCurrentTaskHandle();
   }
   lwip_core_lock_count++;
}

/**
 * @brief  解锁 TCPIP 内核
 */
void
sys_unlock_tcpip_core(void)
{
   lwip_core_lock_count--;
   if (lwip_core_lock_count == 0) {
       lwip_core_lock_holder_thread = 0;
   }
   sys_mutex_unlock(&lock_tcpip_core);
}

#endif

#if !NO_SYS
static TaskHandle_t lwip_tcpip_thread;
#endif

/**
 * @brief  标记当前线程为TCPIP主线程
 */
void
sys_mark_tcpip_thread(void)
{
#if !NO_SYS
  lwip_tcpip_thread = xTaskGetCurrentTaskHandle();
#endif
}

/**
 * @brief  检查当前是否在合法TCPIP线程/已加内核锁
 */
void
sys_check_core_locking(void)
{
  /* 简单判断是否在中断上下文 */
  taskENTER_CRITICAL();
  taskEXIT_CRITICAL();

#if !NO_SYS
  if (lwip_tcpip_thread != 0) {
    TaskHandle_t current_thread = xTaskGetCurrentTaskHandle();

#if LWIP_TCPIP_CORE_LOCKING
    LWIP_ASSERT("Function called without core lock",
                current_thread == lwip_core_lock_holder_thread && lwip_core_lock_count > 0);
#else 
    LWIP_ASSERT("Function called from wrong thread", current_thread == lwip_tcpip_thread);
#endif
  }
#endif
}

#endif

/**
 * @brief  实现lwip_fuzz_rand 解决未定义报错
 * @retval 固定32位伪随机数，供DNS模块调用
 */
u32_t lwip_fuzz_rand(void)
{
    return 0x12345678; 
}
