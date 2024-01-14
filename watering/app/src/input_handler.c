#include <stdio.h>
#include <stdint.h>
#include "input_handler.h"
#include "display_oled.h"
#include "hal_input.h"
#include "pump_runner.h"
#include "log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Priorities at which the tasks are created. */
#define TASK_PRIORITY (tskIDLE_PRIORITY + 1)

typedef struct StateMachine tStateMachine;

typedef struct
{
    hal_input_key key;
    hal_input_key_state state;
} KeyEvt;

typedef tStateMachine *pKeyEventHandler(tStateMachine *, void *);
typedef void stateEnterHandler(tStateMachine *);

typedef struct StateMachine
{
    pKeyEventHandler *keyEvtHandler;
    stateEnterHandler *enterHandler;
    pKeyEventHandler *stepOutHandler;
    pKeyEventHandler *settingsChangeHandler;

    uint32 settingsIdx;

    tStateMachine *stepInState;
    tStateMachine *stepOutState;
    tStateMachine *timeoutState;
    tStateMachine *nextState;
    tStateMachine *prevState;
} tStateMachine;

static QueueHandle_t keySrcQueue;

static void inputKeyCallback(hal_input_key key, hal_input_key_state state)
{
    KeyEvt evt;
    evt.key = key;
    evt.state = state;
    xQueueSendToBack(keySrcQueue, &evt, 0);
}

static tStateMachine *enterState(tStateMachine *, tStateMachine *);

static void sleepStateEnter(tStateMachine *);
static void holdStateEnter(tStateMachine *);
static void settingsStateEnter(tStateMachine *);
static tStateMachine *stepOutCurrentState(tStateMachine *);

static tStateMachine *sleepStateEvtHandler(tStateMachine *, void *);
static tStateMachine *holdStateEvtHandler(tStateMachine *, void *);
static tStateMachine *holdStateOutHandler(tStateMachine *, void *);
static tStateMachine *settingsStateEvtHandler(tStateMachine *, void *);
static tStateMachine *settingsStateOutHandler(tStateMachine *, void *);

static tStateMachine *onOffChangeHandler(tStateMachine *, void *);
static tStateMachine *holdTimeChangeHandler(tStateMachine *, void *);
static tStateMachine *periodTimeChangeHandler(tStateMachine *, void *);

static tStateMachine tSleepState = {sleepStateEvtHandler, sleepStateEnter};
static tStateMachine tHoldState = {holdStateEvtHandler, holdStateEnter, holdStateOutHandler};
static tStateMachine tSetOnOffState = {settingsStateEvtHandler, settingsStateEnter, settingsStateOutHandler, onOffChangeHandler, 1};
static tStateMachine tSetHoldTimeState = {settingsStateEvtHandler, settingsStateEnter, settingsStateOutHandler, holdTimeChangeHandler, 2};
static tStateMachine tSetPeriodTimeState = {settingsStateEvtHandler, settingsStateEnter, settingsStateOutHandler, periodTimeChangeHandler, 3};

static tStateMachine *tpCurrentState = &tHoldState;

static void stateInit(void)
{
    tSleepState.stepInState = &tHoldState;
    tSleepState.stepOutState = &tHoldState;
    tSleepState.nextState = &tHoldState;
    tSleepState.prevState = &tHoldState;

    tHoldState.stepOutState = &tSleepState;
    tHoldState.stepInState = &tSetOnOffState;

    tSetOnOffState.stepOutState = &tHoldState;
    tSetOnOffState.prevState = &tSetPeriodTimeState;
    tSetOnOffState.nextState = &tSetHoldTimeState;

    tSetHoldTimeState.stepOutState = &tHoldState;
    tSetHoldTimeState.prevState = &tSetOnOffState;
    tSetHoldTimeState.nextState = &tSetPeriodTimeState;

    tSetPeriodTimeState.stepOutState = &tHoldState;
    tSetPeriodTimeState.prevState = &tSetHoldTimeState;
    tSetPeriodTimeState.nextState = &tSetOnOffState;
}

static void inputEventHandlerTask(void *pvParameters)
{
    KeyEvt currentKeyEvt;
    tStateMachine *nextState;

    stateInit();
    keySrcQueue = xQueueCreate(32, sizeof(KeyEvt));
    hal_input_init(inputKeyCallback);
    pumpStartExecute();

    /* Remove compiler warning about unused parameter. */
    (void)pvParameters;
    tpCurrentState->enterHandler(tpCurrentState);

    for (;;)
    {
        if (pdPASS != xQueueReceive(keySrcQueue, &currentKeyEvt, pdMS_TO_TICKS(60 * 1000)))
        {
            nextState = stepOutCurrentState(tpCurrentState);
            tpCurrentState = enterState(tpCurrentState, nextState);
            continue;
        }
#if INPUT_HANDLER_LOG_ENABLE
        printf("\r\ninput_event:%d %d", currentKeyEvt.key, currentKeyEvt.state);
#endif
        if (tpCurrentState->keyEvtHandler)
        {
            nextState = tpCurrentState->keyEvtHandler(tpCurrentState, &currentKeyEvt);
            tpCurrentState = enterState(tpCurrentState, nextState);
        }
    }
}

