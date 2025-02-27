/*
 * I2C client/driver for the ST M41T80 family of i2c rtc chips.
 *
 * Author: Alexander Bigga <ab@mycable.de>
 *
 * Based on m41t00.c by Mark A. Greer <mgreer@mvista.com>
 *
 * 2006 (c) mycable GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#ifdef CONFIG_RTC_DRV_M41T80_WDT
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#endif

#define M41T80_REG_SSEC	0
#define M41T80_REG_SEC	1
#define M41T80_REG_MIN	2
#define M41T80_REG_HOUR	3
#define M41T80_REG_WDAY	4
#define M41T80_REG_DAY	5
#define M41T80_REG_MON	6
#define M41T80_REG_YEAR	7
#define M41T80_REG_ALARM_MON	0xa
#define M41T80_REG_ALARM_DAY	0xb
#define M41T80_REG_ALARM_HOUR	0xc
#define M41T80_REG_ALARM_MIN	0xd
#define M41T80_REG_ALARM_SEC	0xe
#define M41T80_REG_FLAGS	0xf
#define M41T80_REG_SQW	0x13

#define M41T80_DATETIME_REG_SIZE	(M41T80_REG_YEAR + 1)
#define M41T80_ALARM_REG_SIZE	\
	(M41T80_REG_ALARM_SEC + 1 - M41T80_REG_ALARM_MON)

#define M41T80_SEC_ST		(1 << 7)	/* ST: Stop Bit */
#define M41T80_ALMON_AFE	(1 << 7)	/* AFE: AF Enable Bit */
#define M41T80_ALMON_SQWE	(1 << 6)	/* SQWE: SQW Enable Bit */
#define M41T80_ALHOUR_HT	(1 << 6)	/* HT: Halt Update Bit */
#define M41T80_FLAGS_AF		(1 << 6)	/* AF: Alarm Flag Bit */
#define M41T80_FLAGS_BATT_LOW	(1 << 4)	/* BL: Battery Low Bit */

#define M41T80_FEATURE_HT	(1 << 0)
#define M41T80_FEATURE_BL	(1 << 1)

#define DRV_VERSION "0.06"

struct m41t80_chip_info {
	const char *name;
	u8 features;
};

static const struct m41t80_chip_info m41t80_chip_info_tbl[] = {
	{
		.name		= "m41t80",
		.features	= 0,
	},
	{
		.name		= "m41t81",
		.features	= M41T80_FEATURE_HT,
	},
	{
		.name		= "m41t81s",
		.features	= M41T80_FEATURE_HT | M41T80_FEATURE_BL,
	},
	{
		.name		= "m41t82",
		.features	= M41T80_FEATURE_HT | M41T80_FEATURE_BL,
	},
	{
		.name		= "m41t83",
		.features	= M41T80_FEATURE_HT | M41T80_FEATURE_BL,
	},
	{
		.name		= "m41st84",
		.features	= M41T80_FEATURE_HT | M41T80_FEATURE_BL,
	},
	{
		.name		= "m41st85",
		.features	= M41T80_FEATURE_HT | M41T80_FEATURE_BL,
	},
	{
		.name		= "m41st87",
		.features	= M41T80_FEATURE_HT | M41T80_FEATURE_BL,
	},
};

struct m41t80_data {
	const struct m41t80_chip_info *chip;
	struct rtc_device *rtc;
	struct i2c_client *client;
	struct work_struct work;
	struct mutex work_lock;
	u8 flags;
	spinlock_t flags_lock;
	unsigned exiting: 1;
};

static irqreturn_t m41t80_rtc_interrupt(int irq, void *dev_id)
{
	struct i2c_client *client = dev_id;
	struct m41t80_data *clientdata = i2c_get_clientdata(client);

	disable_irq_nosync(irq);
	schedule_work(&clientdata->work);

	return IRQ_HANDLED;
}

