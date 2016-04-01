/* 
 * Some Nordic SDK libraries rely on boards.h, which includes an option for a custom_board.h header
 * In order to use a custom-named header without modifying the SDK, this file "redirects"
 * an include of custom_board.h to other file(s)
 *
 */
#ifndef CUSTOM_BOARD_H
#define CUSTOM_BOARD_H

#if defined(BOARD_BADGE_03)
  #include "badge_03.h"
#elif defined(BOARD_BADGE_03V2_RIGADO)
  #include "badge_03v2_rigado.h"
#elif defined(BOARD_BADGE_03V2_DYNASTREAM)
  #include "badge_03v2_dynastream.h"
#elif defined(BOARD_BADGE_03V4)
  #include "badge_03v4.h"
#else
  #error "Board is not defined"
#endif


#endif //CUSTOM_BOARD_H