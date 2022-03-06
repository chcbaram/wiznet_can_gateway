#ifndef SRC_COMMON_HW_INCLUDE_BUTTON_H_
#define SRC_COMMON_HW_INCLUDE_BUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "hw_def.h"


#ifdef _USE_HW_BUTTON

#define BUTTON_MAX_CH         HW_BUTTON_MAX_CH


typedef struct
{
  uint8_t  ch;
  uint8_t  state;
  uint32_t pressed_time;
  uint32_t repeat_start_time;
  uint32_t repeat_pressed_time;
  uint32_t pre_time;

  uint8_t event_flag;
  uint8_t state_flag;
  uint8_t click_count;
} button_obj_t;

enum ButtonEvt
{
  BUTTON_EVT_PRESSED  = (1<<0),
  BUTTON_EVT_RELEASED = (1<<1),
  BUTTON_EVT_CLICKED  = (1<<2),
  BUTTON_EVT_REPEATED = (1<<3),
};

enum ButtonState
{
  BUTTON_STATE_PRESSED  = (1<<0),
  BUTTON_STATE_RELEASED = (1<<1),
  BUTTON_STATE_REPEATED = (1<<2),
};

bool buttonInit(void);
bool buttonGetPressed(uint8_t ch);
uint16_t buttonGetData(void);

void buttonObjCreate(button_obj_t *p_obj, uint8_t ch, uint32_t pressed_time, uint32_t repeat_start_time, uint32_t repeat_pressed_time);
bool buttonObjUpdate(button_obj_t *p_obj);
uint8_t buttonObjGetEvent(button_obj_t *p_obj);
void buttonObjClearEvent(button_obj_t *p_obj);
uint8_t buttonObjGetState(button_obj_t *p_obj);

#endif


#ifdef __cplusplus
}
#endif


#endif