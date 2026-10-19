// Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)
/*
 * Xiaomi 17 (SM8850 / popsicle) 指纹 = Goodix US2 超声波方案，与触摸一体；
 * 真机注册面由官方 mi_fp.ko 提供（对 vendor_dlkm/mi_fp.ko 反汇编定案）：
 *   - platform driver 名 "xiaomi-fp"
 *   - class "xiaomi_fp"（HAL 侧 /dev/xiaomi-fp 由 mi_fp 的 cdev 提供）
 *   - input 设备名 "uinput-xiaomi"，id.bustype/version=0、vendor=0x666、
 *     product=0x888，按键 HOME/MENU/BACK/POWER/BTN_C
 *   - DT 节点 xiaomi_fingerprint(compatible "xiaomi,xiaomi-fp")
 * 云机 vendor_dlkm 为自产模块集、不含官方 mi_fp.ko，本模块只补上述可见
 * 注册面（不发输入事件、不实现指纹功能）。
 */
#include <linux/device/class.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define CDH_FP_DRIVER_NAME "xiaomi-fp"
#define CDH_FP_CLASS_NAME "xiaomi_fp"
#define CDH_FP_INPUT_NAME "uinput-xiaomi"
#define CDH_FP_INPUT_VENDOR 0x666
#define CDH_FP_INPUT_PRODUCT 0x888

static struct input_dev *cdh_fp_input;
static struct class *cdh_fp_class;

static struct platform_driver cdh_fp_platform_driver = {
    .driver =
        {
            .name = CDH_FP_DRIVER_NAME,
            .owner = THIS_MODULE,
        },
};

/* 按键集取自 mi_fp.ko 的 input_set_capability 调用点。 */
static const unsigned int cdh_fp_keys[] = {
    KEY_HOME, KEY_MENU, KEY_BACK, KEY_POWER, BTN_C,
};

static int cdh_fp_register_input(void) {
  struct input_dev *input;
  int ret;
  size_t i;

  input = input_allocate_device();
  if (!input)
    return -ENOMEM;

  input->name = CDH_FP_INPUT_NAME;
  input->phys = "xiaomi-fp/input0";
  input->id.bustype = 0;
  input->id.vendor = CDH_FP_INPUT_VENDOR;
  input->id.product = CDH_FP_INPUT_PRODUCT;
  input->id.version = 0;

  for (i = 0; i < ARRAY_SIZE(cdh_fp_keys); i++)
    input_set_capability(input, EV_KEY, cdh_fp_keys[i]);

  ret = input_register_device(input);
  if (ret) {
    input_free_device(input);
    return ret;
  }

  cdh_fp_input = input;
  return 0;
}

static void cdh_fp_unregister_input(void) {
  if (!cdh_fp_input)
    return;
  input_unregister_device(cdh_fp_input);
  cdh_fp_input = NULL;
}

static int __init cdh_fp_init(void) {
  int ret;

  ret = platform_driver_register(&cdh_fp_platform_driver);
  if (ret)
    return ret;

  cdh_fp_class = class_create(CDH_FP_CLASS_NAME);
  if (IS_ERR(cdh_fp_class)) {
    ret = PTR_ERR(cdh_fp_class);
    cdh_fp_class = NULL;
    goto err_unregister_driver;
  }

  ret = cdh_fp_register_input();
  if (ret)
    goto err_class_destroy;

  pr_info("cdh_goodix_fingerprint: registered %s, %s and %s\n",
          CDH_FP_DRIVER_NAME, CDH_FP_CLASS_NAME, CDH_FP_INPUT_NAME);
  return 0;

err_class_destroy:
  class_destroy(cdh_fp_class);
  cdh_fp_class = NULL;
err_unregister_driver:
  platform_driver_unregister(&cdh_fp_platform_driver);
  return ret;
}

static void __exit cdh_fp_exit(void) {
  cdh_fp_unregister_input();
  if (cdh_fp_class) {
    class_destroy(cdh_fp_class);
    cdh_fp_class = NULL;
  }
  platform_driver_unregister(&cdh_fp_platform_driver);
  pr_info("cdh_goodix_fingerprint: removed\n");
}

module_init(cdh_fp_init);
module_exit(cdh_fp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fei_cong <fei_cong@hotmail.com>");
MODULE_DESCRIPTION("Read-only Android16 Xiaomi mi_fp (Goodix US2) sysfs/input "
                   "enumerator for CDH");