static tStateMachine *enterState(tStateMachine *curr, tStateMachine *nextState)
{
    if (nextState && nextState != curr)
    {
        if (nextState->enterHandler)
        {
            nextState->enterHandler(nextState);
        }
        return nextState;
    }
    return curr;
}

static tStateMachine *stepOutCurrentState(tStateMachine *cur)
{
    if (cur->stepOutHandler)
    {
        return cur->stepOutHandler(cur, NULL);
    }
    return cur;
}

static tStateMachine *sleepStateEvtHandler(tStateMachine *state, void *evt)
{
    return state->stepInState;
}

static tStateMachine *holdStateEvtHandler(tStateMachine *state, void *arg)
{
    KeyEvt *evt = (KeyEvt *)arg;

    if (evt->state == RELEASED)
    {
        return state;
    }

    switch (evt->key)
    {
    case KEY_OK:
        return state->stepInState;
        break;

    default:
        return state;
        break;
    }
    return state;
}

static tStateMachine *settingsStateEvtHandler(tStateMachine *state, void *arg)
{
    KeyEvt *evt = (KeyEvt *)arg;

    if (evt->state == RELEASED)
    {
        return state;
    }

    switch (evt->key)
    {
    case KEY_OK:
        return state->nextState;
        break;
    case KEY_CANCEL:
        return state->stepOutState;
        break;
    case KEY_DOWN:
    case KEY_UP:
    {
        if (state->settingsChangeHandler)
        {
            state->settingsChangeHandler(state, arg);
        }
    }
    break;
    default:
        return state;
        break;
    }
    return state;
}
static tStateMachine *onOffChangeHandler(tStateMachine *state, void *arg)
{
    KeyEvt *evt = (KeyEvt *)arg;

    if (evt->state == RELEASED)
    {
        return state;
    }

    if (KEY_UP == evt->key)
    {
        pumpStartExecute();
    }
    else
    {
        pumpStopExecute();
    }
    display_refresh_content();

    return state;
}
static tStateMachine *holdTimeChangeHandler(tStateMachine *state, void *arg)
{
    KeyEvt *evt = (KeyEvt *)arg;

    if (evt->state == RELEASED)
    {
        return state;
    }

    tPumpRunnerCfg cfg = pumpRunnerGetCfg();
    if (KEY_UP == evt->key)
    {
        cfg.pumpKeepSeconds += 1;
    }
    else
    {
        cfg.pumpKeepSeconds -= 1;
    }
    cfg.pumpKeepSeconds = cfg.pumpKeepSeconds < 1 ? 1 : cfg.pumpKeepSeconds;
    pumpRunnerSetCfg(&cfg);
    display_refresh_content();
    return state;
}
static tStateMachine *periodTimeChangeHandler(tStateMachine *state, void *arg)
{
    KeyEvt *evt = (KeyEvt *)arg;

    if (evt->state == RELEASED)
    {
        return state;
    }

    tPumpRunnerCfg cfg = pumpRunnerGetCfg();
    if (KEY_UP == evt->key)
    {
        cfg.pumpRunDurationMinutes += 10;
    }
    else
    {
        cfg.pumpRunDurationMinutes -= 10;
    }
    cfg.pumpRunDurationMinutes = cfg.pumpRunDurationMinutes < 10 ? 10 : cfg.pumpRunDurationMinutes;
    pumpRunnerSetCfg(&cfg);
    display_refresh_content();
    return state;
}

static void sleepStateEnter(tStateMachine *state)
{
    display_highlight_settings(0);
    display_enter_power_save();
    printf("\r\nenter sleep");
}
static void holdStateEnter(tStateMachine *state)
{
    display_highlight_settings(0);
    display_exit_power_save();
    printf("\r\nenter hold");
}
static void settingsStateEnter(tStateMachine *state)
{
    display_highlight_settings(state->settingsIdx);
}

static tStateMachine *holdStateOutHandler(tStateMachine *state, void *args)
{
    if (state->stepOutState)
    {
        return state->stepOutState;
    }
    return state;
}

static tStateMachine *settingsStateOutHandler(tStateMachine *state, void *args)
{
    if (state->stepOutState)
    {
        return state->stepOutState;
    }
    return state;
}

void input_handler_init(void)
{
    xTaskCreate(inputEventHandlerTask, "inputHandler", configMINIMAL_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
}