static int read_m41t80_flags(struct i2c_client *client)
{
	struct m41t80_data *clientdata = i2c_get_clientdata(client);
	int flags;

	flags = i2c_smbus_read_byte_data(client, M41T80_REG_FLAGS);
	if (flags < 0) {
		dev_err(&client->dev, "read error\n");
		goto out;
	}

	spin_lock(&clientdata->flags_lock);
	clientdata->flags &= M41T80_FLAGS_AF;
	clientdata->flags |= flags;
	flags = clientdata->flags;
	spin_unlock(&clientdata->flags_lock);
out:
	return flags;
}

static void clear_m41t80_flags(struct i2c_client *client)
{
	struct m41t80_data *clientdata = i2c_get_clientdata(client);

	spin_lock(&clientdata->flags_lock);
	clientdata->flags = 0;
	spin_unlock(&clientdata->flags_lock);
}

static void m41t80_work(struct work_struct *w)
{
	struct m41t80_data *clientdata = container_of(w, struct m41t80_data, work);
	struct i2c_client *client = clientdata->client;
	struct rtc_device *rtc = clientdata->rtc;
	int flags;

	mutex_lock(&clientdata->work_lock);

	flags = read_m41t80_flags(client);
	if (flags < 0)
		goto out;

	if (flags & M41T80_FLAGS_AF) {
		local_irq_disable();
		rtc_update_irq(rtc, 1, RTC_AF | RTC_IRQF);
		local_irq_enable();
	}

	clear_m41t80_flags(client);
out:
	if (!clientdata->exiting)
		enable_irq(client->irq);
	mutex_unlock(&clientdata->work_lock);
}

/*
 * RTC interfaces
 */
static int m41t80_get_datetime(struct i2c_client *client,
			       struct rtc_time *tm)
{
	u8 buf[M41T80_DATETIME_REG_SIZE], dt_addr[1] = { M41T80_REG_SEC };
	struct i2c_msg msgs[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= dt_addr,
		},
		{
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= M41T80_DATETIME_REG_SIZE - M41T80_REG_SEC,
			.buf	= buf + M41T80_REG_SEC,
		},
	};

	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
		dev_err(&client->dev, "read error\n");
		return -EIO;
	}

	tm->tm_sec = BCD2BIN(buf[M41T80_REG_SEC] & 0x7f);
	tm->tm_min = BCD2BIN(buf[M41T80_REG_MIN] & 0x7f);
	tm->tm_hour = BCD2BIN(buf[M41T80_REG_HOUR] & 0x3f);
	tm->tm_mday = BCD2BIN(buf[M41T80_REG_DAY] & 0x3f);
	tm->tm_wday = buf[M41T80_REG_WDAY] & 0x07;
	tm->tm_mon = BCD2BIN(buf[M41T80_REG_MON] & 0x1f) - 1;

	/* assume 20YY not 19YY, and ignore the Century Bit */
	tm->tm_year = BCD2BIN(buf[M41T80_REG_YEAR]) + 100;
	return 0;
}

/* Sets the given date and time to the real time clock. */
static int m41t80_set_datetime(struct i2c_client *client, struct rtc_time *tm)
{
	u8 wbuf[1 + M41T80_DATETIME_REG_SIZE];
	u8 *buf = &wbuf[1];
	u8 dt_addr[1] = { M41T80_REG_SEC };
	struct i2c_msg msgs_in[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= dt_addr,
		},
		{
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= M41T80_DATETIME_REG_SIZE - M41T80_REG_SEC,
			.buf	= buf + M41T80_REG_SEC,
		},
	};
	struct i2c_msg msgs[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1 + M41T80_DATETIME_REG_SIZE,
			.buf	= wbuf,
		 },
	};

	/* Read current reg values into buf[1..7] */
	if (i2c_transfer(client->adapter, msgs_in, 2) < 0) {
		dev_err(&client->dev, "read error\n");
		return -EIO;
	}

	wbuf[0] = 0; /* offset into rtc's regs */
	/* Merge time-data and register flags into buf[0..7] */
	buf[M41T80_REG_SSEC] = 0;
	buf[M41T80_REG_SEC] =
		BIN2BCD(tm->tm_sec) | (buf[M41T80_REG_SEC] & ~0x7f);
	buf[M41T80_REG_MIN] =
		BIN2BCD(tm->tm_min) | (buf[M41T80_REG_MIN] & ~0x7f);
	buf[M41T80_REG_HOUR] =
		BIN2BCD(tm->tm_hour) | (buf[M41T80_REG_HOUR] & ~0x3f) ;
	buf[M41T80_REG_WDAY] =
		(tm->tm_wday & 0x07) | (buf[M41T80_REG_WDAY] & ~0x07);
	buf[M41T80_REG_DAY] =
		BIN2BCD(tm->tm_mday) | (buf[M41T80_REG_DAY] & ~0x3f);
	buf[M41T80_REG_MON] =
		BIN2BCD(tm->tm_mon + 1) | (buf[M41T80_REG_MON] & ~0x1f);
	/* assume 20YY not 19YY */
	buf[M41T80_REG_YEAR] = BIN2BCD(tm->tm_year % 100);

	if (i2c_transfer(client->adapter, msgs, 1) != 1) {
		dev_err(&client->dev, "write error\n");
		return -EIO;
	}
	return 0;
}

