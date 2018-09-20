
import struct
import sys
import os.path
import re



SUPPORTED_OUTPUT_FORMATS = ['-c', '-python']		

# These special characters should be seperated from other characters
SPECIAL_CHARACTERS = [';', '{', '}', '[' , ']']

FIELD_TYPE_REQUIRED = 1
FIELD_TYPE_OPTIONAL = 2
FIELD_TYPE_REPEATED = 4
DATA_TYPE_INT = 8
DATA_TYPE_UINT = 16
DATA_TYPE_FLOAT = 32
DATA_TYPE_DOUBLE = 64
DATA_TYPE_MESSAGE = 128

	
SUPPORTED_DATA_TYPES =[	['uint8', DATA_TYPE_UINT, 1],
						['int8', DATA_TYPE_INT, 1],
						['uint16', DATA_TYPE_UINT, 2],
						['int16', DATA_TYPE_INT, 2],
						['uint32', DATA_TYPE_UINT, 4],
						['int32', DATA_TYPE_INT, 4],
						['uint64', DATA_TYPE_UINT, 8],
						['int64', DATA_TYPE_INT, 8],
						['float', DATA_TYPE_FLOAT, 4],
						['double', DATA_TYPE_DOUBLE, 8]]
						
SUPPORTED_FIELD_TYPES =[['required', FIELD_TYPE_REQUIRED],
						['optional', FIELD_TYPE_OPTIONAL],
						['repeated', FIELD_TYPE_REPEATED]]
						
# The supported data types for array size (has to be orderd by size increasingly, because creator assumes ordered)
SUPPORTED_SIZE_DATA_TYPES = ['uint8', 'uint16', 'uint32', 'uint64']
						
# Mapping of supported data-types and C-data-types
C_DATA_TYPE_MAPPING =[	['uint8', 'uint8_t'],
						['int8', 'int8_t'],
						['uint16', 'uint16_t'],
						['int16', 'int16_t'],
						['uint32', 'uint32_t'],
						['int32', 'int32_t'],
						['uint64', 'uint64_t'],
						['int64', 'int64_t'],
						['float', 'float'],
						['double', 'double']]
						

C_FIELD_TYPE_NAME 		= "tb_field_t"
C_OFFSETOF_MAKRO_NAME 	= "tb_offsetof"
C_MEMBERSIZE_MAKRO_NAME = "tb_membersize"
C_DELTA_MAKRO_NAME 		= "tb_delta"
C_LAST_FIELD_NAME 		= "TB_LAST_FIELD"

PYTHON_DATA_TYPE_MAPPING = [['uint8', '>B'],
							['int8', '>b'],
							['uint16', '>H'],
							['int16', '>h'],
							['uint32', '>I'],
							['int32', '>i'],
							['uint64', '>Q'],
							['int64', '>q'],
							['float', '>f'],
							['double', '>d']]

