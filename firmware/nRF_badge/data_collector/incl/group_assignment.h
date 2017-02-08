//
// Created by Andrew Bartow on 2/2/17.
//

#ifndef OPENBADGE_GROUP_ASSIGNMENT_H
#define OPENBADGE_GROUP_ASSIGNMENT_H

#include <stdint.h>

typedef struct {
    uint8_t group_id;
    uint16_t member_id;
} GroupAssignment_t;

void GroupAssignment_StoreAssignment(GroupAssignment_t assignment);

GroupAssignment_t GroupAssignment_GetAssignment(void);

#endif //OPENBADGE_GROUP_ASSIGNMENT_H