#if defined(CONFIG_RTC_INTF_PROC) || defined(CONFIG_RTC_INTF_PROC_MODULE)
static int m41t80_rtc_proc(struct device *dev, struct seq_file *seq)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct m41t80_data *clientdata = i2c_get_clientdata(client);
	u8 reg;

	if (clientdata->chip->features & M41T80_FEATURE_BL) {
		reg = read_m41t80_flags(client);
		seq_printf(seq, "battery\t\t: %s\n",
			   (reg & M41T80_FLAGS_BATT_LOW) ? "exhausted" : "ok");
	}
	return 0;
}
#else
#define m41t80_rtc_proc NULL
#endif

static int m41t80_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	return m41t80_get_datetime(to_i2c_client(dev), tm);
}

static int m41t80_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	return m41t80_set_datetime(to_i2c_client(dev), tm);
}

#if defined(CONFIG_RTC_INTF_DEV) || defined(CONFIG_RTC_INTF_DEV_MODULE)
static int
m41t80_rtc_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = to_i2c_client(dev);
	int rc;

	switch (cmd) {
	case RTC_AIE_OFF:
	case RTC_AIE_ON:
		if (client->irq > 0)
			break;
		/* fall-through */
	default:
		return -ENOIOCTLCMD;
	}

	rc = i2c_smbus_read_byte_data(client, M41T80_REG_ALARM_MON);
	if (rc < 0)
		goto err;
	switch (cmd) {
	case RTC_AIE_OFF:
		rc &= ~M41T80_ALMON_AFE;
		break;
	case RTC_AIE_ON:
		rc |= M41T80_ALMON_AFE;
		break;
	}
	if (i2c_smbus_write_byte_data(client, M41T80_REG_ALARM_MON, rc) < 0)
		goto err;
	return 0;
err:
	return -EIO;
}
#else
#define	m41t80_rtc_ioctl NULL
#endif

