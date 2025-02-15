#include "gxht30.h"


GXHT30::GXHT30(uint8_t gxht30_address)
    : sensor_address(gxht30_address) {}

uint8_t GXHT30::split_request(
    uint16_t* req,
    uint8_t significance
) {
    uint8_t split;
    if(!significance) {
        split = (uint8_t)(*req & 0xFF);
    } else {
        split = (uint8_t)((*req >> 8) & 0xFF);
    }
    return split;
}

uint8_t GXHT30::check_crc(
    uint8_t* data,
    uint8_t is_status_register
) {
    uint8_t crc = 0xFF;
    uint8_t crc_validity = 3;
    if(!is_status_register) {
        for (uint8_t i = 5; i > 3; i--) {
                crc = crc8x_lut[data[i] ^ crc];
        }
        if(crc == data[3]) {
            crc_validity -= 1;
        }
        crc = 0xFF;
    }
    for (uint8_t i = 2; i > 0; i--) {
         crc = crc8x_lut[data[i] ^ crc];
    }
    if(crc == data[0]) {
        crc_validity -= 2;
    }
    // 0: all data valid
    // 1: only relative humidity or status register valid
    // 2: only temperature valid
    // 3: no data valid
    return crc_validity;
}

void GXHT30::set_clk_stretching(
    uint8_t req
) {
    if(req == 0x2C) {
        if(!is_clk_stretching) {
            is_clk_stretching = 1;
        }
    } else {
        if(is_clk_stretching) {
            is_clk_stretching = 0;
        }
    }
}

void GXHT30::mark_invalid_data(
    uint8_t* data,
    uint8_t data_length,
    uint8_t data_validity
) {
    switch(data_validity) {
        case 3:
            for(uint8_t i = 0; i < data_length; i++) {
                data[i] = 0xFF;
            }
            break;
        case 2:
            for(uint8_t i = 0; i < 3; i++) {
                data[i] = 0xFF;
            }
            break;
        case 1:
            if(data_length > 3) {
                for(uint8_t i = 3; i < 6; i++) {
                    data[i] = 0xFF;
                }
            }
            break;
    }
}

uint8_t GXHT30::transmit_request(
    uint16_t* req
) {
    Wire.beginTransmission(sensor_address);
    Wire.write(split_request(req, GXHT30_MSB));
    Wire.write(split_request(req, GXHT30_LSB));
    uint8_t tx_status = Wire.endTransmission(1);
    return tx_status;
};

uint8_t GXHT30::request_to_gxht30(
    uint16_t request
) {
    uint8_t tx_status = 4;
    uint8_t req_msb = split_request(&request, GXHT30_MSB);
    uint8_t cams = (
        req_msb == 0x20 || req_msb == 0x21 || req_msb == 0x22 ||
        req_msb == 0x23 || req_msb == 0x27 || req_msb == 0x2B
    );
    if(!is_continuous_dacqm && cams) {
        last_continuous_dacqm = request;
        is_continuous_dacqm = 1;
        is_clk_stretching = 0;
        tx_status = transmit_request(&request);
    } else if (
        is_continuous_dacqm && !cams && (request != GXHT30_FETCH_DATA)
    ) {
        uint16_t trash_req = GXHT30_PAM_STOP;
        tx_status = transmit_request(&trash_req);
        if(!tx_status) {
            uint32_t millis_old = millis();
            set_clk_stretching(split_request(&request, GXHT30_MSB));
            while((millis_old - millis()) < 16) {
                __asm("nop");
            }
            transmit_request(&request);
            if(
                (request == GXHT30_HEATER_ENABLE) ||
                (request == GXHT30_HEATER_DISABLE) ||
                (request == GXHT30_STRG_READ) ||
                (request == GXHT30_STRG_CLEAR)
            ) {
                transmit_request(&last_continuous_dacqm);
            } else {
                is_continuous_dacqm = 0;
            }
        }
    } else {
        if(cams) {
            last_continuous_dacqm = request;
        }
        set_clk_stretching(split_request(&request, GXHT30_MSB));
        tx_status = transmit_request(&request);
    }
    // For return values refer to https://docs.arduino.cc/language-reference/en/functions/communication/wire/endTransmission/
    // 0: success
    // 1: data too long to fit in transmit buffer
    // 2: received NACK on transmit of address
    // 3: received NACK on transmit of data
    // 4: other error
    // 5: timeout
    return tx_status;
}

void GXHT30::convert_temp(
    uint8_t* data
) {
    uint16_t raw = (uint16_t)((data[5] << 8) | data[4]);
    if(raw != GXHT30_INVALID_DATA) { 
        temp_rh[1] = 175*raw/65535-45;
    } else {
        temp_rh[1] = raw;
    }
}

void GXHT30::convert_rh(
    uint8_t* data
) {
    uint16_t raw = (uint16_t)((data[2] << 8) | data[1]);
    if(raw != GXHT30_INVALID_DATA) {
        temp_rh[0] = 100*raw/65535;
    } else {
        temp_rh[0] = raw;
    }
}

void GXHT30::convert_temp_and_rh(
    uint8_t* data
) {
    convert_temp(data);
    convert_rh(data);
}

uint16_t* GXHT30::get_temp_and_rh(
    uint8_t convert
) {
    uint8_t data[6];
    for(uint8_t i = 0; i < 6; i++) {
        data[i] = (uint8_t)((GXHT30_INVALID_DATA >> 8) & 0xFF);
    }
    if(is_clk_stretching != 0xFF) {
        if(is_clk_stretching) {
            Wire.requestFrom(sensor_address, 6, 0);
        } else {
            if(is_continuous_dacqm) {
                uint8_t tx_status = request_to_gxht30(GXHT30_FETCH_DATA);
                if(!tx_status) {
                    Wire.requestFrom(sensor_address, 6, 1);
                }
            } else {
                Wire.requestFrom(sensor_address, 6, 1);
            }
        }
    }
    uint8_t data_validity = 3;
    if(uint8_t i = Wire.available()) {
        for(i; i > 0; i--) {
            data[i - 1] = Wire.read();
        }
        data_validity = check_crc(data, 0);
    }
    mark_invalid_data(data, 6, data_validity);
    if(convert) {
        convert_temp_and_rh(data);
    } else {
        temp_rh[1] = (uint16_t)((data[5] << 8) | data[4]);
        temp_rh[0] = (uint16_t)((data[2] << 8) | data[1]);
    }
    // MSB: temperature
    // LSB: relative humidity
    // 0xFF: invalid data byte
    return temp_rh;
}

uint16_t GXHT30::request_status_register(
    uint16_t request
) {
    uint8_t data[3];
    for(uint8_t i = 0; i < 3; i++) {
        data[i] = (uint8_t)((GXHT30_INVALID_DATA >> 8) & 0xFF);
    }
    uint8_t tx_status = request_to_gxht30(request);
    if(!tx_status && (request == GXHT30_STRG_READ)) {
        uint8_t i = Wire.requestFrom(sensor_address, 3, 1);
        for(i; i > 0; i--) {
            data[i - 1] = Wire.read();
        }
    }
    uint8_t data_validity = check_crc(data, 1);
    mark_invalid_data(data, 3, data_validity);
    uint16_t reg_content = (uint16_t)((data[2] << 8) | data[1]);
    return reg_content;
}
