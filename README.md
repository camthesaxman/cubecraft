# CubeCraft
CubeCraft is a 3D infinite-world voxel engine for the Nintendo GameCube and Wii.

![Screenshot](http://i.imgur.com/ZmHaTfp.png)

## Compiling
Download DevKitPro and run `make` to compile. By default, it builds the GameCube version. To build the Wii version, run `make PLATFORM=wii`.

## Installing
### Wii
Make sure there is an apps directory on the root of your SD card, create a directory called `cubecraft` inside the apps directory, and place boot.dol and meta.xml inside the cubecraft directory. Launch it with the Homebrew Channel on your Wii.
### GameCube
For consoles that have not been soft-modded, you will need to use a game save exploit to run this. The boot.dol file must be converted to a .gci file with dol2gci and placed on the GameCube memory card along with the hacked save file used to launch the exploit. More details on running GameCube homebrew can be found on the GC-Forever Wiki. <http://www.gc-forever.com/wiki/index.php?title=Booting_Homebrew>

## Controls
The controls are simple. This game uses the GameCube controller exclusively.
* ![Control Stick](http://www.ssbwiki.com/images/c/c4/ButtonIcon-GCN-Control_Stick.png) - move around, navigate menus
* Control Pad - navigate menus
* C Stick - change camera angle
* A Button - jump, select menu
* Start - pause
