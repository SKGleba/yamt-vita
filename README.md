# yamt-vita
Yet another (re)mount tool kernel plugin for PS Vita/PS TV

Requires enso, compatible ONLY with firmware 3.60 and 3.65.

# Features
 - Ability to remount all partitions
 - Clean sd2vita patch system
 - No boot delays as found in i.e gamecard-microsd
 - Basic & noob-friendly gui manager included.

# Installation
 1) Remove the previous driver, if you had yamt - remove it with the old installer
 2) Install the vpk
 3) Open the app, the installer will do the rest for you
 4) Reboot, you should be able to access a new menu under the "Devices" tab in the System Settings app.
 
# Usage [basic]
 I think that the basic usage is pretty straightforward, just use the menu in Settings->Devices->Storage Devices.
 
# Usage [advanced]
 - You can remount every partition with yamt.
 - In the 'Advanced" tab you have all the partitions listed excluding pseudo and alias partitions like lma0 and xmc0.
 - You can edit the assignements of the listed partitions, please take a look at https://wiki.henkaku.xyz/vita/SceIofilemgr#Mount_Points.
   - i.e if you set sa0 to [ext; act; entire] it will bind sa0 to the sd2vita's main partition at boot.
 
# Uninstallation
 1) Open the app, the installer will remove it
 2) Remove ur0:tai/yamt.cfg

# Manual Installation
 1) Add yamt.skprx to enso's boot_config.txt
 2) Add yamt.suprx to tai config.txt under \*NPXS10015
 
# Notes
 - The project is still WIP, report all bugs.
 - This plugin patches xmc0/imc0 as ux0, you won't be able to separately mount those
   - i.e you can't mount xmc0 while ux0 is set to sd2vita.
  
 # Credits
  - TheOfficialFlow, xyz for their work on vitashell/gamesd
  - Team Molecule for enso and henkaku
