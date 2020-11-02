/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <version.h>
#include <logging/log.h>
#include <stdlib.h>
#include <device.h>
#include <gpio.h>
#include <pwm.h>
#include <misc/util.h>
#include <misc/printk.h>
#include <pinmux.h>
#include "../boards/x86/galileo/board.h"
#include "../boards/x86/galileo/pinmux_galileo.h"
#include "../drivers/gpio/gpio_dw_registers.h"
#include "../drivers/gpio/gpio_dw.h"

#define EDGE_FALLING    (GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW)
#define EDGE_RISING	(GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH)
#define PULL_UP 	0	/* change this to enable pull-up/pull-down */
#define SLEEP_TIME	10	/* Sleep time */

int intno;		// numver of interrupts

static struct device *pinmux; 

u32_t Array[100];
u32_t start, stop, difference, Total, Average; 

struct data_item_type {
    u32_t field1;
    u32_t field2;
    u32_t field3;
};

void interrupt_cb(struct device *gpiob, struct gpio_callback *cb,
		    u32_t pins)
{
	stop = k_cycle_get_32();

	u32_t val;

	gpio_pin_read(gpiob, 6, &val);
	printk("Interrupt arrives at %d %x\n", k_cycle_get_32(), val);

	intno++;
}

static struct gpio_callback gpio_cb;

LOG_MODULE_REGISTER(app);

extern void foo(void);

void timer_expired_handler(struct k_timer *timer)
{
	LOG_INF("Timer expired.");

	/* Call another module to present logging from multiple sources. */
	foo();
}

K_TIMER_DEFINE(log_timer, timer_expired_handler, NULL);

static int cmd_log_test_start(const struct shell *shell, size_t argc,
			      char **argv, u32_t period)
{
	ARG_UNUSED(argv);

	k_timer_start(&log_timer, period, period);
	shell_print(shell, "Log test started\n");

	return 0;
}

static int cmd_log_test_start_demo(const struct shell *shell, size_t argc,
				   char **argv)
{
	return cmd_log_test_start(shell, argc, argv, 200);
}

static int cmd_log_test_start_flood(const struct shell *shell, size_t argc,
				    char **argv)
{
	return cmd_log_test_start(shell, argc, argv, 10);
}

