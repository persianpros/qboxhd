/*
 * arch/sh/kernel/cpu/clock.c - SuperH clock framework
 *
 *  Copyright (C) 2005, 2006, 2007  Paul Mundt
 *
 * This clock framework is derived from the OMAP version by:
 *
 *	Copyright (C) 2004 - 2005 Nokia Corporation
 *	Written by Tuukka Tikkanen <tuukka.tikkanen@elektrobit.com>
 *
 *  Modified for omap shared clock framework by Tony Lindgren <tony@atomide.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/kref.h>
#include <linux/seq_file.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <asm/clock.h>
#include <asm/timer.h>

static LIST_HEAD(clock_list);
static DEFINE_SPINLOCK(clock_lock);
static DEFINE_MUTEX(clock_list_sem);

static void propagate_rate(struct clk *clk)
{
	struct clk *clkp;

	list_for_each_entry(clkp, &clock_list, node) {
		if (likely(clkp->parent != clk))
			continue;
		if (likely(clkp->ops && clkp->ops->recalc))
			clkp->ops->recalc(clkp);
		if (unlikely(clkp->flags & CLK_RATE_PROPAGATES))
			propagate_rate(clkp);
	}
}

int __clk_enable(struct clk *clk)
{
	kref_get(&clk->kref);

	if (clk->flags & CLK_ALWAYS_ENABLED)
		return 0;

	if (likely(clk->ops && clk->ops->enable))
		clk->ops->enable(clk);

	return 0;
}
EXPORT_SYMBOL_GPL(__clk_enable);

int clk_enable(struct clk *clk)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&clock_lock, flags);
	ret = __clk_enable(clk);
	spin_unlock_irqrestore(&clock_lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(clk_enable);

static void clk_kref_release(struct kref *kref)
{
	/* Nothing to do */
}

void __clk_disable(struct clk *clk)
{
	int count = kref_put(&clk->kref, clk_kref_release);

	if (clk->flags & CLK_ALWAYS_ENABLED)
		return;

	if (!count) {	/* count reaches zero, disable the clock */
		if (likely(clk->ops && clk->ops->disable))
			clk->ops->disable(clk);
	}
}
EXPORT_SYMBOL_GPL(__clk_disable);

void clk_disable(struct clk *clk)
{
	unsigned long flags;

	spin_lock_irqsave(&clock_lock, flags);
	__clk_disable(clk);
	spin_unlock_irqrestore(&clock_lock, flags);
}
EXPORT_SYMBOL_GPL(clk_disable);

int clk_register(struct clk *clk)
{
	mutex_lock(&clock_list_sem);

	list_add(&clk->node, &clock_list);
	kref_init(&clk->kref);

	mutex_unlock(&clock_list_sem);

	if (clk->ops && clk->ops->init)
		clk->ops->init(clk);

	if (clk->flags & CLK_ALWAYS_ENABLED) {
		pr_debug( "Clock '%s' is ALWAYS_ENABLED\n", clk->name);
		if (clk->ops && clk->ops->enable)
			clk->ops->enable(clk);
		pr_debug( "Enabled.");
	}

	return 0;
}
EXPORT_SYMBOL_GPL(clk_register);

void clk_unregister(struct clk *clk)
{
	mutex_lock(&clock_list_sem);
	list_del(&clk->node);
	mutex_unlock(&clock_list_sem);
}
EXPORT_SYMBOL_GPL(clk_unregister);

unsigned long clk_get_rate(struct clk *clk)
{
	return clk->rate;
}
EXPORT_SYMBOL_GPL(clk_get_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	return clk_set_rate_ex(clk, rate, 0);
}
EXPORT_SYMBOL_GPL(clk_set_rate);

int clk_set_rate_ex(struct clk *clk, unsigned long rate, int algo_id)
{
	int ret = -EOPNOTSUPP;

	if (likely(clk->ops && clk->ops->set_rate)) {
		unsigned long flags;

		spin_lock_irqsave(&clock_lock, flags);
		ret = clk->ops->set_rate(clk, rate, algo_id);
		spin_unlock_irqrestore(&clock_lock, flags);
	}

	if (unlikely(clk->flags & CLK_RATE_PROPAGATES))
		propagate_rate(clk);

	return ret;
}
EXPORT_SYMBOL_GPL(clk_set_rate_ex);

void clk_recalc_rate(struct clk *clk)
{
	if (likely(clk->ops && clk->ops->recalc)) {
		unsigned long flags;

		spin_lock_irqsave(&clock_lock, flags);
		clk->ops->recalc(clk);
		spin_unlock_irqrestore(&clock_lock, flags);
	}

	if (unlikely(clk->flags & CLK_RATE_PROPAGATES))
		propagate_rate(clk);
}
EXPORT_SYMBOL_GPL(clk_recalc_rate);

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (likely(clk->ops && clk->ops->round_rate)) {
		unsigned long flags, rounded;

		spin_lock_irqsave(&clock_lock, flags);
		rounded = clk->ops->round_rate(clk, rate);
		spin_unlock_irqrestore(&clock_lock, flags);

		return rounded;
	}

	return clk_get_rate(clk);
}
EXPORT_SYMBOL_GPL(clk_round_rate);

/*
 * Returns a clock. Note that we first try to use device id on the bus
 * and clock name. If this fails, we try to use clock name only.
 */
struct clk *clk_get(struct device *dev, const char *id)
{
	struct clk *p, *clk = ERR_PTR(-ENOENT);
	int idno;

	if (dev == NULL || dev->bus != &platform_bus_type)
		idno = -1;
	else
		idno = to_platform_device(dev)->id;

	mutex_lock(&clock_list_sem);
	list_for_each_entry(p, &clock_list, node) {
		if (p->id == idno &&
		    strcmp(id, p->name) == 0 && try_module_get(p->owner)) {
			clk = p;
			goto found;
		}
	}

	list_for_each_entry(p, &clock_list, node) {
		if (strcmp(id, p->name) == 0 && try_module_get(p->owner)) {
			clk = p;
			break;
		}
	}

found:
	mutex_unlock(&clock_list_sem);

	return clk;
}
EXPORT_SYMBOL_GPL(clk_get);

void clk_put(struct clk *clk)
{
	if (clk && !IS_ERR(clk))
		module_put(clk->owner);
}
EXPORT_SYMBOL_GPL(clk_put);

static int show_clocks(char *buf, char **start, off_t off,
		       int len, int *eof, void *data)
{
	struct clk *clk;
	char *p = buf;

	list_for_each_entry_reverse(clk, &clock_list, node) {
		unsigned long rate = clk_get_rate(clk);

		/*
		 * Don't bother listing dummy clocks with no ancestry
		 * that only support enable and disable ops.
		 */
		if (unlikely(!rate && !clk->parent))
			continue;

		p += sprintf(p, "%-12s\t: %ld.%02ldMHz\n", clk->name,
			     rate / 1000000, (rate % 1000000) / 10000);
	}

	return p - buf;
}

static int __init clk_proc_init(void)
{
	struct proc_dir_entry *p;
	p = create_proc_read_entry("clocks", S_IRUSR, NULL,
				   show_clocks, NULL);
	if (unlikely(!p))
		return -EINVAL;

	return 0;
}
subsys_initcall(clk_proc_init);
