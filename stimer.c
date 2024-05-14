#include "stimer.h"
#include <string.h>

#define NEXT_TASK_ID(id) hstimer.ptasks[(id)].next_id
stimer_t hstimer;

void stimer_init(stimer_task_t *pTasks, uint16_t Size)
{
    STIMER_ASSERT(pTasks != NULL);
    STIMER_ASSERT(Size != 0);

    memset(pTasks, 0, sizeof(stimer_task_t) * Size);
    hstimer.ptasks = pTasks;
    hstimer.size = Size;
    hstimer.timetick = 0;
    hstimer.wait_cnt = 0;
    hstimer.wait_id = 0;
    hstimer.reset_cnt = 0;
    #if !!(STIMER_TASK_HOOK_ENABLE)
    hstimer.task_start_hook = NULL;
    hstimer.task_end_hook = NULL;
    hstimer.task_stop_hook = NULL;
    hstimer.task_schedule_hook = NULL;
    #endif
    #if !!(STIMER_ASSERT_ENABLE)
    hstimer.user_assert_callback = NULL;
    #endif
}

/**
 * @brief Create a new stimer task
 * @param task_callback task callback function
 * @param interval task interval
 * @param priority task priority
 * @param reserved task will be saved unless manually deleted
 * @retval uint16_t task ID
 * @note Use the function after stimer_init()
 */
uint16_t stimer_create_task(stimer_pfunc_t task_callback, stimer_time_t interval, uint8_t priority, uint8_t reserved)
{
    STIMER_ASSERT(task_callback != NULL);
    STIMER_ASSERT(priority <= STIMER_MAX_PRIORITY);
    STIMER_ASSERT(hstimer.wait_cnt < hstimer.size);

    uint32_t i;
    /* 查找空闲的任务槽位 */
    for (i = 0; i < hstimer.size; i++)
    {
        if (hstimer.ptasks[i].task_callback != NULL || hstimer.ptasks[i].reserved)
        {
            continue;
        }
        break;
    }
    /* 写入任务参数 */
    hstimer.ptasks[i].task_callback = task_callback;
    hstimer.ptasks[i].interval = interval;
    hstimer.ptasks[i].priority = priority;
    hstimer.ptasks[i].reserved = reserved ? 1 : 0;
    hstimer.ptasks[i].repetitions = 0;
    return i;
}

/**
 * @brief Create and execute a task once
 * @param task_callback task callback function
 * @param interval time interval
 * @param priority task priority
 * @param arg task argument
 * @retval uint16_t task id
 */
uint16_t stimer_task_oneshot(stimer_pfunc_t task_callback, stimer_time_t interval, uint8_t priority, void *arg)
{
    uint16_t id = stimer_create_task(task_callback, interval, priority, 0);
    stimer_task_start(id, 1, arg);
    return id;
}

/**
 * @brief Delay starting a task
 * @param id task id
 * @param repetitions 
 * @param arg task argument
 * @param delay delay time
 * @note The expiration time is delay+interval
 */
void stimer_task_delay_start(uint16_t id, uint16_t repetitions, void *arg, stimer_time_t delay)
{
    hstimer.ptasks[id].interval += delay;
    stimer_task_start(id, repetitions, arg);
    hstimer.ptasks[id].interval -= delay;
}

/**
 * @brief Start a task
 * @param id task id
 * @param repetitions 
 * @param arg task argument
 */
void stimer_task_start(uint16_t id, uint16_t repetitions, void *arg)
{
    STIMER_ASSERT(id < hstimer.size);
    STIMER_ASSERT(repetitions <= STIMER_MAX_REPETITIONS);
    STIMER_ASSERT(hstimer.ptasks[id].task_callback != NULL);

    hstimer.ptasks[id].repetitions = repetitions;
    #if !!(STIMER_TASK_ARG_ENABLE)
        hstimer.ptasks[id].arg = arg;
    #endif
    /* 将任务加入到等待队列 */
    stimer_scheduler(id);
}

uint16_t stimer_find_taskid(uint16_t id)
{

}

