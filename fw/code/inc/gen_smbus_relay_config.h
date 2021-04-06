
//=========================================================
//
//Copyright 2021 Intel Corporation
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//
//===================

/**
 *
 * @file gen_smbus_relay_config.h
 *
 * @brief This generated header file contains macros and functions to configure SMBus
 *
 */

#ifndef INC_GEN_SMBUX_CONFIG_H_
#define INC_GEN_SMBUX_CONFIG_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"


#define NUM_RELAYS 3
#define MAX_I2C_ADDRESSES_PER_RELAY 13

// Relay1 PMBus1
#define RELAY1_I2C_ADDRESS1 0x90
#define RELAY1_I2C_ADDRESS2 0xAC
#define RELAY1_I2C_ADDRESS3 0xA2
#define RELAY1_I2C_ADDRESS4 0xB2
#define RELAY1_I2C_ADDRESS5 0xA0
#define RELAY1_I2C_ADDRESS6 0xB0
#define RELAY1_I2C_ADDRESS7 0x00
#define RELAY1_I2C_ADDRESS8 0x00
#define RELAY1_I2C_ADDRESS9 0x00
#define RELAY1_I2C_ADDRESS10 0x00
#define RELAY1_I2C_ADDRESS11 0x00
#define RELAY1_I2C_ADDRESS12 0x00
#define RELAY1_I2C_ADDRESS13 0x00
#define RELAY1_I2C_ADDRESSES {0x90, 0xAC, 0xA2, 0xB2, 0xA0, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

// Relay2 PMBus2
#define RELAY2_I2C_ADDRESS1 0xB4
#define RELAY2_I2C_ADDRESS2 0xD4
#define RELAY2_I2C_ADDRESS3 0x4A
#define RELAY2_I2C_ADDRESS4 0x4C
#define RELAY2_I2C_ADDRESS5 0xDC
#define RELAY2_I2C_ADDRESS6 0xEC
#define RELAY2_I2C_ADDRESS7 0xE0
#define RELAY2_I2C_ADDRESS8 0xB0
#define RELAY2_I2C_ADDRESS9 0xC4
#define RELAY2_I2C_ADDRESS10 0xCC
#define RELAY2_I2C_ADDRESS11 0xE4
#define RELAY2_I2C_ADDRESS12 0x00
#define RELAY2_I2C_ADDRESS13 0x00
#define RELAY2_I2C_ADDRESSES {0xB4, 0xD4, 0x4A, 0x4C, 0xDC, 0xEC, 0xE0, 0xB0, 0xC4, 0xCC, 0xE4, 0x00, 0x00}

// Relay3 HSBP
#define RELAY3_I2C_ADDRESS1 0x98
#define RELAY3_I2C_ADDRESS2 0xA4
#define RELAY3_I2C_ADDRESS3 0xD0
#define RELAY3_I2C_ADDRESS4 0xD8
#define RELAY3_I2C_ADDRESS5 0xE8
#define RELAY3_I2C_ADDRESS6 0xE0
#define RELAY3_I2C_ADDRESS7 0xA6
#define RELAY3_I2C_ADDRESS8 0x36
#define RELAY3_I2C_ADDRESS9 0x90
#define RELAY3_I2C_ADDRESS10 0xA0
#define RELAY3_I2C_ADDRESS11 0x92
#define RELAY3_I2C_ADDRESS12 0xA2
#define RELAY3_I2C_ADDRESS13 0x48
#define RELAY3_I2C_ADDRESSES {0x98, 0xA4, 0xD0, 0xD8, 0xE8, 0xE0, 0xA6, 0x36, 0x90, 0xA0, 0x92, 0xA2, 0x48}

#define RELAYS_I2C_ADDRESSES {RELAY1_I2C_ADDRESSES, RELAY2_I2C_ADDRESSES, RELAY3_I2C_ADDRESSES}

#endif /* INC_GEN_SMBUX_CONFIG_H_ */
