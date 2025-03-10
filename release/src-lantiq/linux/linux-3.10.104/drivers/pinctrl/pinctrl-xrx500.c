/*
 *  linux/drivers/pinctrl/pinmux-xrx500.c
 *  based on linux/drivers/pinctrl/pinmux-falcon.c
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2014 Kavitha  Subramanian <s.kavitha.EE@lantiq.com>
 */
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/klogging.h>

#ifdef CONFIG_PINCTRL_SYSFS
#include <linux/device.h>
#include <linux/kdev_t.h>
#endif

#include "pinctrl-lantiq.h"

#include <lantiq_soc.h>

#define PORTS			2
#define PINS			32
#define PORT(x)			(x / PINS)
#define PORT_PIN(x)		(x % PINS)

#define MUX_REG_OFF				0x100
#define MUX_BASE(p)				(MUX_REG_OFF * PORT(p))

#define LTQ_BBSPI_EN_MASK BIT(18)

/* Multiplexer Control Register */
#define LTQ_PADC_MUX(p)			((PORT_PIN(p) * 0x4) + MUX_BASE(p))
/* Pull Up Enable Register */
#define LTQ_PADC_PUEN			0x80
/* Pull Down Enable Register */
#define LTQ_PADC_PDEN			0x84
/* Slew Rate Control Register */
#define LTQ_PADC_SRC			0x88
/* Drive Current Control Register */
#define LTQ_PADC_DCC0			0x8C
/* Drive Current Control Register */
#define LTQ_PADC_DCC1			0x90
/* Open Drain Register */
#define LTQ_PADC_OD				0x94

/* Pad Control Availability Register */
#define LTQ_PADC_AVAIL          0x98

#define pad_r32(p, reg)		ltq_r32(p + reg)
#define pad_w32(p, val, reg)	ltq_w32(val, p + reg)
#define pad_w32_mask(c, clear, set, reg) \
		pad_w32(c, (pad_r32(c, reg) & ~(clear)) | (set), reg)

#define pad_getbit(m, r, p)	(!!(ltq_r32(m + r) & (1 << p)))
#define pad_setbit(m, r, p)	ltq_w32_mask(0, BIT(p), m + r)
#define pad_clearbit(m, r, p)	ltq_w32_mask(BIT(p), 0, m + r)

#define MFP_XRX500(a, f0, f1, f2, f3)		\
{						\
	.name = #a,				\
	.pin = a,				\
	.func = {				\
		XRX500_MUX_##f0,		\
		XRX500_MUX_##f1,		\
		XRX500_MUX_##f2,		\
		XRX500_MUX_##f3,		\
	},					\
}

#define GRP_MUX(a, m, p)	\
{				\
	.name = a,		\
	.mux = XRX500_MUX_##m,	\
	.pins = p,		\
	.npins = ARRAY_SIZE(p),	\
}

enum xrx500_mux {
	XRX500_MUX_GPIO = 0,
	XRX500_MUX_RST,
	XRX500_MUX_NTR,
	XRX500_MUX_PPS,
	XRX500_MUX_MDIO,
	XRX500_MUX_LEDC,
	XRX500_MUX_SPI0,
	XRX500_MUX_SPI1,
	XRX500_MUX_ASC,
	XRX500_MUX_I2C,
	XRX500_MUX_HOSTIF,
	XRX500_MUX_SLIC,
	XRX500_MUX_JTAG,
	XRX500_MUX_PCM,
	XRX500_MUX_MII,
	XRX500_MUX_PHY,
	XRX500_MUX_WLAN,
	XRX500_MUX_USB,
	XRX500_MUX_CGU,
	XRX500_MUX_EBU,
	XRX500_MUX_TDM,
	XRX500_MUX_NONE = 0xffff,
};

#ifdef CONFIG_PINCTRL_SYSFS
struct sys_lantiq_pin_desc {
	unsigned long		flags;
	/* flag symbols are bit numbers */
	#define FLAG_EXPORT	0	/* protected by sysfs_lock */
};
static struct sys_lantiq_pin_desc pin_desc_array[PORTS * PINS];
static struct platform_device *pinctrl_platform_dev;
#endif

static struct pinctrl_pin_desc xrx500_pads[PORTS * PINS];
static int pad_count[PORTS];
static void lantiq_load_pin_desc(struct pinctrl_pin_desc *d, int bank, int len)
{
	int base = bank * PINS;
	int i;
	for (i = 0; i < len; i++) {
		/* strlen("ioXYZ") + 1 = 6 */
		char *name = kzalloc(6, GFP_KERNEL);
		if(name) {
			snprintf(name, 6, "io%d", base + i);
			d[i].name = name;
		} else {
			LOGF_KLOG_ERROR("Failed to alloc for pin desc name\r\n");
		}
		d[i].number = base + i;
	}
	pad_count[bank] = len;
}

/* Reset default values table */
static uint32_t default_reg_bank0[] = {
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
0x1, 0x0, 0x0, 0x0, 0x1, 0x1, 0x1, 0x1,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xF1EFEFFF
};
static uint32_t default_reg_bank1[] = {
0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
0x1, 0x0, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x3BFF0C1F
};