void stimer_scheduler(uint16_t id)
{
    STIMER_ASSERT(id < hstimer.size);
    uint32_t i, min, lmin;
    min = hstimer.wait_id;
    if (hstimer.ptasks[id].repetitions == 0) return;
    /* 计算到期时间 */
    if (STIMER_MAX_TIMETICK - hstimer.timetick < hstimer.ptasks[id].interval)
    {
        /* 若到期时间超过计数上限则重置定时器时间刻*/
        stimer_reset();
    }
    hstimer.ptasks[id].expire = hstimer.ptasks[id].interval + hstimer.timetick;
    /* 将任务安排到计划表,等待列表中存在该任务则重新安排 */

    /* 查找并移除相同id的任务 */
    for (i = 0; i < hstimer.wait_cnt; i++)
    {
        if (id == min)
        {
            if (id == hstimer.wait_id)
            {
                hstimer.wait_id = hstimer.ptasks[min].next_id;
            }
            else
            {
                hstimer.ptasks[lmin].next_id = hstimer.ptasks[id].next_id;
            }
            hstimer.wait_cnt--;
            break;
        }
        lmin = min;
        min = hstimer.ptasks[min].next_id;
    }

    /* 当前没有任务 */
    if (hstimer.wait_cnt == 0)
    {
        hstimer.wait_id = id;
        goto end;
    }

    /* 根据到期时间和优先级找到该任务安排的位置 */
    min = hstimer.wait_id;
    for (i = 0; i < hstimer.wait_cnt; i++)
    {
        if (hstimer.ptasks[min].expire > hstimer.ptasks[id].expire
            || (hstimer.ptasks[min].expire == hstimer.ptasks[id].expire
            && hstimer.ptasks[min].priority < hstimer.ptasks[id].priority))
        {
            hstimer.ptasks[id].next_id = min;
            if (min == hstimer.wait_id)
            {
                hstimer.wait_id = id;
            }
            else
            {
                hstimer.ptasks[lmin].next_id = id;
            }
            goto end;
        }
        lmin = min;
        min = hstimer.ptasks[min].next_id;
    }
    if (i == hstimer.wait_cnt)
    {
        hstimer.ptasks[lmin].next_id = id;
    }

    end:
    hstimer.wait_cnt++;
    /* 执行调度钩子 */
    #if !!(STIMER_TASK_HOOK_ENABLE)
    if (hstimer.task_schedule_hook != NULL)
    {
        hstimer.task_schedule_hook(id);
    }
    #endif

}

/**
 * @brief Run the stimer tick
 * @note Call this function in systick interrupt
 */
void stimer_tick_increase(void)
{
    hstimer.timetick++;
}

void stimer_reset(void)
{
    stimer_time_t tick = hstimer.timetick;
    uint16_t i;
    stimer_task_t *ptask;

    if (hstimer.wait_cnt > 0)
    {
        ptask = &hstimer.ptasks[hstimer.wait_id];
        for (i = 0; i < hstimer.wait_cnt; i++)
        {
            if (ptask->expire > tick)
            {
                ptask->expire -= tick;
            }
            else
            {
                ptask->expire = 0;
            }
            ptask = &hstimer.ptasks[ptask->next_id];
        }
    }
    hstimer.timetick = 0;
    hstimer.reset_cnt++;
}

/**
 * @brief Stop a stimer task
 * @param id task id
 * @note If the task is reserved, it needs to be cancelled first
 */
void stimer_task_stop(uint16_t id)
{
    STIMER_ASSERT(id < hstimer.size);

    uint32_t i;
    stimer_task_t *ptask = &hstimer.ptasks[hstimer.wait_id];
    int flag = 0;
    if (id == hstimer.wait_id && hstimer.wait_cnt > 0)
    {
        hstimer.wait_id = ptask->next_id;
        hstimer.wait_cnt--;
        flag = 1;
    }
    else
    {
        for (i = 1; i < hstimer.wait_cnt; i++)
        {
            if (ptask->next_id == id)
            {
                ptask->next_id = hstimer.ptasks[id].next_id;
                hstimer.wait_cnt--;
                flag = 1;
                break;
            }
            ptask = &hstimer.ptasks[ptask->next_id];
        }
    }
    if (hstimer.ptasks[id].reserved == 0 && flag == 1)
    {
        hstimer.ptasks[id].task_callback = NULL;
        hstimer.ptasks[id].repetitions = 0;
        hstimer.ptasks[id].reserved = 0;
    }
}

/**
 * @brief Stimer serve function
 * @note Called in the main while(1)
 */