class Protocol_parser:
	def __init__(self, file):
		self.file = file
		
	# returns [line_number, ['A','B',...]]
	def read_file(self):
		with open(self.file) as f:
			content = f.readlines()
		# you may also want to remove whitespace characters like `\n` at the end of each line
		content = [x.strip() for x in content] 
		
		lines = []
		line_number = 0
		for line in content:
			
			# prepare the string to split all the special characters:
			for c in SPECIAL_CHARACTERS:
				line = line.replace(c, ' ' + c + ' ')
			
			# split on white space and other splitting characters
			splitted = re.split(' |\n|\r|\t', line)
			
			# Remove zero length chars
			rm = []
			for s in splitted:
				if len(s) > 0:
					rm.append(s)
			
			line_number = line_number + 1
			if(len(rm) > 0):
				lines.append([line_number, rm])
			
		return lines
		
		
	def is_valid_var_name(self, name):
		valid = 1
		if(name[0].isdigit()):
			valid = 0
		return valid
		
	def define_duplicate_var_name_check(self, defines, var_name):
		duplicate = 0
		for i in range(1, len(defines)):
			if(defines[i][0] == var_name):
				duplicate = 1
				break
		return duplicate
		
		
	# checks if variable is duplicate in message structure
	def message_duplicate_var_name_check(self, message, var_name):
		duplicate = 0
		for i in range(1, len(message)):
			if(message[i][0] == var_name):
				duplicate = 1
				break
		return duplicate
		
	# checks if message name is duplicate in messages structure
	def message_duplicate_message_name_check(self, messages, message_name):
		duplicate = 0
		for i in range(0, len(messages)):
			if(messages[i][0] == message_name):
				duplicate = 1
				break
		return duplicate
	
	
	def check_format(self, line, format, format_required):
		if(len(line) != len(format)):
			return 0
		for i in range(0, len(line)):
			if(format_required[i]):
				if(not format[i] == line[i]):
					return 0
		return 1
			
	def check_define_header(self, line):
		format = ["define", "{"]
		format_required = [1, 1]
		if(not self.check_format(line, format, format_required)):		
			raise Exception("Line: " + str(line) + " has not the format " + str(format))
	
	def get_define_field(self, line):
		format = ["'define_name'", "=", "'numeric_value'", ";"]
		format_required = [0, 1, 0, 1]
		if(not self.check_format(line, format, format_required)):		
			raise Exception("Line: " + str(line) + " has not the format " + str(format))
		
		return [line[0], line[2]]
			
	def get_message_header(self, line):
		format = ["message", "'message_name'", "{"]
		format_required = [1, 0, 1]
		if(not self.check_format(line, format, format_required)):		
			raise Exception("Line: " + str(line) + " has not the format " + str(format))
		
		message_name = line[1]
		return message_name
	
	
	def get_message_field_attributes(self, line, defines):
		formats = [ ["'field_type'", "'data_type'", "'field_name'", ";"],
					["'field_type'", "'data_type'", "'field_name'", "[", "'array_size'", "]", ";"]]
		formats_required = [[0, 0, 0, 1],
							[0, 0, 0, 1, 0, 1, 1]]
		valid_format = 0		
		for i in range(0, len(formats)):
			if(self.check_format(line, formats[i], formats_required[i])):
				valid_format = i + 1
				break
		
		if(not valid_format):
			raise Exception("Line: " + str(line) + " has not one of the formats: " + str(formats[0]) + " or " + str(formats[1]))
		
		field_type_str = line[0]
		field_data_type_str = line[1] 
		field_name = line[2]		
		field_array_size_str = "0"
		if(valid_format == 2):	# if it is an array
			field_array_size_str = line[4]
		
		
		
		field_type = 0
		field_data_type_len = 0
		field_array_size = 0
		
		
		found_supported_field_type = 0
		for supported_field_type in SUPPORTED_FIELD_TYPES:
			if(field_type_str == supported_field_type[0]):
				field_type = field_type | supported_field_type[1]
				found_supported_field_type = 1
				break
		
		
		if(not found_supported_field_type):
			raise Exception("Field type: '" + field_type_str + "' is not supported")
			
			
		found_supported_data_type = 0
		for supported_data_type in SUPPORTED_DATA_TYPES:
			if(field_data_type_str == supported_data_type[0]):
				field_type = field_type | supported_data_type[1]
				field_data_type_len = supported_data_type[2]
				found_supported_data_type = 1
				break
		
		if(not found_supported_data_type):
			raise Exception("Field data type: '" + field_data_type_str + "' is not supported")

		found_array_size = 0
		if(field_array_size_str.isdigit()):
			field_array_size = int(field_array_size_str)
			found_array_size = 1
		else:
			# Check if it is an define value
			for define in defines:
				define_name = define[0]
				define_val = define[1]
				if(field_array_size_str == define_name):
					field_array_size = int(define_val)
					found_array_size = 1
				
		if(not found_array_size):
			raise Exception("Could not find array size: " + field_array_size_str)
			
		# Check if array size matches to field_type
		if(field_array_size == 0 and field_type & FIELD_TYPE_REPEATED):
			raise Exception("Need array size > 0 for repeated field '" + field_name + "'")
		
		if(field_array_size > 0 and ((field_type & FIELD_TYPE_OPTIONAL) or (field_type & FIELD_TYPE_OPTIONAL))):
			raise Exception("Need array size == 0 for optional/required field '" + field_name + "'")

		
		return [field_name, field_type, field_data_type_len, field_array_size, field_data_type_str]
		
	
	def parse_defines(self, lines):
		new_lines = []
		defines = [] # Array of enum entries
		found_define_start = 0

		for l in lines:
			line_num = l[0]
			line = l[1]
			
			if(line[0] == 'define'):
				# Check if we are already searching for an message to stop
				if found_define_start:
					raise Exception("Found another define start before closing the former define at line " + str(line_num))
				
				try:
					self.check_define_header(line)
				except Exception as e:
					print  "Exception while parsing input file at line " + str(line_num)
					raise e
				
				
				found_define_start = 1
				continue
				
			if(found_define_start):
				if(line[0] == '}'):
					found_define_start = 0
					continue
					
				# Check if we see another '{'
				if('{' in line):
					raise Exception("Found another '{' before closing the former define with '}' at line " + str(line_num))
				try:
					[define_name, define_val] = self.get_define_field(line)
				except Exception as e:
					print  "Exception while parsing input file at line " + str(line_num)
					raise e
				
				if(not define_val.isdigit()):
					raise Exception("Define-value '" + define_val + "' is not a numeric value at line " + str(line_num))
				
				
				if(not self.is_valid_var_name(define_name)):	
					raise Exception("Define-name is not a valid variable name '" + define_name + "' at line " + str(line_num))
				
				if(self.define_duplicate_var_name_check(defines, define_name)):
					raise Exception("Duplicate define-name found: '" + define_name + "' at line " + str(line_num))
				
				
				defines.append([define_name, define_val])
				
				continue
				
			# Else just append the line to the new lines			
			new_lines.append(l)
				
		if found_define_start:
			raise Exception("Haven't found a closing '}' for define")
			
		#print defines
		return [new_lines, defines]			
			
	
	def parse_messages(self, lines, defines):
		messages = []
		new_lines = []
		found_message_start = 0
		message = []	# ['message_name', ['','']]
		for l in lines:
			line_num = l[0]
			line = l[1]
			
			if(line[0] == 'message'):
				# Check if we are already searching for an message to stop
				if found_message_start:
					raise Exception("Found another message start before closing the former message at line " + str(line_num))
				
				# Reset the message
				message = []
				
				try:
					message_name = self.get_message_header(line)
				except Exception as e:
					print  "Exception while parsing input file at line " + str(line_num)
					raise e
				
				if(self.message_duplicate_message_name_check(messages, message_name)):
					raise Exception("Found duplicate message name '" + message_name + "' at line " + str(line_num))
				
				found_message_start = 1
				message.append(message_name)
				continue
				
				
				
			if(found_message_start):
				if(line[0] == '}'):
					#if(len(message) == 1):
					#	raise Exception("Message '" + message[0] + "' has no variables")
				
					# Append as new data-type
					SUPPORTED_DATA_TYPES.append([message[0], DATA_TYPE_MESSAGE, 0])
					
					messages.append(message)
					found_message_start = 0
					continue
					
				# Check if we see another '{'
				if('{' in line):
					raise Exception("Found another '{' before closing the former message '" + message[0] + "' with '}' at line " + str(line_num))
					
				try:
					[field_name, field_type, field_data_type, field_array_size, field_data_type_str] = self.get_message_field_attributes(line, defines)
				except Exception as e:
					print  "Exception while parsing input file at line " + str(line_num)
					raise e
					
					
				if(not self.is_valid_var_name(field_name)):	
					raise Exception("Message field-name is not a valid variable name '" + field_name + "' at line " + str(line_num))
				
				if(self.message_duplicate_var_name_check(message, field_name)):
					raise Exception("Duplicate message field found: '" + field_name + "' at line " + str(line_num))
				
				message.append([field_name, field_type, field_data_type, field_array_size, field_data_type_str])
				continue
				
			# Else just append the line to the new lines			
			new_lines.append(l)
			
		if found_message_start:
			raise Exception("Haven't found a closing '}' for message " + message[0])
			
		#print messages
		return [new_lines, messages]			
	
	
	
	# returns [defines, messages]:
	# defines: [['Name1', 'Val'],['Name1', 'Val']]
	# messages: [['MessageName1', ['var_name', field_type, field_data_type_len, array_size, 'data_type_str'], ...], ['MessageName2', [...]]
	def parse(self):
		print "Parsing..."
		lines = self.read_file()
		
		
		[lines, defines] = self.parse_defines(lines)
		[lines, messages] = self.parse_messages(lines, defines)
		
		if(len(lines) > 0):
			raise Exception("Could not parse line " + str(lines[0][0]))
		
		return [defines, messages]

