#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <VL53L1X_api.h>

static void clock_setup(void)
{
	/* Enable GPIOD clock for LED & USARTs. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable clocks for USART2. */
	rcc_periph_clock_enable(RCC_USART2);
}

static void usart_setup(void)
{
	/* Setup USART2 parameters. */
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);
}

int _write(int file, char *ptr, int len)
{
	int i;

	if (file == STDOUT_FILENO || file == STDERR_FILENO) {
		for (i = 0; i < len; i++) {
			if (ptr[i] == '\n') {
				usart_send_blocking(USART2, '\r');
			}
			usart_send_blocking(USART2, ptr[i]);
		}
		return i;
	}
	errno = EIO;
	return -1;
}

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO5 on GPIO port A for LED. */
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5 | GPIO6);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO5);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO6);

	/* Setup GPIO pins for USART2 transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);

	/* Setup USART2 TX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2);
}

void i2c_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8 | GPIO9);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO8 | GPIO9);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO8 | GPIO9);

	rcc_periph_clock_enable(RCC_I2C1);
	rcc_periph_reset_pulse(RST_I2C1);
	i2c_set_speed(I2C1, i2c_speed_fm_400k, 50);
	i2c_peripheral_enable(I2C1);
}

void delay_setup(void)
{
	rcc_periph_clock_enable(RCC_TIM9);
	timer_set_prescaler(TIM9, rcc_apb1_frequency / 1e3 - 1); // millisecond counter
	timer_set_period(TIM9, 0xffff);
	timer_one_shot_mode(TIM9);
}

void delay_ms(uint16_t ms)
{
	timer_set_period(TIM9, ms);
	timer_enable_counter(TIM9);
	while (TIM_CR1(TIM9) & TIM_CR1_CEN);
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data) {
	uint8_t wrbuf[] = {
		index >> 8,
		index & 0xFF,
		data
	};
	i2c_transfer7(I2C1, dev, wrbuf, sizeof(wrbuf), 0, 0);
	return 0;
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data) {
	uint8_t wrbuf[] = {
		index >> 8,
		index & 0xFF,
		data >> 8,
		data & 0xFF
	};
	i2c_transfer7(I2C1, dev, wrbuf, sizeof(wrbuf), 0, 0);
	return 0;
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data) {
	uint8_t wrbuf[] = {
		index >> 8,
		index & 0xFF,
		(data >> 24) & 0xFF,
		(data >> 16) & 0xFF,
		data >> 8,
		data & 0xFF
	};
	i2c_transfer7(I2C1, dev, wrbuf, sizeof(wrbuf), 0, 0);
	return 0;
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data) {
	uint8_t addr[] = { index >> 8, index & 0xFF };
	i2c_transfer7(I2C1, dev, addr, 2, data, 1);
	return 0;
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data) {
	uint8_t addr[] = { index >> 8, index & 0xFF };
	uint8_t rdbuf[2];
	i2c_transfer7(I2C1, dev, addr, 2, rdbuf, 2);
	*data = (((uint16_t) rdbuf[0]) << 8) | rdbuf[1];
	return 0;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data) {
	uint8_t addr[] = { index >> 8, index & 0xFF };
	uint8_t rdbuf[4];
	i2c_transfer7(I2C1, dev, addr, 2, rdbuf, 4);
	*data = (((uint32_t) rdbuf[0]) << 24) | (((uint32_t) rdbuf[1]) << 16) | (((uint32_t) rdbuf[2]) << 8) | rdbuf[3];
	return 0;
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count){
	uint8_t addr[] = { index >> 8, index & 0xFF };
	i2c_transfer7(I2C1, dev, addr, 2, pdata, count);
	return 0;
}


int main(void)
{
	int j = 0, c = 0;

	clock_setup();
	gpio_setup();
	usart_setup();
	i2c_setup();
	delay_setup();

	/* Blink the LED (PD12) on the board with every transmitted byte. */
	for(int i = 0; i < 10; i++) {
		gpio_toggle(GPIOA, GPIO5);	/* LED on/off */
		usart_send_blocking(USART2, c + '0'); /* USART2: Send byte. */
		c = (c == 9) ? 0 : c + 1;	/* Increment c. */
		if ((j++ % 80) == 0) {		/* Newline after line full. */
			usart_send_blocking(USART2, '\r');
			usart_send_blocking(USART2, '\n');
		}
		delay_ms(100);
	}
	printf("\nStart test\n");

	uint16_t dev=0x29; // 7-bits addr
	int status=0;
	uint8_t sensorState=0;
	while(sensorState==0){
		status = VL53L1X_BootState(dev, &sensorState);
		delay_ms(2);
	}
	printf("Chip booted\n");

	status = VL53L1X_SensorInit(dev);
	status = VL53L1X_SetDistanceMode(dev, 1); /* 1=short, 2=long */
	status = VL53L1X_SetTimingBudgetInMs(dev, 20); /* in ms possible values [20, 50, 100, 200, 500] */
	status = VL53L1X_SetInterMeasurementInMs(dev, 20); /* in ms, IM must be > = TB */
	//status = VL53L1X_SetOffset(dev, 20); /* offset compensation in mm */
	//status = VL53L1X_SetROI(dev, 16, 16); /* minimum ROI 4,4 */
	//status = VL53L1X_CalibrateOffset(dev, 140, &offset); /* may take few second to perform the offset cal*/
	//status = VL53L1X_CalibrateXtalk(dev, 1000, &xtalk); /* may take few second to perform the xtalk cal */
	printf("VL53L1X Ultra Lite Driver Example running ...\n");
	status = VL53L1X_StartRanging(dev);   /* This function has to be called to enable the ranging */
	while(1){ /* read and display data */
		uint8_t dataReady = 0;
		uint8_t nbLoops = 0;
		while (dataReady == 0 && nbLoops < 50){
			status = VL53L1X_CheckForDataReady(dev, &dataReady);
			delay_ms(2);
			nbLoops++;
		}

		uint8_t RangeStatus;
		status = VL53L1X_GetRangeStatus(dev, &RangeStatus);

		uint16_t Distance;
		status = VL53L1X_GetDistance(dev, &Distance);

		uint16_t SignalRate;
		status = VL53L1X_GetSignalRate(dev, &SignalRate);

		uint16_t AmbientRate;
		status = VL53L1X_GetAmbientRate(dev, &AmbientRate);

		uint16_t SpadNum;
		status = VL53L1X_GetSpadNb(dev, &SpadNum);

		status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
		printf("%u, %u, %u, %u, %u\n", RangeStatus, Distance, SignalRate, AmbientRate, SpadNum);

		const uint16_t th_min = 100, th_max = 1300;
		bool touch = (RangeStatus == 0) && (Distance > th_min) && (Distance < th_max);
		if (touch) {
			gpio_set(GPIOA, GPIO5); // LED on
			gpio_clear(GPIOA, GPIO6); // OD output GND
		} else {
			gpio_clear(GPIOA, GPIO5); // LED off
			gpio_set(GPIOA, GPIO6); // OD output OPEN
		}
	}

	return 0;
}