void stimer_serve(void)
{
    stimer_task_t *ptask;
    /* 判断任务是否到期 */
    while (hstimer.wait_cnt && hstimer.ptasks[hstimer.wait_id].expire <= hstimer.timetick)
    {
        STIMER_ASSERT(hstimer.wait_id < hstimer.size);
        STIMER_ASSERT(hstimer.ptasks[hstimer.wait_id].repetitions > 0);
        STIMER_ASSERT(hstimer.ptasks[hstimer.wait_id].task_callback != NULL);
        ptask = &hstimer.ptasks[hstimer.wait_id];
        if (STIMER_TASK_LOOP != ptask->repetitions)
        {
            ptask->repetitions--;
        }

        /* 执行任务开始钩子 */
        #if !!(STIMER_TASK_HOOK_ENABLE)
        if (hstimer.task_start_hook != NULL)
        {
            hstimer.task_start_hook(hstimer.wait_id);
        }
        #endif

        /* 对定时任务进行回调 */
        #if !!(STIMER_TASK_ARG_ENABLE)
        ptask->task_callback(ptask->arg);
        #else
        ptask->task_callback((void*)0);
        #endif

        /* 执行任务结束钩子 */
        #if !!(STIMER_TASK_HOOK_ENABLE)
        if (hstimer.task_end_hook != NULL)
        {
            hstimer.task_end_hook(hstimer.wait_id);
        }
        #endif

        /* 重新调度该任务 */
        if (ptask->repetitions > 0)
        {
            stimer_scheduler(hstimer.wait_id);
        }
        else
        {
            #if !!(STIMER_TASK_HOOK_ENABLE)
            if (hstimer.task_stop_hook != NULL)
            {
                hstimer.task_stop_hook(hstimer.wait_id);
            }
            #endif
            stimer_task_stop(hstimer.wait_id);
        }
    }
}

stimer_time_t stimer_get_tick(void)
{
    return hstimer.timetick;
}

uint16_t stiemr_get_waitCnt(void)
{
    return hstimer.wait_cnt;
}

uint16_t stimer_get_waitID(void)
{
    return hstimer.wait_id;
}

/**
 * @brief Obtain the expiration time of the next task
 * @retval stimer_time_t expiration time
 * @warning If stiemr_get_waitCnt() is 0, the function always retrun 0
 */
stimer_time_t stimer_get_nextExpire(void)
{
    if (hstimer.wait_cnt == 0) return 0;
    return hstimer.ptasks[hstimer.wait_id].expire;
}

uint16_t stimer_get_resetCnt(void)
{
    return hstimer.reset_cnt;
}

/**
 * @brief Using ID to find waiting task
 * @param id task id
 * @retval stimer_task_t* task handle
 */
stimer_task_t *stimer_find_waitTask(uint16_t id)
{
    STIMER_ASSERT(id < hstimer.size);
    uint16_t i;
    stimer_task_t *ptask;

    if (hstimer.ptasks[id].reserved)
    {
        return &hstimer.ptasks[id];
    }
    if (hstimer.wait_cnt == 0)
    {
        return NULL;
    }
    if (hstimer.wait_id == id)
    {
        return &hstimer.ptasks[id];
    }
    ptask = &hstimer.ptasks[hstimer.wait_id];
    for (i = 1; i < hstimer.wait_cnt; i++)
    {
        if (ptask->next_id == id)
        {
            return &hstimer.ptasks[ptask->next_id];
        }
        ptask = &hstimer.ptasks[ptask->next_id];
    }
    return NULL;
}

stimer_time_t stimer_task_get_interval(uint16_t id)
{
    STIMER_ASSERT(id < hstimer.size);
    return hstimer.ptasks[id].interval;
}

uint8_t stimer_task_get_reserved(uint16_t id)
{
    STIMER_ASSERT(id < hstimer.size);
    return hstimer.ptasks[id].reserved;
}

uint16_t stimer_task_get_priority(uint16_t id)
{
    STIMER_ASSERT(id < hstimer.size);
    return hstimer.ptasks[id].priority;
}

uint16_t stimer_task_get_repetitions(uint16_t id)
{
    STIMER_ASSERT(id < hstimer.size);
    return hstimer.ptasks[id].repetitions;
}

void *stimer_task_get_callback(uint16_t id)
{
    STIMER_ASSERT(id < hstimer.size);
    return hstimer.ptasks[id].task_callback;
}

