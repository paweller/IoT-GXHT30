[![MIT license](https://img.shields.io/badge/license-MIT-brightgreen.svg)](https://opensource.org/licenses/MIT)

## Overview
A C/C++ library for the temperature and relative humidity IoT sensor GXHT30. It works right out of the box for all MCUs that are compatible with the <Wire.h> library. Examples for such MCUs are Arduinos, ESP8266s, and ESP32s.

## API

```c
/************ Constructor ************/
GXHT30(uint8_t gxht30_address = GXHT30_ADDRESS_DEFAULT)

/************ Fuctions ************/
uint8_t request_to_gxht30(uint16_t request);
uint16_t* get_temp_and_rh(uint8_t convert);
uint16_t request_status_register(uint16_t request)

/************ Defines ************/
// I2C addresses
GXHT30_ADDRESS_DEFAULT
GXHT30_ADDRESS_ALTERED

// Single shot mode
GXHT30_SSM_CLKST_ENABLED_RPTY_HIGH
GXHT30_SSM_CLKST_ENABLED_RPTY_MEDIUM
GXHT30_SSM_CLKST_ENABLED_RPTY_LOW
GXHT30_SSM_CLKST_DISABLED_RPTY_HIGH
GXHT30_SSM_CLKST_DISABLED_RPTY_MEDIUM
GXHT30_SSM_CLKST_DISABLED_RPTY_LOW

// Periodic data acquisition mode
GXHT30_PAM_MPS05_HIGH
GXHT30_PAM_MPS05_MEDIUM
GXHT30_PAM_MPS05_LOW
GXHT30_PAM_MPS1_HIGH
GXHT30_PAM_MPS1_MEDIUM
GXHT30_PAM_MPS1_LOW
GXHT30_PAM_MPS2_HIGH
GXHT30_PAM_MPS2_MEDIUM
GXHT30_PAM_MPS2_LOW
GXHT30_PAM_MPS4_HIGH
GXHT30_PAM_MPS4_MEDIUM
GXHT30_PAM_MPS4_LOW
GXHT30_PAM_MPS10_HIGH
GXHT30_PAM_MPS10_MEDIUM
GXHT30_PAM_MPS10_LOW

// Accelerated response time mode
GXHT30_ART

// Read-out of periodic data acquisition
GXHT30_FETCH_DATA

// Stop periodic data acquisition mode
GXHT30_PAM_STOP

// Heater
GXHT30_HEATER_ENABLE
GXHT30_HEATER_DISABLE

// Reset
GXHT30_RESET_SOFT
GXHT30_RESET_GENERAL_CALL

// Status register
GXHT30_STRG_READ
GXHT30_STRG_CLEAR
GXHT30_STRG_ALERT_PENDING
GXHT30_STRG_ALERT_RH
GXHT30_STRG_ALERT_T
GXHT30_STRG_HEATER
GXHT30_STRG_RESET_DETECTED
GXHT30_STRG_LAST_COMMAND
GXHT30_STRG_CHEKSUM

// Data conversion
CONVERSION_FALSE
CONVERSION_TRUE
INVALID_DATA
```

## Dependencies
```c
<Wire.h>
```

## Dummy code examples
Single-shot data acquisition modes
```c
#include "gxht30.h"

GXHT30 gxht30;
// GXHT30 gxht30(GXHT30_ADDRESS_ALTERED);

void setup() {}

void loop() {
    uint8_t tx_status = gxht30.request_to_gxht30(
        GXHT30_SSM_CLKST_ENABLED_RPTY_HIGH;
    );
    delay(16);
    uint16_t* data = gxht30.get_temp_and_rh(CONVERSION_TRUE);
    uint8_t temperature = (&data[1]);
    uint8_t relative_humidity = (&data[0]);
}
```
\
Periodic data acquisition modes
```c
#include "gxht30.h"

GXHT30 gxht30;
// GXHT30 gxht30(GXHT30_ADDRESS_ALTERED);

void setup() {
    uint8_t tx_status = gxht30.request_to_gxht30(
        GXHT30_PAM_MPS10_LOW;
    );
}

void loop() {
    delay(16);
    uint16_t* data = gxht30.get_temp_and_rh(CONVERSION_TRUE);
    uint8_t temperature = (&data[1]);
    uint8_t relative_humidity = (&data[0]);
}
```
\
Controllinge the heater
```c
#include "gxht30.h"

GXHT30 gxht30;
// GXHT30 gxht30(GXHT30_ADDRESS_ALTERED);

void setup() {}

void loop() {
    uint8_t tx_status = gxht30.request_to_gxht30(GXHT30_HEATER_ENABLE);
    delay(2000);
    tx_status = gxht30.request_to_gxht30(GXHT30_HEATER_DISABLE);
}
```
\
Interfacing the status register
```c
#include "gxht30.h"

GXHT30 gxht30;
// GXHT30 gxht30(GXHT30_ADDRESS_ALTERED);

void setup() {}

void loop() {
    uint16_t register_content = gxht30.request_status_register(GXHT30_STRG_READ);
    if((register_content & GXHT30_STRG_ALERT_PENDING) == GXHT30_STRG_ALERT_PENDING) {
        /* do something */
    } else {
        /* do something else */
    }
    uint8_t tx_status = gxht30.request_to_gxht30(GXHT30_STRG_CLEAR);
    // gxht30.request_status_register(GXHT30_STRG_CLEAR);
}
```
