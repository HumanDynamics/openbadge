#include "tinybuf.h"
#include "test_protocol.h"

const tb_field_t Embedded_message1_fields[2] = {
	{18, tb_offsetof(Embedded_message1, e), tb_delta(Embedded_message1, has_e, e), 1, tb_membersize(Embedded_message1, e), 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t Embedded_message_fields[3] = {
	{17, tb_offsetof(Embedded_message, f), 0, 0, tb_membersize(Embedded_message, f), 0, NULL},
	{129, tb_offsetof(Embedded_message, embedded_message1), 0, 0, tb_membersize(Embedded_message, embedded_message1), 0, &Embedded_message1_fields},
	TB_LAST_FIELD,
};

const tb_field_t Empty_message_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t Test_message_fields[10] = {
	{18, tb_offsetof(Test_message, a), tb_delta(Test_message, has_a, a), 1, tb_membersize(Test_message, a), 0, NULL},
	{9, tb_offsetof(Test_message, b), 0, 0, tb_membersize(Test_message, b), 0, NULL},
	{20, tb_offsetof(Test_message, uint16_array), tb_delta(Test_message, uint16_array_count, uint16_array), 1, tb_membersize(Test_message, uint16_array[0]), tb_membersize(Test_message, uint16_array)/tb_membersize(Test_message, uint16_array[0]), NULL},
	{132, tb_offsetof(Test_message, embedded_messages), tb_delta(Test_message, embedded_messages_count, embedded_messages), 1, tb_membersize(Test_message, embedded_messages[0]), tb_membersize(Test_message, embedded_messages)/tb_membersize(Test_message, embedded_messages[0]), &Embedded_message_fields},
	{130, tb_offsetof(Test_message, embedded_message1), tb_delta(Test_message, has_embedded_message1, embedded_message1), 1, tb_membersize(Test_message, embedded_message1), 0, &Embedded_message1_fields},
	{129, tb_offsetof(Test_message, empty_message), 0, 0, tb_membersize(Test_message, empty_message), 0, &Empty_message_fields},
	{20, tb_offsetof(Test_message, uint8_array), tb_delta(Test_message, uint8_array_count, uint8_array), 2, tb_membersize(Test_message, uint8_array[0]), tb_membersize(Test_message, uint8_array)/tb_membersize(Test_message, uint8_array[0]), NULL},
	{66, tb_offsetof(Test_message, c), tb_delta(Test_message, has_c, c), 1, tb_membersize(Test_message, c), 0, NULL},
	{33, tb_offsetof(Test_message, d), 0, 0, tb_membersize(Test_message, d), 0, NULL},
	TB_LAST_FIELD,
};

