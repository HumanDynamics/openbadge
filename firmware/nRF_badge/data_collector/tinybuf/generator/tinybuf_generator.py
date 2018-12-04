
import struct
import sys
import os.path
import re
import traceback



SUPPORTED_OUTPUT_FORMATS = ['-c', '-python']	

BIG_ENDIANNESS = 0
LITTLE_ENDIANNESS = 1
	
SUPPORTED_ENDIANNESS = ['-be', '-le']	




FIELD_RULE_REQUIRED 		= 1
FIELD_RULE_OPTIONAL 		= 2
FIELD_RULE_REPEATED 		= 4
FIELD_RULE_FIXED_REPEATED 	= 8
FIELD_RULE_ONEOF 			= 16
FIELD_TYPE_INT 				= 32
FIELD_TYPE_UINT 			= 64
FIELD_TYPE_FLOAT 			= 128
FIELD_TYPE_DOUBLE 			= 256
FIELD_TYPE_MESSAGE 			= 512
	

# The supported field types. Messages/extern messages are added to this list, so that subsequent messages can use this message as field type.
SUPPORTED_FIELD_TYPES = ['uint8', 'int8', 'uint16', 'int16', 'uint32', 'int32', 'uint64', 'int64', 'float', 'double']


				
Imports = [] 	# Array of imports
Defines = []	# Array of defines
Messages = [] 	# Array of messages



class Block:
	def __init__(self):
		self.lines = []
		self.line_numbers = []
		self.iterator_index = 0
	
	def setup_iterator(self):
		self.iterator_index = 0
		
	def has_next(self):
		if(self.iterator_index + 1 > len(self.lines)):
			return False
		return True
		
	def get_next(self):
		if(not self.has_next()):
			raise Exception("get_next() has no next element")

		ret = [self.line_numbers[self.iterator_index], self.lines[self.iterator_index]]
		self.iterator_index = self.iterator_index + 1
		
		return ret
		
	def extract_block(self, line_number):	# returns a new Block object with this line-number as start, and ends with "}", or None if no Block was found
		index = 0
		
		for i in range(0, len(self.lines)):
			if(self.line_numbers[i] == line_number):
				if(not '{' in self.lines[i]):
					return None
				
				index = i		
		
		block = Block()
		
		bracket_open_counter = 0
		for i in range(index, len(self.lines)):	
			if('{' in self.lines[i]):
				bracket_open_counter = bracket_open_counter + 1
			if('}' in self.lines[i]):
				bracket_open_counter = bracket_open_counter - 1

				
			block.lines.append(self.lines[i])
			block.line_numbers.append(self.line_numbers[i])
			
			
			if(bracket_open_counter == 0):
				return block
			

		return None
		
	def remove_lines(self, line_numbers):	
		
		for j in range(0, len(line_numbers)):
			for i in range(0, len(self.lines)):
				if(line_numbers[j] == self.line_numbers[i]):
					self.setup_iterator()
					del self.lines[i]
					del self.line_numbers[i]
					break
			
		

class File(Block):
	# These special characters should be seperated from other characters
	SPECIAL_CHARACTERS = [';', '{', '}', '[' , ']', '(' , ')']
	
	# returns [line_number, ['A','B',...]]
	def __init__(self, file_name):
		self.file_name = file_name
		Block.__init__(self)
		
		
		with open(self.file_name) as f:
			content = f.readlines()
			
		# you may also want to remove whitespace characters like `\n` at the end of each line
		content = [x.strip() for x in content] 
		
		line_number = 0
		for line in content:
			
			# prepare the string to split all the special characters:
			for c in File.SPECIAL_CHARACTERS:
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
				self.lines.append(rm)
				self.line_numbers.append(line_number)
	
	
		
	
def check_format(line, format, format_required):
	if(len(line) != len(format)):
		return False
	for i in range(0, len(line)):
		if(format_required[i]):
			if(not format[i] == line[i]):
				return False
	return True
	

def name_valid(field_name):
	# TODO: Add more checks here
	if(field_name[0].isdigit()):
		return False
	return True

				
class Define:
	def __init__(self, name, number):
		self.name = name
		self.number = number
		
	def __repr__(self):
		return "Define" + str(self.__dict__)	
		
	@classmethod
	def get_define(cls, line):
		format = ["'define_name'", "=",  "'Integer'", ";"]
		format_required = [0, 1, 0, 1]
		
		if(not check_format(line, format, format_required)): 
			raise Exception('Expects required field format ' + str(format) + "\nBut given is: " + str(line))
			
		define_name = line[0]
		if(not Define.define_name_valid(define_name)):
			raise Exception('Unsupported define name ' + define_name)
			
		define_value = int(line[2])
		
		define = Define(define_name, define_value)
		
		# Check for duplicate define:
		
		for d in Defines:
			if(define.name == d.name):
				raise Exception("Duplicate define name " + define.name)
		
		
		
		return define
		
	@classmethod
	def define_name_valid(cls, define_name):
		return name_valid(define_name)
		
		
		
class Field:	
	
	def __init__(self, name):
		self.name = name
		
	@classmethod
	def field_type_supported(cls, field_type):
		for f in SUPPORTED_FIELD_TYPES:
			if(f == field_type):
				return True
		return False
		
	@classmethod
	def field_name_valid(cls, field_name):
		return name_valid(field_name)
		
		
	@classmethod
	def get_integer(cls, integer_str):
		# Check if integer_str is a number, or if it is already in a define?
		
		if(integer_str.isdigit()):
			return int(integer_str)
		else:
			for define in Defines:
				if(integer_str == define.name):
					return define.number
			
		return None
		