static int m41t80_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *t)
{
	struct i2c_client *client = to_i2c_client(dev);
	u8 wbuf[1 + M41T80_ALARM_REG_SIZE];
	u8 *buf = &wbuf[1];
	u8 *reg = buf - M41T80_REG_ALARM_MON;
	u8 dt_addr[1] = { M41T80_REG_ALARM_MON };
	struct i2c_msg msgs_in[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= dt_addr,
		},
		{
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= M41T80_ALARM_REG_SIZE,
			.buf	= buf,
		},
	};
	struct i2c_msg msgs[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1 + M41T80_ALARM_REG_SIZE,
			.buf	= wbuf,
		 },
	};

	if (client->irq <= 0 && t->enabled)
		return -ENODEV;

	if (i2c_transfer(client->adapter, msgs_in, 2) < 0) {
		dev_err(&client->dev, "read error\n");
		return -EIO;
	}
	reg[M41T80_REG_ALARM_MON] &= ~(0x1f | M41T80_ALMON_AFE);
	reg[M41T80_REG_ALARM_DAY] = 0;
	reg[M41T80_REG_ALARM_HOUR] &= ~(0x3f | 0x80);
	reg[M41T80_REG_ALARM_MIN] = 0;
	reg[M41T80_REG_ALARM_SEC] = 0;

	wbuf[0] = M41T80_REG_ALARM_MON; /* offset into rtc's regs */
	reg[M41T80_REG_ALARM_SEC] |= t->time.tm_sec >= 0 ?
		BIN2BCD(t->time.tm_sec) : 0x80;
	reg[M41T80_REG_ALARM_MIN] |= t->time.tm_min >= 0 ?
		BIN2BCD(t->time.tm_min) : 0x80;
	reg[M41T80_REG_ALARM_HOUR] |= t->time.tm_hour >= 0 ?
		BIN2BCD(t->time.tm_hour) : 0x80;
	reg[M41T80_REG_ALARM_DAY] |= t->time.tm_mday >= 0 ?
		BIN2BCD(t->time.tm_mday) : 0x80;
	if (t->time.tm_mon >= 0)
		reg[M41T80_REG_ALARM_MON] |= BIN2BCD(t->time.tm_mon + 1);
	else
		reg[M41T80_REG_ALARM_DAY] |= 0x40;

	if (i2c_transfer(client->adapter, msgs, 1) != 1) {
		dev_err(&client->dev, "write error\n");
		return -EIO;
	}

	if (t->enabled) {
		reg[M41T80_REG_ALARM_MON] |= M41T80_ALMON_AFE;
		if (i2c_smbus_write_byte_data(client, M41T80_REG_ALARM_MON,
					      reg[M41T80_REG_ALARM_MON]) < 0) {
			dev_err(&client->dev, "write error\n");
			return -EIO;
		}
	}
	return 0;
}

static int m41t80_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *t)
{
	struct i2c_client *client = to_i2c_client(dev);
	u8 buf[M41T80_ALARM_REG_SIZE]; /* all alarm regs */
	u8 dt_addr[1] = { M41T80_REG_ALARM_MON };
	u8 *reg = buf - M41T80_REG_ALARM_MON;
	struct i2c_msg msgs[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= dt_addr,
		},
		{
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= M41T80_ALARM_REG_SIZE,
			.buf	= buf,
		},
	};

	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
		dev_err(&client->dev, "read error\n");
		return -EIO;
	}
	t->time.tm_sec = -1;
	t->time.tm_min = -1;
	t->time.tm_hour = -1;
	t->time.tm_mday = -1;
	t->time.tm_mon = -1;
	if (!(reg[M41T80_REG_ALARM_SEC] & 0x80))
		t->time.tm_sec = BCD2BIN(reg[M41T80_REG_ALARM_SEC] & 0x7f);
	if (!(reg[M41T80_REG_ALARM_MIN] & 0x80))
		t->time.tm_min = BCD2BIN(reg[M41T80_REG_ALARM_MIN] & 0x7f);
	if (!(reg[M41T80_REG_ALARM_HOUR] & 0x80))
		t->time.tm_hour = BCD2BIN(reg[M41T80_REG_ALARM_HOUR] & 0x3f);
	if (!(reg[M41T80_REG_ALARM_DAY] & 0x80))
		t->time.tm_mday = BCD2BIN(reg[M41T80_REG_ALARM_DAY] & 0x3f);
	if (!(reg[M41T80_REG_ALARM_DAY] & 0x40))
		t->time.tm_mon = BCD2BIN(reg[M41T80_REG_ALARM_MON] & 0x1f) - 1;
	t->time.tm_year = -1;
	t->time.tm_wday = -1;
	t->time.tm_yday = -1;
	t->time.tm_isdst = -1;
	t->enabled = !!(reg[M41T80_REG_ALARM_MON] & M41T80_ALMON_AFE);
	t->pending = !!(read_m41t80_flags(client) & M41T80_FLAGS_AF);
	return 0;
}

static struct rtc_class_ops m41t80_rtc_ops = {
	.read_time = m41t80_rtc_read_time,
	.set_time = m41t80_rtc_set_time,
	.read_alarm = m41t80_rtc_read_alarm,
	.set_alarm = m41t80_rtc_set_alarm,
	.proc = m41t80_rtc_proc,
	.ioctl = m41t80_rtc_ioctl,
};

