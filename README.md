# stimer

> a simple software timer driver
>
> 一个简单的软件定时器驱动

## Feature 特性

- Scheduling based on time slice rotation
- Support task priority (Do not support preemption task)
- Support non dynamic memory allocation to reduce security risks
- Provide hook functions for starting, ending, stopping, and scheduling tasks
- Low memory usage and low flash memory usage (The minimum memory occupation of a task is 16 bytes)
- Suitable for running on microcontrollers
- Simple program migration and strong compatibility
- Adopting defensive programming with a large number of assertions at critical locations, facilitating the elimination of most bugs during the development phase

---

- 基于时间片轮换的调度
- 支持任务优先级（不支持抢占任务）
- 支持无动态内存分配，降低安全风险
- 提供任务在开始、结束、停止、调度时的钩子函数
- 低内存使用量和低闪存使用量  (一个任务最低16字节内存占用)
- 适用于在微控制器上运行
- 程序移植简单且兼容性强
- 采用防御式编程，关键位置有大量断言，方便在开发阶段消除大部分bug

## Usage 使用

```c
#include "stimer"

/* [1] Create stimer task buffer */
#define TASK_MAX_SIZE 10
stimer_task_t task_buffer[TASK_MAX_SIZE];

void main()
{
    /* [2] init stimer */
    stimer_init(&task_buffer[0], TASK_MAX_SIZE);
    // other init ...

    /* [3] Create stimer task */
    uint16_t stimer_create_task(void (*task_callback)(const void* arg),
				stimer_time_t interval,
				uint8_t priority,
				uint16_t repetitions,
				void *arg);
    /* main loop */
    while(1)
    {
	/* [4] call serve function in loop*/
        stimer_serve();
    }
}

void systick_interrupt_callback(void)
{
    /* [5] Get tick in systick or timer */
    stimer_tick();
}

```

## Update Log 更新日志

### 2024.02.19

- First create file
- Complete basic functions
- Add Unit Test

### 2024.02.22

- Added and adjusted the definition of task function types
- Added user assertion callback
- Fixed compiler warnings
- Fixed timetick overflow issue
