#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "stdbool.h"

#define MOTOR_Z 1 // 正转 顺时针转动
#define MOTOR_F 0 // 反转 逆时针转动

#define STEP_RCC_PORT RCC_APB2Periph_GPIOB /* GPIO端口时钟 */
#define STEP_GPIO_PORT GPIOB               /* GPIO端口 */
#define STEP_A_PIN GPIO_Pin_12             /* 控制四相的GPIO */
#define STEP_B_PIN GPIO_Pin_13
#define STEP_C_PIN GPIO_Pin_14
#define STEP_D_PIN GPIO_Pin_15

#define AA_0 GPIO_ResetBits(STEP_GPIO_PORT, STEP_A_PIN) // 电机控制口，连接电机驱动板IN1
#define AA_1 GPIO_SetBits(STEP_GPIO_PORT, STEP_A_PIN)   // 电机控制口，连接电机驱动板IN1
#define BB_0 GPIO_ResetBits(STEP_GPIO_PORT, STEP_B_PIN) // 电机控制口，连接电机驱动板IN2
#define BB_1 GPIO_SetBits(STEP_GPIO_PORT, STEP_B_PIN)   // 电机控制口，连接电机驱动板IN2
#define CC_0 GPIO_ResetBits(STEP_GPIO_PORT, STEP_C_PIN) // 电机控制口，连接电机驱动板IN3
#define CC_1 GPIO_SetBits(STEP_GPIO_PORT, STEP_C_PIN)   // 电机控制口，连接电机驱动板IN3
#define DD_0 GPIO_ResetBits(STEP_GPIO_PORT, STEP_D_PIN) // 电机控制口，连接电机驱动板IN4
#define DD_1 GPIO_SetBits(STEP_GPIO_PORT, STEP_D_PIN)   // 电机控制口，连接电机驱动板IN4

// 外部变量声明
extern u8 zhengzhuan_flag, fanzhuan_flag;
extern u16 target_steps;
extern u8 pause_flag;                 // 暂停标志
extern u16 remaining_steps;           // 剩余步数
extern u8 saved_direction;            // 保存的方向
extern u16 current_count;             // 当前步数计数器
extern s32 absolute_position_steps;   // 绝对位置步数
extern s32 last_start_position_steps; // 上次运动开始时的位置
extern bool window_is_open;           // 窗口是否打开

void System_PB34_setIO(void);
void motor_init(void);
void MotorStep(uint8_t X, uint16_t Speed);
void MotorStop(void);
void Motor_Ctrl_Angle_F(int angle, int n);
void Motor_Ctrl_Angle_Z(int angle, int n);
void mortor_zhengzhuan(void);
void mortor_fanzhuan(void);
void motor_control(int angle, uint16_t time_ms);
void motor_pause(void);               // 暂停电机
void motor_resume(void);              // 继续电机
void motor_stop(void);                // 完全停止电机
void motor_home(uint16_t time_ms);    // 归位函数
void StepMotor_TIM3_IRQHandler(void); // TIM3更新中断处理（供stm32f10x_it.c调用）
void StepMotor_TIM4_IRQHandler(void); // TIM4更新中断处理（供stm32f10x_it.c调用）
// 以最快速度按圈数旋转（正为顺时针，负为逆时针）
void motor_rotate_turns_fast(int turns);
void open_window(void);
void close_window(void);
