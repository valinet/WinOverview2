# WinOverview2

WinOverview2 reimplements the classic Task View from earlier builds of Windows 10 that showed the windows around the center of the screen, without a timeline, and most importantly, lag free.

![Screenshot](/docs/screenshot.png?raw=true "Screenshot")

## Configuration

* Clone project

* Compile - I used Visual Studio 2019, so Build Tools v16, cl v19 and Windows 10 SDK version 10.0.18362.0

* ~~Make sure to use Windows 10 Version 2004, otherwise offsets in twinui.dll may be mismatched. If the application crashes when hooking Explorer, obtain correct offsets from twinui.dll by disassembling with IDA or ghidra and recompile.~~ Since 1.1.0.0, the application determines the right offsets from twinui.dll dynamically, at run time.

* Launch "WinOverview2.exe" as administrator. I recommend setting to launch at log on using Task Scheduler. This way, the application is able to listen to key strokes (it binds itself to Caps Lock key) in all processes. Elevation is required so that we can switch to chosen application when clicking a thumbnail in Task View for elevated applications as well (this could be improved, Explorer uses some mechanism to control elevated applications as well, even though it is not elevated)

* You can close an application when task view is active by hovering over it and clicking the x button that appears in the top right corner.

* When in task view, start typing in order to search the computer (same as pressing Windows + Q and typing there).

* Pressing anywhere on the "desktop", Caps Lock, or Escape will dismiss the overview.


## Known issues

* Disables Cortana (Windows + C)
* Overview is displayed only on the main monitor and includes windows from all monitors. This is a limitation of the underlaying architecture, not much can be done about it.
* ~~twinui.dll offsets are hardcoded at the moment.~~ Fixed in 1.1.0.0
* Initial hooking is a bit cumbersome (need to spawn 2 windows and trigger a snap)

## Inner workings

* Application works by hooking Explorer and calling functions from twinui.dll. Specifically, I observed how Snap Assist behaves and I used that to emulate the old task view, which anyway had the same effects and performance as Snap Assist, so probably it is the same code. I tried to use AllUpView, but it seems it has been removed.
* In order to obtain address of CSnapAssistControllerBase in twinui.dll, the application performs a snap of a dummy window at startup so that the hooked functions are called (they will briefly flash for a second in the taskbar - I was unable to determine the address using any other method, maybe someone could help with a better technique; the address is not stored in a global variable, apparently, and ASLR changes its location, as determined with Cheat Engine, at every execution).

## License

Hooking is done using the excellent [funchook](https://github.com/kubo/funchook) library (GPLv2 with linking exception), which in turn is powered by the [diStorm3](https://github.com/gdabah/distorm/) (3-clause BSD) disassembler. Thus, I am offering this under GNU General Public License Version 2.0, which I believe is compatible.

## Compiling

The following prerequisites are necessary in order to compile this project:

* Microsoft C/C++ Optimizing Compiler - this can be obtained by installing either of these packages:

  * Visual Studio - this is a fully featured IDE; you'll need to check "C/C++ application development role" when installing. If you do not require the full suite, use the package bellow.
  * Build Tools for Visual Studio - this just installs the compiler, which you'll be able to use from the command line, or from other applications like CMake

  Download either of those [here](http://go.microsoft.com/fwlink/p/?LinkId=840931). The guide assumes you have installed either Visual Studio 2019, either Build Tools for Visual Studio 2019.

* [CMake](https://cmake.org/) - for easier usage, make sure to have it added to PATH during installation
* Git - you can use [Git for Windows](https://git-scm.com/download/win), or git command via the Windows Subsystem for Linux.

Steps:

1. Clone git repo along with all submodules

   ```
   git clone --recursive https://github.com/valinet/WinOverview2
   ```

   If "git" is not found as a command, type its full path, or have its folder added to PATH, or open Git command window in the respective folder if using Git for Windows.

2. Compile funchook

   ```
   cd libs
   cd funchook
   md build
   cd build
   cmake -G "Visual Studio 16 2019" -A x64 ..
   cmake --build . --config Release
   ```

   If "cmake" is not found as a command, type its full path, or have its folder added to PATH.

   Type "Win32" instead of "x64" above, if compiling for x86. The command above works for x64.

3. Compile WinOverview2

   * Double click the WinOverview2.sln to open the solution in Visual Studio. Choose Release and your processor architecture in the toolbar. Press F6 to compile.

   * Open an "x86 Native Tools Command Prompt for VS 2019" (for x86), or "x64 Native Tools Command Prompt for VS 2019" (for x64) (search that in Start), go to folder containing solution file and type:

     * For x86:

       ```
       msbuild WinOverview2.sln /property:Configuration=Release /property:Platform=x86
       ```

     * For x64:

       ```
       msbuild WinOverview2.sln /property:Configuration=Release /property:Platform=x64
       ```

   The resulting exe and dll will be in "Release" folder (if you chose x86), or "x64\Release" (if you chose x64) in the folder containing the solution file.

That's it. later, if you want to recompile, make sure to update the repository and the submodules first:

```
git pull
git submodule update --init --recursive
```