class OutputFile:
	def __init__(self, output_file):
		self.output_file = output_file
		self.file_output = ""
		
	def append_line(self, s = ""):
		self.file_output += s
		self.file_output += "\n"
		
	def write_to_file(self):
		with open(self.output_file, "w") as f:
			f.write(self.file_output)
		
class Protocol_creator:
	def __init__(self, output_format, output_path, output_name, defines, messages):
		self.output_format = output_format
		self.output_path = output_path
		self.output_name = output_name
		self.defines = defines
		self.messages = messages
		
	
	def create(self):
		if(self.output_format == '-c'):
			self.c_create()
		elif(self.output_format == '-python'):
			self.python_create()
		else:
			raise Exception("Output format '" + self.output_format + "' is not supported")
	
	
	
	def search_size_data_type(self, size):
		for i in range(0, len(SUPPORTED_SIZE_DATA_TYPES)):
			for j in range(0, len(SUPPORTED_DATA_TYPES)):
				if(SUPPORTED_SIZE_DATA_TYPES[i] == SUPPORTED_DATA_TYPES[j][0]):
					data_type_str = SUPPORTED_SIZE_DATA_TYPES[i]
					data_type_size = SUPPORTED_DATA_TYPES[j][2]
					if((((2**8)**data_type_size) > size)):
						return [data_type_str, data_type_size]
					
		raise Exception("Not found a valid data-type for size: " + str(size))				
	
	
	def c_get_data_type_mapping(self, data_type_str):
		for i in range(0, len(C_DATA_TYPE_MAPPING)):
			if(C_DATA_TYPE_MAPPING[i][0] == data_type_str):
				return C_DATA_TYPE_MAPPING[i][1]
		
		raise Exception("Data type '" + data_type_str + "' was not found in C_DATA_TYPE_MAPPING")
		
	
	
	def c_search_size_data_type(self, size):
		[data_type_str, data_type_size] = self.search_size_data_type(size)
		
		c_data_type_str = self.c_get_data_type_mapping(data_type_str)
		
		return [c_data_type_str, data_type_size]
		
	
	
	def c_create_struct(self, message):
		self.h_file.append_line("typedef struct {")
		message_name = message[0]
		variables = message[1:]
		for variable in variables:
			field_name = variable[0]
			field_type = variable[1]
			field_data_type_len = variable[2]
			field_array_size = variable[3]
			field_data_type_str = variable[4]

			
			if((field_type & DATA_TYPE_INT) or (field_type & DATA_TYPE_UINT) or (field_type & DATA_TYPE_FLOAT) or (field_type & DATA_TYPE_DOUBLE)):
				field_data_type_str = self.c_get_data_type_mapping(field_data_type_str)
			elif((field_type & DATA_TYPE_MESSAGE)):
				field_data_type_str = field_data_type_str
			else:
				raise Exception ("Field type " + str(field_type) + " is not supported")
			
			if(field_type & FIELD_TYPE_REQUIRED):					
				self.h_file.append_line("\t" + field_data_type_str + " " + field_name + ";")
			elif(field_type & FIELD_TYPE_OPTIONAL):
				self.h_file.append_line("\t" + "uint8_t has_" + field_name + ";")
				self.h_file.append_line("\t" + field_data_type_str + " " + field_name + ";")					
			elif(field_type & FIELD_TYPE_REPEATED):
				[array_size_data_type_str, array_size_data_type_size] = self.c_search_size_data_type(field_array_size)
				self.h_file.append_line("\t" + array_size_data_type_str + " " + field_name + "_count;")
				self.h_file.append_line("\t" + field_data_type_str + " " + field_name + "[" + str(field_array_size) + "];")		
			else:
				raise Excpetion ("Field type " + str(field_type) + " is not supported")
				
		self.h_file.append_line("} " + message_name + ";")
		
	def c_create_message_fields(self, message):
	
		
		message_name = message[0]
		variables = message[1:]
		num_variables = len(variables)
		
		# Declare the field-array in the H-file
		self.h_file.append_line("extern const " + C_FIELD_TYPE_NAME + " " + message_name + "_fields[" + str(num_variables + 1) + "];")
		
		
		# Create the field-arrays in the C-file
		
		self.c_file.append_line("const " + C_FIELD_TYPE_NAME + " " + message_name + "_fields[" + str(num_variables + 1) + "] = {")
		
		for variable in variables:
			field_name = variable[0]
			field_type = variable[1]
			field_data_type_len = variable[2]
			field_array_size = variable[3]
			field_data_type_str = variable[4]
			
			field_ptr = "NULL"
			if((field_type & DATA_TYPE_INT) or (field_type & DATA_TYPE_UINT) or (field_type & DATA_TYPE_FLOAT) or (field_type & DATA_TYPE_DOUBLE)):
				field_ptr = "NULL"
			elif((field_type & DATA_TYPE_MESSAGE)):
				field_ptr = "&" + field_data_type_str + "_fields"
			else:
				raise Exception ("Field type " + str(field_type) + " is not supported")
			
			if(field_type & FIELD_TYPE_REQUIRED):			
				s = "\t{" + str(field_type) + ", ";
				s += C_OFFSETOF_MAKRO_NAME + "(" + message_name + ", " + field_name + "), " 
				s += "0, 0, "
				s += C_MEMBERSIZE_MAKRO_NAME + "(" + message_name + ", " + field_name + "), 0, " + field_ptr + "},"                    
				self.c_file.append_line(s)
			elif(field_type & FIELD_TYPE_OPTIONAL):
				s = "\t{" + str(field_type) + ", ";
				s += C_OFFSETOF_MAKRO_NAME + "(" + message_name + ", " + field_name + "), " 
				s += C_DELTA_MAKRO_NAME + "(" + message_name + ", " + "has_" + field_name + ", " + field_name + "), 1, "
				s += C_MEMBERSIZE_MAKRO_NAME + "(" + message_name + ", " + field_name + "), 0, " + field_ptr + "},"                    
				self.c_file.append_line(s)			
			elif(field_type & FIELD_TYPE_REPEATED):
				[array_size_data_type_str, array_size_data_type_size] = self.c_search_size_data_type(field_array_size)
				
				s = "\t{" + str(field_type) + ", ";
				s += C_OFFSETOF_MAKRO_NAME + "(" + message_name + ", " + field_name + "), " 
				s += C_DELTA_MAKRO_NAME + "(" + message_name + ", " + field_name + "_count" + ", " + field_name + "), " + str(array_size_data_type_size) + ", "
				s += C_MEMBERSIZE_MAKRO_NAME + "(" + message_name + ", " + field_name + "[0]" + "), " 
				s += C_MEMBERSIZE_MAKRO_NAME + "(" + message_name + ", " + field_name + ")/" + C_MEMBERSIZE_MAKRO_NAME + "(" + message_name + ", " + field_name + "[0]" + "), "
				s += field_ptr + "},"                    
				self.c_file.append_line(s)		
				
			else:
				raise Excpetion ("Field type " + str(field_type) + " is not supported")
			
			
		
		self.c_file.append_line("\t" + C_LAST_FIELD_NAME + ",")
		self.c_file.append_line("};")
		
	
	def c_create(self):
		self.c_file = OutputFile(self.output_path + "/" + self.output_name + ".c")
		self.h_file = OutputFile(self.output_path + "/" + self.output_name + ".h")
	
		print "Creating C/H-Files..."
		
		# First write the include-pattern for header files:
		self.h_file.append_line("#ifndef " + "__" + self.output_name.upper() + "_H")
		self.h_file.append_line("#define " + "__" + self.output_name.upper() + "_H")
		self.h_file.append_line()
		
		# Then write the imports
		self.h_file.append_line("#include <stdint.h>")
		self.h_file.append_line('#include "tinybuf.h"')
		self.h_file.append_line()
		
		self.c_file.append_line('#include "tinybuf.h"')
		self.c_file.append_line('#include "' + self.output_name + ".h" + '"')
		self.c_file.append_line()
		
		# First create the defines
		for define in self.defines:
			self.h_file.append_line("#define " + define[0] + " " + define[1])
		self.h_file.append_line()
			
		# Then create the structs from the messages
		for message in self.messages:
			self.c_create_struct(message)
			self.h_file.append_line()
			
		# Then create the field-arrays from the messages
		for message in self.messages:
			self.c_create_message_fields(message)
			self.c_file.append_line()
		
		# Finally close the header-file with #endif 
		self.h_file.append_line()
		self.h_file.append_line("#endif")
		
		
		#print "H-File:"
		#print self.h_file.file_output
		
		#print "C-File:"
		#print self.c_file.file_output
		
		self.c_file.write_to_file()
		self.h_file.write_to_file()
	
	def python_create_field_declaration(self, field_type, field_name):
		# Add has_'field' when optional field
		if(field_type & FIELD_TYPE_OPTIONAL):
			self.python_file.append_line("\t\tself." + "has_" + field_name + " = 0")
		
		if((field_type & DATA_TYPE_INT) or (field_type & DATA_TYPE_UINT) or (field_type & DATA_TYPE_FLOAT) or (field_type & DATA_TYPE_DOUBLE)):
			if(field_type & FIELD_TYPE_REPEATED):
				self.python_file.append_line("\t\tself." + field_name + " = []")
			elif(field_type & FIELD_TYPE_REQUIRED) or (field_type & FIELD_TYPE_OPTIONAL):
				self.python_file.append_line("\t\tself." + field_name + " = 0")
			else:
				raise Exception ("Field type " + str(field_type) + " is not supported")
			
			
		elif((field_type & DATA_TYPE_MESSAGE)):
			if(field_type & FIELD_TYPE_REPEATED):
				self.python_file.append_line("\t\tself." + field_name + " = []")
			elif(field_type & FIELD_TYPE_REQUIRED) or (field_type & FIELD_TYPE_OPTIONAL):
				self.python_file.append_line("\t\tself." + field_name + " = None")
			else:
				raise Exception ("Field type " + str(field_type) + " is not supported")
		else:
			raise Exception ("Field type " + str(field_type) + " is not supported")
	
	def python_get_data_type_mapping(self, data_type_str):
		for i in range(0, len(PYTHON_DATA_TYPE_MAPPING)):
			if(PYTHON_DATA_TYPE_MAPPING[i][0] == data_type_str):
		#		for j in range(0, len(SUPPORTED_DATA_TYPES)):
		#			if(PYTHON_DATA_TYPE_MAPPING[i][0] == SUPPORTED_DATA_TYPES[j][0]):
				return PYTHON_DATA_TYPE_MAPPING[i][1]
				
		raise Exception("Data type '" + data_type_str + "' was not found in PYTHON_DATA_TYPE_MAPPING")
		
	
	
	def python_create_encode_functions(self, variables):
		# Create the encode function
		self.python_file.append_line()
		self.python_file.append_line("\tdef encode(self):")
		self.python_file.append_line("\t\tostream = _Ostream()")
		self.python_file.append_line("\t\tself.encode_internal(ostream)")
		self.python_file.append_line("\t\treturn ostream.buf")
		
		# Create the encode_internal-function
		self.python_file.append_line()
		self.python_file.append_line("\tdef encode_internal(self, ostream):")
		for variable in variables:
			field_name = variable[0]			
			self.python_file.append_line("\t\tself.encode_" + field_name + "(ostream)")
		self.python_file.append_line("\t\tpass")	# Added this if there is an Empty message, with no fields at all
		
		# Create all the encode-functions for the variables
		self.python_file.append_line()
		for variable in variables:
			field_name = variable[0]
			field_type = variable[1]
			field_array_size = variable[3]
			field_data_type_str = variable[4]	
			
			
			
			self.python_file.append_line("\tdef encode_" + field_name + "(self, ostream):")
			
			
			
			if(field_type & FIELD_TYPE_OPTIONAL): # write has_'field' as one byte 
				self.python_file.append_line("\t\t" + "ostream.write(" + "struct.pack('" + self.python_get_data_type_mapping('uint8') + "', " + "self.has_" + field_name + "))")
				self.python_file.append_line("\t\t" + "if self.has_" + field_name + ":")
				
				if((field_type & DATA_TYPE_INT) or (field_type & DATA_TYPE_UINT) or (field_type & DATA_TYPE_FLOAT) or (field_type & DATA_TYPE_DOUBLE)):
					self.python_file.append_line("\t\t\t" + "ostream.write(" + "struct.pack('" + self.python_get_data_type_mapping(field_data_type_str) + "', " + "self." + field_name + "))")
				elif(field_type & DATA_TYPE_MESSAGE):
					self.python_file.append_line("\t\t\t" + "self." + field_name + ".encode_internal(ostream)")
				else:
					raise Exception ("Field type " + str(field_type) + " is not supported")
					
			elif(field_type & FIELD_TYPE_REPEATED): # write array-size
				[size_data_type_str, size_data_type_size] = self.search_size_data_type(field_array_size)
				
				self.python_file.append_line("\t\t" + "count = len(" + "self." + field_name + ")")
				self.python_file.append_line("\t\t" + "ostream.write(" + "struct.pack('" + self.python_get_data_type_mapping(size_data_type_str) + "', " + "count" + "))")
				self.python_file.append_line("\t\t" + "for i in range(0, " + "count" + "):")
				
				if((field_type & DATA_TYPE_INT) or (field_type & DATA_TYPE_UINT) or (field_type & DATA_TYPE_FLOAT) or (field_type & DATA_TYPE_DOUBLE)):
					self.python_file.append_line("\t\t\t" + "ostream.write(" + "struct.pack('" + self.python_get_data_type_mapping(field_data_type_str) + "', " + "self." + field_name + "[i]))")
				elif(field_type & DATA_TYPE_MESSAGE):
					self.python_file.append_line("\t\t\t" + "self." + field_name + "[i].encode_internal(ostream)" )
				else:
					raise Exception ("Field type " + str(field_type) + " is not supported")
					
			elif(field_type & FIELD_TYPE_REQUIRED): 
				if((field_type & DATA_TYPE_INT) or (field_type & DATA_TYPE_UINT) or (field_type & DATA_TYPE_FLOAT) or (field_type & DATA_TYPE_DOUBLE)):
					self.python_file.append_line("\t\t" + "ostream.write(" + "struct.pack('" + self.python_get_data_type_mapping(field_data_type_str) + "', " + "self." + field_name + "))")
				elif(field_type & DATA_TYPE_MESSAGE):
					self.python_file.append_line("\t\t" + "self." + field_name + ".encode_internal(ostream)")
				else:
					raise Exception ("Field type " + str(field_type) + " is not supported")
			else:
					raise Exception ("Field type " + str(field_type) + " is not supported")
			
			self.python_file.append_line()
	
	
	def python_create_decode_functions(self, variables):
	
		
		# Create the decode function
		self.python_file.append_line()
		self.python_file.append_line("\t@classmethod")
		self.python_file.append_line("\tdef decode(cls, buf):")
		self.python_file.append_line("\t\tobj = cls()")
		self.python_file.append_line("\t\tobj.decode_internal(_Istream(buf))")
		self.python_file.append_line("\t\treturn obj")
		
		
		# Create the decode_internal-function
		self.python_file.append_line()
		self.python_file.append_line("\tdef decode_internal(self, istream):")
		self.python_file.append_line("\t\tself.reset()")
		for variable in variables:
			field_name = variable[0]			
			self.python_file.append_line("\t\tself.decode_" + field_name + "(istream)")
		
		# Create all the encode-functions for the variables
		self.python_file.append_line()
		for variable in variables:
			field_name = variable[0]
			field_type = variable[1]
			field_data_type_len = variable[2]
			field_array_size = variable[3]
			field_data_type_str = variable[4]	
			
			self.python_file.append_line("\tdef decode_" + field_name + "(self, istream):")
			
			
			
			if(field_type & FIELD_TYPE_OPTIONAL): # read has_'field' as one byte 
				self.python_file.append_line("\t\t" + "self.has_" + field_name + "= struct.unpack('" + self.python_get_data_type_mapping('uint8') + "', " + "istream.read(" + "1" + "))[0]" )
				self.python_file.append_line("\t\t" + "if self.has_" + field_name + ":")
				
				if((field_type & DATA_TYPE_INT) or (field_type & DATA_TYPE_UINT) or (field_type & DATA_TYPE_FLOAT) or (field_type & DATA_TYPE_DOUBLE)):
					self.python_file.append_line("\t\t\t" + "self." + field_name + "= struct.unpack('" + self.python_get_data_type_mapping(field_data_type_str) + "', "  + "istream.read(" + str(field_data_type_len) + "))[0]")
				elif(field_type & DATA_TYPE_MESSAGE):
					self.python_file.append_line("\t\t\t" + "self." + field_name + " = " + field_data_type_str + "()")
					self.python_file.append_line("\t\t\t" + "self." + field_name + ".decode_internal(istream)")
				else:
					raise Exception ("Field type " + str(field_type) + " is not supported")
					
			elif(field_type & FIELD_TYPE_REPEATED): # write array-size	
			
				[size_data_type_str, size_data_type_size] = self.search_size_data_type(field_array_size)
				
				self.python_file.append_line("\t\t" + "count = struct.unpack('" + self.python_get_data_type_mapping(size_data_type_str) + "', " + "istream.read(" + str(size_data_type_size) + "))[0]" )
				self.python_file.append_line("\t\t" + "for i in range(0, " + "count" + "):")
				
				
				if((field_type & DATA_TYPE_INT) or (field_type & DATA_TYPE_UINT) or (field_type & DATA_TYPE_FLOAT) or (field_type & DATA_TYPE_DOUBLE)):
					self.python_file.append_line("\t\t\t" + "self." + field_name + ".append(struct.unpack('" + self.python_get_data_type_mapping(field_data_type_str) + "', " + "istream.read(" +  str(field_data_type_len) + "))[0])")
				elif(field_type & DATA_TYPE_MESSAGE):
					self.python_file.append_line("\t\t\t" + "tmp" + " = " + field_data_type_str + "()")
					self.python_file.append_line("\t\t\t" + "tmp.decode_internal(istream)")
					self.python_file.append_line("\t\t\t" + "self." + field_name + ".append(tmp)")
				else:
					raise Exception ("Field type " + str(field_type) + " is not supported")
					
					
			elif(field_type & FIELD_TYPE_REQUIRED): 
				if((field_type & DATA_TYPE_INT) or (field_type & DATA_TYPE_UINT) or (field_type & DATA_TYPE_FLOAT) or (field_type & DATA_TYPE_DOUBLE)):
					self.python_file.append_line("\t\t" + "self." + field_name + "= struct.unpack('" + self.python_get_data_type_mapping(field_data_type_str) + "', "  + "istream.read(" + str(field_data_type_len) + "))[0]")
				elif(field_type & DATA_TYPE_MESSAGE):
					self.python_file.append_line("\t\t" + "self." + field_name + " = " + field_data_type_str + "()")
					self.python_file.append_line("\t\t" + "self." + field_name + ".decode_internal(istream)")
				else:
					raise Exception ("Field type " + str(field_type) + " is not supported")
			else:
					raise Exception ("Field type " + str(field_type) + " is not supported")
			
			self.python_file.append_line()
	
	
	def python_create_bytestring_functions(self, variables):
		for variable in variables:
			field_name = variable[0]
			field_type = variable[1]
			field_data_type_len = variable[2]
			field_array_size = variable[3]
			
			# Check if variable is an array and has data_type_len of 1 and unsigned, so we could represent it as bytestring
			if((field_array_size > 0) and (field_data_type_len == 1) and (field_type & DATA_TYPE_UINT)):
				# First create a function to encode the array to a bytestring
				self.python_file.append_line("\tdef " + "get_" + field_name + "_as_bytestring(self):")
				self.python_file.append_line("\t\tbytestring = b''")
				self.python_file.append_line("\t\tfor b in " + "self." + field_name + ":")
				self.python_file.append_line("\t\t\tbytestring += struct.pack('>B', b)")
				self.python_file.append_line("\t\treturn bytestring")
				self.python_file.append_line()
				
				# Then create a function to decode the bytestring to an array
				self.python_file.append_line("\tdef " + "set_" + field_name + "_as_bytestring(self, bytestring):")
				self.python_file.append_line("\t\t" + "self." + field_name + " = []")
				self.python_file.append_line("\t\tfor b in bytestring:")
				self.python_file.append_line("\t\t\t" + "self." + field_name + ".append(struct.unpack('>B', b)[0])")
				self.python_file.append_line()
				
				
			
			
			
	def python_create_class(self, message):
		message_name = message[0]
		variables = message[1:]
		num_variables = len(variables)

		self.python_file.append_line("class " + message_name + ":")
		self.python_file.append_line()
		self.python_file.append_line("\tdef __init__(self):")
		self.python_file.append_line("\t\tself.reset()")
		self.python_file.append_line()
		self.python_file.append_line("\tdef reset(self):")
		
		for variable in variables:
			field_name = variable[0]
			field_type = variable[1]
			self.python_create_field_declaration(field_type, field_name)
		self.python_file.append_line("\t\tpass")	# Added this if there is an Empty message, with no fields at all
		
		
		self.python_create_encode_functions(variables)
		
		self.python_create_decode_functions(variables)

		self.python_create_bytestring_functions(variables)
		
		
			
	
	def python_create(self):
		self.python_file = OutputFile(self.output_path + "/" + self.output_name + ".py")
		
		print "Creating python-file..."
		
		self.python_file.append_line("import struct")
		self.python_file.append_line()
		
		# First create the defines
		for define in self.defines:
			self.python_file.append_line(define[0] + " = " + define[1])
		self.python_file.append_line()
		
		# Create Ostream-class
		self.python_file.append_line("class _Ostream:")
		self.python_file.append_line("\tdef __init__(self):")
		self.python_file.append_line("\t\tself.buf = b''")
		self.python_file.append_line("\tdef write(self, data):")
		self.python_file.append_line("\t\tself.buf += data")
		self.python_file.append_line()
		
		# Create Istream-class
		self.python_file.append_line("class _Istream:")
		self.python_file.append_line("\tdef __init__(self, buf):")
		self.python_file.append_line("\t\tself.buf = buf")
		self.python_file.append_line("\tdef read(self, l):")
		self.python_file.append_line("\t\tif(l > len(self.buf)):")
		self.python_file.append_line('\t\t\traise Exception("Not enough bytes in Istream to read")')
		self.python_file.append_line("\t\tret = self.buf[0:l]")
		self.python_file.append_line("\t\tself.buf = self.buf[l:]")
		self.python_file.append_line("\t\treturn ret")
		self.python_file.append_line()

		
		# Then create the python-classes from the messages
		for message in self.messages:
			self.python_create_class(message)
			self.python_file.append_line()
		
		#print "Python file:"
		#print self.python_file.file_output
		
		
		self.python_file.write_to_file()

if __name__ == '__main__':
	if(len(sys.argv) < 5):
		raise Exception("Script has to be called with: " + sys.argv[0] + " " + str(SUPPORTED_OUTPUT_FORMATS) + " 'protocol-file' 'output-path' 'output-file name'")

	output_format = sys.argv[1]
	file = sys.argv[2]
	output_path = sys.argv[3]
	output_name = sys.argv[4]
	
	print "Output name"
	print output_name
	
	if not any(output_format == s for s in SUPPORTED_OUTPUT_FORMATS):
		raise Exception("Output format '"+ output_format + "' not supported. Take one of these: " +  str(SUPPORTED_OUTPUT_FORMATS))
	if(not os.path.isfile(file)):
		raise Exception("Protocol-file '" + file + "' does not exist.")
		
	protocol_parser = Protocol_parser(file)
	[defines, messages] = protocol_parser.parse()
	protocol_creator = Protocol_creator(output_format, output_path, output_name, defines, messages)
	protocol_creator.create()
	
