# Light IR Receiver (lirr)
A lightweight library for controlling your Arduino projects with an IR remote

Written by Rylee Isitt

## Features:
* Small footprint suitable for integration into larger projects
* The unused protocols are removed by the optimizer prior to compilation
* Supported protocols:
	* NEC
	* Sony SIRC
	* RC5 (normal and extended)
	* RC6 mode 0
	* Samsung
	* JVC
	* RCA
	* Sharp
* Gives you the following information when a button is pressed on the remote:
	* Its unique 32-bit button code
	* When it was pressed
	* Whether it was initially pressed, is being held, or was just released
	* A toggle boolean that switches between true/false each time a button is pressed

## License:
GNU Lesser General Public License (LGPL) V2.1

**Please see the LICENSE file for details**

## Installation Instructions:
1. Download the source code ZIP for the latest release of the library: https://github.com/risitt/light-ir-receiver/releases
2. In the Arduino IDE main menu, select Sketch > Include Library > Add .ZIP Library, then select the downloaded file and click on "Open"
3. Include the library in your sketch: **#include &lt;lightIRRecv.h&gt;**
4. Read the API Documentation, or see the example sketches for information on how to use the library
