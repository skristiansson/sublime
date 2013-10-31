#include <stdint.h>
#include <i2c.h>
#include <delay.h>

#define SSM2603_DEVICE_ADDR		0x1a

/* Left-channel ADC input volume */
#define SSM2603_LEFT_ADC_VOL_REG	0x00
#define SSM2603_LRINBOTH		(1 << 8)
#define SSM2603_LINMUTE			(1 << 7)
#define SSM2603_LINVOL(x)		((x & 0x3f) << 0)

/* Right-channel ADC input volume */
#define SSM2603_RIGHT_ADC_VOL_REG	0x01
#define SSM2603_RLINBOTH		(1 << 8)
#define SSM2603_RINMUTE			(1 << 7)
#define SSM2603_RINVOL(x)		((x & 0x3f) << 0)

/* Left-channel DAC volume */
#define SSM2603_LEFT_DAC_VOL_REG	0x02
#define SSM2603_LRHPBOTH		(1 << 8)
#define SSM2603_LHPVOL(x)		((x & 0x7f) << 0)

/* Right-channel DAC volume */
#define SSM2603_RIGHT_DAC_VOL_REG	0x03
#define SSM2603_RLHPBOTH		(1 << 8)
#define SSM2603_RHPVOL(x)		((x & 0x7f) << 0)

/* Analog audio path */
#define SSM2603_ANALOG_AUDIO_PATH_REG	0x04
#define SSM2603_SIDETONE_ATT(x)		((x & 0x3) << 6)
#define SSM2603_SIDETONE_EN		(1 << 5)
#define SSM2603_DACSEL			(1 << 4)
#define SSM2603_BYPASS			(1 << 3)
#define SSM2603_INSEL			(1 << 2)
#define SSM2603_MUTEMIC			(1 << 1)
#define SSM2603_MICBOOST		(1 << 0)

/* Digital audio path */
#define SSM2603_DIGITAL_AUDIO_PATH_REG	0x05
#define SSM2603_HPOR			(1 << 4)
#define SSM2603_DACMU			(1 << 3)
#define SSM2603_DEEMPH(x)		((x & 0x3) << 1)
#define SSM2603_ADCHPF			(1 << 0)

/* Power management */
#define SSM2603_POWER_MANAGEMENT_REG	0x06
#define SSM2603_PWROFF			(1 << 7)
#define SSM2603_CLKOUT			(1 << 6)
#define SSM2603_OSC			(1 << 5)
#define SSM2603_OUT			(1 << 4)
#define SSM2603_DAC			(1 << 3)
#define SSM2603_ADC			(1 << 2)
#define SSM2603_MIC			(1 << 1)
#define SSM2603_LINEIN			(1 << 0)

/* Digital audio I/F */
#define SSM2603_DIGITAL_AUDIO_IF_REG	0x07
#define SSM2603_BCLKINV			(1 << 7)
#define SSM2603_MS			(1 << 6)
#define SSM2603_LRSWAP			(1 << 5)
#define SSM2603_LRP			(1 << 4)
#define SSM2603_WL(x)			((x & 0x3) << 2)
#define SSM2603_WL_16BIT		SSM2603_WL(0)
#define SSM2603_WL_20BIT		SSM2603_WL(1)
#define SSM2603_WL_24BIT		SSM2603_WL(2)
#define SSM2603_WL_32BIT		SSM2603_WL(3)
#define SSM2603_FORMAT(x)		((x & 0x3) << 0)

/* Sampling rate */
#define SSM2603_SAMPLING_RATE_REG	0x08
#define SSM2603_CLKODIV2		(1 << 7)
#define SSM2603_CLKDIV2			(1 << 6)
#define SSM2603_SR(x)			((x & 0xf) << 2)
#define SSM2603_BOSR			(1 << 1)
#define SSM2603_USB			(1 << 0)

/* Active */
#define SSM2603_ACTIVE_REG		0x09
#define SSM2603_ACTIVE			(1 << 0)

/* Software reset */
#define SSM2603_SW_RESET_REG		0x0f
#define SSM2603_SW_RESET(x)		((x & 0x1ff) << 0)

/* ALC Control 1 */
#define SSM2603_ALC_CTRL1_REG		0x10
#define SSM2603_ALCSEL(x)		((x & 0x3) << 7)
#define SSM2603_MAXGAIN(x)		((x & 0x7) << 4)
#define SSM2603_ALCL(x)			((x & 0xf) << 0)

/* ALC Control 2 */
#define SSM2603_ALC_CTRL2_REG		0x11
#define SSM2603_DCY(x)			((x & 0xf) << 4)
#define SSM2603_ATK(x)			((x & 0xf) << 0)

/* Noise gate */
#define SSM2603_NOISE_GATE_REG		0x12
#define SSM2603_NGTH(x)			((x & 0x1f) << 0)
#define SSM2603_NGG(x)			((x & 0x3) << 1)
#define SSM2603_NGAT			(1 << 0)

/*
 * Registers are 9-bit wide and address is 7-bit wide.
 * A write consists of two 8 bit i2c writes:
 * write 1: [7:1] = address[6:0], [0] = data[8]
 * write 2: [7:0] = data[7:0]
 */
void ssm2603_write_reg(uint8_t address, uint16_t data)
{
	uint8_t buf[2];

	buf[0] = (address << 1) | ((data >> 8) & 0x1);
	buf[1] = data & 0xff;
	i2c_write(SSM2603_DEVICE_ADDR, buf, /*len =*/ 2, /*stop =*/ 1);
}

uint16_t ssm2603_read_reg(uint8_t address)
{
	uint8_t buf[2];

	buf[0] = address << 1;
	i2c_write(SSM2603_DEVICE_ADDR, buf,  /*len =*/ 1, /*stop =*/ 0);
	i2c_read(SSM2603_DEVICE_ADDR, buf, /*len =*/ 2, /*stop =*/ 1);

	return (uint16_t)buf[1] << 8 | buf[0];
}

void ssm2603_init(void)
{
	uint16_t reg;


	/* Reset all registers to default */
	ssm2603_write_reg(SSM2603_SW_RESET_REG, 0);

	ssm2603_write_reg(SSM2603_POWER_MANAGEMENT_REG, SSM2603_OUT);

	/* Select DAC to the analog output path */
	reg = ssm2603_read_reg(SSM2603_ANALOG_AUDIO_PATH_REG) | SSM2603_DACSEL;
	ssm2603_write_reg(SSM2603_ANALOG_AUDIO_PATH_REG, reg);

	/* Unmute DAC */
	reg = ssm2603_read_reg(SSM2603_DIGITAL_AUDIO_PATH_REG) & ~SSM2603_DACMU;
	ssm2603_write_reg(SSM2603_DIGITAL_AUDIO_PATH_REG, reg);

	/* Set word length */
	reg = ssm2603_read_reg(SSM2603_DIGITAL_AUDIO_IF_REG);
	reg = (reg & ~SSM2603_WL(0xffff)) | SSM2603_WL_32BIT;
	ssm2603_write_reg(SSM2603_DIGITAL_AUDIO_IF_REG, reg);

	/* Set sampling rate (mclk/128)*/
	reg = ssm2603_read_reg(SSM2603_SAMPLING_RATE_REG);
	reg = (reg & ~SSM2603_SR(0xffff)) | SSM2603_SR(7);
	ssm2603_write_reg(SSM2603_SAMPLING_RATE_REG, reg);

	delay_ms(100);
	ssm2603_write_reg(SSM2603_ACTIVE_REG, SSM2603_ACTIVE);

	ssm2603_write_reg(SSM2603_POWER_MANAGEMENT_REG, 0);
}
