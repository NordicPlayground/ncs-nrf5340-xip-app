/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <ram_pwrdn.h>
#include <zb_nrf_platform.h>
#include <zboss_api.h>
#include <zboss_api_addons.h>
#include <zephyr/kernel.h>
#include <zigbee/zigbee_app_utils.h>
#include <zigbee/zigbee_error_handler.h>

#ifdef CONFIG_USB_DEVICE_STACK
#include <zephyr/usb/usb_device.h>
#endif /* CONFIG_USB_DEVICE_STACK */

#if CONFIG_ZIGBEE_FOTA
#include <zigbee/zigbee_fota.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/dfu/mcuboot.h>

#endif /* CONFIG_ZIGBEE_FOTA */

#include <zephyr/stats/stats.h>

#ifdef CONFIG_MCUMGR_GRP_FS
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#endif
#ifdef CONFIG_MCUMGR_GRP_STAT
#include <zephyr/mgmt/mcumgr/grp/stat_mgmt/stat_mgmt.h>
#endif


#include "weather_station.h"
#include "common.h"


#define STORAGE_PARTITION_LABEL	storage_partition
#define STORAGE_PARTITION_ID	FIXED_PARTITION_ID(STORAGE_PARTITION_LABEL)

/* Delay for console initialization */
#define WAIT_FOR_CONSOLE_MSEC 100
#define WAIT_FOR_CONSOLE_DEADLINE_MSEC 2000

/* Weather check period */
#define WEATHER_CHECK_PERIOD_MSEC (1000 * CONFIG_WEATHER_CHECK_PERIOD_SECONDS)

/* Delay for first weather check */
#define WEATHER_CHECK_INITIAL_DELAY_MSEC (1000 * CONFIG_FIRST_WEATHER_CHECK_DELAY_SECONDS)

/* Time of LED on state while blinking for identify mode */
#define IDENTIFY_LED_BLINK_TIME_MSEC 500

/* In Thingy53 each LED is a RGB component of a single LED */
#define LED_RED DK_LED1
#define LED_GREEN DK_LED2
#define LED_BLUE DK_LED3

/* LED indicating OTA Client Activity. */
#define OTA_ACTIVITY_LED LED_GREEN

/* LED indicating that device successfully joined Zigbee network */
#define ZIGBEE_NETWORK_STATE_LED LED_BLUE

/* LED used for device identification */
#define IDENTIFY_LED LED_RED

/* Button used to enter the Identify mode */
#define IDENTIFY_MODE_BUTTON DK_BTN1_MSK

/* Button to start Factory Reset */
#define FACTORY_RESET_BUTTON IDENTIFY_MODE_BUTTON

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console),
				zephyr_cdc_acm_uart),
	     "Console device is not ACM CDC UART device");
LOG_MODULE_REGISTER(app, CONFIG_ZIGBEE_WEATHER_STATION_LOG_LEVEL);


/* From SMP server main.c. */
/* Define an example stats group; approximates seconds since boot. */
STATS_SECT_START(smp_svr_stats)
STATS_SECT_ENTRY(ticks)
STATS_SECT_END;

/* Assign a name to the `ticks` stat. */
STATS_NAME_START(smp_svr_stats)
STATS_NAME(smp_svr_stats, ticks)
STATS_NAME_END(smp_svr_stats);

/* Define an instance of the stats group. */
STATS_SECT_DECL(smp_svr_stats) smp_svr_stats;

#ifdef CONFIG_MCUMGR_GRP_FS
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)STORAGE_PARTITION_ID,
	.mnt_point = "/lfs1"
};
#endif


/* Stores all cluster-related attributes */
static struct zb_device_ctx dev_ctx;

/* Attributes setup */
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(
	basic_attr_list,
	&dev_ctx.basic_attr.zcl_version, &dev_ctx.basic_attr.power_source);

/* Declare attribute list for Identify cluster (client). */
ZB_ZCL_DECLARE_IDENTIFY_CLIENT_ATTRIB_LIST(
	identify_client_attr_list);

