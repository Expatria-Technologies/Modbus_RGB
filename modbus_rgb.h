/*

modbus_io.h - plugin for for MODBUS I/O extension of grblHAL

Copyright (c) 2024 Richard Toth

This plugin is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This plugin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this plugin.  If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef _MBRGB_H_
#define _MBRGB_H_
#define MBRGB_RETRIES     3

typedef struct {
    bool RGB_modbus_enable;
    uint16_t RGB_modbus_address;
    uint16_t RGB_modbus_Coil; //added by empyrean 2025-11-24
} mbrgb_settings_t;



#endif