# Really Quick Player 1.0

Really Quick Player (RQP) is a simple "audio player" plugin for [Far Manager](https://www.farmanager.com/) which helps to pre-listen and sort audio files.

Its main goal is to work with audio files as files and not being another multipurpose player. That's why it doesn't have a playlist, equaliser and any other redundant functionality. But it has the following features which could really help with sorting of audio files:
- (de)select files on the Far's panel during a playback
- demo-mode with a partial tracks playback
- add files to an external playlist or playback them with an external player

# Installation

The installation process is the same as for any other Far plugin: make a subfolder in the Far's `Plugins` folder and put a suitable RQP version into it.

Also you'll need:
- [BASS 2.4](http://www.un4seen.com) library: put bass.dll (bitness should match bitness of Far) near the RQP's dll file and all BASS' subplugins into the `bassplugs` subfolder
- tags.dll to read tags (included with the plugin)
- To play MIDI files, a SF2/SFZ sample bank is required. You can find it on the Internet, then place into the `%windir%\System32` (and `SysWOW64` for Win x64) folder.

On the first launch the plugin will try to import settings from a previous (Far 2) version.

# Known issues

* It's impossible with the current Far Plugins API to properly enumerate files on a virtual file panel (e. g. archives), therefore it was implemented through a hack (glitches are possible).

# Techinfo

There is a number of options which cannot be changed through the UI: colors, UI elements characters etc. You can use the sample `RQP_Def.Farconfig` to change them and then apply with the `/import` Far's command line switch.

Brief options description:

| Option | Description |
| -- | - |
| ButtonsChars | Player buttons' characters |
| Colors | UI colors (see below for details) |
| PanSliderChars | Panning slider's characters |
| PosSliderChars | Time position slider's characters |
| RedrawPanels | A flag which controls if Far's panels redraw during the files navigation and selection. Mostly obsolete |
| TagsTemplate | A tags template. You can see its description in the documentation for [tags.dll](http://www.un4seen.com) |
| VisibleLinesMask | Visible parts (lines) of the UI (see below) |
| VolSliderChars | Volume slider's characters |
| DescriptionFile | A file name for a description file. It could be either absolute or relative path, environment variables are supported. Default is `descript.ion` |
| AutoLoop | Turns on loop mode automatically for OGG files with `ANDROID_LOOP=true` tag (Android ringtones usually) |
| FirstRun | A technical flag, its absence will trigger a search and import of previous (Far 2) version settings |

Additional details about some parameters:

- Colors is a byte array, every byte specify a background (high nibble) and foreground (low nibble) color of some part of the UI. Order of the colors is: base background, button, highlighted button, text, highlighted text, volume bars, volume slider, pannig slider, position slider, time, border, title and 2 values are reserved.
- The volume slider could consists of 1-15 characters (excluding the "empty space" character).
- VisibleLinesMask is a bit-mask for a so-calles mini-mode which hides some lines of the UI. Only low 8 bits of the value are used, the least significant bit controls the first (upper) line (excluding the border). By default this mode is disabled (all bits are set). You can change them if you want to hide unneeded UI lines.
- You can use `RQP_Unicode.farconfig` to set alternative Unicode characters for a better UI experience. You'll need a font which supports all of them ([FiraCode NF](https://github.com/ryanoasis/nerd-fonts/releases) is recommended).

# Build

To build the plugin:
- Put BASS library files inside the `lib\bass` subfolder. You'll need at least the BASS library itself and tags.dll.
- Put `DlgBuilder.hpp`, `farcolor.hpp`, and `plugin.hpp` files (they are part of Far plugin SDK) into the `lib\Farapi3` subfolder.

# Links

[Topic on Far Manager's forum (in Russian)](http://forum.Farmanager.com/viewtopic.php?f=11&t=5000)

[Discussion on Far Manager's forum (mostly in Russian)](https://forum.farmanager.com/viewtopic.php?p=57628)

[PlugRing](https://plugring.farmanager.com/plugin.php?pid=863)

# Legal

This plugin is provided "as is". The author is not responsible for any consequences of use or misuse of this software.

Copyright (c) 2010-2023 [montonero](https://github.com/montoner0/rqp)

BASS Audio Library: Copyright (c) 1999-2023 [Un4seen Developments Ltd.](http://www.un4seen.com)
