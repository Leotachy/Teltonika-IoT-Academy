

#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/gpio.h>
#include <linux/gpio/consumer.h>

#include <linux/property.h>

#include "dt_parse.h"

// Blink every blinky_leds_priv::interval seconds while staying on for
// BLINKING_ON_TIME seconds
#define BLINKING_ON_TIME 1

struct blinky_led_data {
  const char *name;
  struct gpio_desc *gpiod;
};

struct blinky_leds_priv {
  struct delayed_work blink_work;
  struct device *dev; // link back to device
  int num_leds;
  u32 interval;

  int enabled : 1;
  int blink_state : 1;
  int default_state : 1;

  struct blinky_led_data leds[] __counted_by(num_leds);
};

static void update_gpiod_values(struct blinky_leds_priv *priv) {
  if (priv) {
    int state = priv->enabled && (!priv->interval || priv->blink_state);
    for (int i = 0; i < priv->num_leds; i++)
      gpiod_set_value_cansleep(priv->leds[i].gpiod, state);

    priv->blink_state = !state;
  }
}

static void gpio_blinky_toggle_blinking(struct blinky_leds_priv *priv) {
  if (priv == NULL)
    return;

  if (priv->interval && priv->enabled)
    schedule_delayed_work(&priv->blink_work, secs_to_jiffies(priv->interval));
  else
    cancel_delayed_work_sync(&priv->blink_work);
}

// sysfs

static ssize_t value_show(struct device *dev, struct device_attribute *attr,
                          char *buf) {
  struct blinky_leds_priv *priv = dev_get_drvdata(dev);
  return sysfs_emit(buf, "%d\n", priv->enabled);
}

static ssize_t value_store(struct device *dev, struct device_attribute *attr,
                           const char *buf, size_t count) {
  struct blinky_leds_priv *priv = dev_get_drvdata(dev);

  int value;
  int ret;

  ret = kstrtoint(buf, 10, &value);
  if (ret)
    return ret;

  priv->enabled = value != 0;
  update_gpiod_values(priv);
  gpio_blinky_toggle_blinking(priv);

  return count;
}

static DEVICE_ATTR_RW(value);

static void gpio_blinky_blink_work(struct work_struct *work) {
  struct delayed_work *dwork = to_delayed_work(work);
  struct blinky_leds_priv *priv =
      container_of(dwork, struct blinky_leds_priv, blink_work);

  dev_dbg(priv->dev, "Blink work callback. Blink state: %d", priv->blink_state);

  update_gpiod_values(priv);

  if (priv->blink_state == 0) {
    schedule_delayed_work(&priv->blink_work, secs_to_jiffies(BLINKING_ON_TIME));
  } else if (priv->enabled &&
             priv->interval) // if check in case interval got disabled??
  {
    schedule_delayed_work(&priv->blink_work, secs_to_jiffies(priv->interval));
  }
}

static ssize_t interval_show(struct device *dev, struct device_attribute *attr,
                             char *buf) {
  struct blinky_leds_priv *priv = dev_get_drvdata(dev);
  return sysfs_emit(buf, "%u\n", priv->interval);
}

static ssize_t interval_store(struct device *dev, struct device_attribute *attr,
                              const char *buf, size_t count) {
  struct blinky_leds_priv *priv = dev_get_drvdata(dev);

  u32 value;
  int ret;

  ret = kstrtoint(buf, 10, &value);
  if (ret)
    return ret;

  priv->interval = value;
  gpio_blinky_toggle_blinking(priv);
  update_gpiod_values(priv);

  return count;
}

static DEVICE_ATTR_RW(interval);

static struct blinky_leds_priv *gpio_blinky_create_leds(struct device *dev) {
  struct blinky_leds_priv *priv;

  int count = device_get_child_node_count(dev);
  if (!count)
    return ERR_PTR(-ENODEV);
  int ret;
  priv = devm_kzalloc(dev, struct_size(priv, leds, count), GFP_KERNEL);
  if (!priv)
    return ERR_PTR(-ENOMEM);
  priv->num_leds = count;
  int used = 0;

  priv->default_state = gpio_blinky_default_state_get(dev_fwnode(dev));
  priv->interval = gpio_blinky_interval_get(dev_fwnode(dev));
  priv->enabled = priv->default_state;

  device_for_each_child_node_scoped(dev, child) {
    struct blinky_led_data *led_dat = &priv->leds[used];

    led_dat->gpiod = devm_fwnode_gpiod_get(
        dev, child, NULL, priv->default_state ? GPIOD_OUT_HIGH : GPIOD_OUT_LOW,
        NULL);

    if (IS_ERR(led_dat->gpiod)) {
      dev_err_probe(dev, PTR_ERR(led_dat->gpiod), "Failed to get GPIO '%pfw'\n",
                    child);
      return ERR_CAST(led_dat->gpiod);
    }

    led_dat->name = gpio_blinky_name_get(child, dev);

    ret = gpiod_set_consumer_name(led_dat->gpiod, led_dat->name);
    if (ret) {
      dev_err_probe(dev, ret, "Failed to set consumer name for gpiod '%pfw'\n",
                    child);
      return ERR_PTR(ret);
    }

    used++;
  }
  priv->num_leds = used;
  priv->dev = dev;

  INIT_DELAYED_WORK(&priv->blink_work, gpio_blinky_blink_work);
  gpio_blinky_toggle_blinking(priv);
  update_gpiod_values(priv);

  return priv;
}

static int gpio_blinky_led_probe(struct platform_device *pdevice) {
  struct device *dev = &pdevice->dev;
  struct blinky_leds_priv *priv = platform_get_drvdata(pdevice);

  if (priv == NULL) {
    priv = gpio_blinky_create_leds(dev);
    if (IS_ERR(priv))
      return PTR_ERR(priv);

    platform_set_drvdata(pdevice, priv);
  }

  int ret = device_create_file(dev, &dev_attr_value);
  if (ret)
    return ret;

  ret = device_create_file(dev, &dev_attr_interval);
  if (ret)
    return ret;

  dev_dbg(dev, "Probed");
  return 0;
}

static void gpio_blinky_led_remove(struct platform_device *pdevice) {
  device_remove_file(&pdevice->dev, &dev_attr_interval);
  device_remove_file(&pdevice->dev, &dev_attr_value);
  dev_dbg(&pdevice->dev, "Removed");
}

static const struct of_device_id blinky_of_match_table[] = {
    {.compatible = "gpio-blinky-led"}, {}};

MODULE_DEVICE_TABLE(of, blinky_of_match_table);

static struct platform_driver blinky_platform_driver = {

    .probe = gpio_blinky_led_probe,
    .remove = gpio_blinky_led_remove,
    .driver = {.name = "blinky_led_drv",
               .of_match_table = blinky_of_match_table,
               .owner = THIS_MODULE}};

module_platform_driver(blinky_platform_driver);

MODULE_AUTHOR("Lukas Andrijauskas");
MODULE_DESCRIPTION("GPIO BLINKY LED driver");
MODULE_LICENSE("GPL");
