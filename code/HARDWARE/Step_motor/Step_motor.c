#include "Step_motor.h"
#include "delay.h"
#include "stdlib.h"

u8 zhengzhuan_flag = 0, fanzhuan_flag = 0; // 设备状态
u16 target_steps = 0;                      // 目标步数
u8 pause_flag = 0;                         // 暂停标志
u16 remaining_steps = 0;                   // 剩余步数
u16 current_count = 0;                     // 当前步数计数器
u8 saved_direction = 0;                    // 保存的方向（1=正转，0=反转）
s32 absolute_position_steps = 0;           // 绝对位置步数（相对于初始位置）
s32 last_start_position_steps = 0;         // 上次运动开始时的位置
bool window_is_open = false;

// 基于TIM4的非阻塞分段调度状态
static volatile uint32_t rotate_remaining_turns = 0; // 剩余圈数（用于分段）
static volatile int rotate_dir = 1;                  // 方向：1正转，-1反转
static volatile uint8_t tim4_busy = 0;               // TIM4是否正在计时

// 启动一次定时（ms），TIM4一次性中断，用于触发下一段
static void StepMotor_StartOneShotTIM4(uint16_t delay_ms)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    // TIM4 时钟 72MHz，经预分频 7200-1 => 10kHz 计数（0.1ms 分辨率）
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    TIM_InternalClockConfig(TIM4);

    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1; // 10kHz
    // period = delay_ms * 10 （0.1ms tick）
    uint32_t period = (uint32_t)delay_ms * 10u;
    if (period < 1u)
        period = 1u;
    if (period > 65535u)
        period = 65535u;
    TIM_TimeBaseInitStructure.TIM_Period = (uint16_t)(period - 1u);
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);

    TIM_ClearFlag(TIM4, TIM_FLAG_Update);
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
    tim4_busy = 1;
}

// 启动一段分段旋转：最多6圈，按5ms/步的最快速度
static void StepMotor_StartChunk(uint16_t chunk)
{
    // 每圈2048步，5ms/步 => 每圈 10240ms
    uint32_t steps = (uint32_t)chunk * 2048u;
    uint16_t time_ms = (uint16_t)(steps * 5u); // 受 uint16_t 限制，本函数确保 chunk<=6
    // 启动电机运动（非阻塞，由TIM1驱动步序）
    motor_control(rotate_dir * 360 * (int)chunk, time_ms);
    // 使用TIM4在 time_ms+50ms 后唤起下一段，避免并发启动
    StepMotor_StartOneShotTIM4((uint16_t)(time_ms + 50u));
}

/*********************************************************************************
 * @Function	:	系统设置PB3和PB4为普通IO口
 * @Input		:	deviceSta,设备状态
 * @Output		: 	None
 * @Return		: 	None
 * @Others		: 	JTAG调试方式会受影响
 **********************************************************************************/
void System_PB34_setIO(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // 打开AFIO时钟
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
}
/***************************************************************************************************
 * 描  述 : 初始化步进电机用GPIO口
 * 参  数 : 无
 * 返回值 : 无
 **************************************************************************************************/
void motor_init(void)
{
    System_PB34_setIO();
    // 定义IO初始化配置结构体
    GPIO_InitTypeDef GPIO_InitStructure;
    // 打开PA端口时钟
    RCC_APB2PeriphClockCmd(STEP_RCC_PORT, ENABLE);

    // 配置的IO是PF0~PF3
    GPIO_InitStructure.GPIO_Pin = STEP_A_PIN | STEP_B_PIN | STEP_C_PIN | STEP_D_PIN;
    // 配置为推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    // IO口速度为50MHz
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    // 配置IO
    GPIO_Init(STEP_GPIO_PORT, &GPIO_InitStructure);
    // 初始化时所有引脚设为高电平，电机线圈断电
    AA_0;
    BB_0;
    CC_0;
    DD_0;

    // 初始化位置跟踪变量
    absolute_position_steps = 0;
    last_start_position_steps = 0;

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    // TIM1是APB2时钟，高级定时器
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    TIM_InternalClockConfig(TIM1);

    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 50 - 1; // 定时5ms
    TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0; // TIM1需要设置RepetitionCounter
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

    TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn; // TIM1使用TIM1_UP_IRQn
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);
    TIM_Cmd(TIM1, ENABLE);

    // 配置TIM4中断向量（一次性延时调度用），默认不启动
    NVIC_InitTypeDef NVIC_InitStructure2;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    TIM_InternalClockConfig(TIM4);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure2.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure2.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure2.NVIC_IRQChannelPreemptionPriority = 2; // 低于TIM1
    NVIC_InitStructure2.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure2);
    TIM_Cmd(TIM4, DISABLE);
}

void mortor_zhengzhuan(void)
{
    zhengzhuan_flag = 1, fanzhuan_flag = 0;
}

void mortor_fanzhuan(void)
{
    zhengzhuan_flag = 0, fanzhuan_flag = 1;
}

/***************************************************************************
 * 描  述 : 暂停电机转动
 * 参  数 : 无
 * 返回值 : 无
 * 说  明 : 保存当前进度，电机断电
 ***************************************************************************/
