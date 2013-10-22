#include <stdint.h>
#include <config.h>

#define OCI2C_BASE_ADDR	0xa0000000

#define OCI2C_PRERLO	0x0
#define OCI2C_PRERHI	0x1
#define OCI2C_CTR	0x2
#define OCI2C_TXR	0x3
#define OCI2C_RXR	0x3
#define OCI2C_CR	0x4
#define OCI2C_SR	0x4

#define OCI2C_CTR_EN	(1<<7)
#define OCI2C_CTR_IEN	(1<<6)

#define OCI2C_CR_STA	(1<<7)
#define OCI2C_CR_STO	(1<<6)
#define OCI2C_CR_RD	(1<<5)
#define OCI2C_CR_WR	(1<<4)
#define OCI2C_CR_ACK	(1<<3)
#define OCI2C_CR_IACK	(1<<0)

#define OCI2C_SR_RXACK	(1<<7)
#define OCI2C_SR_BUSY	(1<<6)
#define OCI2C_SR_AL	(1<<5)
#define OCI2C_SR_TIP	(1<<1)
#define OCI2C_SR_IF	(1<<0)

static void oci2c_write_reg(uint32_t reg, uint8_t value)
{
	*((volatile uint8_t *)(OCI2C_BASE_ADDR + reg)) = value;
}

static uint8_t oci2c_read_reg(uint32_t reg)
{
	return *((volatile uint8_t *)(OCI2C_BASE_ADDR + reg));
}

static void wait_for_transfer(void)
{
	while (oci2c_read_reg(OCI2C_SR) & OCI2C_SR_TIP)
		;
}

void oci2c_init(void)
{
	/* TODO: make this configurable... */
	uint16_t prescaler = BOARD_CLK_FREQ/(5*100e3) - 1;

	printf("SJK DEBUG: prescaler = %x\r\n", prescaler);

	/* Set prescaler */
	oci2c_write_reg(OCI2C_PRERHI, prescaler >> 8);
	oci2c_write_reg(OCI2C_PRERLO, prescaler & 0xff);

/* SJK DEBUG */
	prescaler = oci2c_read_reg(OCI2C_PRERHI);
	printf("SJK DEBUG: OCI2C_PRERHI = %x\r\n", prescaler);
	prescaler = oci2c_read_reg(OCI2C_PRERLO);
	printf("SJK DEBUG: OCI2C_PRERLO = %x\r\n", prescaler);

	/* Enable core */
	oci2c_write_reg(OCI2C_CTR, OCI2C_CTR_EN);
}

void oci2c_write_addr(uint8_t address, uint8_t rw)
{
	/* bit 0: 0 = write, 1 = read*/
	oci2c_write_reg(OCI2C_TXR, address << 1 | rw);
	oci2c_write_reg(OCI2C_CR, OCI2C_CR_STA | OCI2C_CR_WR);
	wait_for_transfer();
	if (oci2c_read_reg(OCI2C_SR) & OCI2C_SR_RXACK)
		printf("oci2c_write_addr: Error writing  0x%x\r\n", address);
}

void oci2c_write(uint8_t address, uint8_t *buf, uint32_t len, int stop)
{
	uint8_t control_reg = OCI2C_CR_WR;

	oci2c_write_addr(address, 0);
	while (len--) {
		oci2c_write_reg(OCI2C_TXR, *buf++);
		/* generate stop on last xfer */
		if (!len && stop)
			control_reg |= OCI2C_CR_STO;
		oci2c_write_reg(OCI2C_CR, control_reg);
		wait_for_transfer();

		if (oci2c_read_reg(OCI2C_SR) & OCI2C_SR_RXACK) {
			printf("oci2c_write: Error writing data to 0x%x\r\n",
			       address);
			return;
		}
	}
}

void oci2c_read(uint8_t address, uint8_t *buf, uint32_t len, int stop)
{
	uint8_t control_reg = OCI2C_CR_RD;

	oci2c_write_addr(address, 1);
	while (len--) {
		/* generate stop and ack on last xfer */
		if (!len && stop)
			control_reg |= OCI2C_CR_STO | OCI2C_CR_ACK;
		oci2c_write_reg(OCI2C_CR, control_reg);
		wait_for_transfer();

		*buf++ = oci2c_read_reg(OCI2C_RXR);
	}
}
