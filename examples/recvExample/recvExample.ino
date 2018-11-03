#include <lightIRRecv.h>

// This example receives button codes from an IR remote via an receiver attached to your arduino.
// It then outputs the button codes and states via serial
// Be sure to open a serial window to see the results!
// For a real project, instead of outputting to serial, you would do different things depending
// on the values of remoteEvents.buttonState and remoteEvents.buttonCode.

// Be sure to check the pin number and protocol constant below, and adjust them as needed.

const uint8_t pinIRSensor = 2; // this is the pin no. that you have the IR sensor attached to

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void loop(void)
{
	static bool showHeld = true;
	remoteEvents_t remoteEvents = lirrGetEvents();
	
	switch(remoteEvents.buttonState)
	{
		case (BUTTON_PRESSED):
			Serial.print(F("\nPressed: "));
			Serial.println(remoteEvents.buttonCode);
			Serial.print(F("Toggle: "));
			Serial.println(remoteEvents.toggleState);
			break;
		case (BUTTON_HELD):
			if (showHeld)
			{
				Serial.print(F("Held: "));
				Serial.println(remoteEvents.buttonCode);
				showHeld = false;
			}
			break;
		case (BUTTON_RELEASED):
			Serial.print(F("Released: "));
			Serial.println(remoteEvents.buttonCode);
			showHeld = true;
			break;
	}

}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void setup(void)
{
	Serial.begin(9600);
	while (!Serial) {;}
	lirrBegin(pinIRSensor, PROTOCOL_NEC); // change the protocol here to match your remote
}