/* Declare attribute list for Identify cluster (server). */
ZB_ZCL_DECLARE_IDENTIFY_SERVER_ATTRIB_LIST(
	identify_server_attr_list,
	&dev_ctx.identify_attr.identify_time);

ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(
	temperature_measurement_attr_list,
	&dev_ctx.temp_attrs.measure_value,
	&dev_ctx.temp_attrs.min_measure_value,
	&dev_ctx.temp_attrs.max_measure_value,
	&dev_ctx.temp_attrs.tolerance
	);

ZB_ZCL_DECLARE_PRESSURE_MEASUREMENT_ATTRIB_LIST(
	pressure_measurement_attr_list,
	&dev_ctx.pres_attrs.measure_value,
	&dev_ctx.pres_attrs.min_measure_value,
	&dev_ctx.pres_attrs.max_measure_value,
	&dev_ctx.pres_attrs.tolerance
	);

ZB_ZCL_DECLARE_REL_HUMIDITY_MEASUREMENT_ATTRIB_LIST(
	humidity_measurement_attr_list,
	&dev_ctx.humidity_attrs.measure_value,
	&dev_ctx.humidity_attrs.min_measure_value,
	&dev_ctx.humidity_attrs.max_measure_value
	);

/* Clusters setup */
ZB_HA_DECLARE_WEATHER_STATION_CLUSTER_LIST(
	weather_station_cluster_list,
	basic_attr_list,
	identify_client_attr_list,
	identify_server_attr_list,
	temperature_measurement_attr_list,
	pressure_measurement_attr_list,
	humidity_measurement_attr_list);

/* Endpoint setup (single) */
ZB_HA_DECLARE_WEATHER_STATION_EP(
	weather_station_ep,
	WEATHER_STATION_ENDPOINT_NB,
	weather_station_cluster_list);


#ifndef CONFIG_ZIGBEE_FOTA
ZBOSS_DECLARE_DEVICE_CTX_1_EP(weather_station_ctx, weather_station_ep);
#else
  #if WEATHER_STATION_ENDPOINT_NB == CONFIG_ZIGBEE_FOTA_ENDPOINT
    #error "Weather Station and Zigbee OTA endpoints should be different."
  #endif

extern zb_af_endpoint_desc_t zigbee_fota_client_ep;
ZBOSS_DECLARE_DEVICE_CTX_2_EP(weather_station_ctx,
			      zigbee_fota_client_ep,
			      weather_station_ep);
#endif /* CONFIG_ZIGBEE_FOTA */

static void mandatory_clusters_attr_init(void)
{
	/* Basic cluster attributes */
	dev_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
	dev_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_BATTERY;

	/* Identify cluster attributes */
	dev_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
}

static void measurements_clusters_attr_init(void)
{
	/* Temperature */
	dev_ctx.temp_attrs.measure_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_UNKNOWN;
	dev_ctx.temp_attrs.min_measure_value = WEATHER_STATION_ATTR_TEMP_MIN;
	dev_ctx.temp_attrs.max_measure_value = WEATHER_STATION_ATTR_TEMP_MAX;
	dev_ctx.temp_attrs.tolerance = WEATHER_STATION_ATTR_TEMP_TOLERANCE;

	/* Pressure */
	dev_ctx.pres_attrs.measure_value = ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_VALUE_UNKNOWN;
	dev_ctx.pres_attrs.min_measure_value = WEATHER_STATION_ATTR_PRESSURE_MIN;
	dev_ctx.pres_attrs.max_measure_value = WEATHER_STATION_ATTR_PRESSURE_MAX;
	dev_ctx.pres_attrs.tolerance = WEATHER_STATION_ATTR_PRESSURE_TOLERANCE;

	/* Humidity */
	dev_ctx.humidity_attrs.measure_value = ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_UNKNOWN;
	dev_ctx.humidity_attrs.min_measure_value = WEATHER_STATION_ATTR_HUMIDITY_MIN;
	dev_ctx.humidity_attrs.max_measure_value = WEATHER_STATION_ATTR_HUMIDITY_MAX;
	/* Humidity measurements tolerance is not supported at the moment */
}

