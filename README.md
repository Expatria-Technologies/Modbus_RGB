# Modbus_RGB
A plugin to add rgb capacity to a grblhal control board

This was written with the intent of being used with the R4DVI04 eletechsup modbus IO board which carries 4 relays which can be used to drive a simple RGB LED Strip

<img width="419" height="315" alt="image" src="https://github.com/user-attachments/assets/6c3b4d17-78bd-4ff6-89f2-90f75ca304a8" />

Prerequisites:

Modbus MUST be enabled in your firmware.

WARNING:
In order to reduce modbus traffic from the spindle plugin i had to modify the spindle plugin. I used a gs20 spindle equivalent. You can find my files for that here: https://github.com/EmpyreanCNC/Plugins_spindle_with_MBIO/tree/master

FURTHER NOTES:

This required the addition of 2 new settings. I will be confirming with Terje whether these can stay allocated to the numbers I chose and whether the change to the spindle plugin above will be rolled into the main


HOW TO INSTALL
Make a new src folder/directory called rgb in the grblHAL src directory and put the content of this repository in the rgb directory.

<img width="156" height="221" alt="image" src="https://github.com/user-attachments/assets/e5497bdf-81aa-41b0-8230-1667a8ba6cef" />


Edit grbl/plugins_init.h to add the init code of the plugin. You can add it at the end.

#if MBRGB_ENABLE
	    extern void mbrgb_init (void);
	    mbrgb_init();
#endif 

I have been compiling using the platformio.ini file(I obtained a template by downloading a firmware with the features I wanted from here, https://svn.io-engineering.com:8443/?dev=1  it came with the .ini file)
 I added the line,
 
  -D MBRGB_ENABLE=1

  Compile and flash your machine and enjoy.
