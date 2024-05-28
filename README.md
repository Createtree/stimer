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

The user configuration options (are located in `stimer. h`)

配置用户选项 `stimer.h`

```c
/************ User config start ************/

// Timer tick per millisecond
#define STIMER_TICK_PER_MS            (1)
// Using assert [0:disable] [1:c std assert]
//              [2:assert with handle] [3:assert with while(1)]
#define STIMER_ASSERT_ENABLE          (3)
// Using task args [0:disable, 1:enable]
#define STIMER_TASK_ARG_ENABLE        (0)
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
```

API usage examples

API使用示例

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
    uint16_t task_id = 
    stimer_create_task((stimer_pfunc_t) task_callback,
                       (stimer_time_t) interval,
                       (uint8_t) priority,
                       (uint8_t) reserved);
    // other task create ...

    /* [4] Start stimer task */
    void stimer_task_start((uint16_t) task_id,
                           (uint16_t) repetitions,
                           (void *) arg);
  
    // trips: the oneshot created task has already been started
    uint16_t stimer_task_oneshot((stimer_pfunc_t) task_callback,
                                 (stimer_time_t) interval,
                                 (uint8_t) priority,
                                 (void *) arg);
    // other task start ...
   
    /* main loop */
    while(1)
    {
	/* [5] call serve function in loop*/
        stimer_serve();
    }
}

void systick_interrupt_callback(void)
{
    /* [6] Get tick in systick or timer */
    stimer_tick_increase();
}

```

## Update Log 更新日志

### 2024.05.28

- fixed `stimer_task_oneshot` bugs
- 修复 `stimer_task_oneshot` 中的错误

### 2024.05.16

- Add macro configuration for critical state protection
- Improve the use of 'stimer_task_oneshot', The same callback function will update the configuration of the task
- Added testing for newly added features
- 新增临界状态保护的宏配置
- 改善 `stimer_task_oneshot` 的使用，相同的任务回调将只会更新原来的任务配置
- 为新添加的改动添加了测试

### 2024.05.14

- fixed `stimer_task_stop` bugs
- 修复 `stimer_task_stop` 的错误

### 2024.02.26

- Split the task create and task start
- Added reserved task
- Added one shot task create
- Added macro definition for tick/ms unit conversion
- 将创建任务和开始任务进行拆分
- 新增了保留任务功能
- 新增了oneshot任务创建
- 新增了tick/ms单位转换的宏定义

---

### 2024.02.22

- Added and adjusted the definition of task function types
- Added user assertion callback
- Fixed compiler warnings
- Fixed timetick overflow issue

---

### 2024.02.19

- First create file
- Complete basic functions
- Add Unit Test