static void toggle_identify_led(zb_bufid_t bufid)
{
	static bool led_on;

	led_on = !led_on;
	dk_set_led(IDENTIFY_LED, led_on);
	zb_ret_t err = ZB_SCHEDULE_APP_ALARM(toggle_identify_led,
					     bufid,
					     ZB_MILLISECONDS_TO_BEACON_INTERVAL(
						     IDENTIFY_LED_BLINK_TIME_MSEC));
	if (err) {
		LOG_ERR("Failed to schedule app alarm: %d", err);
	}
}

static void start_identifying(zb_bufid_t bufid)
{
	ZVUNUSED(bufid);

	if (ZB_JOINED()) {
		/*
		 * Check if endpoint is in identifying mode,
		 * if not put desired endpoint in identifying mode.
		 */
		if (dev_ctx.identify_attr.identify_time ==
		    ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE) {

			zb_ret_t zb_err_code = zb_bdb_finding_binding_target(
				WEATHER_STATION_ENDPOINT_NB);

			if (zb_err_code == RET_OK) {
				LOG_INF("Manually enter identify mode");
			} else if (zb_err_code == RET_INVALID_STATE) {
				LOG_WRN("RET_INVALID_STATE - Cannot enter identify mode");
			} else {
				ZB_ERROR_CHECK(zb_err_code);
			}
		} else {
			LOG_INF("Manually cancel identify mode");
			zb_bdb_finding_binding_target_cancel();
		}
	} else {
		LOG_WRN("Device not in a network - cannot identify itself");
	}
}

static void identify_callback(zb_bufid_t bufid)
{
	zb_ret_t err = RET_OK;

	if (bufid) {
		/* Schedule a self-scheduling function that will toggle the LED */
		err = ZB_SCHEDULE_APP_CALLBACK(toggle_identify_led, bufid);
		if (err) {
			LOG_ERR("Failed to schedule app callback: %d", err);
		} else {
			LOG_INF("Enter identify mode");
		}
	} else {
		/* Cancel the toggling function alarm and turn off LED */
		err = ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_identify_led,
						   ZB_ALARM_ANY_PARAM);
		if (err) {
			LOG_ERR("Failed to schedule app alarm cancel: %d", err);
		} else {
			dk_set_led_off(IDENTIFY_LED);
			LOG_INF("Cancel identify mode");
		}
	}
}

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (IDENTIFY_MODE_BUTTON & has_changed) {
		if (IDENTIFY_MODE_BUTTON & button_state) {
			/* Button changed its state to pressed */
		} else {
			/* Button changed its state to released */
			if (was_factory_reset_done()) {
				/* The long press was for Factory Reset */
				LOG_DBG("After Factory Reset - ignore button release");
			} else   {
				/* Button released before Factory Reset */

				/* Start identification mode */
				zb_ret_t err = ZB_SCHEDULE_APP_CALLBACK(start_identifying, 0);

				if (err) {
					LOG_ERR("Failed to schedule app callback: %d", err);
				}

				/* Inform default signal handler about user input at the device */
				user_input_indicate();
			}
		}
	}

	check_factory_reset_button(button_state, has_changed);
}