void motor_pause(void)
{
    if (zhengzhuan_flag || fanzhuan_flag) // 只有在转动时才能暂停
    {
        pause_flag = 1; // 设置暂停标志
        // 保存剩余步数和方向
        if (zhengzhuan_flag) {
            remaining_steps = target_steps - current_count;
            saved_direction = 1; // 正转
        } else if (fanzhuan_flag) {
            remaining_steps = target_steps - current_count;
            saved_direction = 0; // 反转
        }

        // 电机断电
        AA_0;
        BB_0;
        CC_0;
        DD_0;

        // 清除转动标志
        zhengzhuan_flag = 0;
        fanzhuan_flag = 0;
    }
}

/***************************************************************************
 * 描  述 : 继续电机转动
 * 参  数 : 无
 * 返回值 : 无
 * 说  明 : 从暂停点继续转动剩余角度
 ***************************************************************************/
void motor_resume(void)
{
    if (pause_flag && remaining_steps > 0) // 只有在暂停状态且有剩余步数时才能继续
    {
        pause_flag = 0; // 清除暂停标志

        // 恢复目标步数
        target_steps = remaining_steps;
        current_count = 0; // 重置计数器

        // 根据之前保存的方向继续转动
        if (saved_direction == 1) {
            mortor_zhengzhuan(); // 继续正转
        } else {
            mortor_fanzhuan(); // 继续反转
        }
    }
}

/***************************************************************************
 * 描  述 : 完全停止电机
 * 参  数 : 无
 * 返回值 : 无
 * 说  明 : 清除所有状态，电机断电
 ***************************************************************************/
void motor_stop(void)
{
    // 清除所有标志
    zhengzhuan_flag = 0;
    fanzhuan_flag = 0;
    pause_flag = 0;
    target_steps = 0;
    remaining_steps = 0;
    saved_direction = 0;

    // 电机断电
    AA_0;
    BB_0;
    CC_0;
    DD_0;
}

/***************************************************************************
 * 描  述 : 步进电机角度控制函数
 * 参  数 : angle - 旋转角度，正值为正转，负值为反转
 *          time_ms - 完成旋转所需的时间（毫秒）
 * 返回值 : 无
 * 说  明 : 28BYJ48步进电机在四拍模式下，实测每圈约2048步
 ***************************************************************************/
void motor_control(int angle, uint16_t time_ms)
{
    // 记录本次运动的开始位置
    last_start_position_steps = absolute_position_steps;

    // 计算需要的步数：根据实际测试校准
    // 实测：90度需要约512步 (90 * 2048 / 360 = 512)
    u16 steps = (u16)(abs(angle) * 2048 / 360);

    // 限制最大步数，防止溢出
    if (steps > 2048)
        steps = 2048;

    // 设置目标步数
    target_steps = steps;

    // 计算每步的时间间隔（微秒）
    // 总时间 = time_ms * 1000 微秒
    // 每步时间 = 总时间 / 步数
    uint32_t step_time_us;
    if (steps > 0) {
        step_time_us = (uint32_t)time_ms * 1000 / steps;
    } else {
        step_time_us = 5000; // 默认5ms
    }

    // 限制最小时间间隔，防止过快导致电机无法响应（过快会只震不转）
    if (step_time_us < 5000)
        step_time_us = 5000; // 最小5ms，更稳

    // 重新配置定时器周期
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    // 计算定时器预分频值和周期值
    // 系统时钟72MHz，预分频7200-1，则定时器时钟为10kHz
    // 周期值 = 10kHz / (1000000 / step_time_us) = step_time_us / 100
    uint16_t period = (uint16_t)(step_time_us / 100);
    if (period < 1)
        period = 1;
    if (period > 65535)
        period = 65535;

    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = period - 1;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0; // TIM1需要设置RepetitionCounter

    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);

    // 根据角度正负值确定旋转方向
    if (angle > 0) {
        // 正值：正转（顺时针）
        mortor_zhengzhuan();
    } else if (angle < 0) {
        // 负值：反转（逆时针）
        mortor_fanzhuan();
    }
    // angle为0时不动作
}

