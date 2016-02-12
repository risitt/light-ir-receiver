/*

Light IR Receiver - a lightweight library for controlling your project using an IR remote control

Written by Rylee Isitt.
License: GNU Lesser General Public License (LGPL) V2.1

What it does:
* Gives you a unique 32 bit number for each button.
* Supports most of the common protocols, including NEC, Sony SIRC (12 bit), and RC5.
* Determines if a button has just been pressed, if it's being held down, and if it's just been released.
* Gives you a time stamp of when the button was first pressed (good for "long pressed" events).
* Can work with your sensor attached to any of digital pins 0 through 7 (minor software adjustment will permit attachment to any digital pin).
* Allows you to specify your own protocols, provided that they use an existing protocol decoding function.

Limitations:
* It doesn't auto-detect your remote's protocol.
	* But you can use the "remoteTest" example sketch to figure out your remote's protocol.
* It interprets all signals as being most significant bit first.
	* So the button codes from this library will often differ from the official values.
* It doesn't split the address code from the command code - they are combined together.
* It doesn't split inverted verification codes from the non-inverted codes - they are combined together.

*/
#include "lightIRRecv.h"

// ----------------------------------------------------------------------------------------------------
// Global data
// ----------------------------------------------------------------------------------------------------

static volatile uint32_t referenceTime;
static uint32_t incomingCode;
static remoteEvents_t remoteEvents = {0,0,0,BUTTON_NONE,false};
#ifdef lirrTest
static volatile uint32_t decodeTime = 0;
static volatile uint32_t decodeCount = 0;
#endif

// pointers to settings structs
static const lirrPulseFractionSettings_t *pPFSettings;
static const lirrBiPhaseSettings_t *pBPSettings;

// pointer to a function that will be called by the ISR
static void (*pDecodeFunction)(bool pinState);

// ----------------------------------------------------------------------------------------------------
// function for handling pulse width and pulse distance encoding
// ----------------------------------------------------------------------------------------------------

static void pulseFractionDecode(bool pinState)
{
	static uint8_t bitsRemaining = 0;

	switch (pinState == pPFSettings->distanceMode)
	{
		case true:
		{
			uint32_t curTime = micros();
			uint32_t measuredTime = curTime - referenceTime;
			referenceTime = curTime;
			
			switch (bitsRemaining == 0)
			{
				case false:
				{
					if ((measuredTime > pPFSettings->bitMin) && (measuredTime < pPFSettings->bitMax))
					{
						bitsRemaining--;

						if (measuredTime > pPFSettings->bitSep)
							incomingCode |= (1UL << bitsRemaining);
						
						if (bitsRemaining == 0)
						{
							if (remoteEvents.buttonState == BUTTON_NONE)
							{
								remoteEvents.buttonCode = incomingCode;
								remoteEvents.pressTime = curTime;
								remoteEvents.toggleState = !remoteEvents.toggleState;
							}
						}
						break;
					}
					else
						bitsRemaining = 0; // if unexpected timing occurs, immediately restart
				}
				case true:
				{
					if (
						((measuredTime < pPFSettings->startMax)
							&& (measuredTime > pPFSettings->startMin))
						|| (pPFSettings->startMax == 0)
					)
					{
						bitsRemaining = pPFSettings->bits;
						incomingCode = 0;
					}
				}
			}
			break;
		}
		case false:
		{
			if (!pPFSettings->distanceMode)
				referenceTime = micros();
		}
	}
}

// ----------------------------------------------------------------------------------------------------
// function for handling bi-phase encoding
// ----------------------------------------------------------------------------------------------------

static void biPhaseDecode(bool pinState)
{

	uint32_t curTime = micros();

	static uint8_t bitsRemaining = 0;
	static const uint16_t * bitTime;

	switch (bitsRemaining == 0)
	{
		case false:
		{
			uint32_t measuredTime = curTime - referenceTime;

			if (measuredTime < bitTime[1])
			{
				if (measuredTime > bitTime[0])
				{
					// we're at the middle of a bit
					referenceTime = curTime;
					
					bitsRemaining--;
					if (pinState == pBPSettings->aceRising)
					{
						if (bitsRemaining != pBPSettings->togglePos)
							incomingCode += (1UL << bitsRemaining);
					}
					
					// if we've received all the bits, indicate that the code is ready
					// otherwise, determine the time until the next bit
					if (bitsRemaining == 0)
					{
						if (remoteEvents.buttonState == BUTTON_NONE)
						{
							remoteEvents.buttonCode = incomingCode;
							remoteEvents.pressTime = curTime;
							remoteEvents.toggleState = !remoteEvents.toggleState;
						}
					}
					else if ((bitsRemaining != (pBPSettings->togglePos+1)) && (bitsRemaining != pBPSettings->togglePos))
						bitTime = pBPSettings->bitTime;
					else
						bitTime = pBPSettings->toggleTime;
					
				}
				break;
			}
			else
				bitsRemaining = 0; // if unexpected timing occurs, immediately restart
		}		
		case true:
		{
			// wait for a rising edge
			if (pinState)
			{
				bitsRemaining = pBPSettings->bits;
				incomingCode = 0;
				bitTime = pBPSettings->startTime;
				referenceTime = curTime;
			}
		}
	}
}

