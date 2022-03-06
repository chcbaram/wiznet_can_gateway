#include "button.h"
#include "cli.h"


#ifdef _USE_HW_BUTTON



typedef struct
{
  uint32_t  pin;
  uint8_t   on_state;
} button_tbl_t;


button_tbl_t button_tbl[BUTTON_MAX_CH] =
    {
        {6, _DEF_LOW},
    };


#ifdef _USE_HW_CLI
static void cliButton(cli_args_t *args);
#endif


bool buttonInit(void)
{
  bool ret = true;


  for (int i=0; i<BUTTON_MAX_CH; i++)
  {
    gpio_set_pulls(button_tbl[i].pin, true, false);
    gpio_set_dir(button_tbl[i].pin, GPIO_IN);
  }

#ifdef _USE_HW_CLI
  cliAdd("button", cliButton);
#endif

  return ret;
}

bool buttonGetPressed(uint8_t ch)
{
  bool ret = false;

  if (ch >= BUTTON_MAX_CH)
  {
    return false;
  }

  if (gpio_get(button_tbl[ch].pin) == button_tbl[ch].on_state)
  {
    ret = true;
  }

  return ret;
}

uint16_t buttonGetData(void)
{
  uint16_t ret = 0;


  for (int i=0; i<BUTTON_MAX_CH; i++)
  {
    ret |= (buttonGetPressed(i)<<i);
  }

  return ret;
}

#if HW_BUTTON_OBJ_USE == 1
void buttonObjCreate(button_obj_t *p_obj, uint8_t ch, uint32_t pressed_time, uint32_t repeat_start_time, uint32_t repeat_pressed_time)
{
  p_obj->ch = ch;
  p_obj->state = 0;
  p_obj->pre_time = millis();
  p_obj->pressed_time = pressed_time;
  p_obj->repeat_start_time = repeat_start_time;
  p_obj->repeat_pressed_time = repeat_pressed_time;
  p_obj->event_flag = 0;
  p_obj->state_flag = 0;
  p_obj->click_count = 0;
}

enum ButtonObjState
{
  BUTTON_OBJ_WAIT_FOR_PRESSED,
  BUTTON_OBJ_PRESSED,
  BUTTON_OBJ_REPEATED_START,
  BUTTON_OBJ_REPEATED,
};

bool buttonObjUpdate(button_obj_t *p_obj)
{
  bool ret = false;


  switch(p_obj->state)
  {
    case BUTTON_OBJ_WAIT_FOR_PRESSED:
      if (buttonGetPressed(p_obj->ch) == true)
      {
        p_obj->state = BUTTON_OBJ_PRESSED;
        p_obj->pre_time = millis();        
        p_obj->click_count = 0;
      }
      break;

    case BUTTON_OBJ_PRESSED:
      if (buttonGetPressed(p_obj->ch) == true)
      {
        if (millis()-p_obj->pre_time >= p_obj->pressed_time)
        {
          ret = true; 
          p_obj->state = BUTTON_OBJ_REPEATED_START;
          p_obj->pre_time = millis();
          p_obj->event_flag |= BUTTON_EVT_CLICKED;
          
          p_obj->state_flag |= BUTTON_STATE_PRESSED;
          p_obj->click_count++;
        }
      }
      else
      {
        p_obj->state = BUTTON_OBJ_WAIT_FOR_PRESSED;
        
        if (p_obj->state_flag & BUTTON_STATE_PRESSED)
        {
          p_obj->event_flag |= BUTTON_EVT_RELEASED;

          p_obj->state_flag |= BUTTON_STATE_RELEASED;
          p_obj->state_flag &= ~BUTTON_STATE_PRESSED;
          p_obj->state_flag &= ~BUTTON_STATE_REPEATED;          
        }
      }
      break;

    case BUTTON_OBJ_REPEATED_START:
      if (buttonGetPressed(p_obj->ch) == true)
      {
        if (millis()-p_obj->pre_time >= p_obj->repeat_start_time)
        {
          ret = true;
          p_obj->pre_time = millis();

          ret = true; 
          p_obj->state = BUTTON_OBJ_REPEATED;

          p_obj->event_flag |= BUTTON_EVT_CLICKED;
          p_obj->event_flag |= BUTTON_EVT_REPEATED;

          p_obj->state_flag |= BUTTON_STATE_REPEATED;
          p_obj->click_count++;
        }
      }
      else
      {
        p_obj->state = BUTTON_OBJ_PRESSED;
      }
      break;

    case BUTTON_OBJ_REPEATED:
      if (buttonGetPressed(p_obj->ch) == true)
      {
        if (millis()-p_obj->pre_time >= p_obj->repeat_pressed_time)
        {
          ret = true;
          p_obj->pre_time = millis();

          p_obj->event_flag |= BUTTON_EVT_CLICKED;
          p_obj->event_flag |= BUTTON_EVT_REPEATED;

          p_obj->state_flag |= BUTTON_STATE_REPEATED;
          p_obj->click_count++;
        }
      }
      else
      {
        p_obj->state = BUTTON_OBJ_PRESSED;
      }
      break;
  }
  
  return ret;
}

uint8_t buttonObjGetEvent(button_obj_t *p_obj)
{
  return p_obj->event_flag;
}

void buttonObjClearEvent(button_obj_t *p_obj)
{
  p_obj->event_flag = 0;
}

uint8_t buttonObjGetState(button_obj_t *p_obj)
{
  return p_obj->state_flag;
}
#endif

#ifdef _USE_HW_CLI

void cliButton(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "show"))
  {
    while(cliKeepLoop())
    {
      for (int i=0; i<BUTTON_MAX_CH; i++)
      {
        cliPrintf("%d", buttonGetPressed(i));
      }
      cliPrintf("\n");

      delay(100);
    }

    ret = true;
  }

  #if HW_BUTTON_OBJ_USE == 1
  if (args->argc == 2 && args->isStr(0, "event"))
  {
    uint8_t button_ch;
    button_obj_t button_sw;    
    uint8_t button_event;

    button_ch = constrain(args->getData(1), 0, BUTTON_MAX_CH-1);
    buttonObjCreate(&button_sw, button_ch, 50, 1000, 100);

    while(cliKeepLoop())
    {
      buttonObjUpdate(&button_sw);

      button_event = buttonObjGetEvent(&button_sw);

      if (button_event > 0)
      {
        if (button_event & BUTTON_EVT_PRESSED)
          cliPrintf("button pressed\n");    
        if (button_event & BUTTON_EVT_CLICKED)
          cliPrintf("button clicked %d\n", button_sw.click_count);    
        if (button_event & BUTTON_EVT_RELEASED)
          cliPrintf("button released\n");    

        buttonObjClearEvent(&button_sw);
      }      

      delay(5);
    }

    ret = true;
  }
  #endif

  if (ret != true)
  {
    cliPrintf("button show\n");
    #if HW_BUTTON_OBJ_USE == 1
    cliPrintf("button event 0~%d\n", BUTTON_MAX_CH-1);
    #endif
  }
}
#endif



#endif