#if defined(CONFIG_RTC_INTF_SYSFS) || defined(CONFIG_RTC_INTF_SYSFS_MODULE)
static ssize_t m41t80_sysfs_show_flags(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int val;

	val = i2c_smbus_read_byte_data(client, M41T80_REG_FLAGS);
	if (val < 0)
		return -EIO;
	return sprintf(buf, "%#x\n", val);
}
static DEVICE_ATTR(flags, S_IRUGO, m41t80_sysfs_show_flags, NULL);

static ssize_t m41t80_sysfs_show_sqwfreq(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int val;

	val = i2c_smbus_read_byte_data(client, M41T80_REG_SQW);
	if (val < 0)
		return -EIO;
	val = (val >> 4) & 0xf;
	switch (val) {
	case 0:
		break;
	case 1:
		val = 32768;
		break;
	default:
		val = 32768 >> val;
	}
	return sprintf(buf, "%d\n", val);
}
static ssize_t m41t80_sysfs_set_sqwfreq(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	int almon, sqw;
	int val = simple_strtoul(buf, NULL, 0);

	if (val) {
		if (!is_power_of_2(val))
			return -EINVAL;
		val = ilog2(val);
		if (val == 15)
			val = 1;
		else if (val < 14)
			val = 15 - val;
		else
			return -EINVAL;
	}
	/* disable SQW, set SQW frequency & re-enable */
	almon = i2c_smbus_read_byte_data(client, M41T80_REG_ALARM_MON);
	if (almon < 0)
		return -EIO;
	sqw = i2c_smbus_read_byte_data(client, M41T80_REG_SQW);
	if (sqw < 0)
		return -EIO;
	sqw = (sqw & 0x0f) | (val << 4);
	if (i2c_smbus_write_byte_data(client, M41T80_REG_ALARM_MON,
				      almon & ~M41T80_ALMON_SQWE) < 0 ||
	    i2c_smbus_write_byte_data(client, M41T80_REG_SQW, sqw) < 0)
		return -EIO;
	if (val && i2c_smbus_write_byte_data(client, M41T80_REG_ALARM_MON,
					     almon | M41T80_ALMON_SQWE) < 0)
		return -EIO;
	return count;
}
static DEVICE_ATTR(sqwfreq, S_IRUGO | S_IWUSR,
		   m41t80_sysfs_show_sqwfreq, m41t80_sysfs_set_sqwfreq);

static struct attribute *attrs[] = {
	&dev_attr_flags.attr,
	&dev_attr_sqwfreq.attr,
	NULL,
};
static struct attribute_group attr_group = {
	.attrs = attrs,
};

static int m41t80_sysfs_register(struct device *dev)
{
	return sysfs_create_group(&dev->kobj, &attr_group);
}
#else
static int m41t80_sysfs_register(struct device *dev)
{
	return 0;
}
#endif

#ifdef CONFIG_RTC_DRV_M41T80_WDT
/*
 *****************************************************************************
 *
 * Watchdog Driver
 *
 *****************************************************************************
 */
static struct i2c_client *save_client;

/* Default margin */
#define WD_TIMO 60		/* 1..31 seconds */

static int wdt_margin = WD_TIMO;
module_param(wdt_margin, int, 0);
MODULE_PARM_DESC(wdt_margin, "Watchdog timeout in seconds (default 60s)");

static unsigned long wdt_is_open;
static int boot_flag;

/**
 *	wdt_ping:
 *
 *	Reload counter one with the watchdog timeout. We don't bother reloading
 *	the cascade counter.
 */
static void wdt_ping(void)
{
	unsigned char i2c_data[2];
	struct i2c_msg msgs1[1] = {
		{
			.addr	= save_client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= i2c_data,
		},
	};
	i2c_data[0] = 0x09;		/* watchdog register */

	if (wdt_margin > 31)
		i2c_data[1] = (wdt_margin & 0xFC) | 0x83; /* resolution = 4s */
	else
		/*
		 * WDS = 1 (0x80), mulitplier = WD_TIMO, resolution = 1s (0x02)
		 */
		i2c_data[1] = wdt_margin<<2 | 0x82;

	i2c_transfer(save_client->adapter, msgs1, 1);
}

