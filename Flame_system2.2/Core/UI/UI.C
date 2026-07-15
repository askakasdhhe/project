#include "UI.h"

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "tim.h"

#include "../esp8266/esp8266.h"
#include "../App/app_onenet_mqtt.h"
#include "../LVGL/lvgl.h"
#include "ctype.h"
#include <stdbool.h>

LV_IMG_DECLARE(title)
LV_IMG_DECLARE(buzzer)
LV_IMG_DECLARE(fire)
LV_IMG_DECLARE(icon_bg)

LV_FONT_DECLARE(syht_font_16)

#define MQTT_QOS   "1"
#define MQTT_SUB_QOS "1"

static lv_style_t TextareaStyle;
static lv_style_t BtnStyle;

lv_obj_t *FlamestateLabel;
lv_obj_t *buzzerstateLabel;

static lv_obj_t *WifiLogo;
static lv_obj_t *KeyBoardObj;

lv_obj_t *UsrCtl;
lv_obj_t *SmartCtl;

lv_obj_t *ADType;
lv_obj_t *IOType;

struct ThresholdT_t Getthreshold = {
    .AD_OR_IO = AD,
    .buzzerofflevel = 300,
    .buzzerOnlevel = 1000
};

lv_obj_t *buzzerSlider;
lv_obj_t *SliderLabel;

lv_obj_t *ThresholdIOvalpanel;
static lv_obj_t *flamelessdropdownIO;
static lv_obj_t *flamedropdownIO;

lv_obj_t *ThresholdADvalpanel;
static lv_obj_t *flameLevel1Drop;
static lv_obj_t *flameLevel1do_drop;
static lv_obj_t *flameLevel2Drop;
static lv_obj_t *flameLevel2do_drop;

lv_obj_t *ThresholdCancelBtn;
lv_obj_t *ThresholdSureBtn;

static lv_obj_t *WifiPasswordTextarea;
static lv_obj_t *WifiUsrnameTextarea;
static lv_obj_t *WifiIPLabel;

// WIFI
static lv_obj_t *WifiConnectBtnLabel = NULL;
static bool wifi_connected = false;

// MQTT
static lv_obj_t *MqttConnectBtnLabel = NULL;

static lv_obj_t *MqttServerTextarea;
static lv_obj_t *MqttPortTextarea;
static lv_obj_t *MqttUsrnameTextarea;
static lv_obj_t *MqttPasswordTextarea;
static lv_obj_t *MqttDeviceNameTextarea;
lv_obj_t *MqttConnectState;
lv_obj_t *MqttConnectBtn;

lv_obj_t *Msgbox;

enum ModeType CurrentMode;
char ThresholdCommit;

char  MQTT_SUB_TOPIC[128];
char  MQTT_PUB_TOPIC[128];
char  MQTT_ENABLE_TLS[2] = {0};

extern char recv_at[256];
extern uint16_t buzzerCurrentstatus;


static int IsNum(char *str)
{
    while(*str)
    {
        if (!isdigit(*str))
            return false;

        str++;
    }
    return true;
}

static int IsIp(char *IP)
{
    int i, num, dots = 0;
    char *ptr;

    if (NULL == IP)
        return false;

    ptr = strtok(IP, ".");
    if (NULL == ptr)
        return false;

    while (ptr)
    {
        if (!IsNum(ptr))
            return false;

        num = atoi(ptr);
        if (num >= 0 && num <= 255) {
            ptr = strtok(NULL, ".");
            if (NULL != ptr)
            {
                dots++;
            }
        } else {
            return false;
        }
    }

    if (3 == dots)
        return true;
    else
        return false;
}

static void MsgboxCb(lv_event_t *e)
{
	 lv_obj_t *target = lv_event_get_current_target(e);

	 if(strcmp(lv_msgbox_get_active_btn_text(target), "Close") == 0)
	 {
		 lv_obj_add_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
		 popup_dismissed = 1;
	 }
}

/**
  * @brief  Textarea event handlers.
  * @param  event.
  * @retval None
  */
static void Textarea_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    char *Userdata = (char*)lv_event_get_user_data(e);

    if (LV_EVENT_FOCUSED == code) {
    	if (NULL != Userdata) {
			lv_keyboard_set_mode(KeyBoardObj, LV_KEYBOARD_MODE_NUMBER);
		}
		if (NULL == Userdata) {
			lv_keyboard_set_mode(KeyBoardObj, LV_KEYBOARD_MODE_TEXT_LOWER);
		}
        if(lv_indev_get_type(lv_indev_get_act()) != LV_INDEV_TYPE_KEYPAD) {
            lv_keyboard_set_textarea(KeyBoardObj, target);
            lv_obj_set_style_max_height(KeyBoardObj, LV_HOR_RES * 1 / 3, 0);
            lv_obj_clear_flag(KeyBoardObj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_scroll_to_view_recursive(target, LV_ANIM_OFF);
        }
        return;
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(KeyBoardObj, NULL);
        lv_obj_add_flag(KeyBoardObj, LV_OBJ_FLAG_HIDDEN);
    }
    else if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(KeyBoardObj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(target, LV_STATE_FOCUSED);
        lv_indev_reset(NULL, target);   /*To forget the last clicked object to make it focusable again*/
    }
}

/**
  * @brief  Switch between user mode and intelligent mode.
  * @param  event.
  * @retval None
  */
