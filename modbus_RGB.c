//Written by EmpyreanCNC

//This plugin is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This plugin is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with this plugin.  If not, see <http://www.gnu.org/licenses/>.

//this plugin was written with the intent of being used with the R4DVI04 eletechsup modbus IO board which carries 4 relays which can be used to drive a simple RGB LED Strip.

#if STATUS_LIGHT_ENABLE
#include <math.h>
#include "grbl/hal.h"
#include "grbl/state_machine.h"
#include "grbl/system.h"
#include "grbl/alarms.h"
#include "grbl/nuts_bolts.h"
#include "grbl/modbus.h"
#include "modbus_rgb.h"
#include "grbl/report.h"
#include "grbl/nvs_buffer.h"


static on_state_change_ptr on_state_change;
static on_report_options_ptr on_report_options;
static on_program_completed_ptr on_program_completed;
static on_spindle_selected_ptr on_spindle_selected;

static nvs_address_t nvs_address;

mbrgb_settings_t mbrgb_config;

static const setting_detail_t mbrgb_settings[] = {
     { Setting_Action0, Group_AuxPorts, "RGB Modbus Device Enable", NULL, Format_Bool, NULL, NULL, NULL, Setting_NonCore, &mbrgb_config.RGB_modbus_enable, NULL, NULL },
     { Setting_Action1, Group_AuxPorts, "RGB Modbus Device Address", "", Format_Int16, "###0", "1", "250", Setting_NonCore, &mbrgb_config.RGB_modbus_address, NULL, NULL },
     { Setting_Action2, Group_AuxPorts, "RGB Modbus Starting Coil Address", "", Format_Int16, "###0", "1", "250", Setting_NonCore, &mbrgb_config.RGB_modbus_Coil, NULL, NULL },
};

static const setting_descr_t mbrgb_settings_descr[] = {
    { Setting_Action0, "RGB Modbus device is enabled" },
    { Setting_Action1, "RGB Modbus Device Address" },
    { Setting_Action2, "RGB Modbus Starting Coil Address" },
};

static void mbrgb_settings_save (void)
{
    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&mbrgb_config, sizeof(mbrgb_settings_t), true);
}

static void mbrgb_settings_restore (void)
{
    mbrgb_config.RGB_modbus_address = 1;
    mbrgb_config.RGB_modbus_Coil = 0;
    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&mbrgb_config, sizeof(mbrgb_settings_t), true);
}

static void mbrgb_settings_load (void)
{
    if((hal.nvs.memcpy_from_nvs((uint8_t *)&mbrgb_config, nvs_address, sizeof(mbrgb_settings_t), true) != NVS_TransferResult_OK))
        mbrgb_settings_restore();
}

static const modbus_callbacks_t callbacks = {
    .retries = MBRGB_RETRIES,
};

#define value 3             //this value defines how many coils to set the state for, IE if we start at address 0 we will write 0,1 & 2             

int RGB_OFF     =0; //turns all (the 3 we are writing to) channels off       
int RGB_WHITE   =7; //turns all 3 on. 7
int RGB_RED     =1; //turns Ch1 on
int RGB_GREEN   =2; //turns Ch2 on
int RGB_BLUE    =4; //turns Ch3 on
int RGB_YELLOW  =3; //turns Ch1 & Ch2 on
int RGB_MAGENTA =5; //turns Ch1 & Ch3 on
int RGB_CYAN    =6; //turns Ch2 & Ch3 on
int pack;           //this variable is what is packed into the modbus message to the device

void mbrgb_ModBus_WriteCoils(int pack) {modbus_message_t _cmd = {
        //.context = (void *)MBIO_Command,
        .crc_check = true,
        .adu[0] = mbrgb_config.RGB_modbus_address,                        // slave device address 
        .adu[1] = 15,                                                     // function code this is the multiple coil write modbus command code
        .adu[2] = MODBUS_SET_MSB16(mbrgb_config.RGB_modbus_Coil),         // start address MSB 
        .adu[3] = MODBUS_SET_LSB16(mbrgb_config.RGB_modbus_Coil),         // start address LSB
        .adu[4] = MODBUS_SET_MSB16(value),                                // quantity MSB
        .adu[5] = MODBUS_SET_LSB16(value),                                // quantity LSB
        .adu[6] = (3 + 7) / 8,                                            // byte count (ceil of bits/8)
        // coil values packed into bytes follow here
        .tx_length = 7 + ((3 + 7) / 8) + 2,                               // header + data + CRC
        .rx_length = 8                                                    // response length (echo start + quantity)
    };

    // Copy coil values into ADU after byte count
    for (int i = 0; i < (3 + 7) / 8; i++) {
        _cmd.adu[7 + i] = pack;
    }
    modbus_send(&_cmd, &callbacks, true); //parcels this up and passes it to the modsbus_send function/queue, the callbacks include retry count. The retry count is set in modbusrgb.h
}

static void rgb_state_changed (sys_state_t state)
{
        if (!mbrgb_config.RGB_modbus_enable)
            return;

        if(state == STATE_IDLE) { //if the machine is in idle state, pass the RGB_white value to Pack
            pack = RGB_WHITE;                     
            }
        else if(state == STATE_CYCLE) {
            pack = RGB_WHITE;
            }
        else if(state == STATE_JOG) {
            pack = RGB_GREEN;                     
            }
        else if(state == STATE_ALARM) {
            pack = RGB_RED;
            }
        else if(state == STATE_HOMING) {
            pack = RGB_CYAN;
            }
        else if(state == STATE_TOOL_CHANGE) {
            pack = RGB_BLUE;
            }
        else if(state == STATE_HOLD) {
            pack = RGB_YELLOW;
            }

        mbrgb_ModBus_WriteCoils(pack); //call our modbus message assembly function

}


static void onStateChanged (sys_state_t state)
{
    static sys_state_t state_prev = STATE_CHECK_MODE; // checks the current machine state agains the previous state and runs rgb_state_changed if they differ

    if(state != state_prev) {
        rgb_state_changed(state);
        state_prev = state;
    } 

}

static void mbrgb_report_options(bool newopt) {
    on_report_options(newopt);

    if (newopt) {
        hal.stream.write("RGB Modbus");
    }
    else {
        hal.stream.write("[PLUGIN:RGB Modbus v1.00]" ASCII_EOL);
    }
}

void status_light_init(void) {
        on_report_options = grbl.on_report_options;         // Subscribe to report options event
        on_state_change = grbl.on_state_change;             // Subscribe to the state changed event by saving away the original
        on_program_completed = grbl.on_program_completed;   // Subscribe to on program completed events (lightshow on complete?)
        //on_execute_realtime = grbl.on_execute_realtime;     // Subscribe to the realtime execution event
        grbl.on_state_change = onStateChanged;
        on_spindle_selected = grbl.on_spindle_selected;
        grbl.on_report_options = mbrgb_report_options;

    static setting_details_t mbrgb_setting_details = {
        .settings = mbrgb_settings,
        .n_settings = sizeof(mbrgb_settings) / sizeof(setting_detail_t),
    #ifndef NO_SETTINGS_DESCRIPTIONS
        .descriptions = mbrgb_settings_descr,
        .n_descriptions = sizeof(mbrgb_settings_descr) / sizeof(setting_descr_t),
    #endif
        .load = mbrgb_settings_load,
        .restore = mbrgb_settings_restore,
        .save = mbrgb_settings_save
    };

        if(modbus_enabled() && (nvs_address = nvs_alloc(sizeof(mbrgb_settings_t)))) 
    {
        settings_register(&mbrgb_setting_details);
    };
    };

#endif


