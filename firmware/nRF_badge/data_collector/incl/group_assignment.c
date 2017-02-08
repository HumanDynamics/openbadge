//
// Created by Andrew Bartow on 2/2/17.
//

#include "flash.h"
#include "flash_layout.h"
#include "group_assignment.h"

void GroupAssignment_StoreAssignment(GroupAssignment_t assignment) {
    flash_write(MISC_FLASH_START, (uint8_t *) &assignment, sizeof(GroupAssignment_t));
}

GroupAssignment_t GroupAssignment_GetAssignment(void) {
    GroupAssignment_t assignment;
    flash_read((uint8_t *) &assignment, MISC_FLASH_START, sizeof(GroupAssignment_t));
}