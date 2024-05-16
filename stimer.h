/* UTF8 Encoding */
/*-----------------------------------------------------------------------
|                            FILE DESCRIPTION                           |
-----------------------------------------------------------------------*/
/*----------------------------------------------------------------------
  - File name     : stimer.h
  - Author        : liuzhihua (liuzhihuawy@163.com)
  - Update date   : 2024.05.14
  -	Brief         : software timer
  - Version       : v0.4
-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------
|                               UPDATE NOTE                             |
-----------------------------------------------------------------------*/
/**
  * Update note:
  * ------------   ---------------   ----------------------------------
  *     Date            Author                      Note
  * ------------   ---------------   ----------------------------------
  *  2024.01.29       liuzhihua                  Create file          
  *  2024.02.22       liuzhihua                Add and modify     
  *  2024.02.27       liuzhihua                Add and modify
  *  2024.05.14       liuzhihua         fixed "stimer_task_stop" bugs
  *  2024.05.16       liuzhihua                add and modify
***/

#ifndef STIMER_H_
#define STIMER_H_


/*-----------------------------------------------------------------------
|                               INCLUDES                                |
-----------------------------------------------------------------------*/
#include <stdint.h>

#ifdef  __cplusplus
    extern "C" {
#endif
/*-----------------------------------------------------------------------
|                                DEFINES                                |
-----------------------------------------------------------------------*/

/************ User config start ************/

// Timer tick per millisecond
#define STIMER_TICK_PER_MS            (1)
// Using assert [0:disable] [1:c std assert]
//              [2:assert with handle] [3:assert with while(1)]
#define STIMER_ASSERT_ENABLE          (3)
// Using task args [0:disable, 1:enable]
#define STIMER_TASK_ARG_ENABLE        (1)
// Using task hook [0:disable, 1:enable]
#define STIMER_TASK_HOOK_ENABLE       (1)
// Using task repetitions bits [1~11]
#define STIMER_MAX_REPETITIONS_BIT    (11)
// Using task priority bits [1~4]
#define STIMER_MAX_PRIORITY_BIT       (4)
// Task Critical section start [example:__disable_irq()]
extern void __disable_irq(void);
#define STIMER_DISABLE_INTERRUPTS()   __disable_irq()
// Task Critical section end [example:__enable_irq()]
extern void __enable_irq(void);
#define STIMER_ENABLE_INTERRUPTS()    __enable_irq()

/************ User config end ************/

#if !!(STIMER_ASSERT_ENABLE)
#define STIMER_ASSERT(x)
#elif ((STIMER_ASSERT_ENABLE) == 1)
#include <assert.h>
#define STIMER_ASSERT(x) assert(x)
#elif ((STIMER_ASSERT_ENABLE) == 2)
void stimer_assert_handle(const char *file_name, uint32_t file_line);
#define STIMER_ASSERT(_Expression) (void) \
((!!(_Expression)) || \
(stimer_assert_handle(__FILE__,__LINE__),0))
#elif ((STIMER_ASSERT_ENABLE) == 3)
#define STIMER_ASSERT(_Expression)  if(!(_Expression))while(1);
#endif

#define STIMER_MAX_REPETITIONS ((1 << STIMER_MAX_REPETITIONS_BIT) - 1)
#define STIMER_MAX_PRIORITY ((1 << STIMER_MAX_PRIORITY_BIT) - 1)
#define STIMER_MAX_TIMETICK ((((1ULL << ((sizeof(stimer_time_t)*8) - 1)) - 1) << 1) + 1)
#define STIMER_TASK_LOOP STIMER_MAX_REPETITIONS

typedef uint32_t stimer_time_t;
typedef struct stimer_structure_type stimer_t;
typedef struct stimer_task_structure_type stimer_task_t;
typedef void (*stimer_pfunc_t)(const void * arg);

struct stimer_structure_type
{
    stimer_task_t *ptasks;   // 任务列表指针
    uint16_t size;           // 任务列表总长度
    uint16_t wait_cnt;       // 等待列表的任务量
    uint16_t wait_id;        // 等待中的任务id
    uint16_t reset_cnt;      // 重置计数
    stimer_time_t timetick;  // 当前时刻

#if !!(STIMER_ASSERT_ENABLE)
    void (*user_assert_callback)(const char *FILE_NAME, uint32_t LINE_NAME);
#endif

#if !!(STIMER_TASK_HOOK_ENABLE)
    void (*task_start_hook)(uint16_t id);       // 任务开始钩子
    void (*task_end_hook)(uint16_t id);         // 任务结束钩子
    void (*task_stop_hook)(uint16_t id);        // 任务停止钩子
    void (*task_schedule_hook)(uint16_t id);    // 任务调度钩子
#endif
};

struct stimer_task_structure_type
{
    stimer_pfunc_t task_callback;
    stimer_time_t interval; // 时间间隔
    stimer_time_t expire;   // 到期时间
    uint16_t reserved:1;    // 保存任务不自动清空
    uint16_t repetitions:STIMER_MAX_REPETITIONS_BIT; // 重复次数,[0,STIMER_MAX_REPETITIONS]
    uint16_t priority:STIMER_MAX_PRIORITY_BIT;       // 优先级[0,STIMER_MAX_PRIORITY], 最小优先级为0
    uint16_t next_id;

#if !!(STIMER_TASK_ARG_ENABLE)
    void *arg;
#endif
};

/**
 * @brief Get current task id
 * @retval uint16_t task id
 * @note Using in callback tasks
 */
#define STIMER_SELF_ID hstimer.wait_id
/**
 * @brief Get current task handle
 * @retval stimer_task_t* timer task handle
 * @note Using in callback tasks
 */
#define STIMER_SELF_TASK hstimer.task_table[hstimer.wait_id]
/**
 * @brief convert ticks to ms
 */
#define STIMER_TICK_TO_MS(tick)  ((double)tick/STIMER_TICK_PER_MS)
/**
 * @brief convert ms to ticks
 */
#define STIMER_MS_TO_TICK(ms)    (ms*STIMER_TICK_PER_MS)

extern stimer_t hstimer;
/*-----------------------------------------------------------------------
|                                  API                                  |
-----------------------------------------------------------------------*/
void stimer_init(stimer_task_t *pTasks, uint16_t Size);
uint16_t stimer_create_task(stimer_pfunc_t task_callback, stimer_time_t interval, uint8_t priority, uint8_t reserved);
void stimer_task_start(uint16_t id, uint16_t repetitions, void *arg);
void stimer_task_delay_start(uint16_t id, uint16_t repetitions, void *arg, stimer_time_t delay);
uint16_t stimer_task_oneshot(stimer_pfunc_t task_callback, stimer_time_t interval, uint8_t priority, void *arg);

void stimer_serve(void);
void stimer_tick_increase(void);
void stimer_task_stop(uint16_t id);

stimer_time_t stimer_get_tick(void);
uint16_t stiemr_get_waitCnt(void);
uint16_t stimer_get_waitID(void);
stimer_time_t stimer_get_nextExpire(void);
uint16_t stimer_get_resetCnt(void);
stimer_task_t *stimer_find_waitTask(uint16_t id);
uint16_t stimer_get_wait_table(uint16_t* task_table, stimer_time_t* time_table, uint16_t size);

stimer_time_t stimer_task_get_interval(uint16_t id);
uint8_t stimer_task_get_reserved(uint16_t id);
uint16_t stimer_task_get_priority(uint16_t id);
uint16_t stimer_task_get_repetitions(uint16_t id);
void *stimer_task_get_callback(uint16_t id);

void stimer_set_waitCnt(uint16_t waitCnt);
void stimer_set_tick(stimer_time_t tick);
void stimer_task_set_interval(uint16_t id, stimer_time_t interval);
void stimer_task_set_reserved(uint16_t id, uint8_t reserved);
void stimer_task_set_priority(uint16_t id, uint16_t priority);
void stimer_task_set_repetitions(uint16_t id, uint16_t repetitions);
void stimer_task_set_callback(uint16_t id, stimer_pfunc_t task_callback);

#if !!(STIMER_ASSERT_ENABLE)
    void stimer_set_assert_callback(void (*user_assert_callback)(const char *FILE_NAME, uint32_t LINE_NAME));
#else
#define stimer_set_assert_callback(NO_EFFECT)
#endif

#if !!(STIMER_TASK_ARG_ENABLE)
void *stimer_task_get_arg(uint16_t id);
void stimer_task_set_arg(uint16_t id, void *arg);
#endif

#if !!(STIMER_TASK_HOOK_ENABLE)
void *stimer_get_task_start_hook(void);
void *stiemr_get_task_end_hook(void);
void *stiemr_get_task_stop_hook(void);
void *stiemr_get_task_schedule_hook(void);
void stimer_set_task_start_hook(void (*task_start_hook)(uint16_t id));
void stimer_set_task_end_hook(void (*task_end_hook)(uint16_t id));
void stimer_set_task_stop_hook(void (*task_stop_hook)(uint16_t id));
void stimer_set_task_schedule_hook(void (*task_schedule_hook)(uint16_t id));
#endif

#ifdef __cplusplus
	}
#endif
#endif
