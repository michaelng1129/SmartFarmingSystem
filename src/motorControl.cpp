#include "config.h"
#include <Arduino.h>
#include <driver/mcpwm.h>

void setupFanMotor()
{
    pinMode(INA_PIN, OUTPUT);
    pinMode(INB_PIN, OUTPUT);

    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, INA_PIN);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, INB_PIN);

    mcpwm_config_t pwm_config_motor;
    pwm_config_motor.frequency = 500;
    pwm_config_motor.cmpr_a = 0;
    pwm_config_motor.cmpr_b = 0;
    pwm_config_motor.counter_mode = MCPWM_UP_COUNTER;
    pwm_config_motor.duty_mode = MCPWM_DUTY_MODE_0;

    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config_motor);
}

void setFanSpeed(float duty_cycle)
{
    if (duty_cycle > 0)
    {
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty_cycle);
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 0);
    }
    else
    {
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0);
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, -duty_cycle);
    }
}