void StepMotor_TIM1_IRQHandler(void)
{
    // 使用全局变量current_count替代局部静态变量count

    if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET) {

        if (zhengzhuan_flag) {
            if (current_count < target_steps) {
                current_count++;
                // 更新绝对位置（正转时增加）
                absolute_position_steps++;

                switch (current_count % 4) {
                    case 1:
                        DD_0;
                        CC_0;
                        BB_1;
                        AA_1;
                        break;
                    case 2:
                        DD_0;
                        CC_1;
                        BB_1;
                        AA_0;
                        break;
                    case 3:
                        DD_1;
                        CC_1;
                        BB_0;
                        AA_0;
                        break;
                    case 0:
                        DD_1;
                        CC_0;
                        BB_0;
                        AA_1;
                        break;
                }
            } else {
                zhengzhuan_flag = 0;
                current_count = 0; // 重置计数器
                target_steps = 0;  // 重置目标步数
                // 电机停止时断电
                AA_0;
                BB_0;
                CC_0;
                DD_0;
            }
        }

        if (fanzhuan_flag) {
            if (current_count < target_steps) {
                current_count++;
                // 更新绝对位置（反转时减少）
                absolute_position_steps--;

                // 反转时按相反顺序执行步序
                switch (current_count % 4) {
                    case 1:
                        DD_1;
                        CC_0;
                        BB_0;
                        AA_1;
                        break;
                    case 2:
                        DD_1;
                        CC_1;
                        BB_0;
                        AA_0;
                        break;
                    case 3:
                        DD_0;
                        CC_1;
                        BB_1;
                        AA_0;
                        break;
                    case 0:
                        DD_0;
                        CC_0;
                        BB_1;
                        AA_1;
                        break;
                }
            } else {
                fanzhuan_flag = 0;
                current_count = 0; // 重置计数器
                target_steps = 0;  // 重置目标步数
                // 电机停止时断电
                AA_0;
                BB_0;
                CC_0;
                DD_0;
            }
        }

        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
    }
}

/***************************************************************************
 * 描  述 : 步进电机归位函数
 * 参  数 : time_ms - 完成归位所需的时间（毫秒）
 * 返回值 : 无
 * 说  明 : 回到上一次运动的起始位置
 ***************************************************************************/
void motor_home(uint16_t time_ms)
{
    // 计算需要回到起始位置的步数
    s32 steps_to_home = last_start_position_steps - absolute_position_steps;

    // 如果没有位置变化，直接返回
    if (steps_to_home == 0) {
        return;
    }

    // 计算步数的绝对值
    u16 abs_steps = (u16)abs(steps_to_home);

    // 限制最大步数，防止溢出
    if (abs_steps > 2048)
        abs_steps = 2048;

    // 设置目标步数
    target_steps = abs_steps;
    current_count = 0; // 重置计数器

    // 计算每步的时间间隔（微秒）
    uint32_t step_time_us;
    if (abs_steps > 0) {
        step_time_us = (uint32_t)time_ms * 1000 / abs_steps;
    } else {
        step_time_us = 5000; // 默认5ms
    }

    // 限制最小时间间隔，防止过快导致电机无法响应
    if (step_time_us < 1000)
        step_time_us = 1000; // 最小1ms

    // 重新配置定时器周期
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    // 计算定时器预分频值和周期值
    // 系统时钟72MHz，预分频7200-1，则定时器时钟为10kHz
    // 周期值 = 10kHz / (1000000 / step_time_us) = step_time_us / 100
    uint16_t period = (uint16_t)(step_time_us / 100);
    if (period < 1)
        period = 1;
    if (period > 65535)
        period = 65535;

    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = period - 1;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0; // TIM1需要设置RepetitionCounter

    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);

    // 根据步数正负值确定旋转方向
    if (steps_to_home > 0) {
        // 正值：正转（顺时针）回到起始位置
        mortor_zhengzhuan();
    } else {
        // 负值：反转（逆时针）回到起始位置
        mortor_fanzhuan();
    }
}

// 非阻塞实现：将圈数转换为步数，以每步最小5ms速度驱动，分段由TIM4定时调度
void motor_rotate_turns_fast(int turns)
{
    // 初始化方向与剩余圈数
    rotate_dir = (turns >= 0) ? 1 : -1;
    rotate_remaining_turns = (turns >= 0) ? (uint32_t)turns : (uint32_t)(-turns);

    // 若无任务，直接返回
    if (rotate_remaining_turns == 0) {
        return;
    }

    // 防止在尚未结束的TIM4调度期间重复启动
    if (tim4_busy) {
        // 若上一次调度仍在计时中，保持原计划；用户应在外部等待当前序列完成
        return;
    }

    // 先启动第一段（最多6圈）
    uint16_t chunk = (rotate_remaining_turns > 6u) ? 6u : (uint16_t)rotate_remaining_turns;
    rotate_remaining_turns -= chunk;
    StepMotor_StartChunk(chunk);
}

// TIM4更新中断：用于在每段运动完成后触发下一段或结束
void StepMotor_TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET) {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        TIM_Cmd(TIM4, DISABLE);
        tim4_busy = 0;

        // 若仍有剩余圈数，继续下一段
        if (rotate_remaining_turns > 0u) {
            uint16_t chunk = (rotate_remaining_turns > 6u) ? 6u : (uint16_t)rotate_remaining_turns;
            rotate_remaining_turns -= chunk;
            StepMotor_StartChunk(chunk);
        } else {
            // 全部完成，确保电机断电（如果TIM1已停）
            // 注意：TIM1中的停止逻辑会在目标步数完成后断电
        }

    }

}

void TIM1_UP_IRQHandler(void)
{
    StepMotor_TIM1_IRQHandler();
}

void TIM4_IRQHandler(void)
{
    StepMotor_TIM4_IRQHandler();
}

void open_window(void)
{
    if (window_is_open)
        return;
    motor_control(90, 2500);
    window_is_open = true;
}

void close_window(void)
{
    if (!window_is_open)
        return;
    motor_control(-90, 2500);
    window_is_open = false;
}
