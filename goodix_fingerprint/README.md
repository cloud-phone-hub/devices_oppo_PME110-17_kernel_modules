# Goodix Fingerprint KO（popsicle-16 / Xiaomi 17）

指纹注册面只读枚举 KO。Xiaomi 17 指纹为 Goodix US2 超声波方案（与触摸一体），真机注册面由官方 `mi_fp.ko` 提供；云机 vendor_dlkm 为自产模块集、不含官方 `mi_fp.ko`，本模块只补 kernel 可见注册面（AIDA64/DevInfo 类读取路径），不提供真实指纹能力、不发送任何 input event。

## 注册面（对 vendor_dlkm/mi_fp.ko 反汇编 + dtbo 节点定案）

| 项 | 真机值 | 实现 |
| --- | --- | --- |
| platform driver | `xiaomi-fp` | 空 platform_driver，生成 `/sys/bus/platform/drivers/xiaomi-fp` |
| class | `xiaomi_fp` | `class_create("xiaomi_fp")`，生成 `/sys/class/xiaomi_fp` |
| input 设备名 | `uinput-xiaomi` | `input_register_device` |
| input id | bus/version=0、vendor=0x666、product=0x888 | `input_dev.id` 同值 |
| 按键 capability | KEY_HOME / KEY_MENU / KEY_BACK / KEY_POWER / BTN_C | 取自 mi_fp.ko `input_set_capability` 调用点 |
| DT 节点 | `xiaomi_fingerprint`（compatible `xiaomi,xiaomi-fp`，gpio_irq=<&tlmm 0x63>） | 仅在 dtbo 中，guest DT 不需补 |

## 边界

- 不注册真正向 HAL 提供 `fpid`/`qbt_ipc` 的 cdev（真机 `/dev/xiaomi-fp` 节点不做），HAL 若要 open 该节点会失败——先记录为已知差异，等指纹链路联调再决策。
- 不注册 platform device、不触发真实硬件 probe。
- 不使用 uinput daemon、framework/native hook、kallsyms、kprobe、ftrace、syscall hook 或 text patch。
- 不发送 `KEY_POWER`、`KEY_HOME` 或任何其它 input event；不影响 QEMU tablet/mouse 或 Android input dispatcher。

## 构建

由设备仓统一构建：

```text
cd devices/xiaomi/popsicle-16
python3 build/scripts/build-kernel.py
```

产物：`out/kernel/modules/cdh_goodix_fingerprint.ko`（`vermagic=6.12.23-4k-cdh`）。

## 验证

```text
adb -s 127.0.0.1:5901 shell 'cat /proc/modules | grep cdh_goodix_fingerprint'
adb -s 127.0.0.1:5901 shell 'ls -ld /sys/bus/platform/drivers/xiaomi-fp /sys/class/xiaomi_fp'
adb -s 127.0.0.1:5901 shell 'getevent -il | sed -n "/uinput-xiaomi/,+25p"'
adb -s 127.0.0.1:5901 shell 'dumpsys input | grep -A30 -B5 uinput-xiaomi'
```

预期：driver/class 目录存在；`getevent -il` 与 `dumpsys input` 能看到 `uinput-xiaomi`（id 666:0888、bus 0）；无事件注入。