static void gpio_init(void)
{
	int err = dk_buttons_init(button_changed);

	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

#ifdef CONFIG_USB_DEVICE_STACK
static void wait_for_console(void)
{
	const struct device *console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	uint32_t dtr = 0;
	uint32_t time = 0;

	/* Enable the USB subsystem and associated HW */
	if (usb_enable(NULL)) {
		LOG_ERR("Failed to enable USB");
	} else {
		/* Wait for DTR flag or deadline (e.g. when USB is not connected) */
		while (!dtr && time < WAIT_FOR_CONSOLE_DEADLINE_MSEC) {
			uart_line_ctrl_get(console, UART_LINE_CTRL_DTR, &dtr);
			/* Give CPU resources to low priority threads */
			k_sleep(K_MSEC(WAIT_FOR_CONSOLE_MSEC));
			time += WAIT_FOR_CONSOLE_MSEC;
		}
	}
}
#endif /* CONFIG_USB_DEVICE_STACK */

static void check_weather(zb_bufid_t bufid)
{
	ZVUNUSED(bufid);

	int err = weather_station_check_weather();

	if (err) {
		LOG_ERR("Failed to check weather: %d", err);
	} else {
		err = weather_station_update_temperature();
		if (err) {
			LOG_ERR("Failed to update temperature: %d", err);
		}

		err = weather_station_update_pressure();
		if (err) {
			LOG_ERR("Failed to update pressure: %d", err);
		}

		err = weather_station_update_humidity();
		if (err) {
			LOG_ERR("Failed to update humidity: %d", err);
		}
	}

	zb_ret_t zb_err = ZB_SCHEDULE_APP_ALARM(check_weather,
						0,
						ZB_MILLISECONDS_TO_BEACON_INTERVAL(
							WEATHER_CHECK_PERIOD_MSEC));
	if (zb_err) {
		LOG_ERR("Failed to schedule app alarm: %d", zb_err);
	}
}

#ifdef CONFIG_ZIGBEE_FOTA
static void confirm_image(void)
{
	if (!boot_is_img_confirmed()) {
		int ret = boot_write_img_confirmed();

		if (ret) {
			LOG_ERR("Couldn't confirm image: %d", ret);
		} else {
			LOG_INF("Marked image as OK");
		}
	}
}

static void ota_evt_handler(const struct zigbee_fota_evt *evt)
{
	switch (evt->id) {
	case ZIGBEE_FOTA_EVT_PROGRESS:
		dk_set_led(OTA_ACTIVITY_LED, evt->dl.progress % 2);
		break;

	case ZIGBEE_FOTA_EVT_FINISHED:
		LOG_INF("Reboot application.");
		/* Power on unused sections of RAM to allow MCUboot to use it. */
		if (IS_ENABLED(CONFIG_RAM_POWER_DOWN_LIBRARY)) {
			power_up_unused_ram();
		}

		sys_reboot(SYS_REBOOT_COLD);
		break;

	case ZIGBEE_FOTA_EVT_ERROR:
		LOG_ERR("OTA image transfer failed.");
		break;

	default:
		break;
	}
}

/**@brief Callback function for handling ZCL commands.
 *
 * @param[in]   bufid   Reference to Zigbee stack buffer
 *                      used to pass received data.
 */
static void zcl_device_cb(zb_bufid_t bufid)
{
	zb_zcl_device_callback_param_t *device_cb_param =
		ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t);

	if (device_cb_param->device_cb_id == ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID) {
		zigbee_fota_zcl_cb(bufid);
	} else {
		device_cb_param->status = RET_NOT_IMPLEMENTED;
	}
}

/**@brief Function to toggle the identify LED.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void toggle_identify_led_fota(zb_bufid_t bufid)
{
	static int blink_status;

	dk_set_led(IDENTIFY_LED, (++blink_status) % 2);
	ZB_SCHEDULE_APP_ALARM(toggle_identify_led_fota, bufid, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
}

/**@brief Function to handle identify notification events on the first endpoint.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void identify_cb(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;

	if (bufid) {
		/* Schedule a self-scheduling function that will toggle the LED. */
		ZB_SCHEDULE_APP_CALLBACK(toggle_identify_led_fota, bufid);
	} else {
		/* Cancel the toggling function alarm and turn off LED. */
		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_identify_led_fota, ZB_ALARM_ANY_PARAM);
		ZVUNUSED(zb_err_code);

		/* Update network status/idenitfication LED. */
		if (ZB_JOINED()) {
			dk_set_led_on(ZIGBEE_NETWORK_STATE_LED);
		} else {
			dk_set_led_off(ZIGBEE_NETWORK_STATE_LED);
		}
	}
}
#endif /* CONFIG_ZIGBEE_FOTA */