static struct ltq_mfp_pin xrx500_mfp[] = {
	/*	pin		f0	f1	f2	f3 */
	MFP_XRX500(GPIO0,	GPIO,   NONE,	PHY,   NONE),
	MFP_XRX500(GPIO1,	GPIO,   PHY,	SLIC,   NONE),
	MFP_XRX500(GPIO2,	GPIO,   PHY,	USB,   NONE),
	MFP_XRX500(GPIO3,	GPIO,   NONE,	CGU,   NTR),
	MFP_XRX500(GPIO4,	GPIO,   LEDC,	PHY,  PHY),
	MFP_XRX500(GPIO5,	GPIO,   LEDC,	PHY,   PHY),
	MFP_XRX500(GPIO6,	GPIO,   LEDC,	PHY,  PHY),
	MFP_XRX500(GPIO7,	GPIO,   CGU,	PHY,   USB),
	MFP_XRX500(GPIO8,	GPIO,   CGU,	NONE,   PHY),
	MFP_XRX500(GPIO9,	GPIO,   WLAN,	PHY,   NONE),
	MFP_XRX500(GPIO10,	GPIO,   SPI1,	SPI0,   NONE),
	MFP_XRX500(GPIO11,	GPIO,   SPI1,	WLAN,   SPI0),
	MFP_XRX500(GPIO12,	NONE,   NONE,	NONE,   NONE),
	MFP_XRX500(GPIO13,	GPIO,   EBU,	NONE,   NONE),
	MFP_XRX500(GPIO14,	GPIO,   NONE,	SPI1,   PHY),
	MFP_XRX500(GPIO15,	GPIO,   SPI0,	PHY,   NONE),
	MFP_XRX500(GPIO16,	GPIO,   SPI0,	NONE,   PHY),
	MFP_XRX500(GPIO17,	GPIO,	SPI0,	NONE,	PHY),
	MFP_XRX500(GPIO18,	GPIO,	SPI0,	NONE,	PHY),
	MFP_XRX500(GPIO19,	GPIO,	SPI1,	NONE,	PHY),
	MFP_XRX500(GPIO20,	NONE,	NONE,	NONE,	NONE),
	MFP_XRX500(GPIO21,	GPIO,	I2C,	NONE,	NONE),
	MFP_XRX500(GPIO22,	GPIO,	I2C,	NONE,	PHY),
	MFP_XRX500(GPIO23,	GPIO,	EBU,	NONE,	NONE),
	MFP_XRX500(GPIO24,	GPIO,	EBU,	NONE,	NONE),
	MFP_XRX500(GPIO25,	NONE,	NONE,	NONE,	NONE),
	MFP_XRX500(GPIO26,	NONE,	NONE,	NONE,	NONE),
	MFP_XRX500(GPIO27,	NONE,	NONE,	NONE,	NONE),
	MFP_XRX500(GPIO28,	GPIO,	MII,	PHY,	TDM),
	MFP_XRX500(GPIO29,	GPIO,	MII,	PHY,	TDM),
	MFP_XRX500(GPIO30,	GPIO,	MII,	PHY,	TDM),
	MFP_XRX500(GPIO31,	GPIO,	MII,	NONE,	TDM),

	MFP_XRX500(GPIO32,	GPIO,   MDIO,	NONE,   NONE),
	MFP_XRX500(GPIO33,	GPIO,   MDIO,	NONE,   NONE),
	MFP_XRX500(GPIO34,	GPIO,   NONE,	SLIC,   PHY),
	MFP_XRX500(GPIO35,	GPIO,   NONE,	SLIC,   PHY),
	MFP_XRX500(GPIO36,	GPIO,   NONE,	SLIC,  PHY),
	MFP_XRX500(GPIO37,	NONE,	NONE,	NONE,   NONE),
	MFP_XRX500(GPIO38,	NONE,	NONE,	NONE,   NONE),
	MFP_XRX500(GPIO39,	NONE,	NONE,	NONE,   NONE),
	MFP_XRX500(GPIO40,	NONE,	NONE,	NONE,   NONE),
	MFP_XRX500(GPIO41,	NONE,	NONE,	NONE,   NONE),
	MFP_XRX500(GPIO42,	GPIO,   MDIO,	NONE,  NONE),
	MFP_XRX500(GPIO43,	GPIO,   MDIO,	NONE,  NONE),
	MFP_XRX500(GPIO44,	NONE,   NONE,	NONE,  NONE),
	MFP_XRX500(GPIO45,	NONE,   NONE,	NONE,  NONE),
	MFP_XRX500(GPIO46,	NONE,   NONE,	NONE,  NONE),
	MFP_XRX500(GPIO47,	NONE,   NONE,	NONE,  NONE),
	MFP_XRX500(GPIO48,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO49,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO50,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO51,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO52,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO53,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO54,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO55,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO56,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO57,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO58,	NONE,   NONE,	NONE,  NONE),
	MFP_XRX500(GPIO59,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO60,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO61,	GPIO,   EBU,	NONE,  NONE),
	MFP_XRX500(GPIO62,	NONE,   NONE,	NONE,  NONE),
	MFP_XRX500(GPIO63,	NONE,   NONE,	NONE,  NONE),
};

static const unsigned pins_spi0[] = {GPIO16, GPIO17, GPIO18};
static const unsigned pins_spi0_cs1[] = {GPIO15};
static const unsigned pins_spi0_cs4[] = {GPIO10};
static const unsigned pins_spi0_cs6[] = {GPIO11};
static const unsigned pins_spi1[] = {GPIO10, GPIO11, GPIO19};
static const unsigned pins_spi1_cs0[] = {GPIO14};
static const unsigned pins_ledc[] = {GPIO4, GPIO5, GPIO6};
static const unsigned pins_nand_ale[] = {GPIO13};
static const unsigned pins_i2c[] = {GPIO21, GPIO22};
static const unsigned pins_nand_cs1[] = {GPIO23};
static const unsigned pins_nand_cle[] = {GPIO24};
static const unsigned pins_nand_rdy[] = {GPIO48};
static const unsigned pins_nand_rd[] = {GPIO49};
static const unsigned pins_nand_d1[] = {GPIO50};
static const unsigned pins_nand_d0[] = {GPIO51};
static const unsigned pins_nand_d2[] = {GPIO52};
static const unsigned pins_nand_d7[] = {GPIO53};
static const unsigned pins_nand_d6[] = {GPIO54};
static const unsigned pins_nand_d5[] = {GPIO55};
static const unsigned pins_nand_d4[] = {GPIO56};
static const unsigned pins_nand_d3[] = {GPIO57};
static const unsigned pins_nand_wr[] = {GPIO59};
static const unsigned pins_nand_wp[] = {GPIO60};
static const unsigned pins_nand_se[] = {GPIO61};
static const unsigned pins_mdio_l[] = {GPIO42, GPIO43};
static const unsigned pins_mdio_r[] = {GPIO32, GPIO33};
static const unsigned pins_vcodec[] = {GPIO1, GPIO34, GPIO35, GPIO36};
static const unsigned pins_clkout0[] = {GPIO8};
static const unsigned pins_tdm[] = {GPIO28, GPIO29, GPIO30, GPIO31};
static const unsigned pins_25MHz[] = {GPIO3};