class RequiredField(Field):
	def __init__(self, name, type):
		Field.__init__(self, name)
		self.type = type
	
	def get_name(self):
		names = [self.name]
		return names
		
	def __repr__(self):
		return "RequiredField" + str(self.__dict__)
	
	@classmethod
	def get_field(cls, line):
		if(line[0] == 'required'):
			
			format = ["required", "'field_type'", "'field_name'", ";"]					
			format_required = [1, 0, 0, 1]		
			if(not check_format(line, format, format_required)): 
				raise Exception('Expects required field format ' + str(format) + "\nBut given is: " + str(line))
			
			field_type = line[1]
			if(not Field.field_type_supported(field_type)):
				raise Exception('Unsupported field type ' + field_type)
			
			
			field_name = line[2]
			if(not Field.field_name_valid(field_name)):
				raise Exception('Unsupported field name ' + field_name)
				
			
			
			return RequiredField(field_name, field_type)
			
		else:
			return None
			
		
class RepeatedField(Field):
	def __init__(self, name, type, size):
		Field.__init__(self, name)
		self.type = type
		self.size = size
		
	def get_name(self):
		names = [self.name]
		return names
		
	def __repr__(self):
		return "RepeatedField" + str(self.__dict__)
		
		
	@classmethod
	def get_field(cls, line):
		if(line[0] == 'repeated'):
			
			format = ["repeated", "'field_type'", "'field_name'", "[", "'array_size'", "]", ";"]
			format_required = [1, 0, 0, 1, 0, 1, 1]
			if(not check_format(line, format, format_required)): 
				raise Exception('Expects repeated field format ' + str(format) + "\nBut given is: " + str(line))
			
			field_type = line[1]
			if(not Field.field_type_supported(field_type)):
				raise Exception('Unsupported field type ' + field_type)
			
			
			field_name = line[2]
			if(not Field.field_name_valid(field_name)):
				raise Exception('Unsupported field name ' + field_name)
			
			array_size_str = line[4]
			array_size = Field.get_integer(array_size_str)
			if(array_size == None):
				raise Exception('Unsupported array size ' + array_size_str)
			
			
			return RepeatedField(field_name, field_type, array_size)
			
		else:
			return None
			
			
class FixedRepeatedField(Field):
	def __init__(self, name, type, size):
		Field.__init__(self, name)
		self.type = type
		self.size = size
		
	def get_name(self):
		names = [self.name]
		return names
		
	def __repr__(self):
		return "FixedRepeatedField" + str(self.__dict__)
		
		
	@classmethod
	def get_field(cls, line):
		if(line[0] == 'fixed_repeated'):
			
			format = ["fixed_repeated", "'field_type'", "'field_name'", "[", "'array_size'", "]", ";"]
			format_required = [1, 0, 0, 1, 0, 1, 1]
			if(not check_format(line, format, format_required)): 
				raise Exception('Expects fixed repeated field format ' + str(format) + "\nBut given is: " + str(line))
			
			field_type = line[1]
			if(not Field.field_type_supported(field_type)):
				raise Exception('Unsupported field type ' + field_type)
			
			
			field_name = line[2]
			if(not Field.field_name_valid(field_name)):
				raise Exception('Unsupported field name ' + field_name)
			
			array_size_str = line[4]
			array_size = Field.get_integer(array_size_str)
			if(array_size == None):
				raise Exception('Unsupported array size ' + array_size_str)
			
			
			return FixedRepeatedField(field_name, field_type, array_size)
			
		else:
			return None
		
class OptionalField(Field):
	def __init__(self, name, type):
		Field.__init__(self, name)
		self.type = type
	
	def get_name(self):
		names = [self.name]
		return names
		
	def __repr__(self):
		return "OptionalField" + str(self.__dict__)
	
	@classmethod
	def get_field(cls, line):
		if(line[0] == 'optional'):
			
			format = ["optional", "'field_type'", "'field_name'", ";"]					
			format_required = [1, 0, 0, 1]		
			if(not check_format(line, format, format_required)): 
				raise Exception('Expects optional field format ' + str(format) + "\nBut given is: " + str(line))
			
			field_type = line[1]
			if(not Field.field_type_supported(field_type)):
				raise Exception('Unsupported field type ' + field_type)
			
			
			field_name = line[2]
			if(not Field.field_name_valid(field_name)):
				raise Exception('Unsupported field name ' + field_name)
				
			
			
			return OptionalField(field_name, field_type)
			
		else:
			return None
			
class OneofInnerField(Field):
	def __init__(self, name, type, tag):
		Field.__init__(self, name)
		self.type = type
		self.tag = tag
	
	def __repr__(self):
		return  "OneofInnerField" + str(self.__dict__)
	
	@classmethod
	def get_field(cls, line):
		format = ["'field_type'", "'field_name'", "(", "'oneof_tag'" , ")", ";"]					
		format_required = [0, 0, 1, 0, 1, 1]		
		if(not check_format(line, format, format_required)): 
			raise Exception('Expects oneof field format ' + str(format) + "\nBut given is: " + str(line))
		
		field_type = line[0]
		if(not Field.field_type_supported(field_type)):
			raise Exception('Unsupported field type ' + field_type)
		
		
		field_name = line[1]
		if(not Field.field_name_valid(field_name)):
			raise Exception('Unsupported field name ' + field_name)
			
		oneof_tag_str = line[3]
		oneof_tag = Field.get_integer(oneof_tag_str)
		if(oneof_tag == None):
			raise Exception('Unsupported oneof tag ' + oneof_tag_str)
			
			
		return OneofInnerField(field_name, field_type, oneof_tag)
	
	
class OneofField(Field):
	def __init__(self, name):
		Field.__init__(self, name)
		self.inner_fields = []
		
	def get_name(self):
		names = []
		names.append(self.name)
		for f in self.inner_fields:
			names.append(f.name)
		
		return names
		
	def __repr__(self):
		return  "OneofField" +  str(self.__dict__)
		
	@classmethod
	def is_field(cls, line):
		if(line[0] == 'oneof'):
			
			format = ["oneof", "'field_name'", "{"]					
			format_required = [1, 0, 1]		
			if(not check_format(line, format, format_required)): 
				raise Exception('Expects oneof field format ' + str(format) + "\nBut given is: " + str(line))
			
			
			field_name = line[1]
			if(not Field.field_name_valid(field_name)):
				raise Exception('Unsupported field name ' + field_name)
				
			return True			
		else:
			return False
	
	@classmethod
	def get_field(cls, block):	# returns a oneof field instance from a block
		
		if(not OneofField.is_field(block.lines[0])): # Just for safety, check if block starts with a oneof-field declaration
			raise Exception('Is no oneof field')
			
		
		oneof_field_name = block.lines[0][1]
		oneof_field = OneofField(oneof_field_name)		
		
		
		for i in range(1, len(block.lines) - 1):
			
			line = block.lines[i]
			
			oneof_inner_field = OneofInnerField.get_field(line)
			#print(oneof_inner_field)
			
			
			# Check oneof-field tag duplicates
			
			for inner_field in oneof_field.inner_fields:
				if(inner_field.tag == oneof_inner_field.tag):
					raise Exception('Duplicate oneof tag: ' + str(oneof_inner_field.tag))
			
			oneof_field.inner_fields.append(oneof_inner_field)
			
		
		return oneof_field
		
	
