Quake for PSP v2.0
==================

Peter Mackay and Chris Swindle
31th March 2006


Introduction
------------

This is a port of id Software's Quake to the PSP, ported from the original source code.

We have combined the source for the original release with the network code from Multiplayer QUake and we have submitted all of the code to a SourceForge project. The project page is at https://sourceforge.net/cvs/?group_id=158726 where the full source is available.


Features
--------

The following is working fine:
- Software rendering
- Sound
- Adhoc connection for PSP - PSP games
- Infrastructure mode for connecting to servers
- Stable firmware V2+ support, including PSP-PSP & Access point network support via the latest eLoader (DHCP is recommended for v2.5+)
- On screen keyboard (not in V2+ at the moment)

Things missing:
- OSK support in V2+, never finishes initialising in v2+, looks like it might be due to the way the exploit works
- Hardware rendering. I have little PSP GU experience, so it'll take a while to do.

Known bugs:
- Fast key taps may go unnoticed when the frame rate is low. This is because we're not sure how to get buffered input on the PSP.

If you've found a bug, please post about it on the release's thread at DCEmu.


Installation
------------

Installation instructions:
1. READ THE CONTROLS SECTION BELOW.
2. Firmware v1.0 users:
   Copy the Quake folder from the 1.0 folder to the PSP/GAME folder on your PSP.
   Firmware v1.5 users:
   Copy the Quake and Quake% folders from the 1.5 folder to the PSP/GAME folder on your PSP.
   Firmware v2+ users:
   Copy the Quake folder from the 2.0+ folder to the PSP/GAME folder on your PSP.
3. Download the shareware version of Quake for the PC from here:
   https://sourceforge.net/project/showfiles.php?group_id=158726
4. Unzip or Unrar the shareware version you downloaded.
5. Copy the ID1 folder from the shareware version of Quake to inside your PSP/GAME/Quake folder.


Controls
--------

Regarding the buttons:

The PSP buttons are connected to the following keys during the game and when the menu is shown. You will need to go into Quake's options and configure the keys you want to use.

PSP      | Game key   | Menu key   | Default game function
---------+------------+------------+----------------------
SELECT   | ~          | ~          | Toggle console
START    | ESCAPE     | ESCAPE     | Show menu
LTRIGGER | LTRIGGER   |            |
RTRIGGER | RTRIGGER   |            |
UP       | UPARROW    | UPARROW    | Move forward
RIGHT    | RIGHTARROW | RIGHTARROW | Turn right
DOWN     | DOWNARROW  | DOWNARROW  | Move backwards
LEFT     | LEFTARROW  | LEFTARROW  | Turn left
TRIANGLE | TRIANGLE   |            |
CIRCLE   | CIRCLE     | ESCAPE     |
CROSS    | CROSS      | ENTER      |
SQUARE   | SQUARE     | OSK        |

For example, when you press CROSS, Quake gets a CROSS key press, which you will need to set to your desired action in the game options screen.

Regarding the analog nub:

If mouselook is turned on, then the analog nub is used to look around. You'll need to allocate other buttons for movement. If mouselook is turned off, which is the default, then the analog nub is used for movement.


Network
-------

There are two forms of networking support included in the this version of Quake (compatible with all Firmware versions capable of running homebrew).

Adhoc (PSP-PSP communication):
In order to use this mode go to the network menu and select adhoc, this will start the adhoc communication and then you can start a server or connect to another PSP using the usual method.

Infrastructure Mode:
If you have an access point then you can use this mode to connect to servers on the internet. Before connecting using this method for the first time please go to the setup menu and ensure that the access point is correct. If it is not then you can press left and right to select the correct one. If it does not show up then you have probably deleted an access point and there is currently a bug that if one access point is deleted it will not find the correct one to connect to, you can set the accesspoint variable to the config number in order to avoid this.

The following commands have been added to Quake for network support:
	net_adhoc 			(initialise adhoc libraries and connect)
	net_infra 			(connect to the default access point)
	accesspoint <access point #> 	(set the access point to the default, this will be saved on exit)


Support
-------

If you would like to help out the project then you can make a donation to the following paypal accounts so that we are able to buy extra equipment to make it more compatible with different firmware versions and to test adhoc play:

Peter Mackay  - mackay.pete+paypal@gmail.com
Chris Swindle - c_swindle@hotmail.com

If you can't get Quake running, try posting a comment on the DCEmu thread for the release and one of us should help you. Please make sure you've followed the installation instructions before posting.


Thanks
------

Big thanks go out to:
- All the great guys on the PS2Dev forum for all your hard work on the PSPSDK, and your helpful replies on the forum
- id Software, for supporting the game porting community by making their game sources GPL
- Hazel, for putting up with the complete lack of attention from Pete while he was working on this.
- Fanjita for the help with some code from the loader to help work out networking support via GTA and VSH
- McZonk for the OSK sample that he posted

Have fun and enjoy Quake!

End of file
-----------
