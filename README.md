# Talking Bass

Talking Bass is a bass instrument for music-making software. It includes three
sounds: **Talking Bass**, **Talking Bass Growl**, and **Talking Bass Whisper**.

## Download

Choose the download for your computer from the current **v1.0.0-rc1 release candidate**:

- [Windows 64-bit](https://github.com/realrandolph/TalkingBass/releases/download/v1.0.0-rc1/talkingbass-windows-x86_64.zip)
- [macOS (Intel and Apple Silicon)](https://github.com/realrandolph/TalkingBass/releases/download/v1.0.0-rc1/talkingbass-macos-universal.zip)
- [Linux 64-bit Intel/AMD](https://github.com/realrandolph/TalkingBass/releases/download/v1.0.0-rc1/talkingbass-linux-x86_64.zip)

For a newer version, visit [Releases](https://github.com/realrandolph/TalkingBass/releases) and choose the matching download.

## Install on Windows

1. Download [**talkingbass-windows-x86_64.zip**](https://github.com/realrandolph/TalkingBass/releases/download/v1.0.0-rc1/talkingbass-windows-x86_64.zip). Your browser will usually put it in **Downloads**.
2. Open File Explorer (press **Windows+E**) and click **Downloads** in the left-hand list.
3. Find **talkingbass-windows-x86_64.zip**. Right-click it, choose **Extract All…**, then click **Extract** in the window that appears. Windows will open the extracted files.
4. In the extracted files, click the folder named **talkingbass.lv2** once, then press **Ctrl+C**. This copies the whole plugin folder.
5. Open another File Explorer window (press **Windows+E** again). Click the long address bar across the top of the window—not the search box—type `%APPDATA%`, and press **Enter**. You should now be in a folder named **Roaming**.
6. Look for a folder named **LV2**. If it is there, double-click it. If it is not there, click **New** near the top of the window, choose **Folder**, type `LV2`, press **Enter**, then double-click the new **LV2** folder. (On some Windows versions the button is labelled **New folder**.)
7. Click an empty area inside the **LV2** folder and press **Ctrl+V**. The **talkingbass.lv2** folder will be copied here.
8. Restart your music app, or use its plugin-rescan option.

To check that it is in the right place, the address bar should end with `Roaming\LV2\talkingbass.lv2`, and you should see these three items inside:

```text
manifest.ttl
talkingbass.dll
talkingbass.ttl
```

The `.dll` is part of the plugin folder; do not open it or move it on its own.

The complete location is:

```text
%APPDATA%\LV2\talkingbass.lv2\
  manifest.ttl
  talkingbass.dll
  talkingbass.ttl
```

## Install on macOS

1. Download **talkingbass-macos-universal.zip** and double-click it to extract **talkingbass.lv2**.
2. In Finder, choose **Go → Go to Folder…** (or press **Shift+Command+G**) and enter `~/Library/Audio/Plug-Ins/LV2`.
3. Move the complete **talkingbass.lv2** folder into the folder that opens. If Finder says the location is missing, use **Go to Folder…** to open `~/Library`, then create `Audio`, `Plug-Ins`, and `LV2` folders in that order. Open `LV2` and move the bundle there.
4. Restart your music app, or use its plugin-rescan option.

## Install on Linux

1. Download **talkingbass-linux-x86_64.zip** and extract it. You should see a folder named **talkingbass.lv2**.
2. Open your Home folder and show hidden files (usually **Ctrl+H**). Create a folder named `.lv2` there if it is not already present.
3. Move the complete **talkingbass.lv2** folder into `.lv2`.
4. Restart your music app, or use its plugin-rescan option.

## If it doesn't show up

Talking Bass is an **LV2 instrument**, not a standalone program or a VST/AU plugin. Your music app must support LV2 instruments on your operating system. If it still does not appear after restarting or rescanning, check that support first. In particular, check the exact Windows build of LMMS before installing: not every LMMS build can load LV2 instruments.

Once loaded, look for **Talking Bass**, **Talking Bass Growl**, or **Talking Bass Whisper** in your instrument list. Keep the whole `talkingbass.lv2` folder together; do not open or move the `.dll`/`.dylib`/`.so` file by itself.

<details>
<summary>For developers: building from source</summary>

You need a C99 compiler, GNU Make, pkg-config/pkgconf, and the LV2 development headers. Run `make bundle`; the finished bundle is written to `build/talkingbass.lv2/`. The Windows CI build uses MSYS2 UCRT64; the macOS CI build is universal for Intel and Apple Silicon.

</details>

## License

MIT. See [LICENSE](LICENSE).