static struct ltq_pin_group xrx500_grps[] = {
	GRP_MUX("spi0", SPI0, pins_spi0),
	GRP_MUX("spi0_cs1", SPI0, pins_spi0_cs1),
	GRP_MUX("spi0_cs4", SPI0, pins_spi0_cs4),
	GRP_MUX("spi0_cs6", SPI0, pins_spi0_cs6),
	GRP_MUX("spi1", SPI1, pins_spi1),
	GRP_MUX("spi1_cs0", SPI1, pins_spi1_cs0),
	GRP_MUX("ledc", LEDC, pins_ledc),
	GRP_MUX("nand ale", EBU, pins_nand_ale),
	GRP_MUX("i2c", I2C, pins_i2c),
	GRP_MUX("nand cs1", EBU, pins_nand_cs1),
	GRP_MUX("nand cle", EBU, pins_nand_cle),
	GRP_MUX("nand rdy", EBU, pins_nand_rdy),
	GRP_MUX("nand rd", EBU, pins_nand_rd),
	GRP_MUX("nand d1", EBU, pins_nand_d1),
	GRP_MUX("nand d0", EBU, pins_nand_d0),
	GRP_MUX("nand d2", EBU, pins_nand_d2),
	GRP_MUX("nand d7", EBU, pins_nand_d7),
	GRP_MUX("nand d6", EBU, pins_nand_d6),
	GRP_MUX("nand d5", EBU, pins_nand_d5),
	GRP_MUX("nand d4", EBU, pins_nand_d4),
	GRP_MUX("nand d3", EBU, pins_nand_d3),
	GRP_MUX("nand wr", EBU, pins_nand_wr),
	GRP_MUX("nand wp", EBU, pins_nand_wp),
	GRP_MUX("nand se", EBU, pins_nand_se),
	GRP_MUX("mdio_l", MDIO, pins_mdio_l),
	GRP_MUX("mdio_r", MDIO, pins_mdio_r),
	GRP_MUX("clkout0", CGU, pins_clkout0),
	GRP_MUX("25MHz", CGU, pins_25MHz),
	GRP_MUX("vcodec", SLIC, pins_vcodec),
	GRP_MUX("tdm", TDM, pins_tdm),
};

static const char * const xrx500_spi0_grps[] = {"spi0", "spi0_cs1",
						"spi0_cs4", "spi0_cs6"};

static const char * const xrx500_spi1_grps[] = {"spi1", "spi1_cs0"};
static const char * const xrx500_ledc_grps[] = {"ledc"};
static const char * const xrx500_i2c_grps[] = {"i2c"};
static const char * const xrx500_ebu_grps[] = { "nand ale", "nand cs1",
						"nand cle", "nand rdy",
						"nand rd",	"nand d1",
						"nand d0", "nand d2",
						"nand d7", "nand d6",
						"nand d5", "nand d4",
						"nand d3", "nand wr",
						"nand wp", "nand se"};
static const char * const xrx500_mdio_grps[] = {"mdio_l", "mdio_r"};
static const char * const xrx500_vcodec_grps[] = {"vcodec", "clkout0"};
static const char * const xrx500_25MHz_grps[] = {"25MHz"};
static const char * const xrx500_tdm_grps[] = {"tdm"};

static struct ltq_pmx_func xrx500_funcs[] = {
	{"spi0",	ARRAY_AND_SIZE(xrx500_spi0_grps)},
	{"spi1",	ARRAY_AND_SIZE(xrx500_spi1_grps)},
	{"ledc",	ARRAY_AND_SIZE(xrx500_ledc_grps)},
	{"ebu",		ARRAY_AND_SIZE(xrx500_ebu_grps)},
	{"i2c",		ARRAY_AND_SIZE(xrx500_i2c_grps)},
	{"25MHz",	ARRAY_AND_SIZE(xrx500_25MHz_grps)},
	{"mdio",	ARRAY_AND_SIZE(xrx500_mdio_grps)},
	{"vcodec",	ARRAY_AND_SIZE(xrx500_vcodec_grps)},
	{"tdm", 	ARRAY_AND_SIZE(xrx500_tdm_grps)}
};

/* ---------  pinconf related code --------- */
static int xrx500_pinconf_group_get(struct pinctrl_dev *pctrldev,
				unsigned group, unsigned long *config)
{
	return -ENOTSUPP;
}

static int xrx500_pinconf_group_set(struct pinctrl_dev *pctrldev,
				unsigned group, unsigned long config)
{
	return -ENOTSUPP;
}