void stimer_set_waitCnt(uint16_t waitCnt)
{
    STIMER_ASSERT(waitCnt < hstimer.size);
    hstimer.wait_cnt = waitCnt;
}

void stimer_set_tick(stimer_time_t tick)
{
    hstimer.timetick = tick;
}

#if !!(STIMER_TASK_ARG_ENABLE)
void stimer_task_set_arg(uint16_t id, void *arg)
{
    STIMER_ASSERT(id < hstimer.size);
    hstimer.ptasks[id].arg = arg;
}

void *stimer_task_get_arg(uint16_t id)
{
    STIMER_ASSERT(id < hstimer.size);
    return hstimer.ptasks[id].arg;
}
#endif

void stimer_task_set_interval(uint16_t id, stimer_time_t interval)
{
    STIMER_ASSERT(id < hstimer.size);
    hstimer.ptasks[id].interval = interval;
}

void stimer_task_set_reserved(uint16_t id, uint8_t reserved)
{
    STIMER_ASSERT(id < hstimer.size);
    hstimer.ptasks[id].reserved = reserved;
}

void stimer_task_set_priority(uint16_t id, uint16_t priority)
{
    STIMER_ASSERT(id < hstimer.size);
    STIMER_ASSERT(priority <= STIMER_MAX_PRIORITY);
    hstimer.ptasks[id].priority = priority;
}

void stimer_task_set_repetitions(uint16_t id, uint16_t repetitions)
{
    STIMER_ASSERT(id < hstimer.size);
    hstimer.ptasks[id].repetitions = repetitions;
}

void stimer_task_set_callback(uint16_t id, stimer_pfunc_t task_callback)
{
    STIMER_ASSERT(id < hstimer.size);
    hstimer.ptasks[id].task_callback = task_callback;
}

/**
 * @brief Get wait list table
 * @param task_table task id buffer
 * @param time_table task expire buffer
 * @param size buffer size
 * @retval uint16_t wait list length
 */
uint16_t stimer_get_wait_table(uint16_t* task_table, stimer_time_t* time_table, uint16_t size)
{
    STIMER_ASSERT(task_table != NULL);
    STIMER_ASSERT(time_table != NULL);
    uint16_t i, id;
    size = hstimer.wait_cnt > size ? size : hstimer.wait_cnt;
    id = hstimer.wait_id;
    for (i = 0; i < size; i++)
    {
        task_table[i] = id;
        time_table[i] = hstimer.ptasks[id].expire;
        id = hstimer.ptasks[id].next_id;
    }
    return size;
}

#if !!(STIMER_TASK_HOOK_ENABLE)
void *stimer_get_task_start_hook(void)
{
    return hstimer.task_start_hook;
}

void *stiemr_get_task_end_hook(void)
{
    return hstimer.task_end_hook;
}

void *stiemr_get_task_stop_hook(void)
{
    return hstimer.task_stop_hook;
}

void *stiemr_get_task_schedule_hook(void)
{
    return hstimer.task_schedule_hook;
}

void stimer_set_task_start_hook(void (*task_start_hook)(uint16_t id))
{
    hstimer.task_start_hook = task_start_hook;
}

void stimer_set_task_end_hook(void (*task_end_hook)(uint16_t id))
{
    hstimer.task_end_hook = task_end_hook;
}

void stimer_set_task_stop_hook(void (*task_stop_hook)(uint16_t id))
{
    hstimer.task_stop_hook = task_stop_hook;
}

void stimer_set_task_schedule_hook(void (*task_schedule_hook)(uint16_t id))
{
    hstimer.task_schedule_hook = task_schedule_hook;
}
#endif

#if !!(STIMER_ASSERT_ENABLE)
void stimer_set_assert_callback(void (*user_assert_callback)(const char *FILE_NAME, uint32_t LINE_NAME))
{
    hstimer.user_assert_callback = user_assert_callback;
}
#endif

void stimer_assert_handle(const char *FILE_NAME, uint32_t LINE_NAME)
{

#if !!(STIMER_ASSERT_ENABLE)
    if (hstimer.user_assert_callback != NULL)
    {
        hstimer.user_assert_callback(FILE_NAME, LINE_NAME);
    }
#endif

    while(1)
    {
        ;
    }
}
