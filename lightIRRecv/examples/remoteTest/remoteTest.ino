#include <lightIRRecv.h>

// This sketch allows you to see which of the supported protocols (if any)
// your remote is using as well as the values returned for each button.
// Once known, then you can use the protocol name as a named constant when
// calling lirrBegin() in other sketches designed for that specific remote.

const uint8_t pinIRSensor = 2;
const uint8_t PROTOCOL_COUNT = 8;
const uint32_t TEST_TIME = 2000000UL;

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void loop(void)
{
	static bool identified = false;
	static bool showHeld = true;
	
	if (!identified)
	{
		uint32_t lastSwitch;
		for (uint8_t i = PROTOCOL_COUNT; i > 0; i--)
		{
			String protocolName;
			switch (i)
			{
				case 8:
					protocolName = "PROTOCOL_NEC";
					lirrBegin(pinIRSensor, PROTOCOL_NEC);
					break;
				case 7:
					protocolName = "PROTOCOL_RC6_MODE0";
					lirrBegin(pinIRSensor, PROTOCOL_RC6_MODE0);
					break;
				case 6:
					protocolName = "PROTOCOL_RC5";
					lirrBegin(pinIRSensor, PROTOCOL_RC5);
					break;
				case 5:
					protocolName = "PROTOCOL_SIRC";
					lirrBegin(pinIRSensor, PROTOCOL_SIRC);
					break;
				case 4:
					protocolName = "PROTOCOL_SAMSUNG";
					lirrBegin(pinIRSensor, PROTOCOL_SAMSUNG);
					break;
				case 3:
					protocolName = "PROTOCOL_RCA";
					lirrBegin(pinIRSensor, PROTOCOL_RCA);
					break;
				case 2:
					protocolName = "PROTOCOL_JVC";
					lirrBegin(pinIRSensor, PROTOCOL_JVC);
					break;
				case 1:
					protocolName = "PROTOCOL_SHARP";
					lirrBegin(pinIRSensor, PROTOCOL_SHARP);
					break;		
			}
			
			Serial.print(F("\nTrying "));
			Serial.print(protocolName);
			Serial.println(F("..."));
			Serial.flush();
			
			lastSwitch = micros();
			while (((uint32_t)micros() - lastSwitch) < TEST_TIME)
			{
				remoteEvents_t remoteEvents = lirrGetEvents();
				if (remoteEvents.buttonState == BUTTON_PRESSED)
				{
					identified = true;
					Serial.println(F("Match found!"));
					Serial.print(F("\nPressed: "));
					Serial.println(remoteEvents.buttonCode);
					Serial.flush();
					break;
				}
			}
			if (identified) break;
		}
	}
	else
	{
		remoteEvents_t remoteEvents = lirrGetEvents();
		switch(remoteEvents.buttonState)
		{
			case (BUTTON_PRESSED):
				Serial.print(F("\nPressed: "));
				Serial.println(remoteEvents.buttonCode);
				Serial.flush();
				break;
			case (BUTTON_HELD):
				if (showHeld)
				{
					Serial.print(F("Held: "));
					Serial.println(remoteEvents.buttonCode);
					Serial.flush();
					showHeld = false;
				}
				break;
			case (BUTTON_RELEASED):
				Serial.print(F("Released: "));
				Serial.println(remoteEvents.buttonCode);
				Serial.flush();
				showHeld = true;
				break;
		}
	}
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void setup(void)
{
	Serial.begin(9600);
	while (!Serial) {;}

	Serial.println(F("Repeatedly press a button on your remote."));
	Serial.flush();
}