/**
 *	wdt_disable:
 *
 *	disables watchdog.
 */
static void wdt_disable(void)
{
	unsigned char i2c_data[2], i2c_buf[0x10];
	struct i2c_msg msgs0[2] = {
		{
			.addr	= save_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= i2c_data,
		},
		{
			.addr	= save_client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= i2c_buf,
		},
	};
	struct i2c_msg msgs1[1] = {
		{
			.addr	= save_client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= i2c_data,
		},
	};

	i2c_data[0] = 0x09;
	i2c_transfer(save_client->adapter, msgs0, 2);

	i2c_data[0] = 0x09;
	i2c_data[1] = 0x00;
	i2c_transfer(save_client->adapter, msgs1, 1);
}

/**
 *	wdt_write:
 *	@file: file handle to the watchdog
 *	@buf: buffer to write (unused as data does not matter here
 *	@count: count of bytes
 *	@ppos: pointer to the position to write. No seeks allowed
 *
 *	A write to a watchdog device is defined as a keepalive signal. Any
 *	write of data will do, as we we don't define content meaning.
 */
static ssize_t wdt_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	/*  Can't seek (pwrite) on this device
	if (ppos != &file->f_pos)
	return -ESPIPE;
	*/
	if (count) {
		wdt_ping();
		return 1;
	}
	return 0;
}

static ssize_t wdt_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	return 0;
}

/**
 *	wdt_ioctl:
 *	@inode: inode of the device
 *	@file: file handle to the device
 *	@cmd: watchdog command
 *	@arg: argument pointer
 *
 *	The watchdog API defines a common set of functions for all watchdogs
 *	according to their available features. We only actually usefully support
 *	querying capabilities and current status.
 */
static int wdt_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		     unsigned long arg)
{
	int new_margin, rv;
	static struct watchdog_info ident = {
		.options = WDIOF_POWERUNDER | WDIOF_KEEPALIVEPING |
			WDIOF_SETTIMEOUT,
		.firmware_version = 1,
		.identity = "M41T80 WTD"
	};

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		return copy_to_user((struct watchdog_info __user *)arg, &ident,
				    sizeof(ident)) ? -EFAULT : 0;

	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
		return put_user(boot_flag, (int __user *)arg);
	case WDIOC_KEEPALIVE:
		wdt_ping();
		return 0;
	case WDIOC_SETTIMEOUT:
		if (get_user(new_margin, (int __user *)arg))
			return -EFAULT;
		/* Arbitrary, can't find the card's limits */
		if (new_margin < 1 || new_margin > 124)
			return -EINVAL;
		wdt_margin = new_margin;
		wdt_ping();
		/* Fall */
	case WDIOC_GETTIMEOUT:
		return put_user(wdt_margin, (int __user *)arg);

	case WDIOC_SETOPTIONS:
		if (copy_from_user(&rv, (int __user *)arg, sizeof(int)))
			return -EFAULT;

		if (rv & WDIOS_DISABLECARD) {
			printk(KERN_INFO
			       "rtc-m41t80: disable watchdog\n");
			wdt_disable();
		}

		if (rv & WDIOS_ENABLECARD) {
			printk(KERN_INFO
			       "rtc-m41t80: enable watchdog\n");
			wdt_ping();
		}

		return -EINVAL;
	}
	return -ENOTTY;
}

/**
 *	wdt_open:
 *	@inode: inode of device
 *	@file: file handle to device
 *
 */
static int wdt_open(struct inode *inode, struct file *file)
{
	if (MINOR(inode->i_rdev) == WATCHDOG_MINOR) {
		if (test_and_set_bit(0, &wdt_is_open))
			return -EBUSY;
		/*
		 *	Activate
		 */
		wdt_is_open = 1;
		return 0;
	}
	return -ENODEV;
}

/**
 *	wdt_close:
 *	@inode: inode to board
 *	@file: file handle to board
 *
 */
