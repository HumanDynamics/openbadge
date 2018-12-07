#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "test_protocol.h"

#define EXPECT_ARRAY_EQ(A, B, len) {for(uint32_t i = 0; i < len; i++) {EXPECT_EQ(A[i], B[i])}}
#define EXPECT_EQ(A, B)	{if(A != B) { printf("Error EXPECT_EQ at line %u\n", __LINE__); return 0;}}

char output_file_name[] = "/output_file.bin";
char input_file_name[] = "/input_file.bin";

uint8_t create_test_message(uint8_t* buf, uint32_t max_size, uint32_t* len) {
	
	tb_ostream_t ostream = tb_ostream_from_buffer(buf, max_size);

	Test_message test_message;
	memset(&test_message, 0, sizeof(test_message));
	
	test_message.fixed_array[0] = 100;
	test_message.fixed_array[1] = 101;
	test_message.fixed_array[2] = 102;
	test_message.fixed_array[3] = 103;
	
	
	test_message.has_a = 0;
	test_message.a = 10000;
	
	test_message.b = -100000;
	
	test_message.uint16_array_count = 3;
	test_message.uint16_array[0] = TEST1;
	test_message.uint16_array[1] = TEST2;
	test_message.uint16_array[2] = TEST3;
	
	
	test_message.embedded_messages_count = 2;
	test_message.embedded_messages[0].f = 100;
	test_message.embedded_messages[0].embedded_message1[0].has_e = 1;
	test_message.embedded_messages[0].embedded_message1[0].e = 0xDEADBEEF;
	test_message.embedded_messages[0].embedded_message1[1].has_e = 0;
	test_message.embedded_messages[0].which_embedded_payload = Embedded_message_g_tag;
	test_message.embedded_messages[0].embedded_payload.g = 100;
	test_message.embedded_messages[1].f = 101;
	test_message.embedded_messages[1].embedded_message1[0].has_e = 0;
	test_message.embedded_messages[1].embedded_message1[1].has_e = 1;
	test_message.embedded_messages[1].embedded_message1[1].e = 0xBEEFDEAD;
	test_message.embedded_messages[1].which_embedded_payload = Embedded_message_g_tag;
	test_message.embedded_messages[1].embedded_payload.g = 200;
	
	test_message.has_embedded_message1 = 1;
	test_message.embedded_message1.has_e = 1;
	test_message.embedded_message1.e = 0xBAADFEED;
	
	test_message.uint8_array_count = 5;
	test_message.uint8_array[0] = 200;
	test_message.uint8_array[1] = 201;
	test_message.uint8_array[2] = 202;
	
	test_message.has_c = 1;
	test_message.c = 1.123;
	
	test_message.d = 11.23f;
	
	test_message.which_payload = Test_message_embedded_message_oneof_tag;
	test_message.payload.embedded_message_oneof.f = 11;
	test_message.payload.embedded_message_oneof.embedded_message1[0].has_e = 1;
	test_message.payload.embedded_message_oneof.embedded_message1[0].e = 0xFF;
	test_message.payload.embedded_message_oneof.embedded_message1[1].has_e = 1;
	test_message.payload.embedded_message_oneof.embedded_message1[1].e = 0xAA;
	test_message.payload.embedded_message_oneof.which_embedded_payload = Embedded_message_g_tag;
	test_message.payload.embedded_message_oneof.embedded_payload.g = 100;
	
	
	
	uint8_t encode_status = tb_encode(&ostream, Test_message_fields, &test_message, TB_BIG_ENDIAN);
	*len = ostream.bytes_written;
	printf("Encode status: %u, Len: %u\n", encode_status, *len);
	
	
	return encode_status;
	
}