static int xrx500_pinconf_get(struct pinctrl_dev *pctrldev,
				unsigned pin, unsigned long *config)
{
	struct ltq_pinmux_info *info = pinctrl_dev_get_drvdata(pctrldev);
	enum ltq_pinconf_param param = LTQ_PINCONF_UNPACK_PARAM(*config);
	void __iomem *mem = info->membase[PORT(pin)];
	int temp;
	switch (param) {
	case LTQ_PINCONF_PARAM_DRIVE_CURRENT:
		temp = (PORT_PIN(pin) <= 15) ? LTQ_PADC_DCC0 : LTQ_PADC_DCC1;
		*config = LTQ_PINCONF_PACK(param,
		!!pad_getbit(mem,
		temp,
		PORT_PIN(pin)));
		break;

	case LTQ_PINCONF_PARAM_SLEW_RATE:
		*config = LTQ_PINCONF_PACK(param,
			!!pad_getbit(mem, LTQ_PADC_SRC, PORT_PIN(pin)));
		break;

	case LTQ_PINCONF_PARAM_PULL:
		if (pad_getbit(mem, LTQ_PADC_PDEN, PORT_PIN(pin)))
			*config = LTQ_PINCONF_PACK(param, 1);
		else if (pad_getbit(mem, LTQ_PADC_PUEN, PORT_PIN(pin)))
			*config = LTQ_PINCONF_PACK(param, 2);
		else
			*config = LTQ_PINCONF_PACK(param, 0);

		break;

	case LTQ_PINCONF_PARAM_OPEN_DRAIN:
		*config = LTQ_PINCONF_PACK(param,
			pad_getbit(mem, LTQ_PADC_OD, PORT_PIN(pin)));
		break;

	default:
		return -ENOTSUPP;
	}

	return 0;
}

static int xrx500_pinconf_set(struct pinctrl_dev *pctrldev,
			unsigned pin, unsigned long config)
{
	enum ltq_pinconf_param param = LTQ_PINCONF_UNPACK_PARAM(config);
	int arg = LTQ_PINCONF_UNPACK_ARG(config);
	struct ltq_pinmux_info *info = pinctrl_dev_get_drvdata(pctrldev);
	void __iomem *mem = info->membase[PORT(pin)];
	u32 reg;

	LOGF_KLOG_DEBUG("%s called with pin: %d and param:%x\n",
__func__, pin, param);
	switch (param) {
	case LTQ_PINCONF_PARAM_DRIVE_CURRENT:
		reg = (PORT_PIN(pin) <= 15) ? LTQ_PADC_DCC0 : LTQ_PADC_DCC1;
		break;

	case LTQ_PINCONF_PARAM_SLEW_RATE:
		reg = LTQ_PADC_SRC;
		break;

	case LTQ_PINCONF_PARAM_PULL:
		if (arg == 1)
			reg = LTQ_PADC_PDEN;
		else
			reg = LTQ_PADC_PUEN;
		break;

	case LTQ_PINCONF_PARAM_OPEN_DRAIN:
		reg = LTQ_PADC_OD;
		break;
	default:
		LOGF_KLOG_ERROR("%s: Invalid config param %04x\n",
		pinctrl_dev_get_name(pctrldev), param);
		return -ENOTSUPP;
	}
	pad_setbit(mem, reg, PORT_PIN(pin));
	LOGF_KLOG_DEBUG("DIR0: %x\n", pad_r32(mem, reg));
	return 0;
}

static void xrx500_pinconf_dbg_show(struct pinctrl_dev *pctrldev,
			struct seq_file *s, unsigned offset)
{
	unsigned long config;
	struct pin_desc *desc;
	char buf[64];

	struct ltq_pinmux_info *info = pinctrl_dev_get_drvdata(pctrldev);
	int port = PORT(offset);
	sprintf(buf, " (port %d) mux %d -- ", port,
		pad_r32(info->membase[port], LTQ_PADC_MUX(PORT_PIN(offset))));
	seq_puts(s, buf);

	config = LTQ_PINCONF_PACK(LTQ_PINCONF_PARAM_PULL, 0);
	if (!xrx500_pinconf_get(pctrldev, offset, &config)) {
		sprintf(buf, "pull %d ",
			(int)LTQ_PINCONF_UNPACK_ARG(config));
	}

	config = LTQ_PINCONF_PACK(LTQ_PINCONF_PARAM_DRIVE_CURRENT, 0);
	if (!xrx500_pinconf_get(pctrldev, offset, &config)) {
		seq_printf(s, "drive-current %d ",
			(int)LTQ_PINCONF_UNPACK_ARG(config));
		seq_puts(s, buf);
	}

	config = LTQ_PINCONF_PACK(LTQ_PINCONF_PARAM_SLEW_RATE, 0);
	if (!xrx500_pinconf_get(pctrldev, offset, &config)) {
		sprintf(buf, "slew-rate %d ",
			(int)LTQ_PINCONF_UNPACK_ARG(config));
		seq_puts(s, buf);
	}

	desc = pin_desc_get(pctrldev, offset);
	if (desc) {
		if (desc->gpio_owner) {
			sprintf(buf, " owner: %s", desc->gpio_owner);
			seq_puts(s, buf);
		}
	} else {
		seq_puts(s, "not registered");
	}
}

static void xrx500_pinconf_group_dbg_show(struct pinctrl_dev *pctrldev,
			struct seq_file *s, unsigned selector)
{
}

static const struct pinconf_ops xrx500_pinconf_ops = {
	.pin_config_get			= xrx500_pinconf_get,
	.pin_config_set			= xrx500_pinconf_set,
	.pin_config_group_get		= xrx500_pinconf_group_get,
	.pin_config_group_set		= xrx500_pinconf_group_set,
	.pin_config_dbg_show		= xrx500_pinconf_dbg_show,
	.pin_config_group_dbg_show	= xrx500_pinconf_group_dbg_show,
};

static struct pinctrl_desc xrx500_pctrl_desc = {
	.owner		= THIS_MODULE,
	.pins		= xrx500_pads,
	.confops	= &xrx500_pinconf_ops,
};