static int wdt_release(struct inode *inode, struct file *file)
{
	if (MINOR(inode->i_rdev) == WATCHDOG_MINOR)
		clear_bit(0, &wdt_is_open);
	return 0;
}

/**
 *	notify_sys:
 *	@this: our notifier block
 *	@code: the event being reported
 *	@unused: unused
 *
 *	Our notifier is called on system shutdowns. We want to turn the card
 *	off at reboot otherwise the machine will reboot again during memory
 *	test or worse yet during the following fsck. This would suck, in fact
 *	trust me - if it happens it does suck.
 */
static int wdt_notify_sys(struct notifier_block *this, unsigned long code,
			  void *unused)
{
	if (code == SYS_DOWN || code == SYS_HALT)
		/* Disable Watchdog */
		wdt_disable();
	return NOTIFY_DONE;
}

static const struct file_operations wdt_fops = {
	.owner	= THIS_MODULE,
	.read	= wdt_read,
	.ioctl	= wdt_ioctl,
	.write	= wdt_write,
	.open	= wdt_open,
	.release = wdt_release,
};

static struct miscdevice wdt_dev = {
	.minor = WATCHDOG_MINOR,
	.name = "watchdog",
	.fops = &wdt_fops,
};

/*
 *	The WDT card needs to learn about soft shutdowns in order to
 *	turn the timebomb registers off.
 */
static struct notifier_block wdt_notifier = {
	.notifier_call = wdt_notify_sys,
};
#endif /* CONFIG_RTC_DRV_M41T80_WDT */

/*
 *****************************************************************************
 *
 *	Driver Interface
 *
 *****************************************************************************
 */
static int m41t80_probe(struct i2c_client *client)
{
	int i, rc = 0;
	struct rtc_device *rtc = NULL;
	struct rtc_time tm;
	const struct m41t80_chip_info *chip;
	struct m41t80_data *clientdata = NULL;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C
				     | I2C_FUNC_SMBUS_BYTE_DATA)) {
		rc = -ENODEV;
		goto exit;
	}

	dev_info(&client->dev,
		 "chip found, driver version " DRV_VERSION "\n");

	chip = NULL;
	for (i = 0; i < ARRAY_SIZE(m41t80_chip_info_tbl); i++) {
		if (!strcmp(m41t80_chip_info_tbl[i].name, client->name)) {
			chip = &m41t80_chip_info_tbl[i];
			break;
		}
	}
	if (!chip) {
		dev_err(&client->dev, "%s is not supported\n", client->name);
		rc = -ENODEV;
		goto exit;
	}

	clientdata = kzalloc(sizeof(*clientdata), GFP_KERNEL);
	if (!clientdata) {
		rc = -ENOMEM;
		goto exit;
	}

	rtc = rtc_device_register(client->name, &client->dev,
				  &m41t80_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		rc = PTR_ERR(rtc);
		rtc = NULL;
		goto exit;
	}

	clientdata->rtc = rtc;
	clientdata->chip = chip;
	clientdata->client = client;
	i2c_set_clientdata(client, clientdata);
	INIT_WORK(&clientdata->work, m41t80_work);
	mutex_init(&clientdata->work_lock);
	spin_lock_init(&clientdata->flags_lock);

	/* Make sure HT (Halt Update) bit is cleared */
	rc = i2c_smbus_read_byte_data(client, M41T80_REG_ALARM_HOUR);
	if (rc < 0)
		goto ht_err;

	if (rc & M41T80_ALHOUR_HT) {
		if (chip->features & M41T80_FEATURE_HT) {
			m41t80_get_datetime(client, &tm);
			dev_info(&client->dev, "HT bit was set!\n");
			dev_info(&client->dev,
				 "Power Down at "
				 "%04i-%02i-%02i %02i:%02i:%02i\n",
				 tm.tm_year + 1900,
				 tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
				 tm.tm_min, tm.tm_sec);
		}
		if (i2c_smbus_write_byte_data(client,
					      M41T80_REG_ALARM_HOUR,
					      rc & ~M41T80_ALHOUR_HT) < 0)
			goto ht_err;
	}

	/* Make sure ST (stop) bit is cleared */
	rc = i2c_smbus_read_byte_data(client, M41T80_REG_SEC);
	if (rc < 0)
		goto st_err;

	if (rc & M41T80_SEC_ST) {
		if (i2c_smbus_write_byte_data(client, M41T80_REG_SEC,
					      rc & ~M41T80_SEC_ST) < 0)
			goto st_err;
	}

	if (client->irq > 0) {
		rc = request_irq(client->irq, m41t80_rtc_interrupt,
				 IRQF_DISABLED, client->name, client);
		if (rc) {
			dev_err(&client->dev, "Can't alloc IRQ %d\n",
				client->irq);
			client->irq = -1;
		}
	}
	dev_info(&client->dev, "with%s alarm support\n",
			client->irq > 0 ? "" : "out");

	rc = m41t80_sysfs_register(&client->dev);
	if (rc)
		goto irq_err;