class Message:
	def __init__(self, name):
		self.name = name
		self.fields = []	
		
	def __repr__(self):
		return str(self.__dict__)
	
	@classmethod
	def get_message(cls, line):
		format = ["message", "'message_name'", "{"]					
		format_required = [1, 0, 1]		
		if(not check_format(line, format, format_required)): 
			raise Exception('Expects message format ' + str(format) + "\nBut given is: " + str(line))
			
		
		message_name = line[1]
		if(not name_valid(message_name)):
			raise Exception('Unsupported message name ' + message_name)
			
		message = Message(message_name)
		
		# Add the message to the SUPPORTED_FIELD_TYPES, check duplicate names
		for m in SUPPORTED_FIELD_TYPES:
			if(m == message_name):
				raise Exception("Duplicate message " + m)
				
		SUPPORTED_FIELD_TYPES.append(message_name)
		
		return message
		
		
	def add_field(self, field):
		
		# Check all field names before appending it
		names = []
		for f in self.fields:
			for name in f.get_name():
				names.append(name)
		
		for name in field.get_name():
			if(name in names):
				raise Exception("Field name duplicate: " + name)
				
		self.fields.append(field)
		
		
class ExternMessage:
	def __init__(self, name):
		self.name = name	
	
	def __repr__(self):
		return str(self.__dict__)
	
	@classmethod
	def get_extern_message(cls, line):
		format = ["extern", "'extern_message_name'", ";"]					
		format_required = [1, 0, 1]		
		if(not check_format(line, format, format_required)): 
			raise Exception('Expects extern message format ' + str(format) + "\nBut given is: " + str(line))
			
		
		extern_message_name = line[1]
		if(not name_valid(extern_message_name)):
			raise Exception('Unsupported extern message name ' + extern_message_name)
		
		extern_message = ExternMessage(extern_message_name)
		
		
		# Add the message to the SUPPORTED_FIELD_TYPES, check duplicate names
		for m in SUPPORTED_FIELD_TYPES:
			if(m == extern_message_name):
				raise Exception("Duplicate message " + m)
		SUPPORTED_FIELD_TYPES.append(extern_message_name)
			
		return extern_message
		
		
class Import:
	def __init__(self, name):
		self.name = name	
	
	def __repr__(self):
		return "Import" + str(self.__dict__)
	
	@classmethod
	def get_import(cls, line):
		format = ["import", "'import name'"]					
		format_required = [1, 0]		
		if(not check_format(line, format, format_required)): 
			raise Exception('Expects import format ' + str(format) + "\nBut given is: " + str(line))
			
		
		import_name = line[1]
		if(not name_valid(import_name)):
			raise Exception('Unsupported import name ' + import_name)
		
		imp = Import(import_name)
		
		
		# Add the message to the SUPPORTED_FIELD_TYPES, check duplicate names
		for i in Imports:
			if(i.name == imp.name):
				raise Exception("Duplicate import " + imp.name)

			
		return imp
	

		
		
class Parser:
	def __init__(self, file_name):
		self.File = File(file_name)
		
		self.parse_imports()
		self.parse_extern_messages()
		self.parse_defines()
		self.parse_messages()
		
		
	def parse_imports(self):
		self.File.setup_iterator()
		while(self.File.has_next()):
			[line_number, line] = self.File.get_next()
			if(line[0] == 'import'):
				self.File.remove_lines([line_number])
				
				try:
					# Return value is not needed..
					imp = Import.get_import(line)
					Imports.append(imp)
				except Exception as e:
					print("Exception at line " + str(line_number) + ":")
					print(e)
				
	def parse_extern_messages(self):
		self.File.setup_iterator()
		while(self.File.has_next()):
			[line_number, line] = self.File.get_next()
			if(line[0] == 'extern'):
				self.File.remove_lines([line_number])
				try:
					# Return value is not needed..
					ExternMessage.get_extern_message(line)
				except Exception as e:
					print("Exception at line " + str(line_number) + ":")
					print(e)
					

			
	def parse_defines(self):
		self.File.setup_iterator()
		while(self.File.has_next()):
			[line_number, line] = self.File.get_next()
			if(line[0] == 'define'):
				block = self.File.extract_block(line_number)
				if(not block == None):
					self.File.remove_lines(block.line_numbers)
					
					
					for i in range(1, len(block.lines) - 1): # ignore lines with "define {" and "}"
						try:
							# Return value is not needed...
							define = Define.get_define(block.lines[i])
							Defines.append(define)							
						except Exception as e:
							print("Exception at line " + str(block.line_numbers[i]) + ":")
							print(e)
							print(traceback.format_exc())

							
						
				else:
					raise Exception('Could not parse define at line ' + str(line_number))
			
	
	def parse_messages(self):
		self.File.setup_iterator()
		while(self.File.has_next()):
			[line_number, line] = self.File.get_next()
			if(line[0] == 'message'):
				
				# Create new message object
				message = Message.get_message(line)

				
				# Extract message block
				block = self.File.extract_block(line_number)
				if(not block == None):
					self.File.remove_lines(block.line_numbers)			
					
					# iterate through the message block
					block.setup_iterator()
					while(block.has_next()):
						[block_line_number, block_line] = block.get_next()
						
						try:
							# Check different fields
							field = RequiredField.get_field(block_line)
							if(not field == None):
								message.add_field(field)
								block.remove_lines([block_line_number])
								continue
								
								
							field = RepeatedField.get_field(block_line)
							if(not field == None):
								message.add_field(field)
								block.remove_lines([block_line_number])
								continue
								
							field = FixedRepeatedField.get_field(block_line)
							if(not field == None):
								message.add_field(field)
								block.remove_lines([block_line_number])
								continue
								
							field = OptionalField.get_field(block_line)
							if(not field == None):
								message.add_field(field)
								block.remove_lines([block_line_number])
								continue
							
							
							if(OneofField.is_field(block_line)):	# check if oneof-field
								oneof_block = block.extract_block(block_line_number)
								field = OneofField.get_field(oneof_block)
								message.add_field(field)
								block.remove_lines(oneof_block.line_numbers)
								continue
							
							
							
						except Exception as e:
							print("Exception at line " + str(block_line_number) + ":")
							print(e)
							print(traceback.format_exc())
							sys.exit()		
					
					# Add message to Messages
					Messages.append(message)			
					
					
				else:
					raise Exception('Could not parse message at line ' + str(line_number))
					sys.exit()		


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
			

