/**
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */



#include "st/platform.h"
#include "st/vl53l5cx_api.h"
#include "st/vl53l5cx_plugin_detection_thresholds.h"
#include <Arduino.h>
#include <Wire.h>

// Helper
static void start_transfer(TwoWire * wire, uint16_t rgstr)
{
    uint8_t buffer[2] {(uint8_t)(rgstr >> 8),
                       (uint8_t)(rgstr & 0xFF) }; 
    wire->write(buffer, 2);
}


uint8_t RdByte(
	VL53L5CX_Platform* p_platform,
	uint16_t rgstr,
	uint8_t* p_value) {
  auto  status = RdMulti(p_platform, rgstr, p_value, 1);

  if (status) {
    return -1;
  }

  return 0;
}

uint8_t WrByte(
	VL53L5CX_Platform* p_platform,
	uint16_t rgstr,
	uint8_t value) {
return WrMulti(p_platform,rgstr,&value,1);
}

uint8_t WrMulti(
	VL53L5CX_Platform* p_platform,
	uint16_t rgstr,
	uint8_t* data,
	uint32_t count) {
	int status = 0;
	auto wire = static_cast<TwoWire*>(p_platform->device);

	// Partially based on https://github.com/stm32duino/VL53L1 VL53L1_I2CWrite()
	wire->beginTransmission((uint8_t) ((p_platform->address) & 0x7F));

	// Target register address for transfer
	start_transfer(wire, rgstr);
	for (uint32_t i = 0; i < count; i++) {
		wire->write(data[i]);
		if (i > 0 && i < count - 1 && i % 16 == 0) {
			// Flush buffer and end transmission completely
			wire->endTransmission(true);
			i++; // prepare for next byte

			// Restart send
			wire->beginTransmission((uint8_t) ((p_platform->address) & 0x7F));

			start_transfer(wire, rgstr + i);

			if (wire->write(data[i]) == 0) {
				return 4;
			}
		}
	}

	return wire->endTransmission(true);
}

uint8_t RdMulti(
	VL53L5CX_Platform* p_platform,
	uint16_t rgstr,
	uint8_t* data,
	uint32_t count) {
	int status = 0;
	auto wire = static_cast<TwoWire*>(p_platform->device);

	// Loop until the port is transmitted correctly
	do {
		wire->beginTransmission((uint8_t) ((p_platform->address) & 0x7F));

		start_transfer(wire, rgstr);

		status = wire->endTransmission(false);

		// Fix for some STM32 boards
		// Reinitialize the i2c bus with the default parameters
#ifdef ARDUINO_ARCH_STM32
		if (status) {
			wire->end();
			wire->begin();
		}
#endif
		// End of fix

	} while (status != 0);

	uint32_t i = 0;
	if (count > 32) {
		while (i < count) {
			// If still more than 32 bytes to go, 32, else the remaining number
			// of bytes
			byte current_read_count = (count - i > 32 ? 32 : count - i);
			wire->requestFrom(((uint8_t) ((p_platform->address) & 0x7F)),
				current_read_count);
			while (wire->available()) {
				data[i] = wire->read();
				i++;
			}
		}
	}
	else {
		wire->requestFrom(((uint8_t) ((p_platform->address) & 0x7F)), count);
		while (wire->available()) {
			data[i] = wire->read();
			i++;
		}
	}

	return i != count;
}

uint8_t Reset_Sensor(
	VL53L5CX_Platform* p_platform) {
	uint8_t status = 0;

	/* (Optional) Need to be implemented by customer. This function returns 0 if OK */

	pinMode(p_platform->m_lpnPin, OUTPUT);
	digitalWrite(p_platform->m_lpnPin, LOW);
	/* Set pin AVDD to LOW */
	/* Set pin VDDIO  to LOW */
	WaitMs(p_platform, 100);

	pinMode(p_platform->m_lpnPin, OUTPUT);
	digitalWrite(p_platform->m_lpnPin, HIGH);
	/* Set pin AVDD of to HIGH */
	/* Set pin VDDIO of  to HIGH */
	WaitMs(p_platform, 100);

	return status;
}

void SwapBuffer(
	uint8_t* buffer,
	uint16_t 	 	 size) {
	uint32_t i, tmp;

	/* Example of possible implementation using <string.h> */
	for (i = 0; i < size; i = i + 4) {
		tmp = (
			buffer[i] << 24)
			| (buffer[i + 1] << 16)
			| (buffer[i + 2] << 8)
			| (buffer[i + 3]);

		memcpy(&(buffer[i]), &tmp, 4);
	}
}

uint8_t WaitMs(
	VL53L5CX_Platform* p_platform,
	uint32_t TimeMs) {
	uint8_t status = 255;
	delay(TimeMs);
	/* Need to be implemented by customer. This function returns 0 if OK */

	return 0;
}
