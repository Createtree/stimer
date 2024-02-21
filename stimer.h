/* UTF8 Encoding */
/*-----------------------------------------------------------------------
|                            FILE DESCRIPTION                           |
-----------------------------------------------------------------------*/
/*----------------------------------------------------------------------
  - File name     : stimer.h
  - Author        : liuzhihua (liuzhihuawy@163.com)
  - Update date   : 2024.1.29
  -	Brief         : software timer
  - Version       : v0.1
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
***/

#ifndef STIMER_H_
#define STIMER_H_

/*-----------------------------------------------------------------------
|                               INCLUDES                                |
-----------------------------------------------------------------------*/
#include <stdint.h>

#ifdef NODEBUG
#define STIMER_ASSERT(x)
#else
// #include <assert.h>
// #define STIMER_ASSERT(x) assert(x)
void stimer_assert_handle(const char* file_name, uint32_t file_line);
#define STIMER_ASSERT(_Expression) (void) \
((!!(_Expression)) || \
(stimer_assert_handle(__FILE__,__LINE__),0))
#endif

#ifdef  __cplusplus
    extern "C" {
#endif
/*-----------------------------------------------------------------------
|                                DEFINES                                |
-----------------------------------------------------------------------*/
/* User config start*/

#define STIMER_TASK_ARG_ENABLE        (1)
#define STIMER_TASK_HOOK_ENABLE       (1)
#define STIMER_MAX_REPETITIONS_BIT    (12)
#define STIMER_MAX_PRIORITY_BIT       (4)

/* User config end */

#define STIMER_MAX_REPETITIONS ((1 << STIMER_MAX_REPETITIONS_BIT) - 1)
#define STIMER_MAX_PRIORITY ((1 << STIMER_MAX_PRIORITY_BIT) - 1)
#define STIMER_MAX_TIMETICK ((((1ULL << (sizeof(stimer_time_t)*8) - 1) - 1) << 1) + 1)
#define STIMER_TASK_LOOP STIMER_MAX_REPETITIONS

typedef uint32_t stimer_time_t;
typedef struct stimer_structure_type stimer_t;
typedef struct stimer_task_structure_type stimer_task_t;

struct stimer_structure_type
{
    stimer_task_t *ptasks;   // 任务列表指针
    uint16_t size;           // 任务列表总长度
    uint16_t wait_cnt;       // 等待列表的任务量
    uint16_t wait_id;        // 等待中的任务id
    stimer_time_t timetick;  // 当前时刻

#if !!(STIMER_TASK_HOOK_ENABLE)
    void (*task_start_hook)(uint16_t id);       // 任务开始钩子
    void (*task_end_hook)(uint16_t id);         // 任务结束钩子
    void (*task_stop_hook)(uint16_t id);        // 任务停止钩子
    void (*task_schedule_hook)(uint16_t id);    // 任务调度钩子
#endif
};

struct stimer_task_structure_type
{
    void (*task_callback)(void*);
    stimer_time_t interval; // 时间间隔
    stimer_time_t expire;   // 到期时间
    uint16_t repetitions:STIMER_MAX_REPETITIONS_BIT; // 重复次数,[0,STIMER_MAX_REPETITIONS] , 最大值表示无限循环
    uint16_t priority:STIMER_MAX_PRIORITY_BIT;       // 优先级[0,STIMER_MAX_PRIORITY], 最小优先级为0
    uint16_t next_id;

#if !!(STIMER_TASK_ARG_ENABLE)
    void *arg;
#endif
};

/*-----------------------------------------------------------------------
|                                  API                                  |
-----------------------------------------------------------------------*/
void stimer_init(stimer_task_t *pTasks, uint16_t Size);
uint16_t stimer_create_task(void (*task_callback)(void*), stimer_time_t interval, uint8_t priority, uint16_t repetitions, void *arg);
void stimer_scheduler(uint16_t id);
void stimer_serve(void);
void stimer_tick_increase(void);
void stimer_task_stop(uint16_t id);

stimer_time_t stimer_get_tick(void);
uint16_t stiemr_get_waitCnt(void);
uint16_t stimer_get_waitID(void);
stimer_time_t stimer_get_nextExpire(void);
stimer_task_t *stimer_get_task(uint16_t id);
uint16_t stimer_get_wait_table(uint16_t* task_table, stimer_time_t* time_table, uint16_t size);

stimer_time_t stimer_task_get_interval(uint16_t id);
uint16_t stimer_task_get_priority(uint16_t id);
uint16_t stimer_task_get_repetitions(uint16_t id);
void *stimer_task_get_callback(uint16_t id);

void stimer_set_waitCnt(uint16_t waitCnt);
void stimer_set_tick(stimer_time_t tick);
void stimer_task_set_interval(uint16_t id, stimer_time_t interval);
void stimer_task_set_priority(uint16_t id, uint16_t priority);
void stimer_task_set_repetitions(uint16_t id, uint16_t repetitions);
void stimer_task_set_callback(uint16_t id, void (*task_callback)(void*));

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