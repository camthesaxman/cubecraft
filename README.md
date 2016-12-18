# CubeCraft
CubeCraft is a 3D infinite-world voxel engine for the Nintendo GameCube and Wii.

![Screenshot](http://i.imgur.com/KO1o4d5.png)

## Compiling
Download DevKitPro and run `make` to compile. By default, it builds the GameCube version. To build the Wii version, run `make PLATFORM=wii`.

## Installing

### Wii
Make sure there is an apps directory on the root of your SD card, create a directory called `cubecraft` inside the apps directory, and place boot.dol and meta.xml inside the cubecraft directory. Launch it with the Homebrew Channel on your Wii.

### GameCube
For consoles that have not been soft-modded, you will need to use a game save exploit such as Home Bros to run this. The boot.dol file must be converted to a .gci file with dol2gci and placed on the GameCube memory card along with the hacked save file used to launch the exploit. More details on running GameCube homebrew can be found on the GC-Forever Wiki. <http://www.gc-forever.com/wiki/index.php?title=Booting_Homebrew>

Alternatively, you may run the game on a PC using Dolphin Emulator.

## Controls
The controls are simple. This game uses the GameCube controller exclusively.

### Menus
* <img src=http://www.ssbwiki.com/images/e/e0/ButtonIcon-GCN-Control_Stick-U.png width=50><img src=http://www.ssbwiki.com/images/0/0a/ButtonIcon-GCN-Control_Stick-D.png width=50><img src=http://www.ssbwiki.com/images/5/5a/ButtonIcon-GCN-D-Pad-U.png width=50><img src=http://www.ssbwiki.com/images/5/50/ButtonIcon-GCN-D-Pad-D.png width=50> Change selection
* <img src=http://www.ssbwiki.com/images/4/46/ButtonIcon-GCN-A.png width=50> Select
* <img src=http://www.ssbwiki.com/images/9/9f/ButtonIcon-GCN-B.png width=50> Exit menu

### Field
* <img src=http://www.ssbwiki.com/images/c/c4/ButtonIcon-GCN-Control_Stick.png width=50> Walk
* <img src=http://www.ssbwiki.com/images/1/13/ButtonIcon-GCN-C-Stick.png width=50> Change camera angle
* <img src=http://www.ssbwiki.com/images/4/46/ButtonIcon-GCN-A.png width=50> Jump
* <img src=http://www.ssbwiki.com/images/9/9f/ButtonIcon-GCN-B.png width=50> Remove block
* <img src=http://www.ssbwiki.com/images/4/48/ButtonIcon-GCN-Y.png width=50> Place block
* <img src=http://www.ssbwiki.com/images/0/08/ButtonIcon-GCN-D-Pad-L.png width=50><img src=http://www.ssbwiki.com/images/4/47/ButtonIcon-GCN-D-Pad-R.png width=50> Select item
* <img src=http://www.ssbwiki.com/images/b/b7/ButtonIcon-GCN-Start-Pause.png width=50> Pause game