static int cmd_log_test_stop(const struct shell *shell, size_t argc,
			     char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	k_timer_stop(&log_timer);
	shell_print(shell, "Log test stopped");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_log_test_start,
	SHELL_CMD_ARG(demo, NULL,
		  "Start log timer which generates log message every 200ms.",
		  cmd_log_test_start_demo, 1, 0),
	SHELL_CMD_ARG(flood, NULL,
		  "Start log timer which generates log message every 10ms.",
		  cmd_log_test_start_flood, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_STATIC_SUBCMD_SET_CREATE(sub_log_test,
	SHELL_CMD_ARG(start, &sub_log_test_start, "Start log test", NULL, 2, 0),
	SHELL_CMD_ARG(stop, NULL, "Stop log test.", cmd_log_test_stop, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(log_test, &sub_log_test, "Log test", NULL);

static int cmd_measure_1(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "Measure 1");

	struct device *gpiob, *pwm0;
	int ret, flag, iter, total_iter;

	u32_t gpio_d;
	
	printk("Start running gpio, interrupt, and pwm testing program \n");
	
	pinmux=device_get_binding(CONFIG_PINMUX_NAME);

	struct galileo_data *dev = pinmux->driver_data;
	
    	gpiob=dev->gpio_dw; 	//retrieving gpio_dw driver struct from pinmux driver
				// Alternatively, gpiob = device_get_binding(PORT);
	if (!gpiob) {
		printk("error\n");
		return;
	}

	pwm0 = dev->pwm0;
	if (!pwm0) {
		printk("error\n");
		return;
	}	
	
	ret=pinmux_pin_set(pinmux,2,PINMUX_FUNC_A); 	//IO2  - output
	if(ret<0)
		printk("error setting pin for IO2\n");
	
	ret=pinmux_pin_set(pinmux,3,PINMUX_FUNC_B); 	//IO3 for input interrupt
	if(ret<0)
		printk("error setting pin for IO3\n");

	ret=gpio_pin_configure(gpiob, 6,
			   PULL_UP| GPIO_DIR_IN | GPIO_INT | EDGE_RISING); //
	
	gpio_init_callback(&gpio_cb, interrupt_cb, BIT(6));

	ret=gpio_add_callback(gpiob, &gpio_cb);
	if(ret<0)
		printk("error adding callback\n");
	
	ret=gpio_pin_enable_callback(gpiob, 6);
	if(ret<0)
		printk("error enabling callback\n");
	struct gpio_dw_runtime *context = gpiob->driver_data;
	u32_t base_addr = context->base_addr;
	
	iter=0;
	flag=0;

	total_iter=0;

	while (total_iter<100) {
	
		//printk("Start time %d %x\n", start);

		gpio_pin_write(gpiob, 5, 0);
		k_sleep(SLEEP_TIME);
		start = k_cycle_get_32();
		gpio_pin_write(gpiob, 5, 1);
		k_sleep(SLEEP_TIME);

		//printk("Stop time %d %x\n", stop);

		difference = stop - start;

		printk("Difference in time %d \n", difference);

		for(int i=0;i<100;i++){
		Array[i]=difference;
		printk("%d\n",Array[i]);
		Total += Array[i];
		}

		Average = Total/100;
		printk("%d\n",Average);

		total_iter++;

	}


}

static int cmd_measure_2(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "Measure 2");


	struct device *gpiob, *pwm0;
	int ret, flag, iter, total_iter;

	u32_t gpio_d;
	
	printk("Start running gpio, interrupt, and pwm testing program \n");
	
	pinmux=device_get_binding(CONFIG_PINMUX_NAME);

	struct galileo_data *dev = pinmux->driver_data;
	
    	gpiob=dev->gpio_dw; 	//retrieving gpio_dw driver struct from pinmux driver
				// Alternatively, gpiob = device_get_binding(PORT);
	if (!gpiob) {
		printk("error\n");
		return;
	}

	pwm0 = dev->pwm0;
	if (!pwm0) {
		printk("error\n");
		return;
	}	


	ret=pinmux_pin_set(pinmux,2,PINMUX_FUNC_A); 	//IO2
	if(ret<0)
		printk("error setting pin for IO2\n");
	
	ret=pinmux_pin_set(pinmux,3,PINMUX_FUNC_B); 	//IO3 for input interrupt
	if(ret<0)
		printk("error setting pin for IO3\n");



	ret=pinmux_pin_set(pinmux,5,PINMUX_FUNC_C); 	//IO5 for pwm 7
	if(ret<0)
		printk("error setting pin for IO5\n");

	ret = pwm_pin_set_cycles(pwm0, 7, 0, 200);	// start pwm 7 with a small duty cycle
	if(ret<0)
		printk("error pwm_pin_set_cycles on PWM7\n");

	ret = pwm_pin_set_cycles(pwm0, 3, 0, 200);
	if(ret<0)
		printk("error pwm_pin_set_cycles on PWM3\n");
	
	ret=gpio_pin_configure(gpiob, 6,
			   PULL_UP| GPIO_DIR_IN | GPIO_INT | EDGE_RISING); 
	
	gpio_init_callback(&gpio_cb, interrupt_cb, BIT(6));

	ret=gpio_add_callback(gpiob, &gpio_cb);
	if(ret<0)
		printk("error adding callback\n");
	
	ret=gpio_pin_enable_callback(gpiob, 6);
	if(ret<0)
		printk("error enabling callback\n");
	struct gpio_dw_runtime *context = gpiob->driver_data;
	u32_t base_addr = context->base_addr;
	
	iter=0;
	flag=0;

	total_iter=0;

	while (total_iter<40) {
		u32_t val = 0;

		total_iter++; 
		iter++;
		if (iter ==8) 	
			iter=0;
			if (!flag)  
				ret = pwm_pin_set_cycles(pwm0, 3, 0, 3000); // switch to a large duty cycle
			
			else  
				ret = pwm_pin_set_cycles(pwm0, 3, 0, 200); 	// switch to a small duty cycle
			
		
		k_sleep(SLEEP_TIME);
	}

}

static int cmd_measure_3(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "Measure 3");

}

static int cmd_version(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Zephyr version %s", KERNEL_VERSION_STRING);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_measure,
	SHELL_CMD(1, NULL, "", cmd_measure_1),
	SHELL_CMD(2, NULL, "", cmd_measure_2),
	SHELL_CMD(3, NULL, "", cmd_measure_3),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(measure, &sub_measure, " ", NULL);

SHELL_CMD_ARG_REGISTER(version, NULL, "Show kernel version", cmd_version, 1, 0);



void main(void)
{


}
