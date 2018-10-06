
# Physical screen size for the X window system

## Introduction

It's not particularly easy to get the physical screen size in millimeters (or centimeters, inches, however you want to convert it) in the X window system. Using the command line, it's possible to use `xrandr | grep ' connected'`, which might produce output like

    eDP-1-1 connected primary 1920x1080+0+0 (normal left inverted right x axis y axis) 309mm x 173mm
    HDMI-1-1 disconnected (normal left inverted right x axis y axis)
    DP-1-1 disconnected (normal left inverted right x axis y axis)
    HDMI-1-2 disconnected (normal left inverted right x axis y axis)

for my FullHD laptop with 309mm × 173mm resulting in a diagonal measurement of about 14 inches.

But: this only works on the command line. If you'd like to get this information from a native C++ Linux application, you're in trouble. I ran into the same problem, and of course calling shell commands is evil, thus I created this library.

It is based on GitHub user raboof's [xrandr](https://github.com/raboof/xrandr) project. You can find the license for it [here](/LICENSE.xrandr). I extracted the parts responsible for fetching the screen sizes and put them into C++ classes to make using them easier and more intuitive.

## Requirements
- a Linux OS using the X window system
- g++ supporting at least C++11
- installed development files for X11 and Xrandr

## Usage
The only file for you to include is [`XScreenSize.hpp`](/src/XScreenSize.hpp). It defines the `XScreenSize` namespace. Looking at the short piece of demo code in [`main.cpp`](/src/main.cpp), you can see that it's enough to produce output like this:

    screen 0: 1920px x 1080px
        eDP-1-1 [connected] at 60.0079Hz: 1920x1080+0+0, 309mm x 173mm
        HDMI-1-1 [disconnected]
        DP-1-1 [disconnected]
        HDMI-1-2 [disconnected]

- `XScreenSize::XrandrOutput`  contains the members `name`, `connection`, `has_details`, `refresh`, `x`, `y`, `width`, `height`, `mmWidth` and `mmHeight` which should be self-explanatory when looking at the sample output above; only when `has_details` is true, the members following it are well-defined
- note that the internal code for getting all of this information is kept in the constructor of `XScreenSize::Getter`, so right after creating an instance, you can refer to its outputs
- the constructor will throw an `std::runtime_error` in case of an error, so make sure to catch it if needed
- if you'd like to use a different X display or a different screen, pass them to the constructor in that order

To build and run the demo file, run `make`, followed by `bin/x-physical-screen-size`.