static inline int xrx500_mux_apply(struct pinctrl_dev *pctrldev,
			int mfp, int mux)
{
	struct ltq_pinmux_info *info = pinctrl_dev_get_drvdata(pctrldev);
	int port = PORT(info->mfp[mfp].pin);
	LOGF_KLOG_DEBUG("%s called with mux: %d and pin %d\n", __func__,
	mux, info->mfp[mfp].pin);
	if ((port >= PORTS) || (!info->membase[port]))
		return -ENODEV;

	LOGF_KLOG_DEBUG("writing value:%d to register:%x\n", mux,
(unsigned int) info->membase[port] + (PORT_PIN(info->mfp[mfp].pin) * 4));
	pad_w32(info->membase[port], mux, (PORT_PIN(info->mfp[mfp].pin) * 4));
	return 0;
}

static const struct ltq_cfg_param xrx500_cfg_params[] = {
	{"lantiq,pull",			LTQ_PINCONF_PARAM_PULL},
	{"lantiq,drive-current",	LTQ_PINCONF_PARAM_DRIVE_CURRENT},
	{"lantiq,slew-rate",		LTQ_PINCONF_PARAM_SLEW_RATE},
	{"lantiq,open-drain",		LTQ_PINCONF_PARAM_OPEN_DRAIN},
};

static struct ltq_pinmux_info xrx500_info = {
	.desc		= &xrx500_pctrl_desc,
	.apply_mux	= xrx500_mux_apply,
	.params		= xrx500_cfg_params,
	.num_params	= ARRAY_SIZE(xrx500_cfg_params),
};

int pinctrl_xrx500_get_range_size(int id)
{
	u32 avail;

	if ((id >= PORTS) || (!xrx500_info.membase[id]))
		return -EINVAL;

	avail = pad_r32(xrx500_info.membase[id], LTQ_PADC_AVAIL);
	return fls(avail);
}
void pinctrl_xrx500_add_gpio_range(struct pinctrl_gpio_range *range)
{
	pinctrl_add_gpio_range(xrx500_info.pctrl, range);
	LOGF_KLOG_DEBUG("GPIO range %d base %d\n", range->npins, range->base);
}

#ifdef CONFIG_PINCTRL_SYSFS
/* lock protects against unexport_pin() being called while
 * sysfs files are active.
 */
static DEFINE_MUTEX(sysfs_lock);

static ssize_t export_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t len);
static ssize_t unexport_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t len);
static struct class_attribute pinctrl_class_attrs[] = {
	__ATTR(export, 0200, NULL, export_store),
	__ATTR(unexport, 0200, NULL, unexport_store),
	__ATTR_NULL,
};

static struct class pinctrl_class = {
	.name =         "pinctrl",
	.owner =        THIS_MODULE,

	.class_attrs =  pinctrl_class_attrs,
};

static ssize_t pad_ctrl_avail_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;
	ssize_t			status;
	u32 pin, value;
	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	mutex_lock(&sysfs_lock);
	if (!test_bit((FLAG_EXPORT), &(desc->flags)))
		status = -EIO;
	else {
		value = pad_r32(info->membase[PORT(pin)], LTQ_PADC_AVAIL);
		value = (value & BIT(PORT_PIN(pin))) >> PORT_PIN(pin);
		if (value)
			status = sprintf(buf, "%s\r\n", "AV");
		else
			status = sprintf(buf, "%s\r\n", "NAV");
	}
	mutex_unlock(&sysfs_lock);

	return status;
}


static const DEVICE_ATTR(padctrl_availability, 0644,
		pad_ctrl_avail_show, NULL);

static ssize_t pin_pullup_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;

	ssize_t status = 0;
	u32 pin;
	int port;
	u32 reg;
	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);
	mutex_lock(&sysfs_lock);
	if (!test_bit((FLAG_EXPORT), &(desc->flags))) {
		status = -EIO;
	} else {
		reg = LTQ_PADC_PUEN;
		if (pad_getbit(info->membase[PORT(pin)], reg, PORT_PIN(pin)))
			status = sprintf(buf, "%s\n", "EN");
		else
			status = sprintf(buf, "%s\n", "DIS");
	}
	mutex_unlock(&sysfs_lock);

	return status;
}

static ssize_t pin_pullup_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;
	ssize_t			status = 0;
	u32 pin;
	int port;
	u32 reg;
	long value;
	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);
	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	if (sysfs_streq(buf, "DIS"))
		value = 0;
	else if (sysfs_streq(buf, "EN"))
		value = 1;
	else
		status = -EINVAL;
	if (status == 0) {
		reg = LTQ_PADC_PUEN;
		if (value == 0)
			pad_clearbit(info->membase[PORT(pin)],
reg, PORT_PIN(pin));
		else
			pad_setbit(info->membase[PORT(pin)],
reg, PORT_PIN(pin));
	}
	mutex_unlock(&sysfs_lock);
	return status ? : size;
}

static const DEVICE_ATTR(pullup, 0644,
		pin_pullup_show, pin_pullup_store);

static ssize_t pin_pulldown_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;

	ssize_t status = 0;
	u32 pin;
	int port;
	u32 reg;
	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);

	mutex_lock(&sysfs_lock);
	if (!test_bit((FLAG_EXPORT), &(desc->flags))) {
		status = -EIO;
	} else {
		reg = LTQ_PADC_PDEN;
		if (pad_getbit(info->membase[PORT(pin)], reg, PORT_PIN(pin)))
			status = sprintf(buf, "%s\n", "EN");
		else
			status = sprintf(buf, "%s\n", "DIS");
	}
	mutex_unlock(&sysfs_lock);

	return status;
}

static ssize_t pin_pulldown_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{

	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;
	ssize_t			status = 0;
	u32 pin;
	int port;
	u32 reg;
	long value;

	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);
	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;


	if (sysfs_streq(buf, "DIS"))
		value = 0;
	else if (sysfs_streq(buf, "EN"))
		value = 1;
	else
		status = -EINVAL;

	if (status == 0) {
		reg = LTQ_PADC_PDEN;

		if (value == 0)
			pad_clearbit(info->membase[PORT(pin)],
reg, PORT_PIN(pin));
		else
			pad_setbit(info->membase[PORT(pin)],
reg, PORT_PIN(pin));
	}
	mutex_unlock(&sysfs_lock);
	return status ? : size;
}