static void ModeCheckboxCb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if (code != LV_EVENT_CLICKED) return;

    if (target == UsrCtl)
    {
        // 切换到用户控制模式
        lv_obj_add_state(UsrCtl, LV_STATE_CHECKED);
        lv_obj_clear_state(SmartCtl, LV_STATE_CHECKED);

        lv_obj_add_flag(SmartCtl, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(UsrCtl, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_state(UsrCtl, LV_STATE_CHECKED);  // 默认选中用户控制模式

        CurrentMode = USR;
        lv_obj_clear_state(buzzerSlider, LV_STATE_DISABLED);
        lv_obj_add_state(ThresholdSureBtn, LV_STATE_DISABLED);
        lv_obj_add_state(ThresholdCancelBtn, LV_STATE_DISABLED);
        if (mqttconnetState == CONNECTED) {
            onenet_data[5].value.bool_val = 0;
            onenet_mqtt_publish_post_data(onenet_data, ONENET_DATA_NUM);
        }
    }
    else if (target == SmartCtl)
    {
        // 切换到智能控制模式
        lv_obj_add_state(SmartCtl, LV_STATE_CHECKED);
        lv_obj_clear_state(UsrCtl, LV_STATE_CHECKED);

        lv_obj_add_flag(UsrCtl, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(SmartCtl, LV_OBJ_FLAG_CLICKABLE);

        CurrentMode = SMART;
        lv_obj_add_state(buzzerSlider, LV_STATE_DISABLED);
        lv_obj_clear_state(ThresholdSureBtn, LV_STATE_DISABLED);
        lv_obj_clear_state(ThresholdCancelBtn, LV_STATE_DISABLED);
        if (mqttconnetState == CONNECTED) {
            onenet_data[5].value.bool_val = 1;
            onenet_mqtt_publish_post_data(onenet_data, ONENET_DATA_NUM);
        }
    }

    // 无论哪种模式，PWM 都复位到1Hz，报警器关闭
    HAL_TIM_PWM_Stop(&htim, TIMCHANNEL);
    htim.Instance->ARR = FRE_DIV;
    __HAL_TIM_SetCompare(&htim, TIMCHANNEL, 0);
    HAL_TIM_PWM_Start(&htim, TIMCHANNEL);
    lv_slider_set_value(buzzerSlider, 1, LV_ANIM_OFF);
    lv_label_set_text(SliderLabel, "1Hz");
    lv_label_set_text(buzzerstateLabel, "报警器状态:关");
    buzzerCurrentstatus = 1;
    if (mqttconnetState == CONNECTED) {
        onenet_data[6].value.u32_val = buzzerCurrentstatus;
        onenet_mqtt_publish_post_data(onenet_data, ONENET_DATA_NUM);
    }
    if (lv_obj_has_state(ADType, LV_STATE_CHECKED)) {
        lv_obj_clear_flag(ADType, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(IOType, LV_OBJ_FLAG_CLICKABLE);
    } else {
        lv_obj_clear_flag(IOType, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(ADType, LV_OBJ_FLAG_CLICKABLE);
    }

    ThresholdCommit = 0;
}


/**
  * @brief  IO or AD type switch event handlers.
  * @param  event.
  * @retval None
  */
static void TypeCheckboxCb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if (code != LV_EVENT_CLICKED) return;

    if (target == ADType)
    {
        // 切换到 AD 触发
        lv_obj_add_state(ADType, LV_STATE_CHECKED);
        lv_obj_clear_state(IOType, LV_STATE_CHECKED);

        lv_obj_add_flag(IOType, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(ADType, LV_OBJ_FLAG_CLICKABLE);

        Getthreshold.AD_OR_IO = AD;
        lv_obj_add_flag(ThresholdIOvalpanel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ThresholdADvalpanel, LV_OBJ_FLAG_HIDDEN);
        if (mqttconnetState == CONNECTED) {
            strcpy(onenet_data[7].value.string_val, "AD");
            onenet_mqtt_publish_post_data(onenet_data, ONENET_DATA_NUM);
        }
    }
    else if (target == IOType)
    {
        // 切换到 IO 触发
        lv_obj_add_state(IOType, LV_STATE_CHECKED);
        lv_obj_clear_state(ADType, LV_STATE_CHECKED);

        if (lv_obj_has_state(ADType, LV_STATE_CHECKED)) {
            lv_obj_clear_flag(ADType, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(IOType, LV_OBJ_FLAG_CLICKABLE);
        } else {
            lv_obj_clear_flag(IOType, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(ADType, LV_OBJ_FLAG_CLICKABLE);
        }

        Getthreshold.AD_OR_IO = IO;
        lv_obj_add_flag(ThresholdADvalpanel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ThresholdIOvalpanel, LV_OBJ_FLAG_HIDDEN);
        if (mqttconnetState == CONNECTED) {
            strcpy(onenet_data[7].value.string_val, "IO");
            onenet_mqtt_publish_post_data(onenet_data, ONENET_DATA_NUM);
        }
    }
}

/**
  * @brief  slider event handlers.
  * @param  event.
  * @retval None
  */
static void slider_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
	lv_obj_t *usr = (lv_obj_t*)lv_event_get_user_data(e);

	if (code != LV_EVENT_VALUE_CHANGED) return;

	int frequency = (int)lv_slider_get_value(target);
	if (frequency < 1) frequency = 1;

	HAL_TIM_PWM_Stop(&htim, TIMCHANNEL);
	htim.Instance->ARR = FRE_DIV/frequency;
	lv_label_set_text_fmt(usr, "%dHz", frequency);

	if (frequency > 20) {
		__HAL_TIM_SetCompare(&htim, TIMCHANNEL, (FRE_DIV / frequency / 2));
		lv_label_set_text(buzzerstateLabel, "报警器状态:开");
	} else {
		__HAL_TIM_SetCompare(&htim, TIMCHANNEL, 0);
		lv_label_set_text(buzzerstateLabel, "报警器状态:关");
	}

	HAL_TIM_PWM_Start(&htim, TIMCHANNEL);
	buzzerCurrentstatus = frequency;

}

/**
  * @brief  Threshold btn event handlers.
  * @param  event.
  * @retval None
  */
static void ThresholdBtnCb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

	if (code != LV_EVENT_CLICKED) return;

	if (target == ThresholdSureBtn) {
		/* sure btn */
		ThresholdCommit = 1;
		lv_obj_add_state(ThresholdSureBtn, LV_STATE_DISABLED);
		lv_obj_clear_flag(ADType, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(IOType, LV_OBJ_FLAG_CLICKABLE); 
		lv_obj_clear_flag(flameLevel1Drop, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(flameLevel2Drop, LV_OBJ_FLAG_CLICKABLE);
		if (Getthreshold.AD_OR_IO == AD) {
			/* AD */
			char buff1[30];
			char buff2[30];
			lv_dropdown_get_selected_str(flameLevel1Drop, buff1, sizeof(buff1));
			lv_dropdown_get_selected_str(flameLevel2Drop, buff2, sizeof(buff2));
			Getthreshold.buzzerOnlevel = atoi(buff1);
			Getthreshold.buzzerofflevel = atoi(buff2);
		}
	}
	else if (target == ThresholdCancelBtn)
	{
		/* cancel btn */
		ThresholdCommit = 0;

		HAL_TIM_PWM_Stop(&htim, TIMCHANNEL);
		htim.Instance->ARR = FRE_DIV;
		__HAL_TIM_SetCompare(&htim, TIMCHANNEL, 0);
		HAL_TIM_PWM_Start(&htim, TIMCHANNEL);
		lv_slider_set_value(buzzerSlider, 1, LV_ANIM_OFF);
		lv_label_set_text(SliderLabel, "1Hz");
		lv_label_set_text(buzzerstateLabel, "报警器状态:关");
		buzzerCurrentstatus = 1;
		if (mqttconnetState == CONNECTED) {
			onenet_data[6].value.u32_val = buzzerCurrentstatus;
			onenet_mqtt_publish_post_data(onenet_data, ONENET_DATA_NUM);
		}
		lv_obj_add_flag(ADType, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(IOType, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_state(ThresholdSureBtn, LV_STATE_DISABLED);
		lv_obj_add_flag(flameLevel1Drop, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(flameLevel2Drop, LV_OBJ_FLAG_CLICKABLE);
	}
}

static void Threshold_Panel_Create_Dropdown(lv_obj_t *parent, lv_obj_t **label, lv_coord_t x_ofs_lab, lv_coord_t y_ofs_lab, const char *labeltext, 
					lv_obj_t **dropdown, const char *dropdownopts, lv_coord_t x_ofs_drop, lv_coord_t y_ofs_drop)
{
	if (NULL == parent) return;

	if ((*label != NULL) || (*dropdown != NULL)) return;

	*label = lv_label_create(parent);
	lv_label_set_text_fmt(*label, "%s", labeltext);
	lv_obj_align(*label, LV_ALIGN_TOP_MID , x_ofs_lab, y_ofs_lab);

	*dropdown = lv_dropdown_create(parent);
	lv_dropdown_set_options_static(*dropdown, dropdownopts);
	lv_obj_set_style_border_width(*dropdown, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(*dropdown, lv_color_make(145, 151, 254), LV_PART_MAIN);
	lv_obj_set_style_bg_color(*dropdown, lv_color_make(228, 236, 243), LV_PART_MAIN);
	lv_obj_align_to(*dropdown, *label, LV_ALIGN_OUT_RIGHT_MID, x_ofs_drop, y_ofs_drop);

	lv_obj_clear_flag(*dropdown, LV_OBJ_FLAG_CLICKABLE);
}

static void AD_Threshold_Panel_Create_Dropdown(lv_obj_t *parent, lv_obj_t *Firstlabel, const char *text, 
			lv_obj_t **First_dropdown, const char *Firstdropdownopts, lv_obj_t **do_dropdown, const char *do_dropdownopts)
{
	if ((NULL == Firstlabel) || (parent == NULL)) return;

	if ((*First_dropdown != NULL) || (*do_dropdown != NULL)) return;

	lv_label_set_text_fmt(Firstlabel, "%s", text);

	*First_dropdown = lv_dropdown_create(parent);
	lv_obj_set_width(*First_dropdown, 110);
	lv_dropdown_set_options_static(*First_dropdown, Firstdropdownopts);
	lv_obj_set_style_border_width(*First_dropdown, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(*First_dropdown, lv_color_make(145, 151, 254), LV_PART_MAIN);
	lv_obj_set_style_bg_color(*First_dropdown, lv_color_make(228, 236, 243), LV_PART_MAIN);
	lv_obj_align_to(*First_dropdown, Firstlabel, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

	lv_obj_t *dropsymbol1 = lv_label_create(*First_dropdown);
	lv_obj_set_style_text_font(dropsymbol1, &lv_font_montserrat_14, LV_PART_MAIN);
	lv_label_set_text(dropsymbol1, LV_SYMBOL_DOWN);
	lv_obj_align(dropsymbol1, LV_ALIGN_RIGHT_MID, 0, 0);

	lv_obj_t *dolabel = lv_label_create(parent);
	lv_label_set_text(dolabel, "执行");
	lv_obj_align_to(dolabel, *First_dropdown, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

	*do_dropdown = lv_dropdown_create(parent);
	lv_obj_set_width(*do_dropdown, 110);
	lv_dropdown_set_options_static(*do_dropdown, do_dropdownopts);
	lv_obj_set_style_border_width(*do_dropdown, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(*do_dropdown, lv_color_make(145, 151, 254), LV_PART_MAIN);
	lv_obj_set_style_bg_color(*do_dropdown, lv_color_make(228, 236, 243), LV_PART_MAIN);
	lv_obj_clear_flag(*do_dropdown, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_align_to(*do_dropdown, dolabel, LV_ALIGN_OUT_RIGHT_MID, 3, 0);
}

/**
  * @brief  dropdown event handlers.
  * @param  event.
  * @retval None
  */
static void DropdownCb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

	if (code != LV_EVENT_CLICKED)	return;

	lv_obj_t *list = lv_dropdown_get_list(target);
	if (NULL == list) {
		return;
	}
	lv_obj_set_style_text_font(list, &syht_font_16, LV_PART_MAIN);
}

/**
  * @brief  WIFI connects event handlers.
  * @param  event.
  * @retval None
  */
static void WifiConnectBtnCb(lv_event_t * e)
{
    int ret = 0;
    char send_at[128] = {0};
    char *IP;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *user = lv_event_get_user_data(e);

    if (LV_EVENT_CLICKED != code) return;

    if (!wifi_connected) {
        // ============= 连接操作 =============
        if (Esp8266CheckState()) {
            lv_obj_t *text = lv_msgbox_get_text(Msgbox);
            lv_label_set_text(text, "Esp8266 State error");
            lv_obj_clear_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        HAL_Delay(200);

        const char *WifiNameBuffer = lv_textarea_get_text(WifiUsrnameTextarea);
        const char *WifiPassBuffer = lv_textarea_get_text(WifiPasswordTextarea);

        if (0 == strlen(WifiNameBuffer) || strlen(WifiPassBuffer) < 8) {
            lv_obj_t *text = lv_msgbox_get_text(Msgbox);
            lv_label_set_text(text, "The account or password is incorrect");
            lv_obj_clear_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        strcpy(send_at, "AT+CWMODE=1\r\n");
        ret = send_command(send_at, strlen(send_at), "OK", NULL, 2000);
        if (ret != 0) {
            lv_obj_t *text = lv_msgbox_get_text(Msgbox);
            lv_label_set_text(text, "Set WiFi mode failed");
            lv_obj_clear_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        HAL_Delay(200);
        strcpy(send_at, "AT+CWQAP\r\n");
        send_command(send_at, strlen(send_at), "OK", NULL, 2000);
        HAL_Delay(500);

        snprintf(send_at, sizeof(send_at), "AT+CWJAP=\"%s\",\"%s\"\r\n", WifiNameBuffer, WifiPassBuffer);
        ret = send_command(send_at, strlen(send_at), "WIFI CONNECTED", NULL, 15000);
        if (ret != 0) {
            ret = send_command("", 0, "WIFI CONNECTED", NULL, 5000);
        }
        if (0 == ret) {
            // 连接成功
            wifi_connected = true;
            lv_label_set_text(WifiConnectBtnLabel, "断开");          // 按钮文本变为"断开"
            lv_label_set_text(user, "已连接");
            lv_obj_align_to(user, target, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
            lv_obj_clear_flag(WifiLogo, LV_OBJ_FLAG_HIDDEN);       // 显示 WiFi 图标

            // 获取 IP（最多重试3次）
            int retry = 3;
            while (retry > 0) {
                HAL_Delay(500);
                strcpy(send_at, "AT+CIFSR\r\n");
                ret = send_command(send_at, strlen(send_at), "OK", NULL, 5000);
                if (0 == ret) {
                    IP = analysis_cmd(recv_at, "\"", 1);
                    if (IP != NULL) {
                        printf("IP = %s\n\r", IP);
                        lv_label_set_text_fmt(WifiIPLabel, "IP : %s", IP);
                    }
                    lv_obj_align_to(WifiIPLabel, user, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
                    break;
                }
                retry--;
                printf("Retry get IP...\n");
            }
            return;
        }

        // 连接失败 — 不移除 WiFi 状态，只提示错误，用户可重试
        lv_obj_t *text = lv_msgbox_get_text(Msgbox);
        lv_label_set_text(text, "WIFI Connect timeout\n\n请检查 WiFi 名称/密码是否正确\n不要使用 5GHz（ESP8266 仅支持 2.4GHz）");
        lv_obj_clear_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
    } else {
        // ============= 断开操作 =============
        strcpy(send_at, "AT+CWQAP\r\n");   // 断开当前 WiFi 连接
        send_command(send_at, strlen(send_at), "OK", NULL, 2000);

        wifi_connected = false;
        lv_label_set_text(WifiConnectBtnLabel, "连接");            // 按钮恢复为"连接"
        lv_label_set_text(user, "未连接");
        lv_obj_align_to(user, target, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
        lv_obj_add_flag(WifiLogo, LV_OBJ_FLAG_HIDDEN);            // 隐藏 WiFi 图标
        lv_label_set_text(WifiIPLabel, "0.0.0.0");
        lv_obj_align_to(WifiIPLabel, user, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    }
}

/**
  * @brief  MQTT connects event handlers.
  * @param  event.
  * @retval None
  */
static void MqttConnectBtnCb(lv_event_t * e)
{
    int ret = 0;
    char send_at[512] = {0};
    lv_obj_t *text;
    char *IP;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *user = lv_event_get_user_data(e);

    if (LV_EVENT_CLICKED != code) return;

    if (mqttconnetState != CONNECTED) {
        if (Esp8266CheckState()) {
            text = lv_msgbox_get_text(Msgbox);
            lv_label_set_text(text, "ESP8266 State error");
            lv_obj_clear_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        HAL_Delay(200);

        const char *MqttServerIPBuffer = lv_textarea_get_text(MqttServerTextarea);
        const char *MqttPortBuffer = lv_textarea_get_text(MqttPortTextarea);
        const char *MqttUsrnameBuffer = lv_textarea_get_text(MqttUsrnameTextarea);
        const char *MqttPasswordBuffer = lv_textarea_get_text(MqttPasswordTextarea);
        const char *MqttDeviceNameBuffer = lv_textarea_get_text(MqttDeviceNameTextarea);

        strncpy(mqtt_info.host, MqttServerIPBuffer, sizeof(mqtt_info.host) - 1);
        mqtt_info.port = atoi(MqttPortBuffer);
        strncpy(mqtt_info.client_id, MqttDeviceNameBuffer, sizeof(mqtt_info.client_id) - 1);
        strncpy(mqtt_info.username, MqttUsrnameBuffer, sizeof(mqtt_info.username) - 1);
        strncpy(mqtt_info.password, MqttPasswordBuffer, sizeof(mqtt_info.password) - 1);

        snprintf((char *)mqtt_info.subscribe_topic_set, sizeof(mqtt_info.subscribe_topic_set), 
                 "$sys/%s/%s/thing/property/set", MqttUsrnameBuffer, MqttDeviceNameBuffer);
        snprintf((char *)mqtt_info.publish_topic_post, sizeof(mqtt_info.publish_topic_post), 
                 "$sys/%s/%s/thing/property/post", MqttUsrnameBuffer, MqttDeviceNameBuffer);
        snprintf((char *)mqtt_info.publish_topic_set, sizeof(mqtt_info.publish_topic_set), 
                 "$sys/%s/%s/thing/property/set_reply", MqttUsrnameBuffer, MqttDeviceNameBuffer);
        snprintf((char *)mqtt_info.subscribe_topic_post, sizeof(mqtt_info.subscribe_topic_post), 
                 "$sys/%s/%s/thing/property/post/reply", MqttUsrnameBuffer, MqttDeviceNameBuffer);

        HAL_Delay(200);
        strcpy(send_at, "AT+CIFSR\r\n");
        printf("Check WiFi: %s", send_at);
        ret = send_command(send_at, strlen(send_at), "+CIFSR:STAIP", NULL, 3000);
        if (ret) {
            printf("WiFi not connected!\n");
            text = lv_msgbox_get_text(Msgbox);
            lv_label_set_text(text, "WiFi not connected");
            lv_obj_clear_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
            return;
        }
        
        IP = analysis_cmd(recv_at, "\"", 1);
        if (IP == NULL || strcmp(IP, "0.0.0.0") == 0) {
            printf("WiFi not connected (IP is 0.0.0.0)!\n");
            text = lv_msgbox_get_text(Msgbox);
            lv_label_set_text(text, "WiFi not connected");
            lv_obj_clear_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
            return;
        }
        printf("WiFi is connected, IP: %s\n", IP);

        ret = esp8266_mqtt_connect((char *)mqtt_info.client_id, 
                                   (char *)mqtt_info.username, 
                                   (char *)mqtt_info.password, 
                                   (char *)mqtt_info.host, 
                                   mqtt_info.port);
        if (ret != 0) goto MQTTERROR;

        ret = esp8266_mqtt_subscribe((char *)mqtt_info.subscribe_topic_set, 0);
        if (ret != 0) goto MQTTERROR;

        ret = esp8266_mqtt_subscribe((char *)mqtt_info.subscribe_topic_post, 0);
        if (ret != 0) goto MQTTERROR;

        printf("=== MQTT Connect Success (OneNET) ===\n");
        mqttconnetState = CONNECTED;
        lv_label_set_text(MqttConnectBtnLabel, "断开");
        lv_label_set_text(user, "已连接");
        lv_obj_align_to(user, target, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
        return;

MQTTERROR:
        text = lv_msgbox_get_text(Msgbox);
        lv_label_set_text(text, "MQTT Connect timeout");
        lv_obj_clear_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
        strcpy(send_at, "AT+RST\r\n");
        send_command(send_at, strlen(send_at), "OK", NULL, 500);
    } else {
        sprintf(send_at, "AT+MQTTDISCONN=0\r\n");
        send_command(send_at, strlen(send_at), "OK", NULL, 2000);

        mqttconnetState = DISCONNECTED;
        lv_label_set_text(MqttConnectBtnLabel, "连接");
        lv_label_set_text(user, "未连接");
        lv_obj_align_to(user, target, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
    }
}

/**
  * @brief  the UI of Data display.
  * @param  Data display obj.
  * @retval None
  */
static void DataObjInit(lv_obj_t *parent)
{
	static lv_style_t StatePanelStyle;
	lv_style_init(&StatePanelStyle);
	lv_style_set_height(&StatePanelStyle, 50);
	lv_style_set_width(&StatePanelStyle, 155);
	lv_style_set_pad_all(&StatePanelStyle, 0);
	lv_style_set_bg_color(&StatePanelStyle, lv_color_make(109, 117, 254));
	lv_style_set_text_font(&StatePanelStyle, &syht_font_16);
	lv_style_set_text_color(&StatePanelStyle, lv_color_hex(0xFFFFFF));

	lv_obj_t *buzzerimg = lv_img_create(parent);
	lv_img_set_src(buzzerimg, &buzzer);
	lv_obj_align(buzzerimg, LV_ALIGN_TOP_LEFT, 28, 50);

	lv_obj_t *buzzerstatePanel = lv_obj_create(parent);
	lv_obj_add_style(buzzerstatePanel, &StatePanelStyle, LV_PART_MAIN);
	lv_obj_align_to(buzzerstatePanel, buzzerimg, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

	buzzerstateLabel = lv_label_create(buzzerstatePanel);
	lv_label_set_text(buzzerstateLabel, "报警器状态:关");
	lv_obj_center(buzzerstateLabel);

	lv_obj_t *Flamesensorimg = lv_img_create(parent);
	lv_img_set_src(Flamesensorimg, &fire);
	lv_obj_align(Flamesensorimg, LV_ALIGN_BOTTOM_LEFT, 28, -50);

	lv_obj_t *Flamestate = lv_obj_create(parent);
	lv_obj_add_style(Flamestate, &StatePanelStyle, LV_PART_MAIN);
	lv_obj_align_to(Flamestate, Flamesensorimg, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

	FlamestateLabel = lv_label_create(Flamestate);
	lv_label_set_text(FlamestateLabel, "火焰传感器:");
	lv_obj_center(FlamestateLabel);

}

/**
  * @brief  the UI of Local.
  * @param  Local tabview.
  * @retval None
  */
static void LocalObjInit(lv_obj_t *parent)
{
	lv_obj_set_style_text_font(parent, &syht_font_16, LV_PART_MAIN);
	lv_obj_set_style_text_color(parent, lv_color_make(32, 53, 116), LV_PART_MAIN);
	lv_obj_set_style_pad_all(parent, 0, LV_PART_MAIN);

	lv_obj_t *ModeLabel = lv_label_create(parent);
	lv_label_set_text(ModeLabel, "模式选择:");
	lv_obj_align(ModeLabel, LV_ALIGN_TOP_LEFT, 5, 10);

	UsrCtl = lv_checkbox_create(parent);
	lv_checkbox_set_text(UsrCtl, "用户控制模式");

	// 指示器设为圆形
	lv_obj_set_style_radius(UsrCtl, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
	// 未选中时：空心圆环
	lv_obj_set_style_bg_color(UsrCtl, lv_color_white(), LV_PART_INDICATOR);
	lv_obj_set_style_bg_opa(UsrCtl, LV_OPA_COVER, LV_PART_INDICATOR);
	lv_obj_set_style_border_width(UsrCtl, 2, LV_PART_INDICATOR);
	lv_obj_set_style_border_color(UsrCtl, lv_color_make(91, 98, 246), LV_PART_INDICATOR);
	lv_obj_set_style_pad_all(UsrCtl, 0, LV_PART_INDICATOR);

	// 选中时：内部填充实心圆，去掉默认勾图标
	lv_obj_set_style_bg_color(UsrCtl, lv_color_make(91, 98, 246), LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_set_style_bg_opa(UsrCtl, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_set_style_bg_img_src(UsrCtl, NULL, LV_PART_INDICATOR | LV_STATE_CHECKED);  // 隐藏勾

	// 默认不可点击（已选中）
	lv_obj_clear_flag(UsrCtl, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_state(UsrCtl, LV_STATE_CHECKED);
	lv_obj_align_to(UsrCtl, ModeLabel, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

	SmartCtl = lv_checkbox_create(parent);
	lv_checkbox_set_text(SmartCtl, "智能控制模式");

	lv_obj_set_style_radius(SmartCtl, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
	lv_obj_set_style_bg_color(SmartCtl, lv_color_white(), LV_PART_INDICATOR);
	lv_obj_set_style_bg_opa(SmartCtl, LV_OPA_COVER, LV_PART_INDICATOR);
	lv_obj_set_style_border_width(SmartCtl, 2, LV_PART_INDICATOR);
	lv_obj_set_style_border_color(SmartCtl, lv_color_make(91, 98, 246), LV_PART_INDICATOR);
	lv_obj_set_style_pad_all(SmartCtl, 0, LV_PART_INDICATOR);

	lv_obj_set_style_bg_color(SmartCtl, lv_color_make(91, 98, 246), LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_set_style_bg_opa(SmartCtl, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_set_style_bg_img_src(SmartCtl, NULL, LV_PART_INDICATOR | LV_STATE_CHECKED);

	// SmartCtl 默认可点击（未选中）
	lv_obj_align_to(SmartCtl, UsrCtl, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

	lv_obj_t *TypeLabel = lv_label_create(parent);
	lv_label_set_text(TypeLabel, "触发类型:");
	lv_obj_align_to(TypeLabel, ModeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

	ADType = lv_checkbox_create(parent);
	lv_checkbox_set_text(ADType, "AD触发");

	// 指示器圆形样式
	lv_obj_set_style_radius(ADType, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
	lv_obj_set_style_bg_color(ADType, lv_color_white(), LV_PART_INDICATOR);
	lv_obj_set_style_bg_opa(ADType, LV_OPA_COVER, LV_PART_INDICATOR);
	lv_obj_set_style_border_width(ADType, 2, LV_PART_INDICATOR);
	lv_obj_set_style_border_color(ADType, lv_color_make(91, 98, 246), LV_PART_INDICATOR);
	lv_obj_set_style_pad_all(ADType, 0, LV_PART_INDICATOR);
	// 选中状态
	lv_obj_set_style_bg_color(ADType, lv_color_make(91, 98, 246), LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_set_style_bg_opa(ADType, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_set_style_bg_img_src(ADType, NULL, LV_PART_INDICATOR | LV_STATE_CHECKED);

	// 默认选中 AD，设为不可点击
	lv_obj_add_state(ADType, LV_STATE_CHECKED);
	lv_obj_clear_flag(ADType, LV_OBJ_FLAG_CLICKABLE);

	lv_obj_align_to(ADType, TypeLabel, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
	Getthreshold.AD_OR_IO = AD;

	IOType = lv_checkbox_create(parent);
	lv_checkbox_set_text(IOType, "IO触发");

	lv_obj_set_style_radius(IOType, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
	lv_obj_set_style_bg_color(IOType, lv_color_white(), LV_PART_INDICATOR);
	lv_obj_set_style_bg_opa(IOType, LV_OPA_COVER, LV_PART_INDICATOR);
	lv_obj_set_style_border_width(IOType, 2, LV_PART_INDICATOR);
	lv_obj_set_style_border_color(IOType, lv_color_make(91, 98, 246), LV_PART_INDICATOR);
	lv_obj_set_style_pad_all(IOType, 0, LV_PART_INDICATOR);
	lv_obj_set_style_bg_color(IOType, lv_color_make(91, 98, 246), LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_set_style_bg_opa(IOType, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_set_style_bg_img_src(IOType, NULL, LV_PART_INDICATOR | LV_STATE_CHECKED);

	// 默认未选中，可点击
	lv_obj_align_to(IOType, ADType, LV_ALIGN_OUT_RIGHT_MID, 65, 0);

	lv_obj_t *buzzerLabel = lv_label_create(parent);
	lv_label_set_text(buzzerLabel, "报警器控制");
	lv_obj_align(buzzerLabel, LV_ALIGN_TOP_LEFT, 5, 90);

	static const lv_style_prop_t props[] = {LV_STYLE_BG_COLOR, 0};
    static lv_style_transition_dsc_t transition_dsc;
    lv_style_transition_dsc_init(&transition_dsc, props, lv_anim_path_linear, 300, 0, NULL);

    static lv_style_t style_main;
    static lv_style_t style_indicator;
    static lv_style_t style_knob;
    static lv_style_t style_pressed_color;
    lv_style_init(&style_main);
    lv_style_set_bg_opa(&style_main, LV_OPA_COVER);
    lv_style_set_bg_color(&style_main, lv_color_hex3(0xbbb));
    lv_style_set_radius(&style_main, LV_RADIUS_CIRCLE);
    lv_style_set_pad_ver(&style_main, -2); /*Makes the indicator larger*/

    lv_style_init(&style_indicator);
    lv_style_set_bg_opa(&style_indicator, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indicator, lv_color_make(116, 121, 246));
    lv_style_set_radius(&style_indicator, LV_RADIUS_CIRCLE);
    lv_style_set_transition(&style_indicator, &transition_dsc);

    lv_style_init(&style_knob);
    lv_style_set_bg_opa(&style_knob, LV_OPA_COVER);
    lv_style_set_bg_color(&style_knob, lv_color_make(145, 141, 244));
    lv_style_set_border_color(&style_knob, lv_color_make(213, 215, 252));
    lv_style_set_border_width(&style_knob, 2);
    lv_style_set_radius(&style_knob, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_knob, 6); /*Makes the knob larger*/
    lv_style_set_transition(&style_knob, &transition_dsc);

    lv_style_init(&style_pressed_color);
    lv_style_set_bg_color(&style_pressed_color, lv_color_make(249, 248, 254));

	buzzerSlider = lv_slider_create(parent);
	lv_obj_remove_style_all(buzzerSlider);        /*Remove the styles coming from the theme*/
	lv_obj_add_style(buzzerSlider, &style_main, LV_PART_MAIN);
    lv_obj_add_style(buzzerSlider, &style_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(buzzerSlider, &style_pressed_color, LV_PART_INDICATOR | LV_STATE_PRESSED);
    lv_obj_add_style(buzzerSlider, &style_knob, LV_PART_KNOB);
    lv_obj_add_style(buzzerSlider, &style_pressed_color, LV_PART_KNOB | LV_STATE_PRESSED);
	lv_obj_align_to(buzzerSlider, buzzerLabel, LV_ALIGN_OUT_RIGHT_MID, 30, 0);
	lv_slider_set_range(buzzerSlider, 1, 10000);

	SliderLabel = lv_label_create(parent);
	lv_label_set_text(SliderLabel, "0Hz");
	lv_obj_align_to(SliderLabel, buzzerSlider, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

	lv_obj_add_event_cb(buzzerSlider, slider_event_cb, LV_EVENT_VALUE_CHANGED, SliderLabel);

	lv_obj_t *ThresholdPanel = lv_obj_create(parent);
	lv_obj_set_size(ThresholdPanel, 400, 190);
	lv_obj_set_style_bg_color(ThresholdPanel, lv_color_make(250, 248, 255), LV_PART_MAIN);
	lv_obj_set_style_pad_all(ThresholdPanel, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(ThresholdPanel, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(ThresholdPanel, lv_color_make(145, 151, 254), LV_PART_MAIN);
	lv_obj_align(ThresholdPanel, LV_ALIGN_CENTER, 0, 50);
	
	lv_obj_t *ThresholdPanelLabel = lv_label_create(ThresholdPanel);
	lv_label_set_text(ThresholdPanelLabel, "触发阈值");
	lv_obj_align(ThresholdPanelLabel, LV_ALIGN_TOP_MID, 0, 10);

	static lv_point_t line_points[] = { {5, 33}, {390, 33} };

	lv_obj_t *ThresholdPanelLine = lv_line_create(ThresholdPanel);
	lv_obj_set_style_line_width(ThresholdPanelLine, 2, LV_PART_MAIN);
	lv_obj_set_style_line_color(ThresholdPanelLine, lv_color_make(145, 151, 254), LV_PART_MAIN);
	lv_line_set_points(ThresholdPanelLine, line_points, 2);

	/* IO 触发面板 */
	ThresholdIOvalpanel = lv_obj_create(ThresholdPanel);
	lv_obj_set_size(ThresholdIOvalpanel, 380, 100);
	lv_obj_set_style_bg_color(ThresholdIOvalpanel, lv_color_make(250, 248, 255), LV_PART_MAIN);
	lv_obj_set_style_pad_all(ThresholdIOvalpanel, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(ThresholdIOvalpanel, 0, LV_PART_MAIN);
	lv_obj_add_flag(ThresholdIOvalpanel, LV_OBJ_FLAG_HIDDEN);
	lv_obj_align(ThresholdIOvalpanel, LV_ALIGN_TOP_MID, 0, 45);

	lv_obj_t *flamelesslabelIO = NULL;
	static const char *flamelessopts = "关闭报警器";
	Threshold_Panel_Create_Dropdown(ThresholdIOvalpanel, &flamelesslabelIO, -60, 15, "无火焰", 
			&flamelessdropdownIO, flamelessopts, 10, 0);
	
	
	lv_obj_t *flamelabelIO = NULL;
	static const char *lighteropts = "打开报警器";
	Threshold_Panel_Create_Dropdown(ThresholdIOvalpanel, &flamelabelIO, -60, 60, "有火焰", 
			&flamedropdownIO, lighteropts, 10, 0);

	/* AD 触发面板 */
	ThresholdADvalpanel = lv_obj_create(ThresholdPanel);
	lv_obj_set_size(ThresholdADvalpanel, 380, 100);
	lv_obj_set_style_bg_color(ThresholdADvalpanel, lv_color_make(250, 248, 255), LV_PART_MAIN);
	lv_obj_set_style_pad_all(ThresholdADvalpanel, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(ThresholdADvalpanel, 0, LV_PART_MAIN);
	lv_obj_add_state(ThresholdADvalpanel, LV_STATE_DISABLED);
	lv_obj_align(ThresholdADvalpanel, LV_ALIGN_TOP_MID, 0, 45);

	lv_obj_t *flameLevel_1 = lv_label_create(ThresholdADvalpanel);
	lv_obj_align(flameLevel_1, LV_ALIGN_TOP_LEFT,  2, 20);
	static const char *flameLevel1ops = "1000\n"
								  	    "1500\n"
										"2000";
	static const char *buzzeron = "打开报警器";
	AD_Threshold_Panel_Create_Dropdown(ThresholdADvalpanel, flameLevel_1, "火焰值大于", &flameLevel1Drop, flameLevel1ops, &flameLevel1do_drop, buzzeron);

	lv_obj_add_event_cb(flameLevel1Drop, DropdownCb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(flameLevel1do_drop, DropdownCb, LV_EVENT_CLICKED, NULL);

	lv_obj_t *flameLevel_2 = lv_label_create(ThresholdADvalpanel);
	lv_obj_align(flameLevel_2, LV_ALIGN_TOP_LEFT, 2, 65);
	static const char *flameLeve21ops = "300\n"
									  	"600\n"
									    "800";
	static const char *Lighton = "关闭报警器";
	AD_Threshold_Panel_Create_Dropdown(ThresholdADvalpanel, flameLevel_2, "火焰值小于", &flameLevel2Drop, flameLeve21ops, &flameLevel2do_drop, Lighton);

	lv_obj_add_event_cb(flameLevel2Drop, DropdownCb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(flameLevel2do_drop, DropdownCb, LV_EVENT_CLICKED, NULL);

	ThresholdSureBtn = lv_btn_create(ThresholdPanel);
	lv_obj_add_style(ThresholdSureBtn, &BtnStyle, LV_PART_MAIN);
	lv_obj_align(ThresholdSureBtn, LV_ALIGN_BOTTOM_LEFT, 100, -6);
	lv_obj_add_state(ThresholdSureBtn, LV_STATE_DISABLED);

	lv_obj_t *btnsurelabel = lv_label_create(ThresholdSureBtn);
	lv_label_set_text(btnsurelabel, "锁定");
	lv_obj_center(btnsurelabel);

	lv_obj_add_event_cb(ThresholdSureBtn, ThresholdBtnCb, LV_EVENT_CLICKED, NULL);

	ThresholdCancelBtn = lv_btn_create(ThresholdPanel);
	lv_obj_add_style(ThresholdCancelBtn, &BtnStyle, LV_PART_MAIN);
	lv_obj_align(ThresholdCancelBtn, LV_ALIGN_BOTTOM_RIGHT, -100, -6);
	lv_obj_add_state(ThresholdCancelBtn, LV_STATE_DISABLED);

	lv_obj_t *btncancellabel = lv_label_create(ThresholdCancelBtn);
	lv_label_set_text(btncancellabel, "取消");
	lv_obj_center(btncancellabel);

	lv_obj_add_event_cb(ThresholdCancelBtn, ThresholdBtnCb, LV_EVENT_CLICKED, NULL);

	ThresholdCommit = 0;

	lv_obj_add_event_cb(UsrCtl, ModeCheckboxCb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(SmartCtl, ModeCheckboxCb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(ADType, TypeCheckboxCb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(IOType, TypeCheckboxCb, LV_EVENT_CLICKED, NULL);

	CurrentMode = USR;
}

/**
  * @brief  the UI of WIFI.
  * @param  WIFI tabview.
  * @retval None
  */
static void WifiObjInit(lv_obj_t * parent)
{
	lv_obj_set_style_text_font(parent, &syht_font_16, LV_PART_MAIN);
	lv_obj_set_style_text_color(parent, lv_color_make(32, 53, 116), LV_PART_MAIN);
	lv_obj_set_style_pad_all(parent, 0, LV_PART_MAIN);

	lv_obj_t *WifiTvNameLabel = lv_label_create(parent);
	lv_label_set_text(WifiTvNameLabel,"WIFI连接");
	lv_obj_align(WifiTvNameLabel, LV_ALIGN_TOP_MID, 0, 10);

	WifiUsrnameTextarea = lv_textarea_create(parent);
	lv_textarea_set_one_line(WifiUsrnameTextarea, true);
	lv_obj_add_style(WifiUsrnameTextarea, &TextareaStyle, LV_PART_MAIN);
	lv_textarea_set_placeholder_text(WifiUsrnameTextarea, "请输入");
	lv_textarea_set_text(WifiUsrnameTextarea, "wwto");
	lv_obj_align_to(WifiUsrnameTextarea, WifiTvNameLabel, LV_ALIGN_OUT_BOTTOM_MID, 20, 30);

	lv_obj_t *UsrnameLabel = lv_label_create(parent);
	lv_label_set_text(UsrnameLabel, "名称");
	lv_obj_align_to(UsrnameLabel, WifiUsrnameTextarea, LV_ALIGN_OUT_LEFT_MID, -10, 0);

	lv_obj_add_event_cb(WifiUsrnameTextarea, Textarea_cb, LV_EVENT_ALL, NULL);

	WifiPasswordTextarea = lv_textarea_create(parent);
	lv_textarea_set_one_line(WifiPasswordTextarea, true);
	lv_obj_add_style(WifiPasswordTextarea, &TextareaStyle, LV_PART_MAIN);
	lv_textarea_set_placeholder_text(WifiPasswordTextarea, "请输入");
	lv_textarea_set_text(WifiPasswordTextarea, "123456888");
	lv_obj_align_to(WifiPasswordTextarea, WifiUsrnameTextarea, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 20);

	lv_obj_t *passwordLabel = lv_label_create(parent);
	lv_label_set_text(passwordLabel, "密码");
	lv_obj_align_to(passwordLabel, WifiPasswordTextarea, LV_ALIGN_OUT_LEFT_MID, -10, 0);

	lv_obj_add_event_cb(WifiPasswordTextarea, Textarea_cb, LV_EVENT_ALL, NULL);

	lv_obj_t *WifiConnectBtn = lv_btn_create(parent);
	lv_obj_add_style(WifiConnectBtn, &BtnStyle, LV_PART_MAIN);
	lv_obj_align_to(WifiConnectBtn, WifiPasswordTextarea, LV_ALIGN_OUT_BOTTOM_MID, -20, 30);

	lv_obj_t *WifiConnectBtnName = lv_label_create(WifiConnectBtn);
	lv_obj_set_style_opa(WifiConnectBtnName, LV_OPA_COVER, LV_PART_MAIN);
	lv_label_set_text(WifiConnectBtnName, "连接");
	lv_obj_center(WifiConnectBtnName);

	WifiConnectBtnLabel = WifiConnectBtnName;   // 保存标签对象

	lv_obj_t *WifiStateLabel = lv_label_create(parent);
	lv_obj_set_style_text_color(WifiStateLabel, lv_color_make(81, 85, 160), LV_PART_MAIN);
	lv_label_set_text(WifiStateLabel, "未连接");
	lv_obj_align_to(WifiStateLabel, WifiConnectBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

	WifiIPLabel = lv_label_create(parent);
	lv_obj_set_style_text_color(WifiIPLabel, lv_color_make(81, 85, 160), LV_PART_MAIN);
	lv_label_set_text(WifiIPLabel, "0.0.0.0");
	lv_obj_align_to(WifiIPLabel, WifiStateLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

	lv_obj_add_event_cb(WifiConnectBtn, WifiConnectBtnCb, LV_EVENT_CLICKED, WifiStateLabel);
}

/**
  * @brief  the UI of MQTT.
  * @param  MQTT tabview.
  * @retval None
  */
static void MQTTObjInit(lv_obj_t * parent)
{
	lv_obj_set_style_text_font(parent, &syht_font_16, LV_PART_MAIN);
	lv_obj_set_style_text_color(parent, lv_color_make(32, 53, 116), LV_PART_MAIN);
	lv_obj_set_style_pad_all(parent, 0, LV_PART_MAIN);

    lv_obj_t *MqttTvNameLabel = lv_label_create(parent);
    lv_label_set_text(MqttTvNameLabel,"MQTT连接");
    lv_obj_align(MqttTvNameLabel, LV_ALIGN_TOP_MID, 0, 20);

    lv_style_set_width(&TextareaStyle, LV_PCT(60));

    MqttServerTextarea = lv_textarea_create(parent);
    lv_textarea_set_one_line(MqttServerTextarea, true);
    lv_textarea_set_placeholder_text(MqttServerTextarea, "IP");
    lv_obj_add_style(MqttServerTextarea, &TextareaStyle, LV_PART_MAIN);
    lv_textarea_set_text(MqttServerTextarea, "mqtts.heclouds.com");
    lv_obj_align(MqttServerTextarea, LV_ALIGN_TOP_RIGHT, -40, 45);

    lv_obj_t *MqttServerLabel = lv_label_create(parent);
    lv_label_set_text(MqttServerLabel, "服务器地址");
    lv_obj_align_to(MqttServerLabel, MqttServerTextarea, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    lv_obj_add_event_cb(MqttServerTextarea, Textarea_cb, LV_EVENT_ALL, NULL);

    MqttPortTextarea = lv_textarea_create(parent);
    lv_obj_add_style(MqttPortTextarea, &TextareaStyle, LV_PART_MAIN);
    lv_textarea_set_one_line(MqttPortTextarea, true);
    lv_textarea_set_text(MqttPortTextarea, "1883");
    lv_obj_align_to(MqttPortTextarea, MqttServerTextarea, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    lv_obj_t *MqttPortLabel = lv_label_create(parent);
    lv_label_set_text(MqttPortLabel, "端    口    号");
    lv_obj_align_to(MqttPortLabel, MqttPortTextarea, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    lv_obj_add_event_cb(MqttPortTextarea, Textarea_cb, LV_EVENT_ALL, (void *)1);

    MqttUsrnameTextarea = lv_textarea_create(parent);
    lv_obj_add_style(MqttUsrnameTextarea, &TextareaStyle, LV_PART_MAIN);
    lv_textarea_set_one_line(MqttUsrnameTextarea, true);
    lv_textarea_set_text(MqttUsrnameTextarea, "Id96NmW7s1");
    lv_obj_align_to(MqttUsrnameTextarea, MqttPortTextarea, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);

    lv_obj_t *MqttUsrnameLabel = lv_label_create(parent);
    lv_label_set_text(MqttUsrnameLabel, "用    户    名");
    lv_obj_align_to(MqttUsrnameLabel, MqttUsrnameTextarea, LV_ALIGN_OUT_LEFT_MID,-10, 0);

    lv_obj_add_event_cb(MqttUsrnameTextarea, Textarea_cb, LV_EVENT_ALL, NULL);

    MqttPasswordTextarea = lv_textarea_create(parent);
    lv_obj_add_style(MqttPasswordTextarea, &TextareaStyle, LV_PART_MAIN);
    lv_textarea_set_one_line(MqttPasswordTextarea, true);
    lv_textarea_set_text(MqttPasswordTextarea, "version=2018-10-31&res=products%2FId96NmW7s1%2Fdevices%2Ffire&et=1815462806&method=md5&sign=8C2NaYUcKSpwWZyhJWtBgg%3D%3D");
    lv_obj_align_to(MqttPasswordTextarea, MqttUsrnameTextarea, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);

    lv_obj_t *MqttpasswordLabel = lv_label_create(parent);
    lv_label_set_text(MqttpasswordLabel, "密            码");
    lv_obj_align_to(MqttpasswordLabel, MqttPasswordTextarea, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    lv_obj_add_event_cb(MqttPasswordTextarea, Textarea_cb, LV_EVENT_ALL, NULL);

    MqttDeviceNameTextarea = lv_textarea_create(parent);
    lv_obj_add_style(MqttDeviceNameTextarea, &TextareaStyle, LV_PART_MAIN);
    lv_textarea_set_one_line(MqttDeviceNameTextarea, true);
    lv_textarea_set_text(MqttDeviceNameTextarea, "fire");
    lv_obj_align_to(MqttDeviceNameTextarea, MqttPasswordTextarea, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);

    lv_obj_t *MqttDeviceNametLabel = lv_label_create(parent);
    lv_label_set_text(MqttDeviceNametLabel, "设 备 名 称");
    lv_obj_align_to(MqttDeviceNametLabel, MqttDeviceNameTextarea, LV_ALIGN_OUT_LEFT_MID, -15, 0);

    lv_obj_add_event_cb(MqttDeviceNameTextarea, Textarea_cb, LV_EVENT_ALL, NULL);

    MqttConnectBtn = lv_btn_create(parent);
	lv_obj_add_style(MqttConnectBtn, &BtnStyle, LV_PART_MAIN);
   	lv_obj_align_to(MqttConnectBtn, MqttDeviceNameTextarea, LV_ALIGN_OUT_BOTTOM_MID, -35, 8);

    lv_obj_t *MqttConnectBtnName = lv_label_create(MqttConnectBtn);
    lv_obj_set_style_opa(MqttConnectBtnName, LV_OPA_COVER, LV_PART_MAIN);
    lv_label_set_text(MqttConnectBtnName, "连接");
    lv_obj_center(MqttConnectBtnName);
    MqttConnectBtnLabel = MqttConnectBtnName;   // 保存标签对象

    MqttConnectState = lv_label_create(parent);
    lv_obj_set_style_text_color(MqttConnectState, lv_color_make(81, 85, 160), LV_PART_MAIN);
    lv_label_set_text(MqttConnectState, "未连接");
    lv_obj_align_to(MqttConnectState, MqttConnectBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

    lv_obj_add_event_cb(MqttConnectBtn, MqttConnectBtnCb, LV_EVENT_CLICKED, MqttConnectState);
}

void StyleInit(void)
{
	/* Textarea style init */
	lv_style_init(&TextareaStyle);
	lv_style_set_bg_color(&TextareaStyle, lv_color_make(228, 235, 242));
	lv_style_set_border_width(&TextareaStyle, 1);
	lv_style_set_border_color(&TextareaStyle, lv_color_make(161, 183, 214));
	lv_style_set_text_color(&TextareaStyle, lv_color_make(81, 85, 160));

	/* btn style Init */
	lv_style_init(&BtnStyle);
	lv_style_set_width(&BtnStyle, 80);
	lv_style_set_height(&BtnStyle, 35);
	lv_style_set_bg_color(&BtnStyle, lv_color_make(102, 110, 254));
	lv_style_set_radius(&BtnStyle, 6);
}

/**
  * @brief  UI Init.
  * @param  None.
  * @retval None
  */
void UI_Init()
{
	StyleInit();

	/* background color */
	lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(210, 208, 251), LV_PART_MAIN);

	/* Title */
	lv_obj_t *TitleObj = lv_obj_create(lv_scr_act());
	lv_obj_set_size(TitleObj, 800, 50);
	lv_obj_set_style_radius(TitleObj, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(TitleObj, 0, 0);
	lv_obj_set_style_bg_color(TitleObj, lv_color_make(89, 98, 255), LV_PART_MAIN);
	lv_obj_set_style_pad_all(TitleObj, 0, LV_PART_MAIN);
	lv_obj_align(TitleObj, LV_ALIGN_TOP_MID, 0, 0);

	/* Title img */
	lv_obj_t *TitleImg = lv_img_create(TitleObj);
	lv_img_set_src(TitleImg, &title);
	lv_obj_center(TitleImg);

	/* Wifi Connect Logo */
	WifiLogo = lv_label_create(TitleObj);
	lv_label_set_text(WifiLogo, LV_SYMBOL_WIFI);
	lv_obj_set_style_text_color(WifiLogo, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_add_flag(WifiLogo, LV_OBJ_FLAG_HIDDEN);
	lv_obj_align(WifiLogo,LV_ALIGN_RIGHT_MID, -20, 0);

	/* data display panel */
	lv_obj_t *DataObj = lv_obj_create(lv_scr_act());
	lv_obj_set_size(DataObj, 350, 410);
	lv_obj_set_style_pad_all(DataObj, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(DataObj, lv_color_make(244, 248, 255), LV_PART_MAIN);
	lv_obj_set_style_opa(DataObj, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_border_width(DataObj, 2, LV_PART_MAIN);
	lv_obj_set_style_border_color(DataObj, lv_color_make(161, 183, 214), LV_PART_MAIN);
	lv_obj_align(DataObj, LV_ALIGN_BOTTOM_LEFT, 10, -10);

	lv_obj_t *tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 42);
	lv_obj_set_size(tabview, 420, 410);
	lv_obj_set_style_anim_time(lv_tabview_get_content(tabview), 0, LV_PART_MAIN);
	lv_obj_set_style_radius(tabview, 10, LV_PART_MAIN);
	lv_obj_set_style_bg_color(tabview, lv_color_make(244, 248, 255), LV_PART_MAIN);
	lv_obj_set_style_border_width(tabview, 2, LV_PART_MAIN);
	lv_obj_set_style_border_color(tabview, lv_color_make(161, 183, 214), LV_PART_MAIN);
	lv_obj_align_to(tabview, DataObj, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

	lv_obj_t * tab_btns = lv_tabview_get_tab_btns(tabview);
	lv_obj_set_style_radius(tab_btns, 5, 0);
	lv_obj_set_style_border_color(tab_btns, lv_color_make(85, 94, 249), LV_PART_ITEMS | LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(tab_btns, lv_color_make(214, 216, 255), LV_PART_MAIN);
	lv_obj_set_style_bg_color(tab_btns, lv_color_make(147, 142, 252), LV_PART_ITEMS | LV_STATE_CHECKED);
	lv_obj_set_style_text_color(tab_btns, lv_color_make(48, 54, 171), LV_PART_MAIN);
	lv_obj_set_style_text_color(tab_btns, lv_color_hex(0xFFFFFF), LV_PART_ITEMS | LV_STATE_CHECKED);

	lv_obj_t *Local = lv_tabview_add_tab(tabview, "Local");
	lv_obj_t *WIFI = lv_tabview_add_tab(tabview, "WIFI");
	lv_obj_t *MQTT = lv_tabview_add_tab(tabview, "MQTT");

	KeyBoardObj = lv_keyboard_create(tabview);
	lv_obj_add_flag(KeyBoardObj, LV_OBJ_FLAG_HIDDEN);

	/* the msgbox */
	static const char * msgboxbtns[] ={" ", " ", "Close", ""};	static const char * msgboxbtns[] ={" ", " ", "关闭", ""};	static const char * msgboxbtns[] ={" ", " ", "关闭", ""};	static const char * msgboxbtns[] ={" ", " ", "关闭", ""};
	Msgbox = lv_msgbox_create(lv_scr_act(), LV_SYMBOL_WARNING " Notice", "msgbox", msgboxbtns, false);	Msgbox = lv_msgbox_create(lv_scr_act(), LV_SYMBOL_WARNING " 通知", "msgbox", msgboxbtns, false);	Msgbox = lv_msgbox_create(lv_scr_act(), LV_SYMBOL_WARNING “提示”, “msgbox”, msgboxbtns, false);	Msgbox = lv_msgbox_create(lv_scr_act(), LV_SYMBOL_WARNING “通知”, “msgbox”, msgboxbtns, false);
	lv_obj_set_style_bg_opa(Msgbox, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_add_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
	lv_obj_center(Msgbox);

	lv_obj_t *msgboxtitle = lv_msgbox_get_title(Msgbox);
	lv_obj_set_style_text_font(msgboxtitle, &lv_font_montserrat_14, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(msgboxtitle, lv_color_hex(0xFF0000),LV_STATE_DEFAULT);
	lv_obj_add_event_cb(Msgbox, MsgboxCb, LV_EVENT_VALUE_CHANGED, NULL);

	lv_obj_t *msgboxbtn = lv_msgbox_get_btns(Msgbox);
	lv_obj_set_style_bg_opa(msgboxbtn, 0, LV_PART_ITEMS);
	lv_obj_set_style_shadow_width(msgboxbtn, 0, LV_PART_ITEMS);
	lv_obj_set_style_text_color(msgboxbtn, lv_color_hex(0x2271df),LV_PART_ITEMS);

	DataObjInit(DataObj);
	LocalObjInit(Local);
	WifiObjInit(WIFI);
	MQTTObjInit(MQTT);
}
