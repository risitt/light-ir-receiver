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
#ifndef lightIRRecv_H
#define lightIRRecv_H

#include <Arduino.h>

//#define lirrTest

// ----------------------------------------------------------------------------------------------------
// This library supports the three most common encoding types:
// 	* Pulse distance
// 	* Pulse width
// 	* Bi-phase (Manchester)
//
// A big thanks to San Bergmans (sbprojects.com) for an excellent Knowledge base of IR remote protocols.
// Many of the protocols and decoding functions are based on information available there.
// ----------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------
// Important note for contributors:
//
// Aside from decoding IR signals, the goal of this library is to be lightweight!
//
// This means omitting unnecessary features, and using some specific (and perhaps unconventional)
// coding techniques.
//
// This library is carefully designed so that unused protcols and decoding functions will be
// removed by the optimizer prior to compilation. The pin change ISR calls the relevant decoding
// function using a function pointer that is set by an overloaded lirrBegin() function
// based on the chosen protocol.
//
// Make sure that decoding functions are as self-contained as possible, even if that means
// copying some of the same code into them that is used in other decoding functions.
// Take care to ensure that unused protocols and decoding routines are optimized out if unused.
// Implementations shouldn't have to edit the library to remove unnecessary code.
//
// It is perfectly acceptable to make single-protocol decoding functions, as long as
// they are optimized away if unused. However, even single-protocol decoding functions should make
// use of const settings structs to permit their possible re-use for other protocols.
// Remember that each new decoding function will need its own overloaded lirrBegin() function.
//
// Microcontrollers are not high performance devices. Best practices in many other situations
// may not be best practices here. Too much abstraction, for example, often comes at the cost of
// efficiency. For this library, efficient code is more valued than beautiful code.
//
// A good resource: http://www.atmel.com/Images/doc8453.pdf
// Please read it before making contributions.
//
// Before making changes, compile a test sketch that uses this library and note the compiled size
// and use of dynamic memory. Do the same after making changes. Also uncomment #define lirrTest
// in order to benchmark the ISR before & after making changes.
// ----------------------------------------------------------------------------------------------------

// keep the settings structs POD (no constructors or destructors) to allow for proper optimization
// use composition instead of inheritance. Inheritance precludes brace initialization and POD.

struct lirrPulseFractionSettings_t
{
	// 12 bytes
	const uint8_t bits; // the number of bits in a message
	const bool distanceMode; // true for pulse distace encoding, false for pulse width encoding
	const uint16_t startMin; // distance/width of a start/AGC burst (microseconds), minus some amount of tolerance
	const uint16_t startMax; // distance/width of a start/AGC burst (microseconds), plus some amount of tolerance
	const uint16_t bitMin; // distance/width of a logical 0 or 1 (whichever is shorter), minus some amount of tolerance
	const uint16_t bitSep; // the mean of bitMin and bitMax
	const uint16_t bitMax; // distance/width of a logical 0 or 1 (whichever is longer), plus some amount of tolerance
};

struct lirrBiPhaseSettings_t
{
	// 15 bytes
	const uint8_t bits; // the number of bits in a message (do not include the first start pulse)
	const bool aceRising; // true if a rising edge in the middle of a bit indicates a logical 1
	const uint16_t bitTime[2]; // min & max microseconds between the middle of two adjacent command/address bits
	const uint16_t startTime[2]; // min & max microseconds between the first rising edge and the middle of the first bit
	const uint16_t toggleTime[2]; // min & max microseconds between the middle of the toggle bit and the adjacent command/address bits
	const uint8_t togglePos; // the position of the toggle bit (the last bit received is == 0, and the first bit received == (bits-1))
};

// For testing NEC, can use Apple TV remote or Kenwood RC-P400
const lirrPulseFractionSettings_t PROTOCOL_NEC = {32,true,13300,13700,925,1687,2450}; // passed 22/11/15

// For testing JVC, set universal remote for JVC EM55FTR
const lirrPulseFractionSettings_t PROTOCOL_JVC = {16,true,12424,12824,852,1578,2304};

const lirrPulseFractionSettings_t PROTOCOL_RCA = {24,true,7800,8200,1300,2000,2700};

// For testing Sharp, set universal remote for Sharp VC-H813U VCR
const lirrPulseFractionSettings_t PROTOCOL_SHARP = {15,true,0,0,800,1500,2200}; // passed 22/11/15

// For testing Samsung, can use AA59-00666A Remote for Samsung UN39EH5003F LCD TV
const lirrPulseFractionSettings_t PROTOCOL_SAMSUNG = {32,true,8760,9160,920,1680,2440}; // passed 22/11/15

// For testing SIRC, set universal remote for Sony Bravia KDL-55HX850 TV
const lirrPulseFractionSettings_t PROTOCOL_SIRC = {12,false,2200,2600,400,900,1400}; // passed 22/11/15

// For testing RC5, set universal remote for Balanced Audio Technology VK-31 Amp
// will also capture extended RC5
const lirrBiPhaseSettings_t PROTOCOL_RC5 = {13,true,{1578,1978},{1578,1978},{1578,1978},11}; // passed 22/11/15

// For testing RC6 mode 0, set universal remote for Philips 49PFL4909 TV
const lirrBiPhaseSettings_t PROTOCOL_RC6_MODE0 = {21,false,{688,1088},{3796,4196},{1132,1532},16}; // passed 22/11/15

// global constants
// 3 bytes
const uint32_t repeatInt = 100000UL; // how long to wait for repeat codes before determining that a button is no longer being pressed

// not using a true enum here because this allows us to simulate a scoped enum with better Arduino IDE compatibility
const uint8_t BUTTON_NONE = 0;
const uint8_t BUTTON_PRESSED = 1;
const uint8_t BUTTON_HELD = 2;
const uint8_t BUTTON_RELEASED = 3;

struct remoteEvents_t
{
	// 10 bytes
	volatile uint32_t pressTime;
    uint32_t curTime;
	volatile uint32_t buttonCode;
	uint8_t buttonState;
	volatile bool toggleState;
};

// using overloaded functions here is important for allowing unused decoding routines
// to be optimized out while retaining a common API
void lirrBegin(uint8_t pinInterrupt, const lirrPulseFractionSettings_t &remoteProtocol);
void lirrBegin(uint8_t pinInterrupt, const lirrBiPhaseSettings_t &remoteProtocol);

void lirrClearEvents(void);
remoteEvents_t lirrGetEvents(void);

#endif
