#ifndef __TEST_PROTOCOL_H
#define __TEST_PROTOCOL_H

#include <stdint.h>
#include "tinybuf.h"
#include "test_parent_protocol.h"

#define TEST1 10
#define TEST2 12
#define TEST3 1000

#define Embedded_message_g_tag 100
#define Test_message_x_tag 1
#define Test_message_embedded_message_oneof_tag 2

typedef struct {
	uint8_t has_e;
	uint64_t e;
} Embedded_message1;

typedef struct {
	uint8_t f;
	Embedded_message1 embedded_message1[2];
	uint8_t which_embedded_payload;
	union {
		uint8_t g;
	} embedded_payload;
} Embedded_message;

typedef struct {
	uint32_t fixed_array[4];
	uint8_t has_a;
	uint16_t a;
	int32_t b;
	uint8_t uint16_array_count;
	uint16_t uint16_array[10];
	uint8_t embedded_messages_count;
	Embedded_message embedded_messages[12];
	uint8_t has_embedded_message1;
	Embedded_message1 embedded_message1;
	Empty_message empty_message;
	uint16_t uint8_array_count;
	uint8_t uint8_array[1000];
	uint8_t has_c;
	double c;
	float d;
	uint8_t which_payload;
	union {
		uint8_t x;
		Embedded_message embedded_message_oneof;
	} payload;
} Test_message;

extern const tb_field_t Embedded_message1_fields[2];
extern const tb_field_t Embedded_message_fields[4];
extern const tb_field_t Test_message_fields[13];

#endif
