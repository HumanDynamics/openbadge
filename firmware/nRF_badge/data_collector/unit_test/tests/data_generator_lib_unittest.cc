

// Don't forget gtest.h, which declares the testing framework.

#include "data_generator_internal_lib.h"
#include "gtest/gtest.h"

// Declaring the generator-function type
DATA_GENERATOR_FUNCTION_DECLARATION(TEST, uint8_t, uint8_t* x, uint8_t* y);

// Declaring all the needed data-generating functions
DATA_GENERATOR_DECLARATION(TEST, uint8_t, uint8_t* x, uint8_t* y);

// Implementing all the needed data-generating functions
DATA_GENERATOR_IMPLEMENTATION(TEST, uint8_t, uint8_t* x, uint8_t* y) {
	
	if(data_generator_TEST_get_generator() != NULL)
		return data_generator_TEST_get_generator()(x, y);
	
	*x = 0;
	*y = 0;
	return 0;
}


uint8_t generator_handler(uint8_t* x, uint8_t* y) {
	*x = 1;
	*y = 2;
	
	return 1;
}


namespace {
	
class DataGeneratorTest : public ::testing::Test {
	virtual void SetUp() {
		data_generator_TEST_reset();
	}	
};


TEST_F(DataGeneratorTest, DefaultTest) {
	uint8_t x, y;
	uint8_t ret = data_generator_TEST(&x, &y);
	
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(x, 0);
	EXPECT_EQ(y, 0);	
}

TEST_F(DataGeneratorTest, GeneratorTest) {
	uint8_t x, y;
	data_generator_TEST_set_generator(generator_handler);
	uint8_t ret = data_generator_TEST(&x, &y);
	
	EXPECT_EQ(ret, 1);
	EXPECT_EQ(x, 1);
	EXPECT_EQ(y, 2);	
}

};  




