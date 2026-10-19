// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)
 */
#define pr_fmt(fmt) "cdh_device_info_cpu: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <trace/hooks/cpuinfo.h>
#include <trace/hooks/topology.h>

#include "cdh_device_info.h"

/*
 * Features 取当前一代骁龙旗舰(ARMv9 Oryon)公共 HWCAP 集；待真机
 * /proc/cpuinfo 复核后按真机行原样替换。
 */
static const char cdh_cpu_features[] =
	"fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp cpuid asimdrdm lrcpc dcpop asimddp sha3 asimdfhm dit uscat ilrcpc flagm ssbs sb paca pacg dcpodp flagm2 frint i8mm bf16 dgh rng bti";

struct cdh_cpu_core {
	unsigned int cpu;
	const char *bogo_mips;
	const char *implementer;
	unsigned int variant;
	const char *part;
	unsigned int revision;
	unsigned long capacity;
};

static const struct cdh_cpu_core cdh_cpu_cores[] = {
	/*
	 * Xiaomi 17 (SM8850) 6+2 双簇 Oryon。BogoMIPS 38.40 = 2 × 19.2MHz arch
	 * timer，高通平台真机稳定显示值；implementer 0x51=Qualcomm，Oryon 的
	 * variant/part/revision 暂无官方镜像内取证，先按 0x0/0x1f0/0 占位并
	 * 与 midr_sysfs.c 保持一致，待真机 /proc/cpuinfo 复核。
	 * capacity 为 6.12 公式按 DTB capacity-dmips-mhz(0x700/0x766) 的定标值。
	 */
	{ 0, "38.40", "0x51", 0x0, "0x1f0", 0, 968 },
	{ 1, "38.40", "0x51", 0x0, "0x1f0", 0, 968 },
	{ 2, "38.40", "0x51", 0x0, "0x1f0", 0, 968 },
	{ 3, "38.40", "0x51", 0x0, "0x1f0", 0, 968 },
	{ 4, "38.40", "0x51", 0x0, "0x1f0", 0, 968 },
	{ 5, "38.40", "0x51", 0x0, "0x1f0", 0, 968 },
	{ 6, "38.40", "0x51", 0x0, "0x1f0", 0, 1024 },
	{ 7, "38.40", "0x51", 0x0, "0x1f0", 0, 1024 },
};

static bool cdh_cpuinfo_registered;
static bool cdh_cpu_capacity_registered;
static bool cdh_cpu_display_pinned;

static const struct cdh_cpu_core *cdh_cpu_target(unsigned int cpu)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(cdh_cpu_cores); i++) {
		if (cdh_cpu_cores[i].cpu == cpu)
			return &cdh_cpu_cores[i];
	}
	return NULL;
}

static void cdh_cpuinfo_show(void *data, struct seq_file *m)
{
	size_t i;

	m->count = 0;
	m->from = 0;
	for (i = 0; i < ARRAY_SIZE(cdh_cpu_cores); i++) {
		const struct cdh_cpu_core *core = &cdh_cpu_cores[i];

		seq_printf(m, "processor\t: %u\n", core->cpu);
		seq_printf(m, "BogoMIPS\t: %s\n", core->bogo_mips);
		seq_printf(m, "Features\t: %s\n", cdh_cpu_features);
		seq_printf(m, "CPU implementer\t: %s\n", core->implementer);
		seq_puts(m, "CPU architecture: 8\n");
		seq_printf(m, "CPU variant\t: 0x%x\n", core->variant);
		seq_printf(m, "CPU part\t: %s\n", core->part);
		seq_printf(m, "CPU revision\t: %u\n\n", core->revision);
	}
	/* 真机 (SM8850) /proc/cpuinfo 无 Hardware 行，保持与真机一致不输出。 */
}

static void cdh_cpu_capacity_show(void *data, unsigned long *capacity, int cpu)
{
	const struct cdh_cpu_core *core;

	if (!capacity || cpu < 0)
		return;
	core = cdh_cpu_target((unsigned int)cpu);
	if (!core)
		return;
	*capacity = core->capacity;
}

int cdh_cpu_display_init(void)
{
	int ret;

	ret = register_trace_android_rvh_cpuinfo_c_show(cdh_cpuinfo_show, NULL);
	if (ret)
		return ret;
	cdh_cpuinfo_registered = true;

	ret = register_trace_android_rvh_cpu_capacity_show(cdh_cpu_capacity_show, NULL);
	if (ret) {
		pr_err("registered /proc/cpuinfo hook but cpu_capacity hook failed:%d\n", ret);
		__module_get(THIS_MODULE);
		cdh_cpu_display_pinned = true;
		return 0;
	}
	cdh_cpu_capacity_registered = true;

	__module_get(THIS_MODULE);
	cdh_cpu_display_pinned = true;
	pr_info("registered permanent /proc/cpuinfo and cpu_capacity display hooks\n");
	return 0;
}

void cdh_cpu_display_exit(void)
{
	if (!cdh_cpuinfo_registered && !cdh_cpu_capacity_registered)
		return;

	pr_info("restricted CPU display hooks are permanent; pinned=%d\n",
		cdh_cpu_display_pinned);
}