def search_size_type(size):
	size_types =   [["uint8",  1], ["uint16", 2], ["uint32", 4], ["uint64", 8]]
					
	for i in range(0, len(size_types)):
		if((((2**8)**size_types[i][1]) > size)):
			return [size_types[i][0], size_types[i][1]]

	raise Exception("Not found a valid size-type for size: " + str(size))
	


class C_Creator:
	def __init__(self, output_path, output_name, imports, defines, messages):
		self.output_path = output_path
		self.output_name = output_name
		self.imports = imports
		self.defines = defines
		self.messages = messages
		
		self.create()
		
	def create(self):
		print "Creating C/H-Files..."
		
		self.c_file = OutputFile(self.output_path + "/" + self.output_name + ".c")
		self.h_file = OutputFile(self.output_path + "/" + self.output_name + ".h")
		
		# First write the include-pattern for header files:
		self.h_file.append_line("#ifndef " + "__" + self.output_name.upper() + "_H")
		self.h_file.append_line("#define " + "__" + self.output_name.upper() + "_H")
		self.h_file.append_line()
		
		# Then write the imports
		self.h_file.append_line("#include <stdint.h>")
		self.h_file.append_line('#include "tinybuf.h"')		
		self.c_file.append_line('#include "tinybuf.h"')
		self.c_file.append_line('#include "' + self.output_name + ".h" + '"')
		self.c_file.append_line()
		
		# First create the imports
		for imp in self.imports:
			self.h_file.append_line('#include "' + imp.name + '.h"')
		self.h_file.append_line()
		
		# Then create the defines
		for define in self.defines:
			self.h_file.append_line("#define " + define.name + " " + str(define.number))
		self.h_file.append_line()
		
		# Then create the oneof_tags from the messages
		for message in self.messages:
			self.create_oneof_tags(message)
		self.h_file.append_line()
				
		# Then create the structs from the messages
		for message in self.messages:
			self.create_struct(message)
			self.h_file.append_line()
	
		
		
		# Then create the field-arrays from the messages
		for message in self.messages:
			self.create_message_fields(message)
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
		
		
	def create_oneof_tags(self, message):
		for field in message.fields:
			if(isinstance(field, OneofField)): # Is oneof field
				for inner_field in field.inner_fields:
					self.h_file.append_line("#define " + message.name + "_" + inner_field.name +  "_tag " + str(inner_field.tag))
					
	def get_field_type_mapping(self, field_type):
		field_type_mapping = {"uint8": "uint8_t", "int8": "int8_t", "uint16": "uint16_t", "int16": "int16_t", 
							  "uint32": "uint32_t", "int32": "int32_t", "uint64": "uint64_t", "int64": "int64_t", 
							  "float": "float", "double": "double"}
					
		if field_type in field_type_mapping:
			return field_type_mapping[field_type]
		else:	# For example in the case of message as field type
			return field_type
			
	def get_field_type_identifier(self, field_type):
		field_type_identifier ={"uint8": FIELD_TYPE_UINT, "int8": FIELD_TYPE_INT, "uint16": FIELD_TYPE_UINT, "int16": FIELD_TYPE_INT, 
								"uint32": FIELD_TYPE_UINT, "int32": FIELD_TYPE_INT, "uint64": FIELD_TYPE_UINT, "int64": FIELD_TYPE_INT, 
								"float": FIELD_TYPE_FLOAT, "double": FIELD_TYPE_DOUBLE}
		
		if field_type in field_type_identifier:
			return field_type_identifier[field_type]
		else:	# For example in the case of message as field type
			return FIELD_TYPE_MESSAGE
		
		
	def create_struct(self, message):
		self.h_file.append_line("typedef struct {")
		for field in message.fields:
			if(isinstance(field, RequiredField)): # Is required field
				self.h_file.append_line("\t" + self.get_field_type_mapping(field.type) + " " + field.name + ";")
			elif(isinstance(field, OptionalField)): # Is optional field
				self.h_file.append_line("\t" + "uint8_t has_" + field.name + ";")
				self.h_file.append_line("\t" + self.get_field_type_mapping(field.type) + " " + field.name + ";")		
			elif(isinstance(field, RepeatedField)): # Is repeated field
				[size_type, size_type_byte_number] = search_size_type(field.size)
				self.h_file.append_line("\t" + self.get_field_type_mapping(size_type) + " " + field.name + "_count;")
				self.h_file.append_line("\t" + self.get_field_type_mapping(field.type) + " " + field.name + "[" + str(field.size) + "];")				
			elif(isinstance(field, FixedRepeatedField)): # Is fixed repeated field
				self.h_file.append_line("\t" + self.get_field_type_mapping(field.type) + " " + field.name + "[" + str(field.size) + "];")		
			elif(isinstance(field, OneofField)): # Is oneof field	
				self.h_file.append_line("\t" + "uint8_t which_" + field.name + ";")
				self.h_file.append_line("\t" + "union {")
				for inner_field in field.inner_fields:
					self.h_file.append_line("\t\t" + self.get_field_type_mapping(inner_field.type) + " " + inner_field.name + ";")
				self.h_file.append_line("\t" + "} " + field.name + ";")
			else:
				raise Exception ("Field " + str(field) + " is not supported")
		self.h_file.append_line("} " + message.name + ";")
		
	
	def create_message_fields(self, message):
	
		C_FIELD_TYPE_NAME 		= "tb_field_t"
		C_OFFSETOF_MAKRO_NAME 	= "tb_offsetof"
		C_MEMBERSIZE_MAKRO_NAME = "tb_membersize"
		C_DELTA_MAKRO_NAME 		= "tb_delta"
		C_LAST_FIELD_NAME 		= "TB_LAST_FIELD"
		
		
	
	
		# Determine the number of fields in the message
		num_fields = 0
		for field in message.fields:
			if(isinstance(field, OneofField)): # Is oneof field
				num_fields = num_fields + len(field.inner_fields)
			else:
				num_fields = num_fields + 1
		
		
		# Declare the field-array in the H-file
		self.h_file.append_line("extern const " + C_FIELD_TYPE_NAME + " " + message.name + "_fields[" + str(num_fields + 1) + "];")
		
		
		# Create the field-arrays in the C-file		
		self.c_file.append_line("const " + C_FIELD_TYPE_NAME + " " + message.name + "_fields[" + str(num_fields + 1) + "] = {")
		
		
		for field in message.fields:		
			if(isinstance(field, RequiredField)): # Is required field
				field_identifier = self.get_field_type_identifier(field.type) | FIELD_RULE_REQUIRED
				field_ptr = str("&" + field.type + "_fields") if (field_identifier & FIELD_TYPE_MESSAGE) else "NULL"
				s = "\t{" + str(field_identifier) + ", ";
				s += C_OFFSETOF_MAKRO_NAME + "(" + message.name + ", " + field.name + "), " 
				s += "0, 0, "
				s += C_MEMBERSIZE_MAKRO_NAME + "(" + message.name + ", " + field.name + "), 0, " + "0, 0, " + field_ptr + "},"                    
				self.c_file.append_line(s)
			elif(isinstance(field, OptionalField)): # Is optional field
				field_identifier = self.get_field_type_identifier(field.type) | FIELD_RULE_OPTIONAL
				field_ptr = str("&" + field.type + "_fields") if (field_identifier & FIELD_TYPE_MESSAGE) else "NULL"
				s = "\t{" + str(field_identifier) + ", ";
				s += C_OFFSETOF_MAKRO_NAME + "(" + message.name + ", " + field.name + "), " 
				s += C_DELTA_MAKRO_NAME + "(" + message.name + ", " + "has_" + field.name + ", " + field.name + "), 1, "
				s += C_MEMBERSIZE_MAKRO_NAME + "(" + message.name + ", " + field.name + "), 0, " + "0, 0, " + field_ptr + "},"                    
				self.c_file.append_line(s)		
			elif(isinstance(field, RepeatedField)): # Is repeated field
				[size_type, size_type_byte_number] = search_size_type(field.size)
				field_identifier = self.get_field_type_identifier(field.type) | FIELD_RULE_REPEATED
				field_ptr = str("&" + field.type + "_fields") if (field_identifier & FIELD_TYPE_MESSAGE) else "NULL"
				s = "\t{" + str(field_identifier) + ", ";
				s += C_OFFSETOF_MAKRO_NAME + "(" + message.name + ", " + field.name + "), " 
				s += C_DELTA_MAKRO_NAME + "(" + message.name + ", " + field.name + "_count" + ", " + field.name + "), " + str(size_type_byte_number) + ", "
				s += C_MEMBERSIZE_MAKRO_NAME + "(" + message.name + ", " + field.name + "[0]" + "), " 
				s += C_MEMBERSIZE_MAKRO_NAME + "(" + message.name + ", " + field.name + ")/" + C_MEMBERSIZE_MAKRO_NAME + "(" + message.name + ", " + field.name + "[0]" + "), "
				s += "0, 0, "
				s += field_ptr + "},"                    
				self.c_file.append_line(s)
			elif(isinstance(field, FixedRepeatedField)): # Is fixed repeated field
				field_identifier = self.get_field_type_identifier(field.type) | FIELD_RULE_FIXED_REPEATED
				field_ptr = str("&" + field.type + "_fields") if (field_identifier & FIELD_TYPE_MESSAGE) else "NULL"
				s = "\t{" + str(field_identifier) + ", ";
				s += C_OFFSETOF_MAKRO_NAME + "(" + message.name + ", " + field.name + "), " 
				s += "0, 0, "
				s += C_MEMBERSIZE_MAKRO_NAME + "(" + message.name + ", " + field.name + "[0]" + "), " 
				s += C_MEMBERSIZE_MAKRO_NAME + "(" + message.name + ", " + field.name + ")/" + C_MEMBERSIZE_MAKRO_NAME + "(" + message.name + ", " + field.name + "[0]" + "), "
				s += "0, 0, "
				s += field_ptr + "},"                    
				self.c_file.append_line(s)
			elif(isinstance(field, OneofField)): # Is oneof field			
				for i in range(0, len(field.inner_fields)):
					inner_field = field.inner_fields[i]
					inner_field_identifier = self.get_field_type_identifier(inner_field.type) | FIELD_RULE_ONEOF
					inner_field_ptr = str("&" + inner_field.type + "_fields") if (inner_field_identifier & FIELD_TYPE_MESSAGE) else "NULL"
					s = "\t{" + str(inner_field_identifier) + ", ";
					s += C_OFFSETOF_MAKRO_NAME + "(" + message.name + ", " + field.name + "." + inner_field.name + "), " 
					s += C_DELTA_MAKRO_NAME + "(" + message.name + ", " + "which_" + field.name + ", " + field.name + "." + inner_field.name + "), " + "1" + ", "
					s += C_MEMBERSIZE_MAKRO_NAME + "(" + message.name + ", " + field.name + "." + inner_field.name + "), " 
					s += "0, "
					s += str(inner_field.tag) + ", " 
					s += "1"  if i == 0 else "0"
					s += ", " + inner_field_ptr + "},"
					self.c_file.append_line(s)				
			else:
				raise Exception ("Field " + str(field) + " is not supported")
		
		self.c_file.append_line("\t" + C_LAST_FIELD_NAME + ",")
		self.c_file.append_line("};")



