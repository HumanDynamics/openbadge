Protocol:
	-	The Protocol (current version 02v1) is based on Tinybuf messages for the de-/serialization of structured data.
		02v1 uses Big endian for the integer/floating point number representation (it is the badge_protocol.py of 
		the BadgeFramework generated from badge_protocol.tb in the same directory)
		01v1 uses Little endian (it is the badge_protocol_legacy.py of the BadgeFramework)
	
	-	The definition of the 02v1 protocol messages is in incl/protocol_messages_02v1.tb.
		The most important messages are the Request message and the Repsonse message: 
		Both contains a oneof-field of the different messages:
		sent to the badge (request) and sent from the badge (response) 
		
		
		Description:	
		-------------------------------------------------------------------------------------------------------------
		Status Request			|	Request to retrieve the current status of the badge, setting the time, 
									and optionally setting the ID and group number.
			->
		Status Response			|	Response returns the current status of the badge: clock_status, microphone_status, 
									scan_status, accelerometer_interrupt_status, battery status. Additionally, 
									a local timestamp of the badge and the current battery-data is returned.
		-------------------------------------------------------------------------------------------------------------
		Start Request			| 	Request to start a certain data source for recording and storing. 
									Each data source has its own parameters that are specified via the request. 
									Additionally, the time is synchronized via these requests.
									The timeout-parameter specifies the timeout in minutes: 
									If this time expires without contact to a hub the data recording stops.
			->
		Start Response			|	Response containing the local timestamp of the badge.
		-------------------------------------------------------------------------------------------------------------
		Stop Request			|	Request to stop recording + storing of a certain data source.
			->
		No Response
		-------------------------------------------------------------------------------------------------------------
		Data Request			| 	Request to retrieve the stored data since a certain timestamp in the past.
			->
		Data Response			| 	Response containing the stored data chunks. Additionally, a flag "last_response" 
									is set or not. If it is set, it was the last chunk.
		-------------------------------------------------------------------------------------------------------------
		Start Stream Request	|	Starts the streaming of a data source with certain parameters. 
									Can be enabled while storing is active.
									Watch out that the selected parameters influence the recording for storage.
			->
		StreamResponse			|	This is a message that contains all data points of the streamed data sources. 
									It is transmitted continously.
									(If a data source is not activated, the repeated field is empty)
		-------------------------------------------------------------------------------------------------------------
		Stop Stream Request		|	Stops the streaming of a certain data source.
			->
		No Repsonse
		-------------------------------------------------------------------------------------------------------------
		Identify Request		|	Identifies the badge by blinking its LED for a certain time.
			->
		No Response
		-------------------------------------------------------------------------------------------------------------
		Test Request			|	Starts the peripheral testing of the badge.
			->
		Test Response			|	Repsonse indicating whether the test failed or not.
		-------------------------------------------------------------------------------------------------------------
		Restart Request			|	Request to restart the badge.
			->
		No Response
		-------------------------------------------------------------------------------------------------------------

			
			
			

Unit Test:
	To invoke a Unit Test (e.g. TEST_NAME), enter directory /unit_test and 
	call make badge_03v6 TEST_NAME run_TEST_NAME (optionally: LCOV=TRUE)
	
	You can run all unit tests by: make badge_03v6 all run_all
	If LCOV=TRUE, the source code coverage analysis is done:
	It is written into the _build/LCOV-directory for each test
	
	
		
