#include "tinybuf.h"
#include "test_protocol.h"

const tb_field_t Embedded_message1_fields[2] = {
	{66, tb_offsetof(Embedded_message1, e), tb_delta(Embedded_message1, has_e, e), 1, tb_membersize(Embedded_message1, e), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t Embedded_message_fields[4] = {
	{65, tb_offsetof(Embedded_message, f), 0, 0, tb_membersize(Embedded_message, f), 0, 0, 0, NULL},
	{520, tb_offsetof(Embedded_message, embedded_message1), 0, 0, tb_membersize(Embedded_message, embedded_message1[0]), tb_membersize(Embedded_message, embedded_message1)/tb_membersize(Embedded_message, embedded_message1[0]), 0, 0, &Embedded_message1_fields},
	{80, tb_offsetof(Embedded_message, embedded_payload.g), tb_delta(Embedded_message, which_embedded_payload, embedded_payload.g), 1, tb_membersize(Embedded_message, embedded_payload.g), 0, 100, 1, NULL},
	TB_LAST_FIELD,
};

const tb_field_t Test_message_fields[13] = {
	{72, tb_offsetof(Test_message, fixed_array), 0, 0, tb_membersize(Test_message, fixed_array[0]), tb_membersize(Test_message, fixed_array)/tb_membersize(Test_message, fixed_array[0]), 0, 0, NULL},
	{66, tb_offsetof(Test_message, a), tb_delta(Test_message, has_a, a), 1, tb_membersize(Test_message, a), 0, 0, 0, NULL},
	{33, tb_offsetof(Test_message, b), 0, 0, tb_membersize(Test_message, b), 0, 0, 0, NULL},
	{68, tb_offsetof(Test_message, uint16_array), tb_delta(Test_message, uint16_array_count, uint16_array), 1, tb_membersize(Test_message, uint16_array[0]), tb_membersize(Test_message, uint16_array)/tb_membersize(Test_message, uint16_array[0]), 0, 0, NULL},
	{516, tb_offsetof(Test_message, embedded_messages), tb_delta(Test_message, embedded_messages_count, embedded_messages), 1, tb_membersize(Test_message, embedded_messages[0]), tb_membersize(Test_message, embedded_messages)/tb_membersize(Test_message, embedded_messages[0]), 0, 0, &Embedded_message_fields},
	{514, tb_offsetof(Test_message, embedded_message1), tb_delta(Test_message, has_embedded_message1, embedded_message1), 1, tb_membersize(Test_message, embedded_message1), 0, 0, 0, &Embedded_message1_fields},
	{513, tb_offsetof(Test_message, empty_message), 0, 0, tb_membersize(Test_message, empty_message), 0, 0, 0, &Empty_message_fields},
	{68, tb_offsetof(Test_message, uint8_array), tb_delta(Test_message, uint8_array_count, uint8_array), 2, tb_membersize(Test_message, uint8_array[0]), tb_membersize(Test_message, uint8_array)/tb_membersize(Test_message, uint8_array[0]), 0, 0, NULL},
	{258, tb_offsetof(Test_message, c), tb_delta(Test_message, has_c, c), 1, tb_membersize(Test_message, c), 0, 0, 0, NULL},
	{129, tb_offsetof(Test_message, d), 0, 0, tb_membersize(Test_message, d), 0, 0, 0, NULL},
	{80, tb_offsetof(Test_message, payload.x), tb_delta(Test_message, which_payload, payload.x), 1, tb_membersize(Test_message, payload.x), 0, 1, 1, NULL},
	{528, tb_offsetof(Test_message, payload.embedded_message_oneof), tb_delta(Test_message, which_payload, payload.embedded_message_oneof), 1, tb_membersize(Test_message, payload.embedded_message_oneof), 0, 2, 0, &Embedded_message_fields},
	TB_LAST_FIELD,
};