class Python_Creator:
	def __init__(self, output_path, output_name, imports, defines, messages, endianness):
		self.output_path = output_path
		self.output_name = output_name
		self.imports = imports
		self.defines = defines
		self.messages = messages
		self.endianness = endianness
		
		self.create()
		

		
			
	
	def create(self):
		print "Creating python-file..."
		
		self.python_file = OutputFile(self.output_path + "/" + self.output_name + ".py")
		
		self.python_file.append_line("import struct")
		
		# First create the imports
		for imp in self.imports:
			self.python_file.append_line("from " + imp.name + " import *")
			
		self.python_file.append_line()
			
		# Then create the defines
		for define in self.defines:
			self.python_file.append_line(define.name + " = " + str(define.number))
		self.python_file.append_line()
		
		# Then create the oneof_tags from the messages		
		for message in self.messages:
			self.create_oneof_tags(message)
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
			self.create_class(message)
			self.python_file.append_line()
		
		#print "Python file:"
		#print self.python_file.file_output
		
		
		self.python_file.write_to_file()
		
	
	def create_oneof_tags(self, message):
		for field in message.fields:
			if(isinstance(field, OneofField)): # Is oneof field
				for inner_field in field.inner_fields:
					self.python_file.append_line(message.name + "_" + inner_field.name +  "_tag = " + str(inner_field.tag))
					
	
	def get_default_value(self, field_type):
		field_type_values = {"uint8": "0", "int8": "0", "uint16": "0", "int16": "0", 
							  "uint32": "0", "int32": "0", "uint64": "0", "int64": "0", 
							  "float": "0", "double": "0"}
		
		if field_type in field_type_values:
			return field_type_values[field_type]
		else:	# For example in the case of message as field type. Here you can also return an "instance" of the message: field_type + "()"
			return "None"
			
	def get_field_type_mapping(self, field_type):
		if(self.endianness == BIG_ENDIANNESS):
			field_type_mapping = {"uint8": ">B", "int8": ">b", "uint16": ">H", "int16": ">h", 
								  "uint32": ">I", "int32": ">i", "uint64": ">Q", "int64": ">q", 
								  "float": ">f", "double": ">d"}
		else:	  
			field_type_mapping = {"uint8": "<B", "int8": "<b", "uint16": "<H", "int16": "<h", 
								  "uint32": "<I", "int32": "<i", "uint64": "<Q", "int64": "<q", 
								  "float": "<f", "double": "<d"}
					
		if field_type in field_type_mapping:
			return field_type_mapping[field_type]
		else:	# For example in the case of message as field type
			return None
	
	def get_field_type_len(self, field_type):
		field_type_lengths =   {"uint8": 1, "int8": 1, "uint16": 2, "int16": 2, 
								"uint32": 4, "int32": 4, "uint64": 8, "int64": 8, 
								"float": 4, "double": 8}
								
		return field_type_lengths[field_type]
			
	def create_encode_functions(self, message):
		# Create the encode function
		self.python_file.append_line()
		self.python_file.append_line("\tdef encode(self):")
		self.python_file.append_line("\t\tostream = _Ostream()")
		self.python_file.append_line("\t\tself.encode_internal(ostream)")
		self.python_file.append_line("\t\treturn ostream.buf")
		
		# Create the encode_internal-function
		self.python_file.append_line()
		self.python_file.append_line("\tdef encode_internal(self, ostream):")
		
		for field in message.fields:
			if(isinstance(field, OneofField)): # Is oneof field	
				self.python_file.append_line("\t\t" + "self." + field.name + ".encode_internal(ostream)")
			else:
				self.python_file.append_line("\t\tself.encode_" + field.name + "(ostream)")		
		
		self.python_file.append_line("\t\tpass")	# Added this if there is an Empty message, with no fields at all
		
		# Create all the encode-functions for the fields
		self.python_file.append_line()		
		for field in message.fields:
			# Skip oneof fields
			if(isinstance(field, OneofField)): # Is oneof field
				continue
		
			self.python_file.append_line("\tdef encode_" + field.name + "(self, ostream):")
		
			if(isinstance(field, RequiredField)): # Is required field
				mapped_field_type = self.get_field_type_mapping(field.type)
				if(mapped_field_type == None): # is a message
					self.python_file.append_line("\t\t" + "self." + field.name + ".encode_internal(ostream)")
				else:
					self.python_file.append_line("\t\t" + "ostream.write(" + "struct.pack('" + mapped_field_type + "', " + "self." + field.name + "))")		
					
			elif(isinstance(field, OptionalField)): # Is optional field
				self.python_file.append_line("\t\t" + "ostream.write(" + "struct.pack('" + self.get_field_type_mapping('uint8') + "', " + "self.has_" + field.name + "))")
				self.python_file.append_line("\t\t" + "if self.has_" + field.name + ":")				
				mapped_field_type = self.get_field_type_mapping(field.type)
				if(mapped_field_type == None): # is a message
					self.python_file.append_line("\t\t\t" + "self." + field.name + ".encode_internal(ostream)")
				else:
					self.python_file.append_line("\t\t\t" + "ostream.write(" + "struct.pack('" + mapped_field_type + "', " + "self." + field.name + "))")
				
			elif(isinstance(field, RepeatedField)): # Is repeated field			
				[size_type, size_type_number_bytes] = search_size_type(field.size)
				self.python_file.append_line("\t\t" + "count = len(" + "self." + field.name + ")")
				self.python_file.append_line("\t\t" + "ostream.write(" + "struct.pack('" + self.get_field_type_mapping(size_type) + "', " + "count" + "))")
				self.python_file.append_line("\t\t" + "for i in range(0, " + "count" + "):")
				mapped_field_type = self.get_field_type_mapping(field.type)				
				if(mapped_field_type == None): # is a message
					self.python_file.append_line("\t\t\t" + "self." + field.name + "[i].encode_internal(ostream)" )
				else:
					self.python_file.append_line("\t\t\t" + "ostream.write(" + "struct.pack('" + mapped_field_type + "', " + "self." + field.name + "[i]))")
					
			elif(isinstance(field, FixedRepeatedField)): # Is fixed repeated field
				self.python_file.append_line("\t\t" + "count = " + str(field.size))
				self.python_file.append_line("\t\t" + "for i in range(0, " + "count" + "):")
				
				mapped_field_type = self.get_field_type_mapping(field.type)				
				if(mapped_field_type == None): # is a message
					self.python_file.append_line("\t\t\t" + "self." + field.name + "[i].encode_internal(ostream)" )
				else:
					self.python_file.append_line("\t\t\t" + "ostream.write(" + "struct.pack('" + mapped_field_type + "', " + "self." + field.name + "[i]))")
				
			else:
				raise Exception ("Field " + str(field) + " is not supported")
					
			self.python_file.append_line()
			
	def create_decode_functions(self, message):
	
		
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
		for field in message.fields:
			if(isinstance(field, OneofField)): # Is oneof field	
				self.python_file.append_line("\t\t" + "self." + field.name + ".decode_internal(istream)")
			else:
				self.python_file.append_line("\t\tself.decode_" + field.name + "(istream)")		
		
		self.python_file.append_line("\t\tpass")	# Added this if there is an Empty message, with no fields at all
		
		
		# Create all the decode-functions for the fields
		self.python_file.append_line()
		
		
		for field in message.fields:
			# Skip oneof fields
			if(isinstance(field, OneofField)): # Is oneof field
				continue
			
			self.python_file.append_line("\tdef decode_" + field.name + "(self, istream):")
			
			
			
		
			if(isinstance(field, RequiredField)): # Is required field
				mapped_field_type = self.get_field_type_mapping(field.type)
				if(mapped_field_type == None): # is a message
					self.python_file.append_line("\t\t" + "self." + field.name + " = " + field.type + "()")
					self.python_file.append_line("\t\t" + "self." + field.name + ".decode_internal(istream)")
				else:
					self.python_file.append_line("\t\t" + "self." + field.name + "= struct.unpack('" + mapped_field_type + "', "  + "istream.read(" + str(self.get_field_type_len(field.type)) + "))[0]")
					
			elif(isinstance(field, OptionalField)): # Is optional field
				self.python_file.append_line("\t\t" + "self.has_" + field.name + "= struct.unpack('" + self.get_field_type_mapping('uint8') + "', " + "istream.read(" + "1" + "))[0]" )
				self.python_file.append_line("\t\t" + "if self.has_" + field.name + ":")
			
				mapped_field_type = self.get_field_type_mapping(field.type)
				if(mapped_field_type == None): # is a message
					self.python_file.append_line("\t\t\t" + "self." + field.name + " = " + field.type + "()")
					self.python_file.append_line("\t\t\t" + "self." + field.name + ".decode_internal(istream)")
				else:
					self.python_file.append_line("\t\t\t" + "self." + field.name + "= struct.unpack('" + mapped_field_type + "', "  + "istream.read(" + str(self.get_field_type_len(field.type)) + "))[0]")
			
			
			elif(isinstance(field, RepeatedField)): # Is repeated field			
				[size_type, size_type_number_bytes] = search_size_type(field.size)
				self.python_file.append_line("\t\t" + "count = struct.unpack('" + self.get_field_type_mapping(size_type) + "', " + "istream.read(" + str(size_type_number_bytes) + "))[0]" )
				self.python_file.append_line("\t\t" + "for i in range(0, " + "count" + "):")
				
				mapped_field_type = self.get_field_type_mapping(field.type)
				if(mapped_field_type == None): # is a message
					self.python_file.append_line("\t\t\t" + "tmp" + " = " + field.type + "()")
					self.python_file.append_line("\t\t\t" + "tmp.decode_internal(istream)")
					self.python_file.append_line("\t\t\t" + "self." + field.name + ".append(tmp)")
				else:
					self.python_file.append_line("\t\t\t" + "self." + field.name + ".append(struct.unpack('" + mapped_field_type + "', " + "istream.read(" +  str(self.get_field_type_len(field.type)) + "))[0])")
			
			elif(isinstance(field, FixedRepeatedField)): # Is fixed repeated field
				self.python_file.append_line("\t\t" + "count = " + str(field.size) )
				self.python_file.append_line("\t\t" + "for i in range(0, " + "count" + "):")
				
			
				mapped_field_type = self.get_field_type_mapping(field.type)
				if(mapped_field_type == None): # is a message
					self.python_file.append_line("\t\t\t" + "tmp" + " = " + field.type + "()")
					self.python_file.append_line("\t\t\t" + "tmp.decode_internal(istream)")
					self.python_file.append_line("\t\t\t" + "self." + field.name + ".append(tmp)")
				else:
					self.python_file.append_line("\t\t\t" + "self." + field.name + ".append(struct.unpack('" + mapped_field_type + "', " + "istream.read(" +  str(self.get_field_type_len(field.type)) + "))[0])")				
			else:
				raise Exception ("Field " + str(field) + " is not supported")
					
			self.python_file.append_line()
		
	def create_oneof_classes(self, message):
	
	
		for field in message.fields:
			if(isinstance(field, OneofField)): # Is oneof field	
				# Create an own class for this oneof_name
				self.python_file.append_line("\tclass _" + field.name + ":")
				self.python_file.append_line()
				self.python_file.append_line("\t\tdef __init__(self):")
				self.python_file.append_line("\t\t\tself.reset()")
				self.python_file.append_line()
				self.python_file.append_line("\t\tdef __repr__(self):")
				self.python_file.append_line("\t\t\treturn str(self.__dict__)")
				self.python_file.append_line()
				self.python_file.append_line("\t\tdef reset(self):")
				
				
				# Declare the variables in the oneof field
				self.python_file.append_line("\t\t\tself." + "which" + " = 0")
				for inner_field in field.inner_fields:
					self.python_file.append_line("\t\t\tself." + inner_field.name + " = " + self.get_default_value(inner_field.type))
				self.python_file.append_line("\t\t\tpass")
				
		
				############ Encode functions ############
				
				# Create the encode_internal-function
				self.python_file.append_line()
				self.python_file.append_line("\t\tdef encode_internal(self, ostream):")
				self.python_file.append_line("\t\t\t" + "ostream.write(" + "struct.pack('" + self.get_field_type_mapping('uint8') + "', " + "self.which" + "))")
				self.python_file.append_line("\t\t\t" + "options = {" )
				

				for inner_field in field.inner_fields:
					self.python_file.append_line("\t\t\t\t" + str(inner_field.tag) + ": " + "self.encode_" + inner_field.name + ",")
				self.python_file.append_line("\t\t\t" + "}" )
				self.python_file.append_line("\t\t\t" + "options[self.which](ostream)" )
				self.python_file.append_line("\t\t\tpass")
				
				
				
				# Create all the encode-functions for the inner fields
				self.python_file.append_line()
				
				
				for inner_field in field.inner_fields:
					self.python_file.append_line("\t\tdef encode_" + inner_field.name + "(self, ostream):")
					
					mapped_field_type = self.get_field_type_mapping(inner_field.type)
					if(mapped_field_type == None): # is a message
						self.python_file.append_line("\t\t\t" + "self." + inner_field.name + ".encode_internal(ostream)")
					else:
						self.python_file.append_line("\t\t\t" + "ostream.write(" + "struct.pack('" + mapped_field_type + "', " + "self." + inner_field.name + "))")
					
					self.python_file.append_line()
						
				
	
				############ Decode functions ############
				
				# Create the decode_internal-function
				self.python_file.append_line()
				self.python_file.append_line("\t\tdef decode_internal(self, istream):")
				self.python_file.append_line("\t\t\tself.reset()")
				
				self.python_file.append_line("\t\t\t" + "self.which" + "= struct.unpack('" + self.get_field_type_mapping('uint8') + "', " + "istream.read(" + "1" + "))[0]" )
				self.python_file.append_line("\t\t\t" + "options = {" )		
				for inner_field in field.inner_fields:
					self.python_file.append_line("\t\t\t\t" + str(inner_field.tag) + ": " + "self.decode_" + inner_field.name + ",")
				self.python_file.append_line("\t\t\t" + "}" )
				self.python_file.append_line("\t\t\t" + "options[self.which](istream)" )
				self.python_file.append_line("\t\t\tpass")
				
				
				
				# Create all the decode-functions for the inner fields
				self.python_file.append_line()			
				for inner_field in field.inner_fields:
					self.python_file.append_line("\t\tdef decode_" + inner_field.name + "(self, istream):")
					
					mapped_field_type = self.get_field_type_mapping(inner_field.type)
					if(mapped_field_type == None): # is a message
						self.python_file.append_line("\t\t\t" + "self." + inner_field.name + " = " + inner_field.type + "()")
						self.python_file.append_line("\t\t\t" + "self." + inner_field.name + ".decode_internal(istream)")				
					else:
						self.python_file.append_line("\t\t\t" + "self." + inner_field.name + "= struct.unpack('" + mapped_field_type + "', "  + "istream.read(" + str(self.get_field_type_len(inner_field.type)) + "))[0]")
					
					self.python_file.append_line()		
				
			
	def create_class(self, message):


		self.python_file.append_line("class " + message.name + ":")
		self.python_file.append_line()
		self.python_file.append_line("\tdef __init__(self):")
		self.python_file.append_line("\t\tself.reset()")
		self.python_file.append_line()
		self.python_file.append_line("\tdef __repr__(self):")
		self.python_file.append_line("\t\treturn str(self.__dict__)")
		self.python_file.append_line()
		self.python_file.append_line("\tdef reset(self):")
		
		# Declare the fields:
		for field in message.fields:
			if(isinstance(field, RequiredField)): # Is required field
				self.python_file.append_line("\t\tself." + field.name + " = " + self.get_default_value(field.type))
			elif(isinstance(field, OptionalField)): # Is optional field
				self.python_file.append_line("\t\tself." + "has_" + field.name + " = 0")	
				self.python_file.append_line("\t\tself." + field.name + " = " + self.get_default_value(field.type))
			elif(isinstance(field, RepeatedField)): # Is repeated field
				self.python_file.append_line("\t\tself." + field.name + " = []")
			elif(isinstance(field, FixedRepeatedField)): # Is fixed repeated field
				self.python_file.append_line("\t\tself." + field.name + " = []")	
			elif(isinstance(field, OneofField)): # Is oneof field	
				self.python_file.append_line("\t\tself." + field.name + " = " + "self._" + field.name + "()")
			else:
				raise Exception ("Field " + str(field) + " is not supported")
		
		self.python_file.append_line("\t\tpass")	# Added this if there is an Empty message, with no fields at all
		
		
		self.create_encode_functions(message)
		
		self.create_decode_functions(message)
		
		self.create_oneof_classes(message)
		
		
	



if __name__ == '__main__':
	if(len(sys.argv) < 5):
		raise Exception("Script has to be called with: " + sys.argv[0] + " " + str(SUPPORTED_OUTPUT_FORMATS) + " 'protocol-file' 'output-path' 'output-file name'" + " " + str(SUPPORTED_ENDIANNESS))

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
	
	# Parse the file
	Parser(file)
	
	# Create the source code
	if(output_format == '-c'):
		C_Creator(output_path, output_name, Imports, Defines, Messages)
	elif(output_format == '-python'):
		endianness = BIG_ENDIANNESS if sys.argv[5] == '-be' else LITTLE_ENDIANNESS
		Python_Creator(output_path, output_name, Imports, Defines, Messages, endianness)
	
	
