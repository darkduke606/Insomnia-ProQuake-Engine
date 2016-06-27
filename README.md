# PSP_Insomnia_ProQuake_Engine

Insomnia Proquake 4.71 Revision 2

=====INTRODUCTION=====
 
All,
 
With the support of WorldGenesis[/r/psp], Baker, and MDave, I am happy to release this new update to the ProQuake 4.71 engine for the Sony Playstation Portable. This updated engine release & mod compilation features seven fully fledged custom campaigns for Quake and has been optimized for mod usage, with focus on ease of use. It has taken many hours of trial and error to code and compile this edition of the source. Primarily due to the fact that when I began work on this project, I had next to zero experience with developing PSP homebrew and absolutely no experience with C/C++, let alone compiling code for the psptoolchain. Big thanks to WorldGenesis for his patience with me, MDave for creating Kurok and Baker for this amazing ProQuake netcode implementation. Also big thanks to Jurak Styk for his original menu code from an earlier iteration of Quake on PSP!!! I have decided on naming the release "Insomnia", as Insomnia is one of my favorite custom campaigns by czg. Also naming it thus as I had a late night breakthrough and stayed up until dawn working on the project.
 
======FEATURES======
 
This version of the engine is optimized for PSP Slim's 64mb of ram. It also features seven, vanilla Quake user created campaigns.
 
Added functionality with "-prompt" menu.
 
When the "-prompt" command line argument is added to the quake.cmdline file (done be default), the user is prompted to select a mod folder (first option "blank" for default Quake Campaign) upon initial boot of the Insomnia game.

Added texture filtering options for ProQuake Engine via prompt menu
 
Other options include ability to set cpu speed and heapsize for memory allocation.
I recommend using CPU Speed: 333 and Heapsize: 38 for the PSP Slim, and for the PS Vita/PSP Phat to use CPU Speed: 333 and Heapsize: 16
 
======DOWNLOAD======
 
Included in the archive are the respective game folder, eboot, and source code.
PSP SLIM 64MB VERSION [REV2]: [url]https://www.dropbox.com/s/kpvaia5abwz1mcl/Insomnia%20ProQuake%20471%20Rev2.rar?dl=0[/url]
PSP PHAT 32MB VERSION [INITIAL RELEASE](EBOOT ONLY): [url]https://www.dropbox.com/s/r155cd0a2nyxzt0/Insomnia_PSP100X%20PHAT%20EBOOT.rar?dl=0[/url]

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

=====PLANNED FEATURES=====

Hipnotic mode switch on prompt menu (already functioning in unreleased Dev version)

More control switch options/binding settings in menu

Suggestions? Leave a comment!
 
=====REFERENCE FOR CODERS/Developers=====
If you wish to compile the source yourself, I recommend using CYGWIN as well as this mirror of an older iteration of the psptoolchain [url=http://psp.jim.sh/svn/psp/trunk/psptoolchain/]psptoolchain - Revision 2494[/url] 
Please feel free to reach out to me if you would like a precompiled environment with the required version of the psptoolchain

SOURCE CODE: [url="https://www.dropbox.com/s/3nc6i0jv46vb1iw/SOURCE%20Insomnia%20Proquake%20471%20Rev2.rar?dl=0"]Insomnia ProQuake 4.71 Revision 2 Source Code[/url]

=====================CHANGELOG===================

[code]
June 25, 2016 - Revision 2
-------------

Added "Texture Filtering" to prompt menu. 
Equivalent of "gl_texturemode GL_LINEAR"(Texture Filtering On) and
"gl_texturemode GL_NEAREST"(Texture Filtering Off)

Bot Options under Multiplayer tab are now linked for FROGBOT
Bot Options will autohide when not in FROGBOT

Added Crosshair Enable/Disable Option under OPTIONS > SUBMENU MISC OPTIONS
Added Autoaim Enable/Disable Option under OPTIONS > SUBMENU MISC OPTIONS

Fixed mp3/audio issues on startup and when returning from standby mode
