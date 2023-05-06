#pragma once

#include "keys.h"

#define CASE_VOL_UP  \
   case KEY_UP: \
   case KEY_MSWHEEL_UP: \
   case KEY_NUMPAD8:

#define CASE_VOL_DOWN   \
   case KEY_DOWN: \
   case KEY_MSWHEEL_DOWN: \
   case KEY_NUMPAD2:

#define CASE_PREV_POS_FINEST \
   case KEY_MSWHEEL_DOWN+KEY_CTRL+KEY_ALT:  \
   case KEY_MSWHEEL_DOWN+KEY_RCTRL+KEY_RALT: \
   case KEY_LEFT+KEY_ALT: \
   case KEY_LEFT+KEY_RALT:

#define CASE_PREV_POS_FINE \
   case KEY_MSWHEEL_DOWN+KEY_CTRL+KEY_SHIFT: \
   case KEY_MSWHEEL_DOWN+KEY_RCTRL+KEY_SHIFT: \
   case KEY_LEFT+KEY_SHIFT:

#define CASE_PREV_POS \
   case KEY_MSWHEEL_DOWN+KEY_CTRL: \
   case KEY_MSWHEEL_DOWN+KEY_RCTRL: \
   case KEY_LEFT: \
   case KEY_NUMPAD4:

#define CASE_NEXT_POS_FINEST \
   case KEY_MSWHEEL_UP+KEY_CTRL+KEY_ALT: \
   case KEY_MSWHEEL_UP+KEY_RCTRL+KEY_RALT: \
   case KEY_RIGHT+KEY_ALT: \
   case KEY_RIGHT+KEY_RALT:

#define CASE_NEXT_POS_FINE \
   case KEY_MSWHEEL_UP+KEY_CTRL+KEY_SHIFT: \
   case KEY_MSWHEEL_UP+KEY_RCTRL+KEY_SHIFT: \
   case KEY_RIGHT+KEY_SHIFT:

#define CASE_NEXT_POS \
   case KEY_MSWHEEL_UP+KEY_CTRL: \
   case KEY_MSWHEEL_UP+KEY_RCTRL: \
   case KEY_RIGHT: \
   case KEY_NUMPAD6:

#define CASE_PAN_RIGHT \
   case KEY_MSWHEEL_UP+KEY_SHIFT:

#define CASE_PAN_LEFT \
   case KEY_MSWHEEL_DOWN+KEY_SHIFT:

#define CASE_SELECT \
   case KEY_INS:

#define CASE_TOGGLE_MINIMODE \
   case KEY_ADD:

#define CASE_PREV_TRACK \
   case KEY_NUMPAD9: \
   case KEY_PGUP: \
   case KEY_Z: \
   CASE_PREV_TRACK2

#define CASE_PLAY_PAUSE \
   case KEY_X: \
   case KEY_SPACE: \
   case KEY_CLEAR: \
   CASE_PLAY_PAUSE2 \
   CASE_PLAY_PAUSE3

#define CASE_NEXT_TRACK \
   case KEY_NUMPAD3: \
   case KEY_PGDN: \
   case KEY_C: \
   CASE_NEXT_TRACK2

#define CASE_STOP \
   case KEY_V: \
   CASE_STOP2

#define CASE_TOGGLE_REMAIN_TIME \
   case KEY_T: \
   CASE_TOGGLE_REMAIN_TIME2

#define CASE_TOGGLE_TIME_LOOK \
   case KEY_T+KEY_CTRL: \
   CASE_TOGGLE_TIME_LOOK2

#define CASE_DEMO \
   case KEY_D: \
   CASE_DEMO2

#define CASE_CHANGE_DEMO_TIME \
   case KEY_D+KEY_CTRL: \
   CASE_CHANGE_DEMO_TIME2

#define CASE_AUTO_NEXT \
   case KEY_A: \
   CASE_AUTO_NEXT2

#define CASE_LOOP \
   case KEY_A+KEY_CTRL: \
   CASE_LOOP2

#define CASE_EXT_PLAYER_ACTION \
   case KEY_P: \
   CASE_EXT_PLAYER_ACTION2

#define CASE_TOGGLE_VOLUMEBARS \
   case KEY_B: \
   CASE_TOGGLE_VOLUMEBARS2

#define CASE_ENTER_DESCRIPTION \
   case KEY_N: \
   CASE_ENTER_DESCRIPTION2

#define CASE_TOGGLE_DESCRIPTION \
   case KEY_N+KEY_CTRL: \
   CASE_TOGGLE_DESCRIPTION2

#define CASE_TOGGLE_MAXIMIZE \
   case KEY_M: \
   CASE_TOGGLE_MAXIMIZE2

#if KEY_CLEAR!=KEY_NUMPAD5
#define CASE_PLAY_PAUSE3 case KEY_NUMPAD5:
#else
#define CASE_PLAY_PAUSE3 case '5':
#endif

#define CASE_PREV_TRACK2 \
   case 0x42f: \
   case 0x44f:
#define CASE_PLAY_PAUSE2 \
   case 0x427: \
   case 0x447:
#define CASE_NEXT_TRACK2 \
   case 0x421: \
   case 0x441:
#define CASE_STOP2 \
   case 0x41C: \
   case 0x43c:
#define CASE_TOGGLE_REMAIN_TIME2 \
   case 0x415: \
   case 0x435:
#define CASE_TOGGLE_TIME_LOOK2 \
   case 0x415+KEY_CTRL: \
   case 0x435+KEY_CTRL:
#define CASE_DEMO2 \
   case 0x412: \
   case 0x432:
#define CASE_CHANGE_DEMO_TIME2 \
   case 0x412+KEY_CTRL: \
   case 0x432+KEY_CTRL:
#define CASE_AUTO_NEXT2 \
   case 0x424: \
   case 0x444:
#define CASE_LOOP2 \
   case 0x424+KEY_CTRL: \
   case 0x444+KEY_CTRL:
#define CASE_EXT_PLAYER_ACTION2 \
   case 0x417: \
   case 0x437:
#define CASE_TOGGLE_VOLUMEBARS2 \
   case 0x418: \
   case 0x438:
#define CASE_ENTER_DESCRIPTION2 \
   case 0x422: \
   case 0x442:
#define CASE_TOGGLE_DESCRIPTION2 \
   case 0x422+KEY_CTRL: \
   case 0x442+KEY_CTRL:
#define CASE_TOGGLE_MAXIMIZE2 \
   case 0x42c: \
   case 0x44c:
