#if HARMAN_NTC_DETECT_PROTECT || HARMAN_DISCHARGER_NTC_DETECT_PROTECT
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "gap_timer.h"
#include "hal_gpio.h"
#include "section.h"
#include "trace.h"
#include "os_sched.h"
#include "pm.h"
#include "single_tone.h"
#include "app_main.h"
#include "app_dlps.h"
#include "app_cfg.h"
#include "app_gpio.h"
#include "app_io_msg.h"
#include "adc_manager.h"
#include "app_mmi.h"
#include "app_harman_adc.h"
#include "app_ext_charger.h"
#if HARMAN_VBAT_ONE_ADC_DETECTION
#include "app_harman_ntc_const.h"
#endif

#if GFPS_FINDER_SUPPORT
#include "app_gfps_finder.h"
#endif

static uint8_t app_harman_adc_timer_queue_id = 0;
static void *timer_handle_ntc_period_check = NULL;

typedef enum
{
    APP_HARMAN_ADC_TIMER_NTC_CHECK   = 0x00,
} APP_HARMAN_ADC_TIMER_MSG_TYPE;

static bool is_adc_mgr_init = false;
static uint8_t harman_adc_chanel_index = 0 ;
static uint8_t harman_adc_pin = HARMAN_THERMISTOR_PIN;

#if HARMAN_SECOND_NTC_DETECT_PROTECT
static uint8_t harman_adc_pin2 = HARMAN_THERMISTOR_PIN2;
#endif

#if (HARMAN_SECOND_NTC_DETECT_PROTECT | HARMAN_VBAT_ADC_DETECTION)
static uint8_t vbat_adc_check_times = 0;
static int32_t adc_2_battery_sum = 0;
static int32_t adc_3_battery_sum = 0;
#endif

#if HARMAN_VBAT_ONE_ADC_DETECTION
#define	BATTERY_NTC_TIME_5MIN	12*5//5s*12*5
static uint16_t battery_ntc_periodic_time = 0;

#endif

typedef struct
{
    uint16_t adc_2_battery;
    uint16_t adc_3_battery;
    uint16_t voltage_battery;
} APP_HARMAN_ADC_DATA_MGR;

static APP_HARMAN_ADC_DATA_MGR adc_data_mgr = {0};

static uint16_t total_check_times = 0;
static uint8_t need_power_off = 0;

static bool ignore_first_adc_value = false;

/* NTC handle */
ISR_TEXT_SECTION
static void app_harman_adc_interrupt_handler(void *pvPara, uint32_t int_status)
{
    uint16_t adc_data[3];
    T_IO_MSG gpio_msg;

    adc_mgr_read_data_req(harman_adc_chanel_index, adc_data, 0x7);
    adc_data_mgr.adc_2_battery = ADC_GetHighBypassRes(adc_data[0], EXT_SINGLE_ENDED(harman_adc_pin));
#if HARMAN_SECOND_NTC_DETECT_PROTECT
    adc_data_mgr.adc_3_battery = ADC_GetHighBypassRes(adc_data[1], EXT_SINGLE_ENDED(harman_adc_pin2));
#endif
    adc_data_mgr.voltage_battery = ADC_GetRes(adc_data[2], INTERNAL_VBAT_MODE);

    gpio_msg.type = IO_MSG_TYPE_GPIO;

    gpio_msg.subtype = IO_MSG_GPIO_EXT_CHARGER_ADC_VALUE;

    if (!app_io_send_msg(&gpio_msg))
    {
        APP_PRINT_ERROR0("app_harman_adc_interrupt_handler: Send msg error");
    }
}