uint8_t check_test_message(uint8_t* buf, uint32_t len) {
	tb_istream_t istream = tb_istream_from_buffer(buf, len);
	
	Test_message test_message;
	memset(&test_message, 0, sizeof(test_message));
	
	uint8_t decode_status = tb_decode(&istream, Test_message_fields, &test_message, TB_BIG_ENDIAN);
	printf("Decode status: %u\n", decode_status);
	EXPECT_EQ(decode_status, 1);
	
	
	EXPECT_EQ(test_message.fixed_array[0], 100);
	EXPECT_EQ(test_message.fixed_array[1], 101);
	EXPECT_EQ(test_message.fixed_array[2], 102);
	EXPECT_EQ(test_message.fixed_array[3], 103);
	
	EXPECT_EQ(test_message.has_a, 0);
	EXPECT_EQ(test_message.a, 0);
	
	EXPECT_EQ(test_message.b, -100000);
	
	
	
	EXPECT_EQ(test_message.uint16_array_count, 3);
	EXPECT_EQ(test_message.uint16_array[0], TEST1);
	EXPECT_EQ(test_message.uint16_array[1], TEST2);
	EXPECT_EQ(test_message.uint16_array[2], TEST3);
	
	
	EXPECT_EQ(test_message.embedded_messages_count, 2);
	EXPECT_EQ(test_message.embedded_messages[0].f, 100);
	EXPECT_EQ(test_message.embedded_messages[0].embedded_message1[0].has_e, 1);
	EXPECT_EQ(test_message.embedded_messages[0].embedded_message1[0].e, 0xDEADBEEF);
	EXPECT_EQ(test_message.embedded_messages[0].embedded_message1[1].has_e, 0);
	EXPECT_EQ(test_message.embedded_messages[0].which_embedded_payload, Embedded_message_g_tag);
	EXPECT_EQ(test_message.embedded_messages[0].embedded_payload.g, 100);
	EXPECT_EQ(test_message.embedded_messages[1].f, 101);
	EXPECT_EQ(test_message.embedded_messages[1].embedded_message1[0].has_e, 0);
	EXPECT_EQ(test_message.embedded_messages[1].embedded_message1[1].has_e, 1);
	EXPECT_EQ(test_message.embedded_messages[1].embedded_message1[1].e, 0xBEEFDEAD);
	EXPECT_EQ(test_message.embedded_messages[1].which_embedded_payload, Embedded_message_g_tag);
	EXPECT_EQ(test_message.embedded_messages[1].embedded_payload.g, 200);
	
	EXPECT_EQ(test_message.has_embedded_message1, 1);
	EXPECT_EQ(test_message.embedded_message1.has_e, 1);
	EXPECT_EQ(test_message.embedded_message1.e, 0xBAADFEED);
	
	EXPECT_EQ(test_message.uint8_array_count, 5);
	EXPECT_EQ(test_message.uint8_array[0], 200);
	EXPECT_EQ(test_message.uint8_array[1], 201);
	EXPECT_EQ(test_message.uint8_array[2], 202);
	EXPECT_EQ(test_message.uint8_array[3], 0);
	EXPECT_EQ(test_message.uint8_array[4], 0);
	
	EXPECT_EQ(test_message.has_c, 1);
	EXPECT_EQ(test_message.c, 1.123);
	
	EXPECT_EQ(test_message.d, 11.23f);
	
	
	EXPECT_EQ(test_message.which_payload, Test_message_embedded_message_oneof_tag);
	EXPECT_EQ(test_message.payload.embedded_message_oneof.f, 11);
	EXPECT_EQ(test_message.payload.embedded_message_oneof.embedded_message1[0].has_e, 1);
	EXPECT_EQ(test_message.payload.embedded_message_oneof.embedded_message1[0].e, 0xFF);
	EXPECT_EQ(test_message.payload.embedded_message_oneof.embedded_message1[1].has_e, 1);
	EXPECT_EQ(test_message.payload.embedded_message_oneof.embedded_message1[1].e, 0xAA);
	EXPECT_EQ(test_message.payload.embedded_message_oneof.which_embedded_payload, Embedded_message_g_tag);
	EXPECT_EQ(test_message.payload.embedded_message_oneof.embedded_payload.g, 100);

	
	return decode_status;
}

void write_to_file(char* file_name, const uint8_t* buf, uint32_t len) {
	FILE *output_file;
	output_file = fopen(file_name, "wb");
	if(output_file == NULL)
		return;
	
	fwrite(buf, len, 1, output_file);
	fclose(output_file);	
}

void read_from_file(char* file_name, uint8_t* buf, uint32_t* len) {
	FILE *input_file;
	input_file = fopen(file_name, "rb");
	if(input_file == NULL)
		return;
	uint32_t l = 0;
	while(fread(&buf[l], 1, 1, input_file)) {
		l++;
	}
	*len = l;
	fclose(input_file);	
}


	
int main(void) {
	
	char cwd[PATH_MAX];
	if(getcwd(cwd, sizeof(cwd)) == NULL)
		return 0;
	
	
	char output_file_path[PATH_MAX];
	char input_file_path[PATH_MAX];
	strcpy(output_file_path, cwd);	
	strcpy(&output_file_path[strlen(cwd)], output_file_name);
	strcpy(input_file_path, cwd);
	strcpy(&input_file_path[strlen(cwd)], input_file_name);
	
	
	printf("Path: %s\n", output_file_path);
	printf("Path: %s\n", input_file_path);
	
	uint32_t Empty_message_expected_size = 0;
	uint32_t Embedded_message1_expected_size = 1 + 8;
	uint32_t Embedded_message_expected_size = 1 + 2*Embedded_message1_expected_size + (1+1);
	uint32_t Test_message_expected_size  = 4*4 + (1+2) + 4 + (1+2*10) + (1+12*Embedded_message_expected_size)
								+ (1+Embedded_message1_expected_size) + Empty_message_expected_size
								+ (2+1000) + (1+8) + 4 + (1+Embedded_message_expected_size);
	EXPECT_EQ(tb_get_max_encoded_len(Empty_message_fields), Empty_message_expected_size);
	EXPECT_EQ(tb_get_max_encoded_len(Embedded_message1_fields), Embedded_message1_expected_size);
	EXPECT_EQ(tb_get_max_encoded_len(Embedded_message_fields), Embedded_message_expected_size);
	EXPECT_EQ(tb_get_max_encoded_len(Test_message_fields), Test_message_expected_size);
									
	
	uint8_t buf[1000];
	uint32_t len = 0;
	uint8_t ret = create_test_message(buf, sizeof(buf), &len);
	EXPECT_EQ(ret, 1);
	
	
	write_to_file(output_file_path, buf, len);
	
	
	printf("Calling python script\n\n\n############# PYTHON ##############\n\n");
	char command[1024];
	strcpy(command, "python ../tester.py ");
	strcpy(&command[strlen(command)],  output_file_path);
	strcpy(&command[strlen(command)], " " );
	strcpy(&command[strlen(command)],  input_file_path);
	system(command);
	
	
	printf("\n############# PYTHON ##############\n\n\nChecking python script output\n");
	
	uint8_t buf1[1000];
	uint32_t len1 = 0;
	read_from_file(input_file_path, buf1, &len1);
	
	// Check if length and the buffers are the same
	EXPECT_EQ(len, len1);
	EXPECT_ARRAY_EQ(buf, buf1, len);
	
	ret = check_test_message(buf1, len1);
	EXPECT_EQ(ret, 1);
	
	printf("\nTest was successful!\n");
	
}