void zboss_signal_handler(zb_bufid_t bufid)
{
	zb_zdo_app_signal_hdr_t *signal_header = NULL;
	zb_zdo_app_signal_type_t signal = zb_get_app_signal(bufid, &signal_header);
	zb_ret_t err = RET_OK;

	/* Update network status LED but only for debug configuration */
	#ifdef CONFIG_LOG
	zigbee_led_status_update(bufid, ZIGBEE_NETWORK_STATE_LED);
	#endif /* CONFIG_LOG */

#ifdef CONFIG_ZIGBEE_FOTA
	/* Pass signal to the OTA client implementation. */
	zigbee_fota_signal_handler(bufid);
#endif /* CONFIG_ZIGBEE_FOTA */

	/* Detect ZBOSS startup */
	switch (signal) {
	case ZB_ZDO_SIGNAL_SKIP_STARTUP:
		/* ZBOSS framework has started - schedule first weather check */
		err = ZB_SCHEDULE_APP_ALARM(check_weather,
					    0,
					    ZB_MILLISECONDS_TO_BEACON_INTERVAL(
						    WEATHER_CHECK_INITIAL_DELAY_MSEC));
		if (err) {
			LOG_ERR("Failed to schedule app alarm: %d", err);
		}
		break;
	default:
		break;
	}

	/* Let default signal handler process the signal*/
	ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));

	/*
	 * All callbacks should either reuse or free passed buffers.
	 * If bufid == 0, the buffer is invalid (not passed).
	 */
	if (bufid) {
		zb_buf_free(bufid);
	}
}

int main(void)
{
	int rc = STATS_INIT_AND_REG(smp_svr_stats, STATS_SIZE_32, "smp_svr_stats");

	if (rc < 0) {
		LOG_ERR("Error initializing stats system [%d]", rc);
	}

	/* Register the built-in mcumgr command handlers. */
#ifdef CONFIG_MCUMGR_GRP_FS
	rc = fs_mount(&littlefs_mnt);
	if (rc < 0) {
		LOG_ERR("Error mounting littlefs [%d]", rc);
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
	start_smp_bluetooth_adverts();
#endif

	/* Using __TIME__ ensure that a new binary will be built on every
	 * compile which is convenient when testing firmware upgrade.
	 */
	LOG_INF("build time: " __DATE__ " " __TIME__);


	/* Zigbee weather station application code. */
	#ifdef CONFIG_USB_DEVICE_STACK
	wait_for_console();
	#endif /* CONFIG_USB_DEVICE_STACK */

	register_factory_reset_button(FACTORY_RESET_BUTTON);
	gpio_init();
	weather_station_init();

#ifdef CONFIG_ZIGBEE_FOTA
	/* Initialize Zigbee FOTA download service. */
	zigbee_fota_init(ota_evt_handler);

	/* Mark the current firmware as valid. */
	confirm_image();

	/* Register callback for handling ZCL commands. */
	ZB_ZCL_REGISTER_DEVICE_CB(zcl_device_cb);
#endif /* CONFIG_ZIGBEE_FOTA */

	/* Register device context (endpoint) */
	ZB_AF_REGISTER_DEVICE_CTX(&weather_station_ctx);

	/* Init Basic and Identify attributes */
	mandatory_clusters_attr_init();

	/* Init measurements-related attributes */
	measurements_clusters_attr_init();

	/* Register callback to identify notifications */
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(WEATHER_STATION_ENDPOINT_NB, identify_callback);

#ifdef CONFIG_ZIGBEE_FOTA
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(CONFIG_ZIGBEE_FOTA_ENDPOINT, identify_cb);
#endif /* CONFIG_ZIGBEE_FOTA */

	/* Enable Sleepy End Device behavior */
	zb_set_rx_on_when_idle(ZB_FALSE);
	if (IS_ENABLED(CONFIG_RAM_POWER_DOWN_LIBRARY)) {
		power_down_unused_ram();
	}

	/* Start Zigbee stack */
	zigbee_enable();

	/* From SMP Server main.c,
	 * The system work queue handles all incoming mcumgr requests.  Let the
	 * main thread idle while the mcumgr server runs.
	 */
	while (1) {
		k_sleep(K_MSEC(1000));
		STATS_INC(smp_svr_stats, ticks);
	}
	return 0;
}
