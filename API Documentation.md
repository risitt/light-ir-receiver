# Light IR Receiver (lirr) API documentation for Arduino
For a working example, please see the "recvExample" sketch in the examples folder.

To use lirr:

1. Call *lirrBegin()* (usually in *setup()*), passing it the sensor's pin number and the remote's protocol
2. Within your program's main loop, repeatedly call *lirrGetEvents()* and save its return value to a struct of type *remoteEvents_t*
3. Test the member variables (eg, buttonState and buttonCode) within the *remoteEvents_t* struct and make your program do different things when different remote buttons are pressed/held/released.

And now in more detail...

## Table of Contents
* [Starting lirr](#lirrBegin)
* [Getting button events and codes](#lirrGetEvents)
* [Testing a new remote](#testingRemote)
* [Using pins other than D0...D7 (PCINT2)](#changePins)

## <a name="lirrBegin">Starting lirr</a>

This step is necessary for telling lirr what pin your sensor is attached to, and what protocol your remote uses.

If you don't know your remote's protocol, you will need to [test it](#testingRemote).

By default, the sensor pin needs to be one of digital pins 0 through 7. If you are so inclined, you can [use a different pin](#changePins) by making a minor modification to the library.

There are currently 8 supported protocols to choose from, each specified by a unique named constant:
* PROTOCOL_NEC
* PROTOCOL_JVC
* PROTOCOL_RCA
* PROTOCOL_SHARP
* PROTOCOL_SAMSUNG
* PROTOCOL_SIRC
* PROTOCOL_RC5
* PROTOCOL_RC6_MODE0

Finally, here's how you start lirr:

### Example:
```C++
void setup(void) {
	// the sensor is on pin 2
	// the remote uses the NEC protocol
	lirrBegin(2, PROTOCOL_NEC);
}
```

Just change the pin number and protocol as needed.

## <a name="lirrGetEvents">Getting button events and codes</a>

Once you've started lirr, now you need to repeatedly ask the library what button (if any) is being pressed on the remote, and what state it's in. Other information is also available: the time the button was first pressed (in microseconds) and a toggle boolean that flips between true and false every time a button is pressed on the remote.

Getting this information is done with the lirrGetEvents() function, which returns a struct (a type of class/object) of type remoteEvents_t that contains information about the button state. The struct contains variables which are accessed just like you would access a member variable of a class (structs are classes): with the dot syntax (see the example code below).

You need to call this function repeatedly and often, otherwise your program will suffer from lag. It is best put in a program loop that contains no delays.

Here is an example of how to get (and use) the button event struct:

### Example:
```C++
const unsigned long BT_PLAY = 1838336055UL;
const unsigned long BT_STOP = 1838319735UL;const unsigned long BT_REWIND = 1838311575UL;
const unsigned long BT_FASTFORWARD = 1838344215UL;

void loop(void) {
	remoteEvents_t remoteEvents = lirrGetEvents();

	if (remoteEvents.buttonState == BUTTON_RELEASED)
	{
		// a button was released
		// do different things depending on what the button was
		if (remoteEvents.buttonCode == BT_PLAY)
		{
			// play or pause the track
		}
		else if (remoteEvents.buttonCode == BT_STOP)
		{
			// stop playing the track
		}
	}
	else if (remoteEvents.buttonState == BUTTON_HELD)
	{
		// a button is being held down
		// do different things depending on what the button is
		if (remoteEvents.buttonCode == BT_REWIND)
		{
			unsigned long transportSpeed = (micros() - remoteEvents.pressTime);
			// rewind the current track at a rate determined by transportSpeed
		}
		else if (remoteEvents.buttonCode == BT_FASTFORWARD)
		{
			unsigned long transportSpeed = (micros() - remoteEvents.pressTime);
			// fast foward the current track at a rate determined by transportSpeed
		}
	}

	// do other stuff...
}

void setup(void) {
	// the sensor is on pin 2
	// the remote uses the NEC protocol
	lirrBegin(2, PROTOCOL_NEC);
}
```

The remoteEvents_t struct contains four variables:
* pressTime: a uint32_t/unsigned long that tells you when the button was first pressed. It can be used in conjunction with micros() to determine how long it's been since the button was pressed - useful for "long held" events or for things which get faster the longer a button is held.
* buttonCode: a uint32_t/unsigned long that is unique for each button on your remote.
* buttonState: a named constant whose value can be one of BUTTON_NONE, BUTTON_PRESSED, BUTTON_HELD, or BUTTON_RELEASED.
* toggleState: a boolean that flips between true and false each time a button is pressed on the remote, but not while a button is being held down.

The buttonState variable needs a bit of explanation:
* BUTTON_NONE means that no button has been pressed, is being held, or has been released.
* BUTTON_PRESSED means that a button was initially pressed. If the button is held down, buttonState will only equal BUTTON_PRESSED on the very initial press, and will later change to BUTTON_HELD. This allows events to be triggered only once and as soon as a button is pressed on the remote.
* BUTTON_HELD means that a button is being held down on the remote. The value of buttonState will repeatedly equal BUTTON_HELD until the button is finally released. This allows events to triggered repeatedly for as long as a button is held down on the remote.
* BUTTON_RELEASED means that a button has been released. The value of buttonState will only equal BUTTON_RELEASED on the initial release, and will later change to BUTTON_NONE. This allows events to be triggered only once, after a button has been released on the remote.

So when a button is pressed on your remote, the value of buttonState progresses from BUTTON_NONE to BUTTON_PRESSED, then to BUTTON_HELD for as long as the button is held down, then to BUTTON_RELEASED when the button is released, and finally back to BUTTON_NONE. The steps in this progression only happen when lirrGetEvents() is called. This means that you will experience lag unless you call lirrGetEvents() frequently.

The typical usage is to first check the buttonState, then do different things depending on the value of buttonCode. The example above shows this process in action.

## <a name="testingRemote">Testing a new remote</a>

If you have a remote but don't know its protocol (or button codes), here is the process for figuring this out:

1) Connect your IR sensor to your Arduino as it's designed to be used (see the datasheet). Usually one pin goes to ground, one to 3.3/5V, and one to a digital in pin. Keep in mind that you should stick to digital pins 0 through 7 unless you've modified the library to use different pins. If you don't want to have to modify the remoteTest sketch, using digital pin 2.
2) Load up the remoteTest example sketch provided with the library.
3) Modify the remoteTest sketch, if necessary, to change the sensor's pin number (pinIRSensor) to the one you used.
4) Connect your Arduino to your computer and upload the remoteTest sketch to it.
5) Get your remote in hand.
6) Open a serial window at 9600 baud. You should see a message that reads "Repeatedly press a button on your remote."
7) Start pressing a button on your remote a few times per second.
8) Watch the serial output as it switches between trying different protocols.
9) If your remote is compatible, you should see a "Match found!" message before the test wraps around to the start again. Note the protocol name for which the match was found.
10) Once a match is found, you can press all of the buttons you intend to use to get the button codes for them.
11) Now you know your remote's protocol as well as it button codes!
12) If this process doesn't work, you likely have a non-compatible remote, but double check that your remote is working and that your sensor is also working and hooked up correctly, with the correct pin specified in the remoteTest sketch.

## <a name="changePins">Using pins other than D0...D7 (PCINT2)</a>

For Arduino, pins D0...D7 map to PCINT2. If you want to use a different pin, this is easily accomplished by changing the ISR(PCINT2_vect) line in lightIRRecv.cpp to ISR(PCINT0_vect) (for D8...D13) or ISR(PCINT1_vect) (for A0...A5).

For other devices, the PCINTx_vect to pin range (port) mapping may be different. Consult the relevant Atmel datasheet for your microcontroller chip if you are venturing beyond the realm of official Arduino boards.