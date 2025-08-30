#include "config.h"
// #include <BLEDevice.h>
// #include <BLEScan.h>
// #include <BLEAdvertisedDevice.h>

TTGOClass *ttgo;


int lastRSSI = 0;
lv_obj_t *label_btn1 = NULL;

static void event_handler(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED) {
        Serial.printf("Clicked\n");
        ttgo->motor->onec();
        // Scan BLE Holy-IOT
    } else if (event == LV_EVENT_VALUE_CHANGED) {
        Serial.printf("Toggled\n");
    }
}


void setup()
{
    Serial.begin(115200);
    delay(1000);
    ttgo = TTGOClass::getWatch();
    ttgo->begin();
    ttgo->openBL();
    ttgo->motor_begin();
    ttgo->lvgl_begin();

    // Bouton 1
    lv_obj_t *btn1 = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(btn1, event_handler);
    lv_obj_align(btn1, NULL, LV_ALIGN_CENTER, 0, -40);
    label_btn1 = lv_label_create(btn1, NULL);
    lv_label_set_text(label_btn1, "Button");

    // Bouton 2
    lv_obj_t *btn2 = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(btn2, event_handler);
    lv_obj_align(btn2, NULL, LV_ALIGN_CENTER, 0, 40);
    lv_btn_set_checkable(btn2, true);
    lv_btn_toggle(btn2);
    lv_btn_set_fit2(btn2, LV_FIT_NONE, LV_FIT_TIGHT);
    lv_obj_t *label2 = lv_label_create(btn2, NULL);
    lv_label_set_text(label2, "Toggled");

}

void loop()
{
    lv_task_handler();
    delay(5);
}
