# yamt-vita
This is a plugin simillar to storageMgr which mounts your SD2VITA as internal Sony memory card.

Requires enso!

# Features [basic]
 - Mounts SD2VITA as ux0 (internal storage aka Sony's memory cards).
 - Mounts Sony memory card/internal storage as uma0 (it will act as a usb, cannot be used for apps).
 - This plugin makes PsVita/TV boot up to 5 seconds faster 
 - Allows usage of SD2VITA in "safe mode" (booting while holding L)


# Usage [basic]
 1) Remove your current storage plugins (StorageMgr or gamecard-microsd) from your tai folder, to do this open your ux0 and ur0 in vitashell and go to tai folder (go in both ux0 and ur0 tai folders and check if you have them), remove gamesd.skprx and/or storagemgr.skprx (depends on which one you have, if you have both, remove them both)
 2) Copy downloaded VPK to your PsVita/TV with vitashell (either with USB or FTP)
 2) Install the VPK via vitashell.
 3) Open the app, the installer will do everything for you and auto quit.
 4) Restart your PsVita/TV.
