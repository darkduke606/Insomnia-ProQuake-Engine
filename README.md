# PSP_Insomnia_ProQuake_Engine

Insomnia Proquake 4.71 Revision 4

=====INTRODUCTION=====
 
All,
 
With the support of WorldGenesis[/r/psp], Baker, and MDave, I am happy to release this new update to the ProQuake 4.71 engine for the Sony Playstation Portable. This updated engine release & mod compilation features seven fully fledged custom campaigns for Quake and has been optimized for mod usage, with focus on ease of use. It has taken many hours of trial and error to code and compile this edition of the source. Primarily due to the fact that when I began work on this project, I had next to zero experience with developing PSP homebrew and absolutely no experience with C/C++, let alone compiling code for the psptoolchain. Big thanks to WorldGenesis for his patience with me, MDave for creating Kurok and Baker for this amazing ProQuake netcode implementation. Also big thanks to Jurak Styk for his original menu code from an earlier iteration of Quake on PSP!!! I have decided on naming the release "Insomnia", as Insomnia is one of my favorite custom campaigns by czg. Also naming it thus as I had a late night breakthrough and stayed up until dawn working on the project.
 
======FEATURES======
 
This version of the engine is optimized for PSP Slim's 64mb of ram. Also now includes a 32mb PHAT compatible eboot.
 
Added functionality with "-prompt" menu.
 
When the "-prompt" command line argument is added to the quake.cmdline file (done be default), the user is prompted to select a mod folder (first option "blank" for default Quake Campaign) upon initial boot of the Insomnia game.

Added texture filtering options for ProQuake Engine via prompt menu
 
Other options include ability to set cpu speed and heapsize for memory allocation.
I recommend using CPU Speed: 333 and Heapsize: 38 for the PSP Slim, and for the PS Vita/PSP Phat to use CPU Speed: 333 and Heapsize: 14

======INSTALLATION======
 
Insomnia ProQuake 4.71 Rev2 is optimized for PSP Slim, though I have included a PSP Phat compatible eboot (though it is less optimized and may struggle to run mods)
 
1) Copy contents of "PSP" folder to the root of your PSP memory stick
 
2) As usual, copy PAKS ONLY from ID1 folder from to Insomnia\ID1\ path.
 
Should resemble the following
PSPMEMSTICK:\PSP\Game\Insomnia\ID1\PAK0.PAK
PSPMEMSTICK:\PSP\Game\Insomnia\ID1\PAK1.PAK
 
**STEP ONLY FOR PSP 100X/PHAT/VITA: Replace "eboot.pbp" in Insomnia folder with the one included in the PSP Phat 32mb version archive.
 
3) Enjoy :-)
 
Refer to included "Insomnia Proquake_471_readme.txt" for more information

 
=====REFERENCE FOR CODERS/Developers=====
If you wish to compile the source yourself, I recommend using CYGWIN as well as this mirror of an older iteration of the psptoolchain ttp://psp.jim.sh/svn/psp/trunk/psptoolchain/]psptoolchain - Revision 2494
Please feel free to reach out to me if you would like a precompiled environment with the required version of the psptoolchain

=====================CHANGELOG===================

July 1, 2016 - Revision 4
---------------
Added modmusic support
Will load any mp3s labeled track02 or above that is in the modfolders respective path \<modfolder>\music\

Set bobcycles to engine defaults

Added ability to set "defaults" to the prompt menu.

Example, add -setmodmusic or -linear to your quake.cmdline to select Texture Filtering to On and Mod Music to On in the prompt menu by             default. Applies to memory allocation with "-heap 33" or "-cpu 333"

When sv_aim is enabled, defaults to .8

Added keys to smallest visible HUD

Added "Rogue" mode for Dissolution of Eternity -- you must have Dissolution of Eternity

More than likely fixed the disappearing rune bug regarding saves -- not confirmed

Compiled a 32mb version for PSP 1000 models

July 1, 2016 - Revision 3
-------------

Force disabled dynamic lighting in deathmatch mode

Fixed Dynamic Lighting bugs relating to viewmodels/enemy models
illuminating when dynamic lights disabled

Added "Hipnotic" support for mods that require -hipnotic to the prompt menu 
 -- you must have Scourge of Armagon

Added Vertical Aiming slider to MISC OPTIONS

Added more mods

Removed expiremental fog support

Renamed Autoaim to "EasyAim"

June 25, 2016 - Revision 2
-------------

Added "Texture Filtering" to prompt. 
Equivalent of "gl_texturemode GL_LINEAR"(Texture Filtering On) and
"gl_texturemode GL_NEAREST"(Texture Filtering Off)
The difference is texture smoothing vs "pixelated" textures. 
 I prefer the raw, pixelated textures as intended.

Bot Options under Multiplayer tab are now linked for FROGBOT
Bot Options will autohide when not in FROGBOT

Added Crosshair Enable/Disable Option under OPTIONS > SUBMENU MISC OPTIONS
Added Autoaim Enable/Disable Option under OPTIONS > SUBMENU MISC OPTIONS

Fixed mp3/audio issues on startup and when returning from standby mode

***KNOWN BUGS***
If Dynamic Lighting is disabled, certain models and objects will continue to be
affected by dynamic lighting (weapons/character models)


June 21, 2016 - Revision 1
-------------

Bug fixes and more optimization
Corrected heap/cpu speed paramater bugs
Corrected mp3 directory
Set cl_autoaim 0 in compile and config.cfg


June 19, 2016 - Initial Release
-------------

Added -prompt functionality including setting cpu speed, heapsize and mod folder
Mod compilation of user generated mods that function on PSP
Optimized for PSP Slim 64mb version. Will not function as stands for PSP 1000 models
