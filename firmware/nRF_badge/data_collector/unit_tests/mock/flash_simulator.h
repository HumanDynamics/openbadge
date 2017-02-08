//
// Created by Andrew Bartow on 1/11/17.
//

#ifndef OPENBADGE_FLASH_SIMULATOR_H
#define OPENBADGE_FLASH_SIMULATOR_H

// Erase all data stored in the simulated flash.
void reset_simulated_flash(void);

// Waits for 'operations_to_wait' pstorage_update()'s, and then causes the following operation to
// timeout.
void pstorage_update_trigger_timeout(int operations_to_wait);

#endif //OPENBADGE_FLASH_SIMULATOR_H
