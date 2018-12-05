Tinybuf is a de-/serialization library that efficiently encodes structured data into a binary representation.
It enables a platform and programming language indpendend data exchange. 
Currently C and Python is supported as programming languages.

The process is splitted into two parts:
- During the development, the structured data are defined in "messages" in a schema-file. This schema-file is parsed by the tinybuf-generator. The generator automatically creates source code for the chosen programming language.
- During the runtime of the system, the automatically generated source code can be used to access the structured data and to de-/serialize it.
	
	

Schema-file layout:

- Messages defines the structured data with different fields:

		message Test_message {    
			required uint8 a;      
			optional int8 b;      
			repeated uint16 c[5];      
			fixed_repeated int16 d[10];      
			oneof e {      
				uint32 e (1);        
				int32 f	 (2);        
			}      
		}
	
- Define constants:

		define {    
			TEST_CONSTANT = 100;      
			TEST1_CONSTANT = 200;      
		}
	
- Import other automatic generated files:

		import file
	
- If you import another file, you need to declare the messages you want/need in the current schema-file:

		extern Extern_message;
		
	
- Supported field/data-types in messages:
	- Primitive types: uint8, int8, uint16, int16, uint32, int32, uint64, int64, float, double (The encoding is done via their binary representations (Big or Little Endian))
  - Other messages: Either extern messages or messages define before the current message, using this message as field type

- Supported field rules in messages:
  - required: A field that has to be set and which is always encoded into the binary representation.
  - optional: An optional field needs only to be set if necessary. The application needs to set an additional "has"-entry (in form of one byte) whether the field is present or not.
  - repeated: Repeated fields represent "arrays" of the same data type. Only the necessary/specified number of elements is encoded into the binary representation. The number of elements in the array is either an integer numbers or a previoulsy defined constant. It is encoded as "header" in the binary representation.
    - In C: An extra struct entry "_count", representing the number of elements, is generated. This variable has to be set by the application.
    - In Python: The data-structure used is a list. So Python directly knows how many elements are in the list by calling the len() function.
  - fixed_repeated:	Similar to repeated fields, but the number of elements is always constant --> no size information has to be provided by the application and is encoded.
  - oneof: Represents a set of fields, where only one field can be set at a time. An additional "which"-entry (in form of one byte) has to be set, indicating the tag (e.g. "(1)" means the tag is 1) of the field that is set in the oneof field. None of the other field rules is directly supported as field in oneof fields, but can be represented in an own message. It works similar to Protocol Buffers oneof fields (so for better understanding you can look at the doc of Protocol Buffers).
	
	
Invoking the generator:
  
    python tinybuf_generator.py <language> <schema-file> <output-path> <output-name> <endianness>
    <language> 	-c or -python 
    <schema-file> 	Path to schema file
    <output-path> 	Path to output directoy 
    <output-name>	Name of output file
    <endianness>	Parameter for endianness (only effect necessary for Python, not C): -be or -le: Big or Little endian
		
- In C a .h and .c file are generated. The messages are represented as C structures. The tinybuf-module has to be used for encoding or decoding. These two functions are generic and uses the information of the "_fields"-array of the generated .c file to encode/decode correctly.
- In Python a .py file is generated. The messages are represented as classes, and are instanciated during the runtime. Each class/object has its own encoding/decoding function.
	
Attention:
- In C: If multiple schema-files are in the same project containing the same messages, care must be taken that no compilation conflicts occur --> E.g. by manually inserting additional #ifdef/#endif structures in the automated generated source code
	
- In Python: If the developer wants only one file with all messages, either one schema-file has to define all the messages without imports, or the developer has to manually copy all automatically generated python classes into one .py-file (with only one _Istream/_Ostream-class)
	
  
