# Kernel Modules（popsicle-16 内核模块源码）

本目录是 popsicle-16（Xiaomi 17 / canoe / SM8850）云机的 4 个 CDH 内核模块源码树，由 note13-5G-15 同款模块移植并按其口径重标定。`kernel/build/build-kernel.sh` 编译前强校验每个目录含 `Kbuild`，缺失即 `exit 3`：

| 目录 | 产物 | 作用 |
| --- | --- | --- |
| `goodix_fingerprint/` | `cdh_goodix_fingerprint.ko` | mi_fp（Goodix US2）注册面只读枚举 |
| `cpu_probe/` | `cdh_cpu_probe.ko` | CPU 画像只读探针（联调核对） |
| `device_info/` | `cdh_device_info.ko` | UTS / meminfo / uptime / cpuinfo / cpufreq / topology / MIDR / cache 展示对齐 |
| `virtio_media/` | `cdh_media.ko` | virtio-media 视频通路驱动（chromeos upstream v0.0.8） |

产物随内核一起归集到 `out/kernel/modules/`（再同步到 `out/dist/kernel/modules/` 供 `build-vendor_dlkm.py` 打包进 vendor_dlkm.img）。

## ABI 与构建

- 全部模块 `.dddk-config` 固定 `version=android16-6.12`（builder `ghcr.io/feicong/droid-ddk:android16-6.12`），发布 vermagic 门禁 `6.12.23-4k-cdh`（真机官方内核为 `6.12.23-android16-5-...-4k`）。
- 统一入口（相对设备根）：

```text
python3 build/scripts/build-kernel.py            # 内核 + 全部模块 + 归集
python3 build/scripts/build-kernel.py --only-build
```

## 移植口径（与 note13-5G-15 的差异）

- **hook 机制整体复用**：`android_vh_si_meminfo_adjust` / `android_vh_meminfo_proc_show` / `android_rvh_cpuinfo_c_show` / `android_rvh_cpu_capacity_show`、kretprobe `__arm64_sys_sysinfo` / `uptime_proc_show`、kprobe 解析 `cpu_data` 与 cacheinfo 私有符号（`__nocfi` 包装）、空 platform_driver + input 枚举的边界设计均照搬 note13，已在 6.12 源码逐点核验（含 RESTRICTED hook 调用点、EXPORT 面、私有符号存在性）。
- **身份值按 popsicle 重标定**：uname 双串出自官方镜像内核元数据；CPU 6+2 簇、capacity 968/1024、cpufreq 频率、指纹注册面均出自官方 OS3.0.319.0 镜像（DTB / vendor perf 配置 / mi_fp.ko 反汇编）。逐项来源与待真机取证清单见 `device_info/README.md`。
- **note13 值锚点**（仅作对照）：MT6833、`6.6.127-4k-cdh`、`MemTotal=7760724` 等为 note13 真机实测值；popsicle 不复用其设备值（meminfo 数值暂与 note13 相同是云机 8GB 物理内存上限所致，非沿用设备值）。
- **指纹供应商已定案**：Goodix US2 超声波（与触摸一体），注册面 `xiaomi-fp` / `uinput-xiaomi` / class `xiaomi_fp`，见 `goodix_fingerprint/README.md`；note13 的 `goodix_fp` / `goodix_fingerprint` 命名不适用。
- **virtio_media** 与设备无关，代码零改动，仅 `.dddk-config` 与构建 ABI 随 android16-6.12。
