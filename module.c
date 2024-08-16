/*
 * stratopimax
 *
 *     Copyright (C) 2023-2024 Sfera Labs S.r.l.
 *
 *     For information, visit https://www.sferalabs.cc
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * LICENSE.txt file for more details.
 *
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/version.h>

#include "atecc/atecc.h"
#include "commons/commons.h"
#include "gpio/gpio.h"
#include "rp2_i2c.h"

#define DEBUG_I2C 0

#define X2_UPS 2
#define X2_CAN_485 3
#define X2_232_485 4
#define X2_D7 5
#define X2_PCAP 8

#define LOG_TAG "stratopimax: "

#define MCP3204_FACTOR 15ul
#define MCP3204_VREF 3000ul

struct DeviceAttrRegSpecs {
  uint8_t reg;
  uint8_t len;
  uint32_t mask;
  uint8_t shift;
  bool sign;
  uint8_t base;
};

struct DeviceAttrBean {
  struct device_attribute devAttr;
  struct DeviceAttrRegSpecs regSpecs;
  struct DeviceAttrRegSpecs regSpecsStore;
  uint8_t bitMapLen;
  uint8_t bitMapStart;
  struct GpioBean *gpio;
  const char *vals;
};

struct DeviceBean {
  char *name;
  struct DeviceAttrBean *devAttrBeans;
  struct device *device;
  uint8_t *expbTypes;
};

struct DeviceData {
  int8_t expbIdx;
  struct device *device;
  struct DeviceData *next;
};

struct ExpbBean {
  uint8_t type;
  struct DeviceData *data;
};

static ssize_t devAttrI2c_store(struct device *dev,
                                struct device_attribute *attr, const char *buf,
                                size_t count);

static ssize_t devAttrI2c_show(struct device *dev,
                               struct device_attribute *attr, char *buf);

static ssize_t devAttrI2c_show_clear(struct device *dev,
                                     struct device_attribute *attr, char *buf);

static ssize_t devAttrFwVersion_show(struct device *dev,
                                     struct device_attribute *attr, char *buf);

static ssize_t devAttrConfig_store(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t count);

static ssize_t devAttrI2cRead_show(struct device *dev,
                                   struct device_attribute *attr, char *buf);

static ssize_t devAttrI2cRead_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count);

static ssize_t devAttrI2cWrite_store(struct device *dev,
                                     struct device_attribute *attr,
                                     const char *buf, size_t count);

static ssize_t devAttrUpsBatteryV_store(struct device *dev,
                                        struct device_attribute *attr,
                                        const char *buf, size_t count);

static ssize_t devAttrUpsBatteryV_show(struct device *dev,
                                       struct device_attribute *attr,
                                       char *buf);

static ssize_t devAttrLm75a_show(struct device *dev,
                                 struct device_attribute *attr, char *buf);

static ssize_t devAttrLm75a_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count);

static ssize_t devAttrBlink_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count);

static ssize_t devAttrMcp3204Ch0Mv_show(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf);

static ssize_t devAttrMcp3204Ch1Mv_show(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf);

static ssize_t devAttrMax4896Switch_store(struct device *dev,
                                          struct device_attribute *attr,
                                          const char *buf, size_t count);

static const char VALS_SD_SDX_ROUTING[] = {2, 'A', 'B'};

static struct GpioBean gpioSdRoute = {
    .name = "stratopimax_sd_route",
    .flags = GPIOD_IN,
};

static struct DeviceAttrBean devAttrBeansSystem[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "fw_version",
                        .mode = 0440,
                    },
                .show = devAttrFwVersion_show,
                .store = NULL,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "config",
                        .mode = 0220,
                    },
                .show = NULL,
                .store = devAttrConfig_store,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "sys_errs",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show_clear,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_SYSMON_SYS_ERRS,
                .len = 2,
                .mask = 0x1fff,
                .base = 2,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "ioexp_errs",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show_clear,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_SYSMON_IOEXP_ERRS,
                .len = 2,
                .mask = 0x7f,
                .base = 2,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "_i2c_read",
                        .mode = 0660,
                    },
                .show = devAttrI2cRead_show,
                .store = devAttrI2cRead_store,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "_i2c_write",
                        .mode = 0220,
                    },
                .show = NULL,
                .store = devAttrI2cWrite_store,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansSd[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "sd_main_enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_SD,
                .len = 2,
                .mask = 0b1,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "sd_main_enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_SD,
                .len = 2,
                .mask = 0b1,
                .shift = 1,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "sd_sec_enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_SD,
                .len = 2,
                .mask = 0b1,
                .shift = 2,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "sd_sec_enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_SD,
                .len = 2,
                .mask = 0b1,
                .shift = 3,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "sd_main_routing",
                        .mode = 0660,
                    },
                .show = devAttrGpio_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_SD,
                .len = 2,
                .mask = 0b1,
                .shift = 4,
                .sign = false,
            },
        .gpio = &gpioSdRoute,
        .vals = VALS_SD_SDX_ROUTING,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "sd_main_routing_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_SD,
                .len = 2,
                .mask = 0b1,
                .shift = 5,
                .sign = false,
            },
        .vals = VALS_SD_SDX_ROUTING,
    },

    {},
};

static struct DeviceAttrBean devAttrBeansExpb[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s1_enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_EN,
                .len = 2,
                .mask = 0b1,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s1_enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_EN,
                .len = 2,
                .mask = 0b1,
                .shift = 1,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s1_type",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_TYPE_S1,
                .len = 2,
                .mask = 0xff,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s2_enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_EN,
                .len = 2,
                .mask = 0b1,
                .shift = 2,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s2_enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_EN,
                .len = 2,
                .mask = 0b1,
                .shift = 3,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s2_type",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_TYPE_S2,
                .len = 2,
                .mask = 0xff,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s3_enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_EN,
                .len = 2,
                .mask = 0b1,
                .shift = 4,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s3_enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_EN,
                .len = 2,
                .mask = 0b1,
                .shift = 5,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s3_type",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_TYPE_S3,
                .len = 2,
                .mask = 0xff,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s4_enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_EN,
                .len = 2,
                .mask = 0b1,
                .shift = 6,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s4_enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_EN,
                .len = 2,
                .mask = 0b1,
                .shift = 7,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "s4_type",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_EXPB_TYPE_S4,
                .len = 2,
                .mask = 0xff,
                .shift = 0,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansPower[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "down_enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_POWER_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "up_backup_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_POWER_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 1,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "sd_switch_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_POWER_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 2,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "pcie_switch_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_POWER_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 3,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "down_delay_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_POWER_DOWN_DELAY,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "off_time_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_POWER_OFF_TIME,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "up_delay_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_POWER_UP_DELAY,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansWatchdog[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_WDT_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_WDT_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 1,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "heartbeat",
                        .mode = 0220,
                    },
                .show = NULL,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_WDT_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 2,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "expired",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_WDT_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 2,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "timeout",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_WDT_TIMEOUT,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "timeout_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_WDT_TIMEOUT_CFG,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "down_delay_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_WDT_DOWN_DELAY,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "sd_switch_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_WDT_SD_SWITCH_CNT,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "pcie_switch_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_WDT_PCIE_SWITCH_CNT,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansUsb[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "usb1_enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_USB,
                .len = 2,
                .mask = 1,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "usb1_enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_USB,
                .len = 2,
                .mask = 1,
                .shift = 1,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "usb1_err",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
            },
        .regSpecs =
            {
                .reg = I2C_REG_USB,
                .len = 2,
                .mask = 1,
                .shift = 2,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "usb2_enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_USB,
                .len = 2,
                .mask = 1,
                .shift = 8,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "usb2_enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_USB,
                .len = 2,
                .mask = 1,
                .shift = 9,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "usb2_err",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
            },
        .regSpecs =
            {
                .reg = I2C_REG_USB,
                .len = 2,
                .mask = 1,
                .shift = 10,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansPowerIn[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "mon_v",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_SYSMON_VIN_V,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "mon_i",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_SYSMON_VIN_I,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansPcie[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_PCIE,
                .len = 2,
                .mask = 1,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_PCIE,
                .len = 2,
                .mask = 1,
                .shift = 1,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansUps[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 1,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "down_delay_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_POWER_DOWN_DELAY,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "backup",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_STATE,
                .len = 2,
                .mask = 0b1,
                .shift = 7,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "status",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_STATE,
                .len = 2,
                .mask = 0b1111,
                .shift = 0,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansUpsBattery[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "battery_v_config",
                        .mode = 0660,
                    },
                .show = devAttrUpsBatteryV_show,
                .store = devAttrUpsBatteryV_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 2,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "battery_capacity_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_CAPACITY,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "battery_i_max",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_MAX_CHARGE_I,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "battery_i_max_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_MAX_CHARGE_I_CFG,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "battery",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_STATE,
                .len = 2,
                .mask = 0b1,
                .shift = 7,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "charger_mon_v",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_VBAT_V,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "charger_mon_i",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_VBAT_I,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansPowerOut[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "vso_enabled",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 8,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "vso_enabled_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_OFST_UPS_MAIN,
                .len = 2,
                .mask = 0b1,
                .shift = 9,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansButton[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "status",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_BUTTON,
                .len = 2,
                .mask = 1,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "count",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_BUTTON,
                .len = 2,
                .mask = 0xff,
                .shift = 8,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansBuzzer[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "beep",
                        .mode = 0220,
                    },
                .show = NULL,
                .store = devAttrBlink_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_BUZZER_T_ON,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "tone",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_BUZZER_TONE,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansLed[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "red",
                        .mode = 0220,
                    },
                .show = NULL,
                .store = devAttrBlink_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_LED_RED_T_ON,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "green",
                        .mode = 0220,
                    },
                .show = NULL,
                .store = devAttrBlink_store,
            },
        .regSpecs =
            {
                .reg = I2C_REG_LED_GREEN_T_ON,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansAtecc[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "serial_num",
                        .mode = 0440,
                    },
                .show = devAttrAteccSerial_show,
                .store = NULL,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansFan[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "temp",
                        .mode = 0440,
                    },
                .show = devAttrLm75a_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = 0,
                .mask = 0xe0,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "temp_on",
                        .mode = 0660,
                    },
                .show = devAttrLm75a_show,
                .store = devAttrLm75a_store,
            },
        .regSpecs =
            {
                .reg = 3,
                .mask = 0x80,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "temp_off",
                        .mode = 0660,
                    },
                .show = devAttrLm75a_show,
                .store = devAttrLm75a_store,
            },
        .regSpecs =
            {
                .reg = 2,
                .mask = 0x80,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansAccel[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "accel_x",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_ACCEL_X,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = true,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "accel_y",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_ACCEL_Y,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = true,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "accel_z",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_ACCEL_Z,
                .len = 2,
                .mask = 0,
                .shift = 0,
                .sign = true,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansDIn[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "in%d_wb_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 0,
                .len = 2,
                .mask = 0b1,
                .shift = 1,
                .sign = false,
            },
        .bitMapLen = 7,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "in%d_filter_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 1,
                .len = 4,
                .mask = 0xf,
                .shift = 4,
                .sign = false,
            },
        .bitMapLen = 7,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "in%d",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = 4,
                .len = 2,
                .mask = 0x1,
                .shift = 1,
                .sign = false,
            },
        .bitMapLen = 7,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "inputs",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = 4,
                .len = 2,
                .mask = 0x7f,
                .shift = 0,
                .sign = false,
                .base = 2,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "in%d_wb",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 4,
                .len = 2,
                .mask = 0x1,
                .shift = 1,
                .sign = false,
            },
        .bitMapLen = 7,
        .bitMapStart = 8,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "inputs_wb",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 4,
                .len = 2,
                .mask = 0x7f,
                .shift = 8,
                .sign = false,
                .base = 2,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "alarm_t1",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 6,
                .len = 2,
                .mask = 0x1,
                .shift = 0,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "alarm_t2",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 6,
                .len = 2,
                .mask = 0x1,
                .shift = 1,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "over_temp",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 6,
                .len = 2,
                .mask = 0x1,
                .shift = 2,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "fault",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = 6,
                .len = 2,
                .mask = 0x1,
                .shift = 3,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansDOut[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "out%d_pp_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 2,
                .len = 4,
                .mask = 0b1,
                .shift = 1,
                .sign = false,
            },
        .bitMapLen = 7,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "out%d_ol_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 2,
                .len = 4,
                .mask = 0b1,
                .shift = 1,
                .sign = false,
            },
        .bitMapLen = 7,
        .bitMapStart = 8,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "join_l_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 2,
                .len = 4,
                .mask = 0b1,
                .shift = 16,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "join_h_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 2,
                .len = 4,
                .mask = 0b1,
                .shift = 17,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "watchdog_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 2,
                .len = 4,
                .mask = 0b1,
                .shift = 18,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "watchdog_timeout_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 2,
                .len = 4,
                .mask = 0b11,
                .shift = 19,
                .sign = false,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "out%d",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 5,
                .len = 2,
                .mask = 0x1,
                .shift = 1,
                .sign = false,
            },
        .bitMapLen = 7,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "outputs",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 5,
                .len = 2,
                .mask = 0x7f,
                .shift = 0,
                .sign = false,
                .base = 2,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "out%d_ol",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 5,
                .len = 2,
                .mask = 0x1,
                .shift = 1,
                .sign = false,
            },
        .bitMapLen = 7,
        .bitMapStart = 8,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "outputs_ol",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 5,
                .len = 2,
                .mask = 0x7f,
                .shift = 8,
                .sign = false,
                .base = 2,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "out%d_ov",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 7,
                .len = 4,
                .mask = 0x1,
                .shift = 1,
                .sign = false,
            },
        .bitMapLen = 7,
        .bitMapStart = 0,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "outputs_ov",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 7,
                .len = 4,
                .mask = 0x7f,
                .shift = 0,
                .sign = false,
                .base = 2,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "out%d_ot",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 7,
                .len = 4,
                .mask = 0x1,
                .shift = 1,
                .sign = false,
            },
        .bitMapLen = 7,
        .bitMapStart = 8,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "outputs_ot",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 7,
                .len = 4,
                .mask = 0x7f,
                .shift = 8,
                .sign = false,
                .base = 2,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "out%d_ov_lock",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = 7,
                .len = 4,
                .mask = 0x1,
                .shift = 1,
                .sign = false,
            },
        .bitMapLen = 7,
        .bitMapStart = 16,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "outputs_ov_lock",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = 7,
                .len = 4,
                .mask = 0x7f,
                .shift = 16,
                .sign = false,
                .base = 2,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "out%d_ot_lock",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = 7,
                .len = 4,
                .mask = 0x1,
                .shift = 1,
                .sign = false,
            },
        .bitMapLen = 7,
        .bitMapStart = 24,
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "outputs_ot_lock",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = 7,
                .len = 4,
                .mask = 0x7f,
                .shift = 24,
                .sign = false,
                .base = 2,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansRs485[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "echo_config",
                        .mode = 0660,
                    },
                .show = devAttrI2c_show,
                .store = devAttrI2c_store,
            },
        .regSpecs =
            {
                .reg = 0,
                .len = 2,
                .mask = 0b1,
                .shift = 0,
                .sign = false,
            },
    },

    {},
};

static struct DeviceAttrBean devAttrBeansTest[] = {
    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "_ups_vso",
                        .mode = 0440,
                    },
                .show = devAttrMcp3204Ch0Mv_show,
                .store = NULL,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "_ups_vbat",
                        .mode = 0440,
                    },
                .show = devAttrMcp3204Ch1Mv_show,
                .store = NULL,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "_ups_pwr_switch",
                        .mode = 0220,
                    },
                .show = NULL,
                .store = devAttrMax4896Switch_store,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "_ate_adc_ch2",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_ATE_ADC_CH2,
                .len = 2,
            },
    },

    {
        .devAttr =
            {
                .attr =
                    {
                        .name = "_ate_adc_ch3",
                        .mode = 0440,
                    },
                .show = devAttrI2c_show,
                .store = NULL,
            },
        .regSpecs =
            {
                .reg = I2C_REG_ATE_ADC_CH3,
                .len = 2,
            },
    },

    {},
};

static uint8_t devUpsExpbTypes[] = {X2_UPS, X2_PCAP, 0};

static uint8_t devUpsBatteryExpbTypes[] = {X2_UPS, 0};

static uint8_t devDInExpbTypes[] = {X2_D7, 0};

static uint8_t devDOutExpbTypes[] = {X2_D7, 0};

static uint8_t devRs485ExpbTypes[] = {X2_232_485, X2_CAN_485, 0};

static struct DeviceBean devices[] = {
    {
        .name = "system",
        .devAttrBeans = devAttrBeansSystem,
    },

    {
        .name = "exp_boards",
        .devAttrBeans = devAttrBeansExpb,
    },

    {
        .name = "power",
        .devAttrBeans = devAttrBeansPower,
    },

    {
        .name = "watchdog",
        .devAttrBeans = devAttrBeansWatchdog,
    },

    {
        .name = "button",
        .devAttrBeans = devAttrBeansButton,
    },

    {
        .name = "buzzer",
        .devAttrBeans = devAttrBeansBuzzer,
    },

    {
        .name = "led",
        .devAttrBeans = devAttrBeansLed,
    },

    {
        .name = "usb",
        .devAttrBeans = devAttrBeansUsb,
    },

    {
        .name = "sd",
        .devAttrBeans = devAttrBeansSd,
    },

    {
        .name = "pcie",
        .devAttrBeans = devAttrBeansPcie,
    },

    {
        .name = "power_in",
        .devAttrBeans = devAttrBeansPowerIn,
    },

    {
        .name = "fan",
        .devAttrBeans = devAttrBeansFan,
    },

    {
        .name = "accelerometer",
        .devAttrBeans = devAttrBeansAccel,
    },

    {
        .name = "sec_elem",
        .devAttrBeans = devAttrBeansAtecc,
    },

    {
        .name = "_tst",
        .devAttrBeans = devAttrBeansTest,
    },

    {
        .name = "ups",
        .devAttrBeans = devAttrBeansUps,
        .expbTypes = devUpsExpbTypes,
    },

    {
        .name = "ups",
        .devAttrBeans = devAttrBeansUpsBattery,
        .expbTypes = devUpsBatteryExpbTypes,
    },

    {
        .name = "power_out",
        .devAttrBeans = devAttrBeansPowerOut,
        .expbTypes = devUpsExpbTypes,
    },

    {
        .name = "digital_in_s%d",
        .devAttrBeans = devAttrBeansDIn,
        .expbTypes = devDInExpbTypes,
    },

    {
        .name = "digital_out_s%d",
        .devAttrBeans = devAttrBeansDOut,
        .expbTypes = devDOutExpbTypes,
    },

    {
        .name = "rs485_s%d",
        .devAttrBeans = devAttrBeansRs485,
        .expbTypes = devRs485ExpbTypes,
    },

    {},
};

static uint8_t _fwVerMajor;
static uint8_t _fwVerMinor;
static struct class *_pDeviceClass;
static struct ExpbBean _expbs[4];

static struct mutex _i2c_mtx;
static struct i2c_client *rp2_i2c_client = NULL;
static struct i2c_client *lm75a_i2c_client = NULL;
static bool _rp2_probed = false;

static int64_t _i2cReadVal;
static uint16_t _i2cReadSize;

struct spi_data {
  struct spi_device *spi;
  struct spi_message msg;
  struct spi_transfer transfer[2];

  struct regulator *reg;
  struct mutex lock;

  u8 tx_buf ____cacheline_aligned;
  u8 rx_buf[2];
};

static struct spi_data *_mcp3204_data;
static struct spi_data *_max4896_data;

struct GpioBean *gpioGetBean(struct device *dev, struct device_attribute *attr,
                             const char **vals) {
  struct DeviceAttrBean *dab;
  dab = container_of(attr, struct DeviceAttrBean, devAttr);
  if (dab == NULL) {
    return NULL;
  }
  *vals = dab->vals;
  return dab->gpio;
}

static bool _i2c_lock(void) {
  uint8_t i;
  for (i = 0; i < 20; i++) {
    if (mutex_trylock(&_i2c_mtx)) {
      return true;
    }
    msleep(1);
  }
  return false;
}

static void _i2c_unlock(void) { mutex_unlock(&_i2c_mtx); }

static uint8_t _i2c_crc_process(uint8_t crc, uint8_t dByte) {
  uint8_t k;
  crc ^= dByte;
  for (k = 0; k < 8; k++) crc = crc & 0x80 ? (crc << 1) ^ 0x2f : crc << 1;
  return crc;
}

static void _i2c_add_crc(int reg, char *data, uint8_t len) {
  uint8_t i;
  uint8_t crc;

  crc = _i2c_crc_process(0xff, reg);
  for (i = 0; i < len; i++) {
    crc = _i2c_crc_process(crc, data[i]);
  }
  data[len] = crc;
}

static int64_t _i2c_read_no_lock(uint8_t reg, uint8_t len) {
  int64_t res;
  char buf[5];  // max 4 bytes data + 1 byte crc
  uint8_t i;
  uint8_t crc;
#ifdef DEBUG_I2C
  uint8_t j;
#endif

  if (rp2_i2c_client == NULL) {
    return -ENODEV;
  }

  for (i = 0; i < 10; i++) {
    res = i2c_smbus_read_i2c_block_data(rp2_i2c_client, reg, len + 1, buf);
    if (res == len + 1) {
      crc = buf[len];
      _i2c_add_crc(reg, buf, len);
      if (crc == buf[len]) {
        break;
      } else {
#ifdef DEBUG_I2C
        pr_notice(LOG_TAG "i2c read error: reg=%d\n", reg);
        for (j = 0; j < len; j++) {
          pr_notice(LOG_TAG "data[%d]=0x%x\n", j, buf[j]);
        }
        pr_notice(LOG_TAG "crc=0x%x crc_rcv=0x%x\n", buf[len], crc);
#endif
        res = -1;
      }
    }
  }

  if (res != len + 1) {
#ifdef DEBUG_I2C
    pr_notice(LOG_TAG "i2c read error: %lld\n", res);
#endif
    return -EIO;
  }

  res = 0;
  for (i = 0; i < len; i++) {
    res |= (buf[i] & 0xff) << (i * 8);
  }

  return res;
}

static int64_t _i2c_write_no_lock(uint8_t reg, uint8_t len, uint32_t val,
                                  uint32_t mask) {
  char buf[9];  // max 4 bytes data + 4 bytes mask + 1 byte crc
  uint8_t i;

  if (rp2_i2c_client == NULL) {
    return -ENODEV;
  }

  for (i = 0; i < len; i++) {
    buf[i] = val >> (8 * i);
  }
  if (mask != 0) {
    for (i = 0; i < len; i++) {
      buf[i + len] = mask >> (8 * i);
    }
    len *= 2;
  }
  _i2c_add_crc(reg, buf, len);
  len++;

  for (i = 0; i < 10; i++) {
    if (!i2c_smbus_write_i2c_block_data(rp2_i2c_client, reg, len, buf)) {
      return len;
    }
  }
  return -EIO;
}

static int64_t _i2c_read(uint8_t reg, uint8_t len) {
  int64_t res;

  if (len == 0) {
    return -EINVAL;
  }

  if (!_i2c_lock()) {
    return -EBUSY;
  }

  res = _i2c_read_no_lock(reg, len);

  _i2c_unlock();

  return res;
}

static int64_t _i2c_write(uint8_t reg, uint8_t len, uint32_t val,
                          uint32_t mask) {
  int64_t res;

  if (!_i2c_lock()) {
    return -EBUSY;
  }

  res = _i2c_write_no_lock(reg, len, val, mask);

  _i2c_unlock();

  return res;
}

static int64_t _i2c_read_segment(uint8_t reg, uint8_t len, uint32_t mask,
                                 uint8_t shift) {
  int64_t res;
  res = _i2c_read(reg, len);
  if (res < 0) {
    return res;
  }
  res >>= shift;
  if (mask != 0) {
    res &= mask;
  }
  return res;
}

static int64_t _i2c_write_segment(uint8_t reg, uint8_t len, uint32_t mask,
                                  uint8_t shift, uint32_t val, bool readBack) {
  int64_t res = 0;
  uint8_t i, j;

  if (!_i2c_lock()) {
    return -EBUSY;
  }

  val <<= shift;

  if (mask != 0) {
    mask <<= shift;
    val &= mask;
  }

  for (i = 0; i < 5; i++) {
    res = _i2c_write_no_lock(reg, len, val, mask);
    if (res >= 0) {
      if (readBack) {
        for (j = 0; j < 2; j++) {
          res = _i2c_read_no_lock(reg, len);
          if (res >= 0) {
            if (mask != 0) {
              res &= mask;
            }
            if (res == val) {
              res = len;
              break;
            }
            res = -EPERM;
          }
        }
        if (res >= 0) {
          break;
        }
      } else {
        break;
      }
    }
  }

  _i2c_unlock();

  return res;
}

static ssize_t devAttrI2c_show(struct device *dev,
                               struct device_attribute *attr, char *buf) {
  struct DeviceAttrRegSpecs *specs;
  struct DeviceAttrBean *dab;
  struct DeviceData *data;
  uint8_t reg;
  int64_t res;

  data = dev_get_drvdata(dev);
  dab = container_of(attr, struct DeviceAttrBean, devAttr);
  if (dab == NULL) {
    return -EFAULT;
  }
  specs = &dab->regSpecs;
  if (specs->len == 0) {
    return -EFAULT;
  }

  reg = specs->reg;
  if (data != NULL) {
    reg += I2C_EXPB_IDX_TO_REG_START(data->expbIdx);
  }

  res = _i2c_read_segment(reg, specs->len, specs->mask, specs->shift);

  if (res < 0) {
    return res;
  }

  return valToStr(buf, res, dab->vals, specs->sign, specs->len, specs->base,
                  specs->mask);
}

static ssize_t devAttrI2c_show_clear(struct device *dev,
                                     struct device_attribute *attr, char *buf) {
  ssize_t res = devAttrI2c_show(dev, attr, buf);
  if (res >= 0) {
    devAttrI2c_store(dev, attr, "0", 1);
  }
  return res;
}

static ssize_t devAttrI2c_store(struct device *dev,
                                struct device_attribute *attr, const char *buf,
                                size_t count) {
  struct DeviceAttrRegSpecs *specs;
  struct DeviceAttrBean *dab;
  struct DeviceData *data;
  uint8_t reg;
  int64_t res;

  data = dev_get_drvdata(dev);
  dab = container_of(attr, struct DeviceAttrBean, devAttr);
  if (dab == NULL) {
    return -EFAULT;
  }
  specs = &dab->regSpecsStore;
  if (specs->len == 0) {
    specs = &dab->regSpecs;
    if (specs->len == 0) {
      return -EFAULT;
    }
  }

  res = strToVal(buf, dab->vals, specs->sign, specs->base);
  if (res < 0) {
    return res;
  }

  reg = specs->reg;
  if (data != NULL) {
    reg += I2C_EXPB_IDX_TO_REG_START(data->expbIdx);
  }

  res = _i2c_write_segment(reg, specs->len, specs->mask, specs->shift,
                           (uint32_t)res, attr->show != NULL);

  if (res < 0) {
    return res;
  }

  return count;
}

static ssize_t getFwVersion(void) {
  int64_t val;
  val = _i2c_read(1, 2);

  if (val < 0) {
    return val;
  }

  _fwVerMajor = (val >> 8) & 0xff;
  _fwVerMinor = val & 0xff;

  return 0;
}

static ssize_t devAttrFwVersion_show(struct device *dev,
                                     struct device_attribute *attr, char *buf) {
  int32_t res;

  res = getFwVersion();
  if (res < 0) {
    return res;
  }

  return sprintf(buf, "%d.%d\n", _fwVerMajor, _fwVerMinor);
}

static ssize_t devAttrConfig_store(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t count) {
  int64_t res;
  uint32_t val;
  uint16_t i;
  char cmd;
  cmd = toUpper(buf[0]);
  if (cmd == 'R') {
    val = 0x02;
  } else {
    return -EINVAL;
  }

  if (!_i2c_lock()) {
    return -EBUSY;
  }

  res = _i2c_write_no_lock(I2C_REG_USER_CFG_CMD, 2, 0x2a00 | val, 0);
  if (res > 0) {
    for (i = 0; i < 10; i++) {
      msleep(1);
      res = _i2c_read_no_lock(I2C_REG_USER_CFG_STATE, 2);
      if (res > 0) {
        if ((res & 0xff) != val) {
          res = -EBUSY;
          break;
        }
        if (((res >> 10) & 1) == 0) {
          if (((res >> 8) & 1) == 1) {
            res = count;
          } else {
            res = -EFAULT;
          }
          break;
        } else {
          res = -EBUSY;
        }
      }
    }
  }

  _i2c_unlock();

  return res;
}

static ssize_t devAttrI2cRead_show(struct device *dev,
                                   struct device_attribute *attr, char *buf) {
  if (_i2cReadSize == 1) {
    return sprintf(buf, "0x%02x\n", (uint32_t)_i2cReadVal);
  } else if (_i2cReadSize == 2) {
    return sprintf(buf, "0x%04x\n", (uint32_t)_i2cReadVal);
  } else if (_i2cReadSize == 3) {
    return sprintf(buf, "0x%06x\n", (uint32_t)_i2cReadVal);
  } else {
    return sprintf(buf, "0x%08x\n", (uint32_t)_i2cReadVal);
  }
}

static ssize_t devAttrI2cRead_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count) {
  unsigned long reg;
  char *end = NULL;

  reg = simple_strtoul(buf, &end, 10);
  if (++end < buf + count) {
    _i2cReadSize = simple_strtoul(end, NULL, 10);
  } else {
    _i2cReadSize = 2;
  }

  if (reg > 255 || _i2cReadSize > 4) {
    return -EINVAL;
  }

  _i2cReadVal = _i2c_read(reg, _i2cReadSize);

  if (_i2cReadVal < 0) {
    return _i2cReadVal;
  }

  return count;
}

static ssize_t devAttrI2cWrite_store(struct device *dev,
                                     struct device_attribute *attr,
                                     const char *buf, size_t count) {
  unsigned long reg;
  unsigned long val = 0;
  size_t size;
  char *start = NULL;
  char *end = NULL;

  reg = simple_strtoul(buf, &end, 10);

  if (++end >= buf + count) {
    return -EINVAL;
  }

  start = end;
  val = simple_strtoul(start, &end, 16);

  size = (end - start) - 2;

  if (reg > 255 || size > 8 || (size % 2) != 0) {
    return -EINVAL;
  }

  if (_i2c_write(reg, size / 2, val, 0) < 0) {
    return -EIO;
  }

  return count;
}

static ssize_t devAttrUpsBatteryV_show(struct device *dev,
                                       struct device_attribute *attr,
                                       char *buf) {
  int32_t res;
  res = devAttrI2c_show(dev, attr, buf);
  if (res < 0) {
    return res;
  }
  if (buf[0] == '1') {
    return sprintf(buf, "24000\n");
  } else {
    return sprintf(buf, "12000\n");
  }
}

static ssize_t devAttrUpsBatteryV_store(struct device *dev,
                                        struct device_attribute *attr,
                                        const char *buf, size_t count) {
  int ret;
  long val;
  char buf2[2];
  ret = kstrtol(buf, 10, &val);
  if (ret < 0) {
    return -EINVAL;
  }
  if (val == 12000) {
    buf2[0] = '0';
  } else if (val == 24000) {
    buf2[0] = '1';
  } else {
    return -EINVAL;
  }

  buf2[1] = '\0';
  ret = devAttrI2c_store(dev, attr, buf2, 1);
  if (ret < 0) {
    return ret;
  }
  return count;
}

static ssize_t devAttrLm75a_show(struct device *dev,
                                 struct device_attribute *attr, char *buf) {
  int32_t res;
  struct DeviceAttrBean *dab;
  dab = container_of(attr, struct DeviceAttrBean, devAttr);
  if (dab == NULL) {
    return -EFAULT;
  }

  if (lm75a_i2c_client == NULL) {
    return -ENODEV;
  }

  if (!_i2c_lock()) {
    return -EBUSY;
  }

  res = i2c_smbus_read_word_data(lm75a_i2c_client, dab->regSpecs.reg);

  _i2c_unlock();

  if (res < 0) {
    return res;
  }

  res = ((res & 0xff) << 8) + ((res >> 8) & dab->regSpecs.mask);
  res = ((int16_t)res) * 100 / 256;

  return sprintf(buf, "%d\n", res);
}

static ssize_t devAttrLm75a_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count) {
  int32_t res;
  long temp;
  struct DeviceAttrBean *dab;
  dab = container_of(attr, struct DeviceAttrBean, devAttr);
  if (dab == NULL) {
    return -EFAULT;
  }

  if (lm75a_i2c_client == NULL) {
    return -ENODEV;
  }

  res = kstrtol(buf, 10, &temp);
  if (res < 0) {
    return res;
  }
  if (temp > 12750) {
    return -EINVAL;
  }
  if (temp < -12800) {
    return -EINVAL;
  }

  temp = temp * 256 / 100;
  temp = ((temp & dab->regSpecs.mask) << 8) + ((temp >> 8) & 0xff);

  if (!_i2c_lock()) {
    return -EBUSY;
  }

  res = i2c_smbus_write_word_data(lm75a_i2c_client, dab->regSpecs.reg, temp);

  _i2c_unlock();

  if (res < 0) {
    return res;
  }

  return count;
}

static ssize_t devAttrBlink_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count) {
  struct DeviceAttrBean *dab;
  int64_t res;
  uint16_t on = 0;
  uint16_t off = 0;
  uint16_t rep = 0;
  char *end = NULL;

  dab = container_of(attr, struct DeviceAttrBean, devAttr);

  on = simple_strtoul(buf, &end, 10);
  if (++end < buf + count) {
    off = simple_strtoul(end, &end, 10);
    if (++end < buf + count) {
      rep = simple_strtoul(end, NULL, 10);
    }
  }

  res = _i2c_write_segment(dab->regSpecs.reg, dab->regSpecs.len,
                           dab->regSpecs.mask, dab->regSpecs.shift, on, true);
  if (res > 0) {
    res =
        _i2c_write_segment(dab->regSpecs.reg + 1, dab->regSpecs.len,
                           dab->regSpecs.mask, dab->regSpecs.shift, off, true);
    if (res > 0) {
      res = _i2c_write_segment(dab->regSpecs.reg + 2, dab->regSpecs.len,
                               dab->regSpecs.mask, dab->regSpecs.shift, rep,
                               true);
    }
  }

  if (res < 0) {
    return res;
  }

  return count;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0)
static int _i2c_probe(struct i2c_client *client) {
#else
static int _i2c_probe(struct i2c_client *client,
                      const struct i2c_device_id *id) {
#endif
  uint8_t i;
  int64_t res;
  bool ok;

  if (strcmp("stratopimax-rp2", client->name) == 0) {
    rp2_i2c_client = client;

    ok = true;
    for (i = 0; i < 4; i++) {
      res = _i2c_read(I2C_REG_EXPB_TYPE_S1 + i, 2);
      if (res < 0) {
        ok = false;
        break;
      }
      _expbs[i].type = res;
      pr_info(LOG_TAG "Exp %d type=%d\n", (i + 1), _expbs[i].type);
    }

    if (ok) {
      _rp2_probed = true;
    } else {
      rp2_i2c_client = NULL;
      return -1;
    }

  } else if (strcmp("stratopimax-lm75a", client->name) == 0) {
    for (i = 0; i < 4; i++) {
      if (_i2c_lock()) {
        res = i2c_smbus_write_byte_data(client, 1, 0);
        _i2c_unlock();
        if (res >= 0) {
          lm75a_i2c_client = client;
          break;
        }
      }
    }
    if (lm75a_i2c_client == NULL) {
      return -1;
    }

  } else {
    pr_err(LOG_TAG "unknown '%s' i2c_client addr=0x%02hx\n", client->name,
           client->addr);
    return -1;
  }

  pr_info(LOG_TAG "'%s' probed addr=0x%02hx\n", client->name, client->addr);
  return 0;
}

static void _i2c_remove(struct i2c_client *client) {
  pr_info(LOG_TAG "'%s' i2c_client removed addr=0x%02hx\n", client->name,
          client->addr);
}

const struct of_device_id _i2c_of_match[] = {
    {
        .compatible = "sferalabs,stratopimax-rp2",
    },
    {
        .compatible = "sferalabs,stratopimax-lm75a",
    },
    {},
};
MODULE_DEVICE_TABLE(of, _i2c_of_match);

static const struct i2c_device_id _i2c_id[] = {
    {"stratopimax-rp2", 0},
    {"stratopimax-lm75a", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, _i2c_id);

static struct i2c_driver _i2c_driver = {
    .driver =
        {
            .name = "stratopimax",
            .owner = THIS_MODULE,
            .of_match_table = of_match_ptr(_i2c_of_match),
        },
    .probe = _i2c_probe,
    .remove = _i2c_remove,
    .id_table = _i2c_id,
};

static int _spi_transfer(struct spi_data *data, u8 val) {
  memset(&data->rx_buf, 0, sizeof(data->rx_buf));
  data->tx_buf = val;
  return spi_sync(data->spi, &data->msg);
}

static ssize_t devAttrMcp3204_show(char *buf, unsigned int channel, int mult) {
  int i, ret;

  if (_mcp3204_data == NULL) {
    return -ENODEV;
  }

  ret = -EBUSY;
  for (i = 0; i < 40; i++) {
    if (mutex_trylock(&_mcp3204_data->lock)) {
      ret = 1;
      break;
    }
    msleep(1);
  }

  if (ret < 0) {
    return ret;
  }

  _spi_transfer(_mcp3204_data, 0b1100000 | (channel << 2));

  mutex_unlock(&_mcp3204_data->lock);

  if (ret < 0) {
    return ret;
  }

  ret = ((_mcp3204_data->rx_buf[0] & 0xff) << 4) |
        ((_mcp3204_data->rx_buf[1] & 0xf) >> 4);

  return sprintf(buf, "%lu\n", (ret * MCP3204_VREF * mult / 4096ul));
}

static ssize_t devAttrMcp3204Ch0Mv_show(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf) {
  return devAttrMcp3204_show(buf, 0, MCP3204_FACTOR);
}

static ssize_t devAttrMcp3204Ch1Mv_show(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf) {
  return devAttrMcp3204_show(buf, 1, MCP3204_FACTOR);
}

static ssize_t devAttrMax4896Switch_store(struct device *dev,
                                          struct device_attribute *attr,
                                          const char *buf, size_t count) {
  int i, ret;

  if (_max4896_data == NULL) {
    return -ENODEV;
  }

  ret = -EBUSY;
  for (i = 0; i < 40; i++) {
    if (mutex_trylock(&_max4896_data->lock)) {
      ret = 1;
      break;
    }
    msleep(1);
  }

  if (ret < 0) {
    return ret;
  }

  ret = _spi_transfer(_max4896_data, ((u8)(buf[0] - '0')) << 2);

  mutex_unlock(&_max4896_data->lock);

  pr_info(LOG_TAG "devAttrMax4896Switch_store X | %d - %d %d\n",
          _max4896_data->tx_buf, _max4896_data->rx_buf[0],
          _max4896_data->rx_buf[1]);  // TODO remove

  if (ret < 0) {
    return ret;
  }

  return count;
}

static int _spi_probe(const char *name, struct spi_data **data,
                      struct spi_device *spi) {
  int ret;

  *data = devm_kzalloc(&spi->dev, sizeof(struct spi_data), GFP_KERNEL);
  if (!*data) {
    return -ENOMEM;
  }

  (*data)->spi = spi;
  spi_set_drvdata(spi, *data);

  (*data)->reg = devm_regulator_get(&spi->dev, "vref");
  if (IS_ERR((*data)->reg)) {
    return PTR_ERR((*data)->reg);
  }

  ret = regulator_enable((*data)->reg);
  if (ret < 0) {
    return ret;
  }

  mutex_init(&(*data)->lock);

  return 0;
}

static int _mcp3204_spi_probe(struct spi_device *spi) {
  int ret = _spi_probe("mcp3204", &_mcp3204_data, spi);
  if (ret < 0) {
    pr_warn(LOG_TAG "'mcp3204' probe error\n");
    return ret;
  }
  _mcp3204_data->transfer[0].tx_buf = &_mcp3204_data->tx_buf;
  _mcp3204_data->transfer[0].len = 1;
  _mcp3204_data->transfer[1].rx_buf = _mcp3204_data->rx_buf;
  _mcp3204_data->transfer[1].len = 2;
  spi_message_init_with_transfers(&_mcp3204_data->msg, _mcp3204_data->transfer,
                                  2);
  return ret;
}

static int _max4896_spi_probe(struct spi_device *spi) {
  int ret = _spi_probe("max4896", &_max4896_data, spi);
  if (ret < 0) {
    pr_warn(LOG_TAG "'max4896' probe error\n");
    return ret;
  }
  _max4896_data->transfer[0].tx_buf = &_max4896_data->tx_buf;
  _max4896_data->transfer[0].rx_buf = _max4896_data->rx_buf;
  _max4896_data->transfer[0].len = 1;
  spi_message_init_with_transfers(&_max4896_data->msg, _max4896_data->transfer,
                                  1);
  return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
static int _spi_remove(struct spi_device *spi) {
#else
static void _spi_remove(struct spi_device *spi) {
#endif
  struct spi_data *data = spi_get_drvdata(spi);
  if (data != NULL) {
    if (data->reg != NULL && !IS_ERR(data->reg)) {
      regulator_disable(data->reg);
    }
    mutex_destroy(&data->lock);
  }

  pr_info(LOG_TAG "spi %d removed\n", spi->chip_select);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
  return 0;
#endif
}

const struct of_device_id _mcp3204_of_match[] = {
    {
        .compatible = "sferalabs,stratopimax-mcp3204",
    },
    {},
};
MODULE_DEVICE_TABLE(of, _mcp3204_of_match);

const struct of_device_id _max4896_of_match[] = {
    {
        .compatible = "sferalabs,stratopimax-max4896",
    },
    {},
};
MODULE_DEVICE_TABLE(of, _max4896_of_match);

static const struct spi_device_id _mcp3204_ids[] = {
    {"stratopimax-mcp3204", 0},
    {},
};
MODULE_DEVICE_TABLE(spi, _mcp3204_ids);

static const struct spi_device_id _max4896_ids[] = {
    {"stratopimax-max4896", 0},
    {},
};
MODULE_DEVICE_TABLE(spi, _max4896_ids);

static struct spi_driver _mcp3204_driver = {
    .driver =
        {
            .name = "stratopimax-mcp3204",
            .owner = THIS_MODULE,
            .of_match_table = of_match_ptr(_mcp3204_of_match),
        },
    .probe = _mcp3204_spi_probe,
    .remove = _spi_remove,
    .id_table = _mcp3204_ids,
};

static struct spi_driver _max4896_driver = {
    .driver =
        {
            .name = "stratopimax-max4896",
            .owner = THIS_MODULE,
            .of_match_table = of_match_ptr(_max4896_of_match),
        },
    .probe = _max4896_spi_probe,
    .remove = _spi_remove,
    .id_table = _max4896_ids,
};

static int _device_add(struct platform_device *pdev, struct DeviceBean *db,
                       int8_t expbIdx) {
  struct device *dev;
  struct DeviceData *data = NULL;
  struct DeviceData **pData;
  struct DeviceAttrBean *dab;
  struct DeviceAttrBean *dabM;
  char fileNameBuf[32];
  int fi, ai;

  if (db->expbTypes != NULL) {
    data = devm_kzalloc(&pdev->dev, sizeof(struct DeviceData), GFP_KERNEL);
    if (!data) {
      return -ENOMEM;
    }
  }
  dev = class_find_device_by_name(_pDeviceClass, db->name);
  if (!dev) {
    dev = device_create(_pDeviceClass, NULL, 0, data, db->name, (expbIdx + 1));
    if (IS_ERR(dev)) {
      pr_err(LOG_TAG "failed to create device '%s' s=%d\n", db->name,
             (expbIdx + 1));
      return -1;
    }
  }
  db->device = dev;
  if (data != NULL) {
    data->expbIdx = expbIdx;
    data->device = db->device;
    pData = &_expbs[expbIdx].data;
    while (*pData != NULL) {
      pData = &(*pData)->next;
    }
    *pData = data;
  }

  ai = 0;
  while (db->devAttrBeans[ai].devAttr.attr.name != NULL) {
    dab = &db->devAttrBeans[ai];
    fi = 0;
    do {
      if (dab->bitMapLen > 0) {
        dabM =
            devm_kzalloc(&pdev->dev, sizeof(struct DeviceAttrBean), GFP_KERNEL);
        if (!dabM) {
          return -ENOMEM;
        }
        memcpy(dabM, dab, sizeof(struct DeviceAttrBean));
        dabM->regSpecs.shift *= fi;
        dabM->regSpecs.shift += dab->bitMapStart;
        dabM->regSpecsStore.shift *= fi;
        dabM->regSpecsStore.shift += dab->bitMapStart;
        sprintf(fileNameBuf, dab->devAttr.attr.name, (fi + 1));
        dabM->devAttr.attr.name = fileNameBuf;
      } else {
        dabM = dab;
      }

      if (device_create_file(dev, &dabM->devAttr)) {
        pr_err(LOG_TAG "failed to create device file '%s/%s' s=%d\n", db->name,
               dabM->devAttr.attr.name, (expbIdx + 1));
        return -1;
      }
      fi++;
    } while (fi < dab->bitMapLen);
    ai++;
  }
  return 0;
}

static void _device_remove_files(struct DeviceBean *db, struct device *dev) {
  struct DeviceAttrBean *dab;
  int ai;

  ai = 0;
  while (db->devAttrBeans[ai].devAttr.attr.name != NULL) {
    dab = &db->devAttrBeans[ai];
    device_remove_file(dev, &dab->devAttr);
    ai++;
  }
  device_destroy(_pDeviceClass, 0);
}

static void cleanup(void) {
  struct DeviceBean *db;
  struct DeviceData *data;
  int di, ei, ti;

  if (_pDeviceClass != NULL && !IS_ERR(_pDeviceClass)) {
    di = 0;
    while (devices[di].name != NULL) {
      db = &devices[di];

      if (db->device != NULL && !IS_ERR(db->device)) {
        if (db->expbTypes == NULL) {
          _device_remove_files(db, db->device);
        } else {
          ti = 0;
          while (db->expbTypes[ti] != 0) {
            for (ei = 0; ei < 4; ei++) {
              if (_expbs[ei].type == db->expbTypes[ti]) {
                data = _expbs[ei].data;
                while (data != NULL) {
                  if (data->device != NULL) {
                    _device_remove_files(db, data->device);
                    data->device = NULL;
                    break;
                  }
                  data = data->next;
                }
              }
            }
            ti++;
          }
        }
      }
      di++;
    }

    gpioFree(&gpioSdRoute);

    i2c_del_driver(&_i2c_driver);
    spi_unregister_driver(&_mcp3204_driver);
    spi_unregister_driver(&_max4896_driver);

    mutex_destroy(&_i2c_mtx);

    class_destroy(_pDeviceClass);
  }
}

static int stratopimax_init(struct platform_device *pdev) {
  struct DeviceBean *db;
  int i, di, ti, ei;

  pr_info(LOG_TAG "init\n");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0)
  _pDeviceClass = class_create("stratopimax");
#else
  _pDeviceClass = class_create(THIS_MODULE, "stratopimax");
#endif
  if (IS_ERR(_pDeviceClass)) {
    pr_err(LOG_TAG "failed to create device class\n");
    goto fail;
  }

  mutex_init(&_i2c_mtx);
  i2c_add_driver(&_i2c_driver);
  spi_register_driver(&_mcp3204_driver);
  spi_register_driver(&_max4896_driver);
  ateccAddDriver();
  gpioSetPlatformDev(pdev);

  for (i = 0; i < 50; i++) {
    if (_rp2_probed) {
      break;
    }
    msleep(50);
  }

  if (!_rp2_probed) {
    pr_err(LOG_TAG "RP probing failed\n");
    goto fail;
  }

  if (gpioInit(&gpioSdRoute)) {
    pr_err(LOG_TAG "error setting up GPIO %s\n", gpioSdRoute.name);
    goto fail;
  }

  di = 0;
  while (devices[di].name != NULL) {
    db = &devices[di];
    if (db->expbTypes == NULL) {
      if (_device_add(pdev, db, -1)) {
        goto fail;
      }
    } else {
      ti = 0;
      while (db->expbTypes[ti] != 0) {
        for (ei = 0; ei < 4; ei++) {
          if (_expbs[ei].type == db->expbTypes[ti]) {
            if (_device_add(pdev, db, ei)) {
              goto fail;
            }
          }
        }
        ti++;
      }
    }
    di++;
  }

  pr_info(LOG_TAG "ready\n");

  return 0;

fail:
  pr_err(LOG_TAG "init failed\n");
  cleanup();
  return -1;
}

static int stratopimax_exit(struct platform_device *pdev) {
  cleanup();
  pr_info(LOG_TAG "exit\n");
  return 0;
}

const struct of_device_id stratopimax_of_match[] = {
    {
        .compatible = "sferalabs,stratopimax",
    },
    {},
};
MODULE_DEVICE_TABLE(of, stratopimax_of_match);

static struct platform_driver stratopimax_driver = {
    .driver =
        {
            .name = "stratopimax-i2c",
            .owner = THIS_MODULE,
            .of_match_table = stratopimax_of_match,
        },
    .probe = stratopimax_init,
    .remove = stratopimax_exit,
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sfera Labs - http://sferalabs.cc");
MODULE_DESCRIPTION("Strato Pi Max driver module");
MODULE_VERSION("1.8");

module_platform_driver(stratopimax_driver);