void app_harman_adc_io_read(void)
{
    uint8_t input_pin = harman_adc_pin;

    if (!IS_ADC_CHANNEL(input_pin))
    {
        APP_PRINT_ERROR1("app_harman_adc_io_read: invalid ADC IO: 0x%x", input_pin);
        return;
    }

#if HARMAN_SECOND_NTC_DETECT_PROTECT
    uint8_t input_pin2 = harman_adc_pin2;

    if (!IS_ADC_CHANNEL(input_pin))
    {
        APP_PRINT_ERROR1("app_harman_adc_io_read: invalid ADC IO: 0x%x", input_pin2);
        return;
    }
#endif

    if (is_adc_mgr_init)
    {
        APP_PRINT_INFO1("app_harman_adc_io_read: ever registered %d",
                        harman_adc_chanel_index);
        adc_mgr_enable_req(harman_adc_chanel_index);
        return;
    }

    Pad_Config(input_pin, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_PowerOrShutDownValue(input_pin, 0);
    ADC_HighBypassCmd(input_pin, (FunctionalState)ENABLE);

#if HARMAN_SECOND_NTC_DETECT_PROTECT
    Pad_Config(input_pin2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_PowerOrShutDownValue(input_pin2, 0);
    ADC_HighBypassCmd(input_pin2, (FunctionalState)ENABLE);
#endif

    ignore_first_adc_value = true;

    ADC_InitTypeDef ADC_InitStruct;
    ADC_StructInit(&ADC_InitStruct);
    ADC_InitStruct.adcClock = ADC_CLK_39K;
    ADC_InitStruct.bitmap = 0x7;
    ADC_InitStruct.schIndex[0] = EXT_SINGLE_ENDED(input_pin);
#if HARMAN_SECOND_NTC_DETECT_PROTECT
    ADC_InitStruct.schIndex[1] = EXT_SINGLE_ENDED(input_pin2);
#endif
    ADC_InitStruct.schIndex[2] = INTERNAL_VBAT_MODE;

    if (!adc_mgr_register_req(&ADC_InitStruct,
                              (adc_callback_function_t)app_harman_adc_interrupt_handler,
                              &harman_adc_chanel_index))
    {
        APP_PRINT_INFO0("app_harman_adc_io_read: ADC Register Request Fail");
        return;
    }
    else
    {
        is_adc_mgr_init = true;
    }

    adc_mgr_enable_req(harman_adc_chanel_index);

    APP_PRINT_TRACE1("app_harman_adc_io_read: %d", harman_adc_chanel_index);

    return;
}

uint16_t app_harman_adc_voltage_battery_get(void)
{
    return adc_data_mgr.voltage_battery;
}

#if (HARMAN_SECOND_NTC_DETECT_PROTECT | HARMAN_VBAT_ADC_DETECTION | HARMAN_VBAT_ONE_ADC_DETECTION)
void app_harman_adc_vbat_check_times_clear(void)
{
#if !HARMAN_VBAT_ONE_ADC_DETECTION
	vbat_adc_check_times = 0;
#endif

    app_ext_charger_vbat_is_normal_set(VBAT_DETECT_STATUS_ING);
}
#if HARMAN_VBAT_ONE_ADC_DETECTION
void app_harman_update_battery_ntc_value(uint16_t *p_ntc_value)
{
    *p_ntc_value = adc_data_mgr.adc_2_battery;

    if (app_ext_charger_vbat_is_normal_get() == VBAT_DETECT_STATUS_SUCCESS)
    {
        app_cfg_nv.vbat_ntc_value = adc_data_mgr.adc_2_battery;
        app_cfg_store(&app_cfg_nv.vbat_ntc_value, 4);
    }

    APP_PRINT_TRACE4("app_harman_adc_update_cur_ntc_value: cur_ntc_value: %d, adc_2_battery: %d, "
                     "adc_3_battery: %d, vbat_is_normal: %d",
                     *p_ntc_value, adc_data_mgr.adc_2_battery,
                     adc_data_mgr.adc_3_battery, app_ext_charger_vbat_is_normal_get());
}

int8_t app_harman_adc_check_ntc_curve(int saved_temperature, uint16_t current_ntc_value)
{
    int i,index=0;
    uint8_t result = 0xff; 

	for(i = 0;i < BATTERY_NTC_MAX_LENGTH;i++)
	{
		if(battery_ntc_adc_array[i].temperature == saved_temperature)
		{
			index = i;
		}
	}
	if(index >= BATTERY_NTC_MAX_LENGTH)
    {
        return 0xff;
    }

	for(i = 0; i<HARMAN_BATTERY_NTC_MAX; i++)
	{
		if (current_ntc_value >= battery_ntc_adc_array[index].bat_ntc_value_array[i] - battery_ntc_offset[i] && 
			current_ntc_value <= battery_ntc_adc_array[index].bat_ntc_value_array[i] + battery_ntc_offset[i])
		{
			result = i;
			break;
		}
	}
	APP_PRINT_INFO1("---->app_harman_adc_check_ntc_curve curve = %d",result);
    return result;
}

int app_harman_adc_calculate_temperature(uint16_t current_ntc_value,uint8_t ntc_type)
{
    int closest_temp = battery_ntc_adc_array[0].temperature;
    int closest_adc = battery_ntc_adc_array[0].bat_ntc_value_array[ntc_type];
    int min_diff = abs(current_ntc_value - closest_adc);

    // 遍历数组，找到最接近的ADC值对应的温度
    for (int i = 1; i < BATTERY_NTC_MAX_LENGTH; ++i) {
        int current_adc = battery_ntc_adc_array[i].bat_ntc_value_array[ntc_type];
        int diff = abs(current_ntc_value - current_adc);

        if (diff < min_diff) {
            min_diff = diff;
            closest_temp = battery_ntc_adc_array[i].temperature;
            closest_adc = current_adc;
        } else if (diff == min_diff) {
            // 如果有多个相同的差值，选择温度较高的那个
            if (battery_ntc_adc_array[i].temperature > closest_temp) {
                closest_temp = battery_ntc_adc_array[i].temperature;
                closest_adc = current_adc;
            }
        }
    }
	return closest_temp;
}
void app_harman_adc_saved_nv_data(void)
{
	app_cfg_nv.nv_saved_vbat_value = adc_data_mgr.voltage_battery;
	app_cfg_nv.nv_saved_vbat_ntc_value = adc_data_mgr.adc_2_battery;

	app_cfg_store(&app_cfg_nv.nv_saved_vbat_value, 4);
	app_cfg_store(&app_cfg_nv.nv_saved_vbat_ntc_value, 4);	
	APP_PRINT_INFO2("---->saved value voltage_battery=%d adc_2_battery=%d",app_cfg_nv.nv_saved_vbat_value,app_cfg_nv.nv_saved_vbat_ntc_value);
}
#endif

static void app_harman_adc_vbat_detect(uint16_t check_time, uint16_t max_discrepancy_value) // ms
{
#if HARMAN_VBAT_ONE_ADC_DETECTION
	APP_PRINT_INFO1("---->app_harman_adc_vbat_detect! %d",app_cfg_nv.ntc_poweroff_wakeup_flag);

	uint16_t current_battery_ntc = adc_data_mgr.adc_2_battery;
	uint16_t current_battery_value = adc_data_mgr.voltage_battery;
	int current_temperature = 0;
	static bool is_first_detect = false;

	APP_PRINT_INFO4("---->app_harman_adc_vbat_detect adc2=%d,adc3=%d,bat=%d,charge_mode=0x%x",
		adc_data_mgr.adc_2_battery,adc_data_mgr.adc_3_battery,adc_data_mgr.voltage_battery,app_ext_charger_vbat_is_normal_get());
	APP_PRINT_INFO6("---->Read nv data nv_saved_vbat_value=%d nv_saved_vbat_ntc_value=%d saved_bat_err=%d saved_type=%d saved_temper=%d single_ntc_flag %d",
					app_cfg_nv.nv_saved_vbat_value,app_cfg_nv.nv_saved_vbat_ntc_value,app_cfg_nv.nv_saved_battery_err,app_cfg_nv.nv_ntc_resistance_type,
					app_cfg_nv.nv_ntc_vbat_temperature,app_cfg_nv.nv_single_ntc_function_flag);	
	if(app_cfg_nv.nv_single_ntc_function_flag == 0)
	{
		APP_PRINT_INFO0("---->nv_single_ntc_function_flag closed!");
		return;
	}
	if(app_cfg_nv.nv_saved_vbat_value == 0)
	{
		/* first used */
		goto EXIT_HARMAN_BATTERY_DETECT;
	}
	if(++battery_ntc_periodic_time>=BATTERY_NTC_TIME_5MIN || !is_first_detect)
	{
		battery_ntc_periodic_time = 0;
		//time for 5min
		if(!is_first_detect)
		{
			is_first_detect = true;
		}

		current_temperature = app_harman_adc_calculate_temperature(current_battery_ntc,HARMAN_BATTERY_NTC_68K);
		if(app_cfg_nv.nv_saved_battery_err == HARMAN_BATTERY_NOMAL)
		{
			uint16_t vbat_discrepancy_value = 0;
			uint16_t vbat_ntc_discrepancy_value = 0;
			/* 1.check battery value */
			vbat_discrepancy_value = abs(current_battery_value - app_cfg_nv.nv_saved_vbat_value);
			APP_PRINT_INFO1("---->vbat_discrepancy_value %d",vbat_discrepancy_value);
			if(vbat_discrepancy_value < BATTERY_DISCREPANCY_VALUE)
			{
				/* battery OK! */
				app_cfg_nv.nv_saved_battery_err = HARMAN_BATTERY_NOMAL;
				app_ext_charger_vbat_is_normal_set(VBAT_DETECT_STATUS_SUCCESS);

				app_cfg_nv.nv_ntc_vbat_temperature = current_temperature;
				app_cfg_store(&app_cfg_nv.nv_ntc_vbat_temperature, 4);
				app_cfg_store(&app_cfg_nv.nv_saved_battery_err, 4);
				APP_PRINT_INFO0("---->vbat_SUCCESS");
			}
			/* 2.check battery ntc value */
			else
			{
				vbat_ntc_discrepancy_value = abs(current_battery_ntc - app_cfg_nv.nv_saved_vbat_ntc_value);
				APP_PRINT_INFO1("---->vbat_ntc_discrepancy_value %d",vbat_ntc_discrepancy_value);
				if(vbat_ntc_discrepancy_value < BATTERY_NTC_DISCREPANCY_VALUE)
				{
					/* ntc OK! *///仍是68K电池
					app_cfg_nv.nv_saved_battery_err = HARMAN_BATTERY_ERR_VOLTAGE;
				}
				else
				{
					/* ntc NG! */
					uint8_t ntc_curve = app_harman_adc_check_ntc_curve(app_cfg_nv.nv_ntc_vbat_temperature,current_battery_ntc);
					if(ntc_curve!=0xff)
					{
						/*	*///为其他NTC阻值的电池,更换充电曲线
						app_cfg_nv.nv_saved_battery_err = HARMAN_BATTERY_ERR_VOLTAGE;
						
						/* 更换ntc曲线 */
						app_ext_charger_cfg_reload(ntc_curve);
					}
					else
					{
						/*	*///NTC异常电池损坏
						app_cfg_nv.nv_saved_battery_err = HARMAN_BATTERY_ERR_NTC;
					}
					if(ntc_curve != app_cfg_nv.nv_ntc_resistance_type)
					{
						app_cfg_nv.nv_ntc_resistance_type = ntc_curve;
						app_cfg_store(&app_cfg_nv.nv_ntc_resistance_type, 4);
					}					
				}
				app_ext_charger_vbat_is_normal_set(VBAT_DETECT_STATUS_FAIL);
				app_cfg_store(&app_cfg_nv.nv_saved_battery_err, 4);
			}			
		}
		else if(app_cfg_nv.nv_saved_battery_err == HARMAN_BATTERY_ERR_VOLTAGE)
		{
			/* battery NG! */ //小电流充电
			app_ext_charger_vbat_is_normal_set(VBAT_DETECT_STATUS_FAIL);
		}		
		else if(app_cfg_nv.nv_saved_battery_err == HARMAN_BATTERY_ERR_NTC)
		{
			/* battery NG! */ //不充电
			app_ext_charger_vbat_is_normal_set(VBAT_DETECT_STATUS_FAIL);
		}
EXIT_HARMAN_BATTERY_DETECT:
		app_harman_adc_saved_nv_data();
	}
#else

    if (app_ext_charger_vbat_is_normal_get() == VBAT_DETECT_STATUS_ING)
    {
        total_check_times = check_time / DISCHARGER_NTC_CHECK_PERIOD;

        if (vbat_adc_check_times <= total_check_times)
        {
            vbat_adc_check_times ++;

            adc_2_battery_sum += adc_data_mgr.adc_2_battery;
            adc_3_battery_sum += adc_data_mgr.adc_3_battery;
            if (vbat_adc_check_times == total_check_times)
            {
                uint16_t discrepancy_value = 0;

                adc_2_battery_sum = adc_2_battery_sum / total_check_times;
                adc_3_battery_sum = adc_3_battery_sum / total_check_times;
                discrepancy_value = abs(adc_3_battery_sum - adc_2_battery_sum);

#if HARMAN_VBAT_ADC_DETECTION
                max_discrepancy_value = app_cfg_nv.vbat_ntc_value * 0.1; // 10%
                discrepancy_value = abs(adc_2_battery_sum - app_cfg_nv.vbat_ntc_value);
                if (app_cfg_nv.vbat_detect_normal == true &&
                    discrepancy_value < max_discrepancy_value)
                {
                    app_ext_charger_vbat_is_normal_set(VBAT_DETECT_STATUS_SUCCESS);

                    app_cfg_nv.vbat_ntc_value = adc_data_mgr.adc_2_battery;
                    app_cfg_store(&app_cfg_nv.vbat_ntc_value, 4);
                }
                else
                {
                    app_cfg_nv.vbat_detect_normal = false;
                    app_cfg_store(&app_cfg_nv.harman_category_id, 4);
                    app_ext_charger_vbat_is_normal_set(VBAT_DETECT_STATUS_FAIL);

                    // stop charging and discharger NTC protect
                    app_harman_adc_ntc_check_timer_stop();
                }

#else
                if (discrepancy_value <= max_discrepancy_value)
                {
                    app_ext_charger_vbat_is_normal_set(VBAT_DETECT_STATUS_SUCCESS);
                }
                else
                {
                    app_ext_charger_vbat_is_normal_set(VBAT_DETECT_STATUS_FAIL);
                }
#endif

                app_dlps_enable(APP_DLPS_ENTER_VBAT_VALUE_UPDATE);
                APP_PRINT_INFO3("app_harman_adc_vbat_detect: discrepancy_value: %d, max_discrepancy_value: %d, vbat_detect_normal %d",
                                discrepancy_value, max_discrepancy_value, app_cfg_nv.vbat_detect_normal);
            }
        }
    }
    APP_PRINT_INFO3("app_harman_adc_vbat_detect: vbat_adc_check_times: %d, total_check_times: %d, vbat_is_normal: %d",
                    vbat_adc_check_times, total_check_times, app_ext_charger_vbat_is_normal_get());
#endif	
}
#endif

void app_harman_adc_update_cur_ntc_value(uint16_t *p_ntc_value)
{
#if HARMAN_SECOND_NTC_DETECT_PROTECT
    if (app_ext_charger_vbat_is_normal_get() == VBAT_DETECT_STATUS_SUCCESS)
    {
        *p_ntc_value = adc_data_mgr.adc_2_battery;
    }
    else if (app_ext_charger_vbat_is_normal_get() == VBAT_DETECT_STATUS_FAIL)
    {
        *p_ntc_value = adc_data_mgr.adc_3_battery;
    }
#else
    *p_ntc_value = adc_data_mgr.adc_2_battery;
#if HARMAN_VBAT_ADC_DETECTION
    if (app_ext_charger_vbat_is_normal_get() == VBAT_DETECT_STATUS_SUCCESS)
    {
        app_cfg_nv.vbat_ntc_value = adc_data_mgr.adc_2_battery;
        app_cfg_store(&app_cfg_nv.vbat_ntc_value, 4);
    }
#endif
#endif
    APP_PRINT_TRACE6("app_harman_adc_update_cur_ntc_value: cur_ntc_value: %d, adc_2_battery: %d, "
                     "adc_3_battery: %d, VBAT: %d,vbat_is_normal: %d, vbat_detect_status: %d",
                     *p_ntc_value, adc_data_mgr.adc_2_battery,
                     adc_data_mgr.adc_3_battery, adc_data_mgr.voltage_battery,
                     app_ext_charger_vbat_is_normal_get(), app_cfg_nv.vbat_detect_status);
}

#if HARMAN_DISCHARGER_NTC_DETECT_PROTECT
bool app_harman_discharger_ntc_check_valid(void)
{
    bool ret = false;
    uint16_t ntc_value = 0;
	
#if HARMAN_VBAT_ONE_ADC_DETECTION
	if(app_cfg_nv.nv_ntc_resistance_type == 0xff)
	{
		APP_PRINT_TRACE0("app_harman_discharger_ntc_check_valid  type = 0xff");
		return false;
	}
	else
	{
		if(app_cfg_nv.nv_ntc_resistance_type != 0)
		{
			uint8_t temp_index = app_ext_charge_find_index(BATTERY_DISCHARGE_HIGH_TEMP_ERROR_VALUE);
			app_cfg_const.high_temperature_protect_value = battery_ntc_adc_array[temp_index].bat_ntc_value_array[app_cfg_nv.nv_ntc_resistance_type];
			temp_index = app_ext_charge_find_index(BATTERY_DISCHARGE_LOW_TEMP_ERROR_VALUE);
			app_cfg_const.low_temperature_protect_value = battery_ntc_adc_array[temp_index].bat_ntc_value_array[app_cfg_nv.nv_ntc_resistance_type];
		}
		APP_PRINT_TRACE2("app_harman_discharger_ntc_check_valid  high = %d  low = %d",app_cfg_const.high_temperature_protect_value,app_cfg_const.low_temperature_protect_value);
	}
#endif	

    if (!app_cfg_const.discharge_mode_protection_thermistor_option || mp_hci_test_mode_is_running())
    {
        ret = true;
    }
    else if ((app_cfg_const.discharge_mode_protection_thermistor_option) &&
             (app_db.device_state != APP_DEVICE_STATE_ON))
    {
        app_harman_adc_update_cur_ntc_value(&ntc_value);
        if ((ntc_value >= app_cfg_const.high_temperature_protect_value) &&
            (ntc_value <= app_cfg_const.low_temperature_protect_value))
        {
            ret = true;
        }
    }		 
    APP_PRINT_TRACE3("app_harman_discharger_ntc_check_valid: %d, ntc_value: %d, device_state: %d",
                     ret, ntc_value, app_db.device_state);
    return ret;
}

void app_harman_discharger_adc_update(void)
{
    uint16_t ntc_value = 0;
#if HARMAN_VBAT_ONE_ADC_DETECTION
	if(app_cfg_nv.nv_ntc_resistance_type == 0xff)
	{
		APP_PRINT_TRACE0("app_harman_discharger_adc_update  type = 0xff");
		return;
	}
	else
	{
		if(app_cfg_nv.nv_ntc_resistance_type != 0)
		{
			uint8_t temp_index = app_ext_charge_find_index(BATTERY_DISCHARGE_HIGH_TEMP_ERROR_VALUE);
			app_cfg_const.high_temperature_protect_value = battery_ntc_adc_array[temp_index].bat_ntc_value_array[app_cfg_nv.nv_ntc_resistance_type];
			temp_index = app_ext_charge_find_index(BATTERY_DISCHARGE_LOW_TEMP_ERROR_VALUE);
			app_cfg_const.low_temperature_protect_value = battery_ntc_adc_array[temp_index].bat_ntc_value_array[app_cfg_nv.nv_ntc_resistance_type];
		}
		APP_PRINT_TRACE2("app_harman_discharger_ntc_check_valid  high = %d  low = %d",app_cfg_const.high_temperature_protect_value,app_cfg_const.low_temperature_protect_value);
	}	
#endif

    app_harman_adc_update_cur_ntc_value(&ntc_value);
    if (app_db.device_state == APP_DEVICE_STATE_ON)
    {
        if ((ntc_value < app_cfg_const.high_temperature_protect_value) ||
            (ntc_value > app_cfg_const.low_temperature_protect_value))
        {
            if (++ need_power_off == 5)
            {
                need_power_off = 0;
                app_mmi_handle_action(MMI_DEV_POWER_OFF);
            }
        }
        else
        {
            need_power_off = 0;
        }
    }
    else if ((app_db.device_state == APP_DEVICE_STATE_OFF) &&
             app_db.need_delay_power_on)
    {
        app_db.need_delay_power_on = false;
        app_dlps_enable(APP_DLPS_ENTER_VBAT_VALUE_UPDATE);
        if (app_harman_discharger_ntc_check_valid())
        {
            app_mmi_handle_action(MMI_DEV_POWER_ON);
        }
    }
#if HARMAN_VBAT_ONE_ADC_DETECTION			 
	app_harman_adc_saved_nv_data();
#endif			 
    APP_PRINT_TRACE3("app_harman_discharger_adc_update: ntc_value: %d, device_state: %d, need_power_off: %d",
                     ntc_value, app_db.device_state, need_power_off);
}

void app_harman_discharger_ntc_timer_start(void)
{
    if (app_cfg_const.discharge_mode_protection_thermistor_option)
    {
        app_harman_adc_io_read();
        app_harman_adc_ntc_check_timer_start(DISCHARGER_NTC_CHECK_PERIOD);
    }
}
#endif

void app_harman_adc_msg_handle(void)
{
    if (ignore_first_adc_value)
    {
        ignore_first_adc_value = false;
        return;
    }

#if HARMAN_VBAT_ADC_DETECTION
    if (app_cfg_nv.store_adc_ntc_value_when_vbat_in)
    {
        app_cfg_nv.store_adc_ntc_value_when_vbat_in = 0;
        app_cfg_store(&app_cfg_nv.offset_is_dut_test_mode, 4);

        app_cfg_nv.vbat_detect_normal = 1;
        app_cfg_store(&app_cfg_nv.harman_category_id, 4);

        app_cfg_nv.vbat_ntc_value = adc_data_mgr.adc_2_battery;
        app_cfg_store(&app_cfg_nv.vbat_ntc_value, 4);
    }
#endif

#if (HARMAN_SECOND_NTC_DETECT_PROTECT | HARMAN_VBAT_ADC_DETECTION)
    uint16_t check_time = 0;
    uint16_t max_discrepancy_value = 0;

    if (app_ext_charger_check_status())
    {
        check_time = CHARGING_VBAT_DETECT_TOTAL_TIME;
        max_discrepancy_value = CHARGER_VABT_MAX_ADC_DISCREPANCY_VALUE;
    }
#if HARMAN_DISCHARGER_NTC_DETECT_PROTECT
    else
    {
        check_time = DISCHARGER_VBAT_CHECK_TOTAL_TIME;
        max_discrepancy_value = DISCHARGER_VABT_MAX_ADC_DISCREPANCY_VALUE;
    }
#endif

#if HARMAN_DELAY_HANDLE_ADP_OUT_SUPPORT
    if (app_cfg_nv.vbat_detect_status != VBAT_DETECT_STATUS_ING)
    {
        app_ext_charger_vbat_is_normal_set(app_cfg_nv.vbat_detect_status);
    }
#endif
    {
        if (app_ext_charger_vbat_is_normal_get() == VBAT_DETECT_STATUS_ING)
        {
            app_harman_adc_vbat_detect(check_time, max_discrepancy_value);
            if (app_ext_charger_vbat_is_normal_get() == VBAT_DETECT_STATUS_ING)
            {
                return;
            }
#if HARMAN_DELAY_HANDLE_ADP_OUT_SUPPORT
            else
            {
                app_cfg_nv.vbat_detect_status = app_ext_charger_vbat_is_normal_get();
                app_cfg_store(&app_cfg_nv.ota_continuous_packets_max_count, 4);
            }
#endif
        }
    }
#elif HARMAN_VBAT_ONE_ADC_DETECTION
	app_harman_adc_vbat_detect(0, 0);
#endif

    if (app_ext_charger_check_status())
    {
        app_ext_charger_ntc_adc_update();
    }
#if HARMAN_DISCHARGER_NTC_DETECT_PROTECT
    else
    {
        app_harman_discharger_adc_update();
    }
#endif
}

static void app_harman_adc_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_harman_adc_timeout_cb: timer_id 0x%02x, timer_chann %d ",
                     timer_id, timer_chann);
    switch (timer_id)
    {
    case APP_HARMAN_ADC_TIMER_NTC_CHECK:
        {
#if HARMAN_VBAT_ADC_DETECTION
            if ((!app_cfg_nv.vbat_detect_normal) &&
                (!app_cfg_nv.store_adc_ntc_value_when_vbat_in))
            {
                // stop charging and discharger NTC protect
                app_harman_adc_ntc_check_timer_stop();
                break;
            }
#endif
            //read ntc value
            if (app_ext_charger_check_status())
            {
                app_ext_charger_ntc_check_timeout();
                app_harman_adc_ntc_check_timer_start(CHARGING_NTC_CHECK_PERIOD);
            }
#if HARMAN_DISCHARGER_NTC_DETECT_PROTECT
            else
            {
                app_harman_discharger_ntc_timer_start();
            }
#endif
        }
        break;

    default:
        break;
    }
}