// ----------------------------------------------------------------------------------------------------
// ISR that calls the needed decoding function
// ----------------------------------------------------------------------------------------------------

ISR(PCINT2_vect)
{
#ifdef lirrTest
	uint32_t startTime = micros();
#endif
	static bool pinState = false;
	pinState = !pinState;
	pDecodeFunction(pinState);
#ifdef lirrTest
	decodeTime += (micros() - startTime);
	decodeCount++;
#endif
}

// ----------------------------------------------------------------------------------------------------
// Sets up the pin change interrupt
// ----------------------------------------------------------------------------------------------------

static inline void lirrInit(uint8_t pinInterrupt)
{
	// pull sensor pin high
	pinMode(pinInterrupt, INPUT_PULLUP); 
	
	// Set up a pin change interrupt
	// Need to put the sensor on one of the digital pins D0...D7
    *digitalPinToPCMSK(pinInterrupt) |= (1 << digitalPinToPCMSKbit(pinInterrupt));  // enable the pin in the PCI mask
	uint8_t PCICRBitMask = 1 << digitalPinToPCICRbit(pinInterrupt);
    PCIFR |= PCICRBitMask; // clear interrupt
    PCICR |= PCICRBitMask; // enable PCI for the port the pin is on
	
#ifdef lirrTest
	Serial.begin(9600);
#endif
}

// ----------------------------------------------------------------------------------------------------
// These functions set up pointers to the protocol settings and decoding function
// And call the initialization function above
// Having overloaded lirrBegin() functions for each decoding function is what allows the
// unused decoding functions to be optimized out.
//
// The pointer to the protocol struct is typecast to a pointer to its first member (this is legal)
// and it is also legal to go back again (the reverse typecast occurs in the decoding functions)
// this library exploits this to acchieve polymorphism-like behavior with POD structs
// ----------------------------------------------------------------------------------------------------

void lirrBegin(uint8_t pinInterrupt, const lirrPulseFractionSettings_t &remoteProtocol)
{
	pPFSettings = &remoteProtocol;
	pDecodeFunction = pulseFractionDecode;
	lirrInit(pinInterrupt);
}

void lirrBegin(uint8_t pinInterrupt, const lirrBiPhaseSettings_t &remoteProtocol)
{
	pBPSettings = &remoteProtocol;
	pDecodeFunction = biPhaseDecode;
	lirrInit(pinInterrupt);
}

// ----------------------------------------------------------------------------------------------------
// This function captures available button codes and determines pressed, held, and released states
// ----------------------------------------------------------------------------------------------------

remoteEvents_t lirrGetEvents(void)
{

	// get last time a bit was received
	// need to temporarily disable global interrupt bit
	// in order to get atomic access to referenceTime
	uint32_t lastSignalTime;
	uint8_t oldSREG = SREG;
	cli();
	lastSignalTime = referenceTime;
	SREG = oldSREG;
	
	// get current time
	remoteEvents.curTime = micros();
	
	switch(remoteEvents.buttonState)
	{
		case BUTTON_PRESSED:
			// if a button was previously pressed, change the state to held
			remoteEvents.buttonState = BUTTON_HELD;
			break;
		case BUTTON_HELD:
		{
			// if a button has not been pressed in a while, set the released event
			// and start reading new codes
			if ((remoteEvents.curTime - lastSignalTime) > repeatInt)
				remoteEvents.buttonState = BUTTON_RELEASED;
			break;
		}
		case BUTTON_RELEASED:
			// if a button was previously released, change the state to none
			remoteEvents.buttonState = BUTTON_NONE;
			remoteEvents.buttonCode = 0;
			// yes, the lack of a break; is intentional
		case BUTTON_NONE:
			// fetch available code and do some final processing
			if (remoteEvents.buttonCode) {
				remoteEvents.buttonState = BUTTON_PRESSED;
#ifdef lirrTest
				Serial.print("T(ISR): ");
				Serial.print((float)decodeTime / decodeCount);
				Serial.println("us");
#endif
			}
			break;
	}

	return remoteEvents;
	
}