static const DEVICE_ATTR(pulldown, 0644,
		pin_pulldown_show, pin_pulldown_store);

static ssize_t pin_slewrate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;

	ssize_t status = 0;
	u32 pin;
	int port;
	u32 reg;
	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);

	mutex_lock(&sysfs_lock);
	if (!test_bit((FLAG_EXPORT), &(desc->flags))) {
		status = -EIO;
	} else {
		reg = LTQ_PADC_SRC;
		if (pad_getbit(info->membase[PORT(pin)],
reg, PORT_PIN(pin)))
			status = sprintf(buf, "%d\n", 1);
		else
			status = sprintf(buf, "%d\n", 0);
	}
	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t pin_slewrate_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{

	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;
	ssize_t			status = 0;
	u32 pin;
	int port;
	u32 reg;
	long value;
	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);
	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;

	status = kstrtol(buf, 0, &value);
	if (status == 0) {
		reg = LTQ_PADC_SRC;
		if (value == 0)
			pad_clearbit(info->membase[PORT(pin)],
reg, PORT_PIN(pin));
		else
			pad_setbit(info->membase[PORT(pin)],
reg, PORT_PIN(pin));
	}
	mutex_unlock(&sysfs_lock);
	return status ? : size;
}

static const DEVICE_ATTR(slewrate, 0644,
		pin_slewrate_show, pin_slewrate_store);

static ssize_t pin_opendrain_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;

	ssize_t status = 0;
	u32 pin;
	int port;
	u32 reg;

	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);

	mutex_lock(&sysfs_lock);
	if (!test_bit((FLAG_EXPORT), &(desc->flags))) {
		status = -EIO;
	} else {
		reg = LTQ_PADC_OD;
		if (pad_getbit(info->membase[PORT(pin)],
reg, PORT_PIN(pin)))
			status = sprintf(buf, "%s\n", "EN");
		else
			status = sprintf(buf, "%s\n", "NOP");
	}
	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t pin_opendrain_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{

	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;
	ssize_t			status = 0;
	u32 pin;
	int port;
	u32 reg;
	long value;

	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);
	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	if (sysfs_streq(buf, "NOP"))
		value = 0;
	else if (sysfs_streq(buf, "EN"))
		value = 1;
	else
		status = -EINVAL;

	if (status == 0) {
		reg = LTQ_PADC_OD;
		if (value == 0)
			pad_clearbit(info->membase[PORT(pin)],
reg, PORT_PIN(pin));
		else
			pad_setbit(info->membase[PORT(pin)],
reg, PORT_PIN(pin));
	}
	mutex_unlock(&sysfs_lock);
	return status ? : size;
}

static const DEVICE_ATTR(opendrain, 0644,
		pin_opendrain_show, pin_opendrain_store);

static ssize_t currentcontrol_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;
	ssize_t status = 0;
	u32 pin;
	int port;
	u32 drive_ctrl, reg;

	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);

	mutex_lock(&sysfs_lock);
	if (!test_bit((FLAG_EXPORT), &(desc->flags))) {
		status = -EIO;
	} else {
		reg = (PORT_PIN(pin) <= 15) ? LTQ_PADC_DCC0 : LTQ_PADC_DCC1;
		drive_ctrl = pad_r32(info->membase[port], reg)
>> ((pin % 16) * 2);
		status = sprintf(buf, "%d\n", drive_ctrl);
	}

	mutex_unlock(&sysfs_lock);

	return status;
}

static ssize_t currentcontrol_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;
	ssize_t			status = 0;
	u32 pin;
	int port;
	long int value, temp, reg;

	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);
	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags)) {
		status = -EIO;
	} else {
		status = kstrtol(buf, 0, &value);
		if (status == 0) {
			if ((value >= 0) && (value <= 3)) {
				value = value << ((pin % 16) * 2);
				LOGF_KLOG_DEBUG("value %ld 0x%x 0x%x\r\n",
				value, (unsigned int)info->membase[port],
				(PORT_PIN(pin) <= 15)
				? LTQ_PADC_DCC0 : LTQ_PADC_DCC1);
				reg = (PORT_PIN(pin) <= 15)
				? LTQ_PADC_DCC0 : LTQ_PADC_DCC1;
				temp = pad_r32(info->membase[port], reg);
				temp &= ~(0x3 << ((pin % 16) * 2));
				ltq_w32(temp | value,
				info->membase[port] + reg);
			} else {
				LOGF_KLOG_ERROR("%s: Invalid input for current control\r\n",
				__func__);
			}
		}
	}
	mutex_unlock(&sysfs_lock);
	return status ? : size;
}

static const DEVICE_ATTR(currentcontrol, 0644,
		currentcontrol_show, currentcontrol_store);


static ssize_t pinmux_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;
	ssize_t status = 0;
	u32 pin;
	int port;
	u32 mux;

	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);

	mutex_lock(&sysfs_lock);
	if (!test_bit((FLAG_EXPORT), &(desc->flags))) {
		status = -EIO;
	} else {
		LOGF_KLOG_DEBUG("addr 0x%x \r\n",
(unsigned int)info->membase[port] + (PORT_PIN(pin) * 4));
		mux = pad_r32(info->membase[port], (PORT_PIN(pin) * 4));
		status = sprintf(buf, "%d\n", mux);
	}

	mutex_unlock(&sysfs_lock);

	return status;
}

