# yamt-vita
Yet another (re)mount tool kernel plugin for PS Vita/PS TV

Requires enso, compatible ONLY with firmware 3.60 and 3.65.

# Features
 - Ability to remount all partitions
 - Clean sd2vita patch system
 - No boot delays as found in i.e gamecard-microsd
 - Basic & noob-friendly gui manager included.

# Installation
 1) Remove the previous driver, if you had yamt - remove it with the NEW installer.
 2) Install the vpk
 3) Open the app, choose the option that suits you.
	- Lite version is intended for normal users, it provides basic mounting options.
	- Full version is recommended for advanced users, it packs all the important storage managing tools.
 4) Reboot, you should be able to access a new menu under the "Devices" tab in the System Settings app.
 
# Usage [basic]
 I think that the basic usage is pretty straightforward, just use the menu in Settings->Devices->Storage Devices.
 
# Usage [advanced]
 - You can remount every partition with yamt.
 - Developer options in driver settings provide some useful storage functions like formatting.
 - In the 'Custom partitions" tab you have all the partitions listed excluding pseudo partitions like lma0.
 - You can edit the assignements of the listed partitions, please take a look at https://wiki.henkaku.xyz/vita/SceIofilemgr#Mount_Points.
   - i.e if you set sa0 to [ext; act; entire] it will bind sa0 to the sd2vita's main partition at boot.
 
# Uninstallation
 - You can uninstall YAMT via the provided installler.

# Manual Installation
 1) Add yamt.skprx to enso's boot_config.txt
 2) Add yamt.suprx to tai config.txt under \*NPXS10015
 3) Add yamt_helper.skprx to tai config under \*KERNEL
 
# Notes
 - To compile simply run ". create_vpk.sh".
 - The project is still WIP, report all bugs.
 - Before updating from a beta release, use enso's "fix boot configuration".
 - You can format the SD/USB to TexFAT from the developer options menu in the driver settings tab.
 - If the USB-as-ux0 mounting fails, setting legacy mode in driver settings may help.
  
# Credits
  - TheOfficialFlow, xyz for their work on vitashell/gamesd
  - Team Molecule for enso and henkaku
