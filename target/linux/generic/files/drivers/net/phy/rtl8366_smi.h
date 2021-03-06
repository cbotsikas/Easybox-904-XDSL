/*
 * Realtek RTL8366 SMI interface driver defines
 *
 * Copyright (C) 2009-2010 Gabor Juhos <juhosg@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _RTL8366_SMI_H
#define _RTL8366_SMI_H

#include <linux/phy.h>
#include <linux/switch.h>
#include <linux/platform_device.h>

#define RTL_MDIO_PHYID		0
#define MDC_MDIO_CTRL0_REG	31
#define MDC_MDIO_START_REG	29
#define MDC_MDIO_CTRL1_REG	21
#define MDC_MDIO_ADDRESS_REG	23
#define MDC_MDIO_DATA_WRITE_REG	24
#define MDC_MDIO_DATA_READ_REG	25

#define MDC_MDIO_START_OP	0xFFFF
#define MDC_MDIO_ADDR_OP	0x000E
#define MDC_MDIO_READ_OP	0x0001
#define MDC_MDIO_WRITE_OP	0x0003

#define RTL8366_SMI_ACK_RETRY_COUNT	5

#define RTL8366_SMI_HW_STOP_DELAY	25	/* msecs */
#define RTL8366_SMI_HW_START_DELAY	100	/* msecs */

#define rtl8366_smi_clk_delay(smi) ndelay(smi->clk_delay)

struct rtl8366_smi_ops;
struct rtl8366_vlan_ops;
struct mii_bus;
struct dentry;
struct inode;
struct file;

struct rtl8366_mib_counter {
	unsigned	base;
	unsigned	offset;
	unsigned	length;
	const char	*name;
};

struct rtl8366_smi {
	struct device		*parent;
	unsigned int		gpio_sda;
	unsigned int		gpio_sck;
	bool			mdio_enabled;
	void			(*hw_reset)(bool active);
	unsigned int		clk_delay;	/* ns */
	u8			cmd_read;
	u8			cmd_write;
	spinlock_t		lock;
	struct mii_bus		*mii_bus;
	int			mii_irq[PHY_MAX_ADDR];
	struct switch_dev	sw_dev;
	unsigned int		chip_ver;

	unsigned int		cpu_port;
	unsigned int		num_ports;
	unsigned int		num_vlan_mc;
	unsigned int		num_mib_counters;
	struct rtl8366_mib_counter *mib_counters;

	struct rtl8366_smi_ops	*ops;

	int			vlan_enabled;
	int			vlan4k_enabled;

	char			buf[4096];
#ifdef CONFIG_RTL8366_SMI_DEBUG_FS
	struct dentry           *debugfs_root;
	u16			dbg_reg;
	u8			dbg_vlan_4k_page;
#endif
};

struct rtl8366_vlan_mc {
	u16	vid;
	u16	untag;
	u16	member;
	u8	fid;
	u8	priority;
};

struct rtl8366_vlan_4k {
	u16	vid;
	u16	untag;
	u16	member;
	u8	fid;
};

struct rtl8366_smi_ops {
	int	(*detect)(struct rtl8366_smi *smi);
	int	(*reset_chip)(struct rtl8366_smi *smi);
	int	(*setup)(struct rtl8366_smi *smi);

	int	(*mii_read)(struct mii_bus *bus, int addr, int reg);
	int	(*mii_write)(struct mii_bus *bus, int addr, int reg, u16 val);

	int	(*get_vlan_mc)(struct rtl8366_smi *smi, u32 index,
			       struct rtl8366_vlan_mc *vlanmc);
	int	(*set_vlan_mc)(struct rtl8366_smi *smi, u32 index,
			       const struct rtl8366_vlan_mc *vlanmc);
	int	(*get_vlan_4k)(struct rtl8366_smi *smi, u32 vid,
			       struct rtl8366_vlan_4k *vlan4k);
	int	(*set_vlan_4k)(struct rtl8366_smi *smi,
			       const struct rtl8366_vlan_4k *vlan4k);
	int	(*get_mc_index)(struct rtl8366_smi *smi, int port, int *val);
	int	(*set_mc_index)(struct rtl8366_smi *smi, int port, int index);
	int	(*get_mib_counter)(struct rtl8366_smi *smi, int counter,
				   int port, unsigned long long *val);
	int	(*is_vlan_valid)(struct rtl8366_smi *smi, unsigned vlan);
	int	(*enable_vlan)(struct rtl8366_smi *smi, int enable);
	int	(*enable_vlan4k)(struct rtl8366_smi *smi, int enable);
	int	(*enable_port)(struct rtl8366_smi *smi, int port, int enable);
};

struct rtl8366_smi *rtl8366_smi_alloc(struct device *parent);
int rtl8366_smi_init(struct rtl8366_smi *smi);
void rtl8366_smi_cleanup(struct rtl8366_smi *smi);
int rtl8366_smi_write_reg(struct rtl8366_smi *smi, u32 addr, u32 data);
int rtl8366_smi_write_reg_noack(struct rtl8366_smi *smi, u32 addr, u32 data);
int rtl8366_smi_read_reg(struct rtl8366_smi *smi, u32 addr, u32 *data);
int rtl8366_smi_rmwr(struct rtl8366_smi *smi, u32 addr, u32 mask, u32 data);

int rtl8366_reset_vlan(struct rtl8366_smi *smi);
int rtl8366_enable_vlan(struct rtl8366_smi *smi, int enable);
int rtl8366_enable_all_ports(struct rtl8366_smi *smi, int enable);

#ifdef CONFIG_RTL8366_SMI_DEBUG_FS
int rtl8366_debugfs_open(struct inode *inode, struct file *file);
#endif

static inline struct rtl8366_smi *sw_to_rtl8366_smi(struct switch_dev *sw)
{
	return container_of(sw, struct rtl8366_smi, sw_dev);
}

int rtl8366_sw_reset_switch(struct switch_dev *dev);
int rtl8366_sw_get_port_pvid(struct switch_dev *dev, int port, int *val);
int rtl8366_sw_set_port_pvid(struct switch_dev *dev, int port, int val);
int rtl8366_sw_get_port_mib(struct switch_dev *dev,
			    const struct switch_attr *attr,
			    struct switch_val *val);
int rtl8366_sw_get_vlan_info(struct switch_dev *dev,
			     const struct switch_attr *attr,
			     struct switch_val *val);
int rtl8366_sw_get_vlan_fid(struct switch_dev *dev,
			     const struct switch_attr *attr,
			     struct switch_val *val);
int rtl8366_sw_set_vlan_fid(struct switch_dev *dev,
			     const struct switch_attr *attr,
			     struct switch_val *val);
int rtl8366_sw_get_vlan_ports(struct switch_dev *dev, struct switch_val *val);
int rtl8366_sw_set_vlan_ports(struct switch_dev *dev, struct switch_val *val);
int rtl8366_sw_get_vlan_enable(struct switch_dev *dev,
			       const struct switch_attr *attr,
			       struct switch_val *val);
int rtl8366_sw_set_vlan_enable(struct switch_dev *dev,
			       const struct switch_attr *attr,
			       struct switch_val *val);
int rtl8366_sw_get_port_stats(struct switch_dev *dev, int port,
				struct switch_port_stats *stats,
				int txb_id, int rxb_id);

struct rtl8366_smi* rtl8366_smi_probe(struct platform_device *pdev);

#endif /*  _RTL8366_SMI_H */
