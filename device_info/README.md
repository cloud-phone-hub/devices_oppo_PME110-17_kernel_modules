# Device Info KO（popsicle-16 / Xiaomi 17）

Android16 设备信息展示对齐模块。链接为单个 `cdh_device_info.ko`，覆盖 UTS/`uname(2)` 可见字段、`/proc/meminfo` capacity 展示字段、`sysinfo(2).uptime` / `/proc/uptime` 启动时长、`/proc/cpuinfo` 与 `cpu_capacity` 展示字段、只读 CPU cpufreq sysfs、`cpu_topology[]` 派生 topology sysfs、arm64 `cpu_data.reg_midr`（`midr_el1` sysfs 来源）以及 `cpuN/cache` 父设备隐藏。

## 边界

- 目标设备：`popsicle-16`（Xiaomi 17 / canoe / SM8850，HyperOS OS3.0.319.0.WPBCNXM / Android 16）。
- guest 内核 ABI：Android16 GKI 6.12；模块固定用 `ghcr.io/feicong/droid-ddk:android16-6.12` 构建，`vermagic=6.12.23-4k-cdh`。
- 用户态展示目标 `uname -r`：`6.12.23-android16-5-g75e9b1c7ae7c-abogki463945075-4k`；ABI 校验目标：`gki_abi_release=6.12.23-4k-cdh`。
- `init_uts_ns.name` 用真机 HyperOS 字段改写（release/version 双串），App、toybox `uname` 与 libc `uname(2)` 同源。
- `android_vh_si_meminfo_adjust` 改写 `si_meminfo()` 的 `totalram`；`android_vh_meminfo_proc_show` 只在 `/proc/meminfo` 已生成缓冲区内原位替换 `SwapTotal` / `CommitLimit` / `CmaTotal` / `VmallocTotal`，不追加重复字段。
- kretprobe 同时处理 `__arm64_sys_sysinfo` 与 `uptime_proc_show`，为两条 uptime 读取路径增加相同 offset（`uptime_offset_seconds`，默认 172800）。
- `android_rvh_cpuinfo_c_show` 伪造 8 个 CPU 的 `/proc/cpuinfo` 块（SM8850 无 `Hardware` 行）；`android_rvh_cpu_capacity_show` 输出 6+2 簇容量（968/1024）。
- `get_cpu_device()` + 只读 `cpufreq` kobject 补 `cpuinfo_min/max_freq`、`scaling_min/max/cur_freq`、`scaling_governor`、`scaling_available_governors`、`affected_cpus`、`related_cpus`（virtio guest 无真实 cpufreq policy 目录）。
- 直接改写 `cpu_topology[]`（topology sysfs canonical source）：cluster0=cpu0-5、cluster1=cpu6-7，core_siblings=ff、cluster_cpus=3f/c0、LLC(L2)=ff，卸载恢复。
- kprobe 解析未导出的 per-cpu `cpu_data`，写 `struct cpuinfo_arm64.reg_midr`（`midr_el1` sysfs canonical source，不建替代节点）。
- kprobe 解析私有 `cpu_cache_sysfs_exit` / `cacheinfo_cpu_online` / `cache_dev_map`，隐藏 `cpuN/cache` 父设备并在卸载时恢复；私有回调必须走 `__nocfi` 包装（GKI kCFI 对模块间间接调用会 panic）。
- 不改真实 boot image、`vermagic`、QEMU/HVF CPU 能力或 Android property source；不伪造 Free/抖动类字段（`MemFree`、`MemAvailable` 等只保证关系约束）。

## 取值依据与待真机取证项

镜像内已定案（值出自官方 OS3.0.319.0 镜像）：

| 项 | 值 | 来源 |
| --- | --- | --- |
| uname release/version | `6.12.23-android16-5-...-4k` / `#1 SMP PREEMPT Thu Nov 27 08:45:40 UTC 2025` | factory 包 kernel cmdline / 镜像元数据 |
| 簇结构 | cpu0-5 + cpu6-7（6+2） | canoe DTB `cpus` 节点 |
| capacity 定标 | 968 / 1024 | DTB `capacity-dmips-mhz` 0x700/0x766 按 6.12 公式 |
| cpufreq | gold 787200-3628800、prime 614400-4396800 | canoe DTB `qcom,memlat` 表下限 + vendor perf 配置上限 |
| 指纹注册面 | `xiaomi-fp` / `uinput-xiaomi` / class `xiaomi_fp` / 0x666·0x888 / 5 键 | vendor_dlkm `mi_fp.ko` 反汇编 + dtbo `xiaomi_fingerprint` 节点 |

占位待真机复核（真机 `cat /proc/cpuinfo` / `midr_el1` sysfs / `meminfo` 取证后替换）：

- MIDR：implementer `0x51`（Qualcomm，高置信）；variant/part/revision `0x0/0x1f0/0` 为占位（与 `cpu_display.c`、`midr_sysfs.c` 三口一致）。
- BogoMIPS `38.40`（2 × 19.2MHz arch timer，高通平台惯例值）。
- Features 行：取当前代骁龙旗舰 ARMv9 公共 HWCAP 集（含 sha3/asimdfhm/i8mm/bf16/paca/pacg 等）。
- meminfo：`MemTotal=7760724` 等 5 值为 8GB 档同级数值——上限受云机 guest 物理内存约束（device.go `Popsicle16 MemoryMB=8192`）；Xiaomi 17 最小在售 12GB，真机取证或云机扩容后需同步上调（`CommitLimit` 满足 overcommit_ratio=50% 口径）。

## 源码组织

```text
core/    模块 init/exit、module_param 与统一调度
uts/     UTS namespace / uname 展示层
meminfo/ si_meminfo/sysinfo 与 /proc/meminfo capacity 展示层
time/    sysinfo(2).uptime 与 /proc/uptime 启动时长展示层
cpu/     /proc/cpuinfo、cpu_capacity、topology、MIDR、cache 与只读 cpufreq sysfs 展示层
include/ 模块内共享头文件
```

新增展示项时只追加子目录源码并更新 `Kbuild` 的 `$(MODULE_NAME)-objs`，最终仍链接为一个 `cdh_device_info.ko`。

## 构建

由设备仓统一构建（模块源码随内核一起进 builder 容器）：

```text
cd devices/xiaomi/popsicle-16
python3 build/scripts/build-kernel.py            # 内核 + 全部模块 + 归集 out/dist/kernel/modules
python3 build/scripts/build-kernel.py --dry-run  # 仅打印构建配置
```

产物：`out/kernel/modules/cdh_device_info.ko`。vermagic 校验：

```text
strings out/kernel/modules/cdh_device_info.ko | rg '^(description=|license=|vermagic=)'
# 期望 vermagic=6.12.23-4k-cdh
```

`insmod` 报 `Unknown symbol` / `Invalid module format` / `Exec format error` / UTS 或 MIDR `cpu_data` 解析失败时直接失败并归档 `dmesg`，不回退到 App/framework/native hook。
