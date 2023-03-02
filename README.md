# Pong
![image](https://user-images.githubusercontent.com/122756045/222469135-4056483a-d0dd-4827-9b31-f73bc76b2a3b.png)
A Pong clone written from scratch in C. By scratch I mean: it only uses the Windows APIs and two third party libraries. The C standard library is neither used nor linked in the executable. Functions from it, like `printf`, `memset`, `memcpy`, etc. I've implemented myself. This makes the executable much smaller.

The two third party libraries used are:
1. Ryu (https://github.com/ulfjack/ryu): Used in `strings.c` to convert floating point numbers into strings, when replacing `printf`;
2. PCG Basic (https://github.com/imneme/pcg-c-basic): Good random number generator.

It should only work in Windows 10 or later. Even though I wrote it in a manner that makes it easy to port to other platforms, currently I have no plans in doing so. Once I've wrote some Linux platform layer in the future, then I can make it work here.

I tried making it as close to the original as possible, this is why the sound effects are just simple square waves! Here's a video of the original Atari Pong: https://youtu.be/fiShX2pTz9A

## How to build it
You'll need the following softwares installed in your machine if you haven't already:
1. LLVM (for the Clang compiler). Link: https://github.com/llvm/llvm-project/releases/ (look for the `win64` file);
2. Either the whole Visual Studio IDE (https://visualstudio.microsoft.com/) with "Desktop development with C++" workload installed, or just the Windows SDK (https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/);
3. Python 3 (https://www.python.org/downloads/).

Once you've installed them, open a terminal window at the `code` directory. You have 3 build options:
1. `$ python build.py --slow` or just `$ python build.py`;
2. `$ python build.py --fast`;
3. `$ python build.py --release`.

The slow build has no optimizations at all and comes with debug information so that you can step into the functions in a debugger. The fast build also comes with debug information but uses `-O3` optimization flag, which is the highest, and so there may be harder to step through the code. The release build has maximum optimizations, no debug information, and starts in a fullscreen window.

After you ran any of the commands, a `build` directory containing the executable will be created at the root of the project, alongside `code`.

## How to play
- Press `ENTER` to start the match/round;
- `W` and `S` controls the left paddle;
- `Up` and `Down` arrows controls the right paddle;
- `F11` or `Alt+ENTER` toggles fullscreen;
- `Alt+F4` or `ESC` quits the program.

## Download
You can download a precompiled release build in the Releases section of this repository. Here's the link: https://github.com/serafaleo/Pong/releases

If when you try to run the executable you see a warning message asking whether you really want to open it or not, just ignore it and open. It's just Microsoft wanting me to pay them to make the executable trustworthy.