static ssize_t pinmux_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	const struct sys_lantiq_pin_desc *desc = dev_get_drvdata(dev);
	struct ltq_pinmux_info *info;
	ssize_t			status = 0;
	u32 pin;
	int port;
	long value;

	info = platform_get_drvdata(pinctrl_platform_dev);
	pin = desc - pin_desc_array;
	port  = PORT(pin);
	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags)) {
		status = -EIO;
	} else {
		status = kstrtol(buf, 0, &value);
		if (status == 0) {
			LOGF_KLOG_DEBUG("writing value:%ld to register:%x\n",
			value,
			(unsigned int) info->membase[port]
			+ (PORT_PIN(pin) * 4));
			pad_w32(info->membase[port], value, PORT_PIN(pin) * 4);
		}
	}

	mutex_unlock(&sysfs_lock);
	return status ? : size;
}

static const DEVICE_ATTR(pinmux, 0644,
		pinmux_show, pinmux_store);


static const struct attribute *pin_attrs[] = {

	&dev_attr_pullup.attr,
	&dev_attr_pulldown.attr,
	&dev_attr_opendrain.attr,
	&dev_attr_pinmux.attr,
	&dev_attr_slewrate.attr,
	&dev_attr_currentcontrol.attr,
	&dev_attr_padctrl_availability.attr,
	NULL,
};

static const struct attribute_group pin_attr_group = {
	.attrs = (struct attribute **) pin_attrs,
};

static int pin_export(unsigned int pin)
{
	int		status;
	struct device	*dev;

	/* Many systems register gpio chips for SOC support very early,
	 * before driver model support is available.  In those cases we
	 * export this later, in gpiolib_sysfs_init() ... here we just
	 * verify that _some_ field of gpio_class got initialized.
	 */
	if (!pinctrl_class.p)
		return 0;

	/* use chip->base for the ID; it's already known to be unique */
	mutex_lock(&sysfs_lock);
	if (pin >= (PORTS*PINS)) {
		LOGF_KLOG_WARN("%s: invalid pin\n", __func__);
		status = -ENODEV;
		goto fail_unlock;
	}
	if (test_bit((FLAG_EXPORT), &pin_desc_array[pin].flags)) {
		LOGF_KLOG_ERROR("Pin %d already exported\r\n", pin);
		status = -EPERM;
		goto fail_unlock;
	}
	dev = device_create(&pinctrl_class,
	&pinctrl_platform_dev->dev,
	MKDEV(0, 0),
	&pin_desc_array[pin],
	"pin_%d", pin);
	if (IS_ERR(dev)) {
		status = PTR_ERR(dev);
		goto fail_unlock;
	}
	set_bit((FLAG_EXPORT), &pin_desc_array[pin].flags);
	status = sysfs_create_group(&dev->kobj, &pin_attr_group);
	if (status)
		goto fail_unregister_device;
	mutex_unlock(&sysfs_lock);
	return 0;

fail_unregister_device:
	device_unregister(dev);
fail_unlock:
	mutex_unlock(&sysfs_lock);
	LOGF_KLOG_DEBUG("%s:  status %d\n", __func__, status);
	return status;
}
static int match_export(struct device *dev, const void *data)
{
	return dev_get_drvdata(dev) == data;
}

static int pin_unexport(unsigned int pin)
{
	int			status = 0;
	struct device		*dev = NULL;

	if (pin >= (PORTS*PINS)) {
		LOGF_KLOG_WARN("%s: invalid pin\n", __func__);
		status = -ENODEV;
		goto fail_unlock;
	}
	mutex_lock(&sysfs_lock);
	if (!(test_bit((FLAG_EXPORT), &pin_desc_array[pin].flags))) {
		status = -ENODEV;
		goto fail_unlock;
	}
	dev = class_find_device(&pinctrl_class,
	NULL, &pin_desc_array[pin], match_export);
	if (dev) {
		clear_bit((FLAG_EXPORT), &pin_desc_array[pin].flags);
	} else {
		status = -ENODEV;
		goto fail_unlock;
	}
	if (dev) {
		device_unregister(dev);
		put_device(dev);
	}
	mutex_unlock(&sysfs_lock);
	return 0;
fail_unlock:
	mutex_unlock(&sysfs_lock);
	LOGF_KLOG_DEBUG("%s:  status %d\n", __func__, status);
	return status;
}

static ssize_t export_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t len)
{
	long			pin;
	int			status;

	status = kstrtol(buf, 0, &pin);
	if (status < 0)
		goto done;
	/* No extra locking here; FLAG_SYSFS just signifies that the
	 * request and export were done by on behalf of userspace, so
	 * they may be undone on its behalf too.
	 */
	status = pin_export((unsigned int)pin);
	if (status < 0) {
		if (status == -EPROBE_DEFER)
			status = -ENODEV;
		goto done;
	}
done:
	if (status)
		LOGF_KLOG_DEBUG("%s: status %d\n", __func__, status);
	return status ? : len;
}

static ssize_t unexport_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t len)
{
	long	pin;
	int			status;

	status = kstrtol(buf, 0, &pin);
	if (status < 0)
		goto done;
	status = pin_unexport(pin);
	/* No extra locking here; FLAG_SYSFS just signifies that the
	 * request and export were done by on behalf of userspace, so
	 * they may be undone on its behalf too.
	 */
done:
	if (status)
		LOGF_KLOG_DEBUG("%s: status %d\n", __func__, status);
	return status ? : len;
}


static int pinctrl_sysfs_init(struct device *dev)
{
	int	status;
	status = class_register(&pinctrl_class);
	return status;

}
#endif

/* Debug API print pinmux pins initialized before pinctrl driver */
#ifdef DEBUG
static void dbg_print_init_pin(struct device *dev,
			       void __iomem *base, unsigned int pinbase)
{
	int i;
	u32 mux;

	dev_info(dev, "check base: 0x%x, pinbase: %u\n",
		(u32)base, pinbase);

	for (i = 0; i < PINS; i++) {
		mux = pad_r32(base, (i << 2));
		if (mux != 0)
			dev_info(dev, "pin %u has been set to %u\n",
				pinbase + i, mux);
	}
}
#else
static void dbg_print_init_pin(struct device *dev,
			       void __iomem *base, unsigned int pinbase)
{
}
#endif

