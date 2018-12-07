import sys
import test_protocol



def read_bytes(file):
	bytes_read = b''
	with open(file, "rb") as f:			
		bytes_read = f.read()
	
	return bytes_read
	
def write_bytes(file, byte_write):
	with open(file, "wb") as f:	
		f.write(bytes_write)


if __name__ == '__main__':
	if(len(sys.argv) < 3):
		raise Exception("Too less arguments")

	input_file = sys.argv[1]
	output_file = sys.argv[2]

	bytes_read = read_bytes(input_file)
	
	test_message = test_protocol.Test_message.decode(bytes_read)

	
	print "Decoded test_message"
	print test_message.__dict__
	print test_message.payload.__dict__
	
	
	
	bytes_write = test_message.encode()
	
	
	print "Encoded test_message: Len: " + str(len(bytes_write))
	print repr(bytes_write)
	
	write_bytes(output_file, bytes_write)
	