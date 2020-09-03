# WinOverview2

WinOverview2 reimplements the classic Task View from earlier builds of Windows 10 that showed the windows around the center of the screen, without a timeline, and most importantly, lag free.

![Screenshot](/docs/screenshot.png?raw=true "Screenshot")

## Configuration

* Clone project

* Compile - I used Visual Studio 2019, so Build Tools v16, cl v19 and Windows 10 SDK version 10.0.18362.0

* Make sure to use Windows 10 Version 2004, otherwise offsets in twinui.dll may be mismatched. If the application crashes when hooking Explorer, obtain correct offsets from twinui.dll by disassembling with IDA or ghidra and recompile.

* Launch "WinOverview2.exe" as administrator. I recommend setting to launch at log on using Task Scheduler. This way, the application is able to listen to key strokes (it binds itself to Caps Lock key) in all processes. Elevation is required so that we can switch to chosen application when clicking a thumbnail in Task View.

* You can close an application when task view is active by hovering over it and clicking the x button that appears in the top right corner.

* When in task view, start typing in order to search the computer (same as pressing Windows + Q and typing there).

* Pressing anywhere on the "desktop", Caps Lock, or Escape will dismiss the overview.

* In order to compile, you need to statically link the 2 dependencies (funchook and diStorm3). Thus, you have to do statically build them (it is super simple, with great instructions provided on the project pages). Then, place the files in a folder called *libs*, like in the following tree:

  ```
  repo
  ├── .git
  ├── ...
  └── libs
      ├── distorm
      │   ├── include
      │   │   ├── distorm.h
      │   │   └── mnemonics.h
      │   └── lib
      │       └── distorm.lib
      └── funchook
          ├── include
          │   └── funchook.h
          └── lib
              └── funchook.lib
  ```


## Known issues

* Disables Cortana (Windows + C)
* Overview is displayed only on the main monitor and includes windows from all monitors. This is a limitation of the underlaying architecture, not much can be done about it.
* twinui.dll offsets are hardcoded at the moment.
* Initial hooking is a bit cumbersome

## Inner workings

* Application works by hooking Explorer and calling functions from twinui.dll. Specifically, I observed how Snap Assist behaves and I used that to emulate the old task view, which anyway had the same effects and performance as Snap Assist, so probably it is the same code. I tried to use AllUpView, but it seems it has been removed.
* In order to obtain address of CSnapAssistControllerBase in twinui.dll, the application performs a snap of a dummy window at startup so that the hooked functions are called (they will briefly flash for a second in the taskbar - I was unable to determine the address using any other method, maybe someone could help with a better technique; the address is not stored in a global variable, apparently, and ASLR changes its location, as determined with Cheat Engine, at every execution).

## License

Hooking is done using the excellent [funchook](https://github.com/kubo/funchook) library (GPLv2 with linking exception), which in turn is powered by the [diStorm3](https://github.com/gdabah/distorm/) (3-clause BSD) disassembler. Thus, I am offering this under GNU General Public License Version 2.0, which I believe is compatible.