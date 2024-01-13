#ifndef _PUMP_RUNNER_H_
#define _PUMP_RUNNER_H_

#include "types.h"

#define INFINITY_TIME 0xffffffff

typedef struct
{
    // 水泵开启时间：秒
    uint32 pumpKeepSeconds;
    // 运行周期：分钟
    uint32 pumpRunDurationMinutes;
} tPumpRunnerCfg;

// 水泵状态
typedef enum
{
    PUMP_INIT = 1, // 未启动
    PUMP_WAITING,  // 正在倒计时等待泵水
    PUMP_RUNNING,  // 正在泵水
} ePumpState;

/**
 * 获取下次水泵运行的时间(秒)
 */
uint32 pumpNextRunTimeSecondsGet(void);

/**
 * 获取水泵剩余运行的时间，即多久会停止（秒）
 */
uint32 pumpRunningRemainTimeSecondsGet(void);

/**
 * 初始化
 */
void pump_runner_init(void);

/**
 * 开始执行
 */
void pumpStartExecute(void);

/**
 * 停止执行
 */
void pumpStopExecute(void);

/**
 * 配置
 */
void pumpRunnerSetCfg(tPumpRunnerCfg *);

/*
 * 配置获取
 */
tPumpRunnerCfg pumpRunnerGetCfg(void);

#endif
