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
- Each line of this file should contain a single location in which to look for scripts.
- The actual scripts still go into game-specific sub-folders of each listed location as described above.
    - For example, if the file lists `C:\foo\luascript`, then scripts for Kingdom Hearts go in `C:\foo\luascript\kh1`.
- If the configuration file is present, only the location(s) configured are searched for scripts.
  This allows you to completely override the default location if the default location is causing problems.
- If the configuration file is absent, only the default location from above is searched for scripts.