/* --------- register the pinctrl layer --------- */
static int pinctrl_xrx500_probe(struct platform_device *pdev)
{
	struct device_node *np;
	int pad_count = 0;
	int ret = 0, i = 0, j = 0;
	__be32 *bbspi = NULL;

	LOGF_KLOG_DEBUG("[%s] .. [%d]\n", __func__, __LINE__);

	/* load and remap the pad resources of the different banks */
	for_each_compatible_node(np, NULL, "lantiq,pad-xrx500") {
		struct platform_device *ppdev = of_find_device_by_node(np);
		const __be32 *bank = of_get_property(np, "lantiq,bank", NULL);
		struct resource res;

		if (!of_device_is_available(np))
			continue;

		if (!ppdev) {
			LOGF_KLOG_DEV_ERROR(&pdev->dev, "failed to find pad pdev\n");
			continue;
		}
		if (!bank || *bank >= PORTS)
			continue;
		if (of_address_to_resource(np, 0, &res))
			continue;
#if 0
		xrx500_info.clk[*bank] = clk_get(&ppdev->dev, NULL);
		if (IS_ERR(xrx500_info.clk[*bank])) {
			LOGF_KLOG_DEV_ERROR(&ppdev->dev, "failed to get clock\n");
			return PTR_ERR(xrx500_info.clk[*bank]);
		}
		clk_activate(xrx500_info.clk[*bank]);
#endif
		xrx500_info.membase[*bank] = devm_ioremap_resource(&pdev->dev,
								   &res);
		LOGF_KLOG_DEBUG("Bank: [%d] .. [%x]\n",
		*bank, (unsigned int)xrx500_info.membase[*bank]);
		if (IS_ERR(xrx500_info.membase[*bank]))
			return PTR_ERR(xrx500_info.membase[*bank]);
		lantiq_load_pin_desc(&xrx500_pads[pad_count], *bank, PINS);
		pad_count += PINS;
		LOGF_KLOG_DEV_DBG(&pdev->dev, "found %s with %d pads\n",
				res.name, PINS);
		LOGF_KLOG_DEBUG("found %s with %d pads\n", res.name, PINS);
		dbg_print_init_pin(&pdev->dev, xrx500_info.membase[*bank], pad_count - PINS);
		/* Set Registers to default values as the HW doesn't do it */
		if (!(*bank)) {
			for (i = 0, j = 0; i < sizeof(default_reg_bank0)/sizeof(uint32_t); j += 4) {
				pad_w32(xrx500_info.membase[*bank], default_reg_bank0[i], j);
				i++;
			}
		} else {
			for (i = 0, j = 0; i < sizeof(default_reg_bank1)/sizeof(uint32_t); j += 4) {
				if ((j == 0x7C) && (j == 0x78)) {
					continue;
				}
				pad_w32(xrx500_info.membase[*bank], default_reg_bank1[i], j);
				if (j == 0x74) {
					j = 0x7c;
				}
				i++;
			}
			bbspi = (__be32 *)of_get_property(np, "lantiq,bbspi-en", NULL);
			if (bbspi) {
				if (*bbspi) {
					LOGF_KLOG_INFO("enabling the BBSPI at top-mux\n");
					ltq_w32_mask(0, LTQ_BBSPI_EN_MASK, (void __iomem *)(0xb6080120));
				} else {
					LOGF_KLOG_INFO("disabling the BBSPI at top-mux\n");
					ltq_w32_mask(LTQ_BBSPI_EN_MASK, 0, (void __iomem *)(0xb6080120));
				}
			}
		}
	}
	LOGF_KLOG_DEV_DBG(&pdev->dev, "found a total of %d pads\n", pad_count);
	LOGF_KLOG_DEBUG("found a total of %d pads\n", pad_count);

	/* Init the Pin controller */
	xrx500_pctrl_desc.name	= dev_name(&pdev->dev);
	xrx500_pctrl_desc.npins	= pad_count;

	xrx500_info.mfp		= xrx500_mfp;
	xrx500_info.num_mfp	= ARRAY_SIZE(xrx500_mfp);
	xrx500_info.grps	= xrx500_grps;
	xrx500_info.num_grps	= ARRAY_SIZE(xrx500_grps);
	xrx500_info.funcs	= xrx500_funcs;
	xrx500_info.num_funcs	= ARRAY_SIZE(xrx500_funcs);

	ret = ltq_pinctrl_register(pdev, &xrx500_info);
	if (ret) {
		LOGF_KLOG_DEV_ERROR(&pdev->dev, "Failed to register pinctrl driver\n");
		return ret;
	}

#ifdef CONFIG_PINCTRL_SYSFS
	pinctrl_platform_dev = pdev;
	pinctrl_sysfs_init(&pdev->dev);
#endif
	LOGF_KLOG_DEV_INFO(&pdev->dev, "Init done\n");
	return ret;
}

static const struct of_device_id xrx500_match[] = {
	{ .compatible = "lantiq,pinctrl-xrx500" },
	{},
};
MODULE_DEVICE_TABLE(of, xrx500_match);

static struct platform_driver pinctrl_xrx500_driver = {
	.probe = pinctrl_xrx500_probe,
	.driver = {
		.name = "pinctrl-xrx500",
		.owner = THIS_MODULE,
		.of_match_table = xrx500_match,
	},
};

int __init pinctrl_xrx500_init(void)
{
	return platform_driver_register(&pinctrl_xrx500_driver);
}

postcore_initcall_sync(pinctrl_xrx500_init);
