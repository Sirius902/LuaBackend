## Installing LuaBackend Hook

### Internal LuaBackend Hook [DLL]
- Download ``DBGHELP.zip`` from the Releases tab.
- Extract ``DBGHELP.dll`` from ``DBGHELP.zip`` to the directory Kingdom Hearts 1.5+2.5 is installed which should be `KH_1.5_2.5`.
- Place your scripts in a folder named `scripts/[gameid]` in the `KINGDOM HEARTS HD 1.5+2.5 ReMIX` folder in your Documents folder.
    - For Kingdom Hearts Final Mix the folder will be `scripts/kh1`.
    - For Kingdom Hearts Re: Chain of Memories the folder will be `scripts/recom`.
    - For Kingdom Hearts II Final Mix the folder will be `scripts/kh2`.
    - For Kingdom Hearts Birth by Sleep Final Mix the folder will be `scripts/bbs`.
- The installation is now finished. LuaBackend will be automatically started with the game and can be easily uninstalled
by simply removing the ``DBGHELP.dll``. To verify it is installed correctly, you can open the LuaBackend console using
the F2 key on the keyboard in game.

### Custom script locations

A configuration file can be used to customize the script location(s).

- Create a file called `LuaScriptLocations.txt` in the same folder as `DBGHELP.dll` (the game install folder).
- This file can contain location(s) in which to look for scripts for each game.
- If _any_ valid locations are listed for a game, _only_ those location(s) are searched for scripts.
  This allows you to completely override the default location if the default location is causing problems.
- If _no_ valid locations are listed for a game, the default location from above is searched for scripts.

Sample file:

```
[kh1]
D:\kh1-scripts
[kh2]
C:\Users\johndoe\Documents\KINGDOM HEARTS HD 1.5+2.5 ReMIX\scripts\kh2
C:\Users\johndoe\bin\KH\PC\openkh\mod\luascript
```
