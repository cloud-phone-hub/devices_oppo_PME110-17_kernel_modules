# CPU Probe KO（popsicle-16 / Xiaomi 17）

只读 CPU 画像探针。用于联调时核对模块集工作状态：验证 DroidDDK 构建、模块加载、只读 proc/sysfs 输出与卸载清理；画像值与 `cdh_device_info.ko` 运行时展示保持同一口径。

## 边界

- 目标设备：`popsicle-16`（Xiaomi 17 / canoe / SM8850）。
- guest 内核 ABI：Android16 GKI 6.12，`vermagic=6.12.23-4k-cdh`；由 `.dddk-config` 固定 target `android16-6.12`。
- 只创建 `/proc/cdh_cpu_profile` 与 `/sys/kernel/cdh_cpu/{profile,clusters,midr_el1,cpufreq}`。
- 不修改 `/proc/cpuinfo`、`/sys/devices/system/cpu`，不注册 cpufreq policy、不装 hook。
- MIDR part/variant 与 meminfo 容量等占位项口径见 `../device_info/README.md`「取值依据与待真机取证项」。

## 构建

由设备仓统一构建：

```text
cd devices/xiaomi/popsicle-16
python3 build/scripts/build-kernel.py
```

产物：`out/kernel/modules/cdh_cpu_probe.ko`。

## 验证

加载后应出现：

```text
/proc/cdh_cpu_profile
/sys/kernel/cdh_cpu/profile
/sys/kernel/cdh_cpu/clusters
/sys/kernel/cdh_cpu/midr_el1
/sys/kernel/cdh_cpu/cpufreq
```

`profile` 输出（节选）应与真机画像一致：

```text
device=popsicle-16
android=16
cluster0=cpu0-5 oryon capacity=968 min=787200 max=3628800 midr=0x0000000051001f00
cluster1=cpu6-7 oryon capacity=1024 min=614400 max=4396800 midr=0x0000000051001f00
```