bool app_harman_adc_ntc_check_timer_started(void)
{
    if (timer_handle_ntc_period_check != NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void app_harman_adc_ntc_check_timer_stop(void)
{
    gap_stop_timer(&timer_handle_ntc_period_check);
}

void app_harman_adc_ntc_check_timer_start(uint32_t time)
{
#if (F_APP_PERIODIC_WAKEUP_RECHARGE && GFPS_FINDER_SUPPORT)
    APP_PRINT_INFO4("app_harman_adc_ntc_check_timer_start: finder_privisiond: 0x%x, wakeup_reason: 0x%x, "
                    "device_state: %d, dlps_bitmap: 0x%x",
                    app_gfps_finder_provisioned(), app_db.wake_up_reason,
                    app_db.device_state, app_dlps_get_dlps_bitmap());
    if ((!extend_app_cfg_const.disable_finder_adv_when_power_off)
        && (app_gfps_finder_provisioned())
        && (app_db.wake_up_reason & WAKE_UP_RTC)
        && (app_db.device_state == APP_DEVICE_STATE_OFF))
    {
#if HARMAN_VBAT_ADC_DETECTION
        if ((app_dlps_get_dlps_bitmap() & APP_DLPS_ENTER_VBAT_VALUE_UPDATE) !=
            APP_DLPS_ENTER_VBAT_VALUE_UPDATE)
#endif
        {
            return;
        }
    }
#endif

#if HARMAN_VBAT_ADC_DETECTION
    if ((!app_cfg_nv.vbat_detect_normal) &&
        (!app_cfg_nv.store_adc_ntc_value_when_vbat_in))
    {
        return;
    }
#endif

    if (timer_handle_ntc_period_check == NULL)
    {
        gap_start_timer(&timer_handle_ntc_period_check, "ntc_check",
                        app_harman_adc_timer_queue_id, APP_HARMAN_ADC_TIMER_NTC_CHECK, 0,
                        time);
        power_register_excluded_handle(&timer_handle_ntc_period_check, PM_EXCLUDED_TIMER);
    }
    else
    {
        gap_stop_timer(&timer_handle_ntc_period_check);
        gap_start_timer(&timer_handle_ntc_period_check, "ntc_check",
                        app_harman_adc_timer_queue_id, APP_HARMAN_ADC_TIMER_NTC_CHECK, 0,
                        time);
    }
}

bool app_harman_adc_adp_in_handle(void)
{
#if HARMAN_VBAT_ADC_DETECTION
    if ((!app_cfg_nv.vbat_detect_normal) &&
        (!app_cfg_nv.store_adc_ntc_value_when_vbat_in))
    {
        return true;
    }
#endif

    app_harman_adc_io_read();
    app_harman_adc_ntc_check_timer_start(2 * CHARGING_NTC_CHECK_PERIOD);

    return false;
}

void app_harman_adc_adp_out_handle(void)
{
#if (HARMAN_SECOND_NTC_DETECT_PROTECT | HARMAN_VBAT_ADC_DETECTION)
    app_harman_adc_vbat_check_times_clear();
#endif

#if HARMAN_DISCHARGER_NTC_DETECT_PROTECT
    app_harman_discharger_ntc_timer_start();
#endif
}

void app_harman_adc_init(void)
{
    gap_reg_timer_cb(app_harman_adc_timeout_cb, &app_harman_adc_timer_queue_id);

#if (HARMAN_SECOND_NTC_DETECT_PROTECT | HARMAN_VBAT_ADC_DETECTION | HARMAN_VBAT_ONE_ADC_DETECTION)
    app_harman_adc_vbat_check_times_clear();
#endif

#if HARMAN_DISCHARGER_NTC_DETECT_PROTECT
    app_harman_discharger_ntc_timer_start();
#endif
}
#endif
