#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <assert.h>
#include "stimer.h"

/* Unit test define */
int test_count = 0, test_pass = 0, main_ret = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_PTR(expect, actual) EXPECT_EQ_BASE(((void*)expect) == (void*)(actual), (void*)expect, (void*)actual, "%p")

/* End unit test define */

/* Test value define */
uint16_t *run_task_result;
stimer_time_t *run_task_time;
uint16_t run_task_cnt;
uint32_t run_task_size;

uint16_t priority_table[] = {
    1, 1, 1, 1, 2
};

uint16_t interval_table[] = {
    1, 2, 3, 4, 1
};
uint16_t repeat_table[] = {
    2, 2, 2, 2, 8
};

#define task_function_template(NAME) \
    void NAME(void const *arg){\
        static int cnt = 0;\
        printf("[%d]taskName:[" #NAME "] arg:[%X] cnt:[%d]\n", stimer_get_tick(), arg, ++cnt);\
    }

task_function_template(task0)
task_function_template(task1)
task_function_template(task2)
task_function_template(task3)
task_function_template(task4)

stimer_pfunc_t task_func_table[] = {
     task0, task1, task2, task3, task4
};

#define TASK_SIZE (sizeof(task_func_table)/sizeof(task_func_table[0]))


stimer_task_t task_buffer[TASK_SIZE];
uint16_t task_table[TASK_SIZE];
uint32_t time_table[TASK_SIZE];


/* Start test value define */

static void test_task_create(stimer_pfunc_t *taskTable, uint16_t *intervalTable, uint16_t *priorityTable, uint16_t *repeatTable, uint16_t tableSize)
{
    for (size_t i = 0; i < tableSize; i++)
    {
        stimer_create_task(taskTable[i],
                           intervalTable[i],
                           priorityTable[i],
                           0);
        stimer_task_start(i, repeatTable[i], (void*)i);
    }
    
    for (size_t i = 0; i < TASK_SIZE; i++)
    {
        EXPECT_EQ_PTR(taskTable[i], stimer_task_get_callback(i));
        EXPECT_EQ_INT(intervalTable[i], stimer_task_get_interval(i));
        EXPECT_EQ_INT(priorityTable[i], stimer_task_get_priority(i));
        EXPECT_EQ_INT(repeatTable[i], stimer_task_get_repetitions(i));
    }
}

static void test_task_scheduler(uint32_t startTime, uint32_t endTime, const uint16_t *expect_idTable, const stimer_time_t *expect_timeTable, uint16_t tableSize)
{
    stimer_set_tick(startTime);
    while (stimer_get_tick() < endTime)
    {
        stimer_serve();
        stimer_tick_increase();
    }

    for (size_t i = 0; i < tableSize; i++)
    {
        EXPECT_EQ_INT(expect_idTable[i], run_task_result[i]);
        EXPECT_EQ_INT(expect_timeTable[i], run_task_time[i]);
    }
    
}

static void test_task_oneshot(stimer_pfunc_t *taskFuncTable, uint16_t tableSize)
{
    assert(tableSize >= 3);
    uint16_t id0, id1, id2, id3;
    run_task_cnt = 0;
    memset(run_task_result, 0xFA, sizeof(run_task_result));
    stimer_set_waitCnt(0);
    stimer_set_tick(0);
    id0 = stimer_task_oneshot(taskFuncTable[0], 1, 1, NULL);
    id1 = stimer_task_oneshot(taskFuncTable[1], 1, 2, NULL);
    id2 = stimer_task_oneshot(taskFuncTable[2], 1, 3, NULL);
    id3 = stimer_task_oneshot(taskFuncTable[2], 2, 3, NULL);
    stimer_tick_increase();
    stimer_serve();
    stimer_tick_increase();
    stimer_serve();
    stimer_tick_increase();
    stimer_serve();
    EXPECT_EQ_INT(run_task_cnt, 3);
    EXPECT_EQ_INT(id1, run_task_result[0]);
    EXPECT_EQ_INT(id0, run_task_result[1]);
    EXPECT_EQ_INT(id2, run_task_result[2]);
    EXPECT_EQ_INT(id2, id3);
}

static void test_task_insert(stimer_pfunc_t *taskFuncTable, uint16_t tableSize)
{
    assert(tableSize >= 2);
    uint16_t id0, id1;
    stimer_set_waitCnt(0);
    stimer_set_tick(0);

    run_task_cnt = 0;
    // insert task during timer serve phase
    id0 = stimer_create_task(taskFuncTable[0], 1, 0, 0);
    stimer_task_start(id0, 2, NULL);
    stimer_set_tick(stimer_get_nextExpire());
    stimer_serve();
    id1 = stimer_create_task(taskFuncTable[1], 1, 1, 0);
    stimer_task_start(id1, 1, NULL);
    stimer_set_tick(stimer_get_nextExpire());
    stimer_serve();
    stimer_set_tick(stimer_get_nextExpire());
    stimer_serve();

    EXPECT_EQ_INT(3, run_task_cnt);
    EXPECT_EQ_INT(id0, run_task_result[0]);
    EXPECT_EQ_INT(id1, run_task_result[1]);
    EXPECT_EQ_INT(id0, run_task_result[2]);

}

static void test_task_stop(stimer_pfunc_t *taskFuncTable, uint16_t tableSize)
{
    assert(tableSize >= 2);
    uint16_t id0, id1;
    stimer_set_waitCnt(0);
    stimer_set_tick(0);
    memset(run_task_result, 0xFA, sizeof(run_task_result));
    run_task_cnt = 0;
    // stop task during timer serve phase
    id0 = stimer_create_task(taskFuncTable[0], 1, 1, 0);
    stimer_task_start(id0, 2, NULL);
    stimer_set_tick(stimer_get_nextExpire());
    stimer_serve(); // run id0 task
    id1 = stimer_create_task(taskFuncTable[1], 1, 0, 0);
    stimer_task_start(id1, 2, NULL);
    stimer_task_stop(id0); // stop id0 (repetition = 1)
    stimer_set_tick(stimer_get_nextExpire());
    stimer_serve(); // run id1 task
    stimer_task_stop(id1);

    EXPECT_EQ_INT(0, stiemr_get_waitCnt());
    EXPECT_EQ_INT(id0, run_task_result[0]);
    EXPECT_EQ_INT(id1, run_task_result[1]);

    // The situation where the testing task has not yet been started
    hstimer.wait_id = 0;
    id0 = stimer_create_task(taskFuncTable[0], 1, 1, 1);
    stimer_task_stop(id0);
    EXPECT_EQ_PTR(hstimer.ptasks[id0].task_callback , taskFuncTable[0]);
    stimer_task_start(id0, 2, NULL);
    EXPECT_EQ_PTR(&hstimer.ptasks[id0], stimer_find_waitTask(id0));
    stimer_set_tick(stimer_get_nextExpire());
    stimer_serve();
    stimer_task_set_reserved(id0, 0);
    stimer_task_stop(id0);


}

static void test_task_tick_overflow(stimer_pfunc_t *taskFuncTable, uint16_t tableSize)
{
    assert(tableSize >= 2);
    uint16_t id0, id1;
    stimer_set_waitCnt(0);
    stimer_set_tick(STIMER_MAX_TIMETICK - 1);
    run_task_cnt = 0;
    /*  test case:
            time : [max-1]  max  0  1  2  ...
            task0:    0      -   0  -  -
            task1:    -      -   1  -  -
            convert to:
            time : 0  1  2  3...
            task0: 0  -  0  -
            task1: -  -  1  -
    */
    id0 = stimer_create_task(taskFuncTable[0], 1, 1, 0);
    stimer_task_start(id0, 2, NULL);
    id1 = stimer_create_task(taskFuncTable[1], 2, 0, 0);
    stimer_task_start(id1, 1, NULL);
    stimer_tick_increase();
    stimer_serve();
    stimer_tick_increase();
    stimer_serve();
    stimer_tick_increase();
    stimer_serve();
    EXPECT_EQ_INT(0, stiemr_get_waitCnt());
    EXPECT_EQ_INT(id0, run_task_result[0]);
    EXPECT_EQ_INT(id0, run_task_result[1]);
    EXPECT_EQ_INT(id1, run_task_result[2]);
}

static void test_task_preserve(stimer_pfunc_t *taskFuncTable, uint16_t tableSize)
{
    assert(tableSize >= 1);
    uint16_t id0, id1;
    stimer_set_waitCnt(0);
    stimer_set_tick(0);
    run_task_cnt = 0;

    id0 = stimer_create_task(taskFuncTable[0], 1, 1, 1);
    stimer_task_start(id0, 1, NULL);
    stimer_set_tick(stimer_get_nextExpire());
    stimer_serve();
    stimer_serve();
    EXPECT_EQ_PTR(taskFuncTable[0], stimer_find_waitTask(id0)->task_callback);
}

static void test_task_repete(stimer_pfunc_t *taskFuncTable, uint16_t tableSize)
{
    assert(tableSize >= 1);
    uint16_t id0, id1;
    int i = tableSize;
    stimer_set_waitCnt(0);
    stimer_set_tick(0);
    run_task_cnt = 0;

    id0 = stimer_create_task(taskFuncTable[0], 1, 1, 0);
    stimer_task_start(id0, STIMER_TASK_LOOP, NULL);
    while (i--)
    {
        stimer_set_tick(stimer_get_nextExpire());
        stimer_serve();
    }
    EXPECT_EQ_INT(tableSize, run_task_cnt);
}

int critical_counter = 0;
void __disable_irq(void)
{
    critical_counter++;
}
void __enable_irq(void)
{
    critical_counter--;
}

void task_run_start_hook(uint16_t id)
{
    run_task_result[run_task_cnt] = id;
    run_task_time[run_task_cnt] = stimer_get_tick();
    run_task_cnt++;
#if 0
    printf("task %d run at %d\n", id, stimer_get_tick());
#endif
}

void task_run_schedule_hook(uint16_t id)
{
#if 0
    int cnt = stimer_get_wait_table(task_table, time_table, TASK_SIZE);
    printf("schedule: %d", task_table[0]);
    for (size_t i = 1; i < cnt; i++)
    {
        printf(" -> %d", task_table[i]);
    }
    printf("\n");
#endif
}

int main(int argc, char *argv[])
{
    /*
    tick 0 1 2 3 4 5 6 7 8 9
    task0| 0 0 - - - - - - -
    task1| - 1 - 1 - - - - -
    task2| - - 2 - - 2 - - -
    task3| - - - 3 - - - 3 -
    task4| 4 4 4 4 4 4 4 4 -

    result 4, 0, 4, 1, 0, 4, 2, 4, 3, 1, 4, 4, 2, 4, 4, 3
    tick   1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 5, 6, 6, 7, 8, 8
    */
    stimer_init(task_buffer, TASK_SIZE);
    stimer_set_task_start_hook(task_run_start_hook);
    stimer_set_task_schedule_hook(task_run_schedule_hook);

    run_task_cnt = 0;
    for (size_t i = 0; i < TASK_SIZE; i++)
    {
        run_task_size += repeat_table[i];
    }
   
    test_task_create(task_func_table,
                     interval_table, 
                     priority_table, 
                     repeat_table, 
                     TASK_SIZE);

    run_task_result = malloc(sizeof(void*) * run_task_size);
    run_task_time = malloc(sizeof(stimer_time_t) * run_task_size);
    run_task_cnt = 0;
    uint16_t expect_id_table[] = {4, 0, 4, 1, 0, 4, 2, 4, 3, 1, 4, 4, 2, 4, 4, 3};
    stimer_time_t expect_time_table[] = {1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 5, 6, 6, 7, 8, 8};

    test_task_scheduler(0, 10,
                        expect_id_table,
                        expect_time_table,
                        run_task_size);
    EXPECT_EQ_INT(0, critical_counter);
    test_task_oneshot(task_func_table, TASK_SIZE);
    EXPECT_EQ_INT(0, critical_counter);
    test_task_insert(task_func_table, TASK_SIZE);
    EXPECT_EQ_INT(0, critical_counter);
    test_task_stop(task_func_table, TASK_SIZE);
    EXPECT_EQ_INT(0, critical_counter);
    test_task_tick_overflow(task_func_table, TASK_SIZE);
    EXPECT_EQ_INT(0, critical_counter);
    test_task_preserve(task_func_table, TASK_SIZE);
    EXPECT_EQ_INT(0, critical_counter);
    test_task_repete(task_func_table, TASK_SIZE);
    EXPECT_EQ_INT(0, critical_counter);

    free(run_task_result);
    free(run_task_time);

    printf("all test done\n");
    printf("result: %d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
}