#ifdef CONFIG_RTC_DRV_M41T80_WDT
	if (chip->features & M41T80_FEATURE_HT) {
		rc = misc_register(&wdt_dev);
		if (rc)
			goto exit;
		rc = register_reboot_notifier(&wdt_notifier);
		if (rc) {
			misc_deregister(&wdt_dev);
			goto exit;
		}
		save_client = client;
	}
#endif
	device_init_wakeup(&rtc->dev, 1);
	return 0;

st_err:
	rc = -EIO;
	dev_err(&client->dev, "Can't clear ST bit\n");
	goto exit;
ht_err:
	rc = -EIO;
	dev_err(&client->dev, "Can't clear HT bit\n");
	goto exit;

irq_err:
	if (client->irq > 0)
		free_irq(client->irq, client);
exit:
	if (rtc)
		rtc_device_unregister(rtc);
	kfree(clientdata);
	return rc;
}

static int m41t80_remove(struct i2c_client *client)
{
	struct m41t80_data *clientdata = i2c_get_clientdata(client);
	struct rtc_device *rtc = clientdata->rtc;

	mutex_lock(&clientdata->work_lock);
	clientdata->exiting = 1;
	mutex_unlock(&clientdata->work_lock);

	if (client->irq > 0)
		free_irq(client->irq, client);
	flush_scheduled_work();

#ifdef CONFIG_RTC_DRV_M41T80_WDT
	if (clientdata->chip->features & M41T80_FEATURE_HT) {
		misc_deregister(&wdt_dev);
		unregister_reboot_notifier(&wdt_notifier);
	}
#endif
	if (rtc) {
		device_init_wakeup(&rtc->dev, 0);
		rtc_device_unregister(rtc);
	}
	kfree(clientdata);

	return 0;
}

#ifdef CONFIG_PM
static int m41t80_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct m41t80_data *clientdata;

	clientdata = i2c_get_clientdata(client);
	if (device_may_wakeup(&clientdata->rtc->dev))
		enable_irq_wake(client->irq);
	return 0;
}

static int m41t80_resume(struct i2c_client *client)
{
	struct m41t80_data *clientdata;

	clientdata = i2c_get_clientdata(client);
	if (device_may_wakeup(&clientdata->rtc->dev))
		disable_irq_wake(client->irq);
	return 0;
}
#endif

static struct i2c_driver m41t80_driver = {
	.driver = {
		.name = "rtc-m41t80",
	},
	.probe = m41t80_probe,
	.remove = m41t80_remove,
#ifdef CONFIG_PM
	.suspend = m41t80_suspend,
	.resume = m41t80_resume,
#endif
};

static int __init m41t80_rtc_init(void)
{
	return i2c_add_driver(&m41t80_driver);
}

static void __exit m41t80_rtc_exit(void)
{
	i2c_del_driver(&m41t80_driver);
}

MODULE_AUTHOR("Alexander Bigga <ab@mycable.de>");
MODULE_DESCRIPTION("ST Microelectronics M41T80 series RTC I2C Client Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

module_init(m41t80_rtc_init);
module_exit(m41t80_rtc_exit);
