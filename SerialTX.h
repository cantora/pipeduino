/* adapted from the SoftwareSerial arduino library by anthony cantor.
 * why? the atmega8 has issues with some features software serial uses
 * for receiving, so this minimizes the software serial library into
 * code that only supports transmitting.
 *
 * the following is the original license from SoftwareSerial.h:
 * Multi-instance software serial library for Arduino/Wiring
 * -- Interrupt-driven receive and other improvements by ladyada
 *    (http://ladyada.net)
 * -- Tuning, circular buffer, derivation from class Print/Stream,
 *    multi-instance support, porting to 8MHz processors,
 *    various optimizations, PROGMEM delay tables, inverse logic and
 *    direct port writing by Mikal Hart (http://www.arduiniana.org)
 * -- Pin change interrupt macros by Paul Stoffregen (http://www.pjrc.com)
 * -- 20MHz processor support by Garrett Mace (http://www.macetech.com)
 * -- ATmega1280/2560 support by Brett Hagman (http://www.roguerobotics.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef SerialTX_h
#define SerialTX_h

#include <inttypes.h>

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

class SerialTX
{
private:
  uint8_t _transmitBitMask;
  volatile uint8_t *_transmitPortRegister;
  uint16_t _tx_delay;
  uint16_t _inverse_logic:1;

  // private methods
  void tx_pin_write(uint8_t pin_state);
  void setTX(uint8_t transmitPin);

  // private static method for timing
  static inline void tunedDelay(uint16_t delay);

public:
  // public methods
  SerialTX(uint8_t transmitPin, bool inverse_logic = false);
  void begin(long speed);
  virtual size_t write(uint8_t byte);
  
  //using Print::write;
};

// Arduino 0012 workaround
#undef int
#undef char
#undef long
#undef byte
#undef float
#undef abs
#undef round

#endif /* SerialTX_h */
