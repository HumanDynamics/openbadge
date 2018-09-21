#ifndef __TINYBUF_H
#define __TINYBUF_H


#include <stdint.h>
#include <stddef.h>

#define tb_membersize(st, m) 	((uint32_t)(sizeof ((st*)0)->m))
#define tb_offsetof(st, m)		((uint32_t) (offsetof(st, m)))
#define tb_delta(st, m1, m2) 	(((int32_t)tb_offsetof(st, m1)) - ((int32_t)tb_offsetof(st, m2)))


typedef enum {
	TB_BIG_ENDIAN = 0,
	TB_LITTLE_ENDIAN = 1
} tb_endian_t;

typedef enum {
	FIELD_TYPE_REQUIRED 		= (1 << 0),
	FIELD_TYPE_OPTIONAL 		= (1 << 1),
	FIELD_TYPE_REPEATED		 	= (1 << 2),
	FIELD_TYPE_FIXED_REPEATED 	= (1 << 3),
	FIELD_TYPE_ONEOF			= (1 << 4),
} tb_field_type_t;

typedef enum {
	DATA_TYPE_INT 				= (1 << 5),
	DATA_TYPE_UINT 				= (1 << 6),
	DATA_TYPE_FLOAT 			= (1 << 7),
	DATA_TYPE_DOUBLE 			= (1 << 8),
	DATA_TYPE_MESSAGE 			= (1 << 9),
} tb_data_type_t;


#define TB_LAST_FIELD {0, 0, 0, 0, 0, 0, 0, 0, NULL}	/**< Marker for the last field in a field-array */

typedef struct {
	uint16_t 	type; 			/**< optional/repeated/required. uint, int, float, double, submessage */
	uint32_t 	data_offset;	/**< Offset of data relative to begin of struct */
	int32_t 	size_offset;	/**< Offset to bool flag when optional-field or to size when repeated or to which_ when oneof, relative to field data */
	uint32_t	size_size;		/**< Size of bool flag or the size of size/count of an array or the which_-field of oneof-field */
	uint32_t	data_size;		/**< Size of one data entry (1,2,4,8,submessage_size) */
	uint32_t	array_size;		/**< Size of array. 0 if it is not an array */
	uint8_t		oneof_tag;		/**< The tag-number/identifier of a oneof-field. 0 if not an oneof-field. */
	uint8_t		oneof_first;	/**< Flag if the field is the first oneof-entry. */
	const void*	ptr;	  		/**< Pointer to submessage description, otherwise NULL */
} tb_field_t;



typedef struct {
	uint8_t* buf;
	uint32_t buf_size;
	uint32_t bytes_written;
} tb_ostream_t;

typedef struct {
	uint8_t* buf;
	uint32_t buf_size;
	uint32_t bytes_read;
} tb_istream_t;


/**@brief Function to create an output-stream from a buffer.
 *
 * @param[in]	buf				Pointer to the buffer that should be used by the output-stream.
 * @param[in]	buf_size		Maximal size of the buffer.
 *
 * @retval 		tb_ostream_t	Output-stream structure.
 */
tb_ostream_t tb_ostream_from_buffer(uint8_t* buf, uint32_t buf_size);


/**@brief Function to create an input-stream from a buffer.
 *
 * @param[in]	buf				Pointer to the buffer that should be used by the output-stream.
 * @param[in]	buf_size		Size of the input-buffer.
 *
 * @retval 		tb_istream_t	Input-stream structure.
 */
tb_istream_t tb_istream_from_buffer(uint8_t* buf, uint32_t buf_size);



/**@brief Function to serialize a structure into an output-stream.
 *
 * @param[in]	ostream		Pointer to output-stream structure.
 * @param[in]	fields		Pointer to the array of structure-fields.
 * @param[in]	src_struct	Pointer to structure that should be serialized.
 * @param[in] 	output_endianness	The desired endianness of the output-data.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations or invalid structure.
 */
uint8_t tb_encode(tb_ostream_t* ostream, const tb_field_t fields[], void* src_struct, tb_endian_t output_endianness);


/**@brief Function to deserialize an input-stream to a structure.
 *
 * @param[in]	istream		Pointer to input-stream structure.
 * @param[in]	fields		Pointer to the array of structure-fields.
 * @param[in]	src_struct	Pointer to structure where the deserialized data should be stored to.
 * @param[in] 	input_endianness	The endianness of the input-data.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations or invalid structure.
 */
uint8_t tb_decode(tb_istream_t* istream, const tb_field_t fields[], void* dst_struct, tb_endian_t input_endianness);


/**@brief Function to retrieve the max encoded length of a message.
 *
 * @param[in]	fields		Pointer to the array of structure-fields.
 *
 * @retval 		Maximum encoded length.
 */
uint32_t tb_get_max_encoded_len(const tb_field_t fields[]);


#endif

