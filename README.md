# C++ NES Emulator (name pending)

A cycle acurate NES emulator written in c++

![Super Mario Bros.](https://i.imgur.com/gA96Uwy.png)
![Legend of Zelda](https://i.imgur.com/81l9FRR.png)
![Final Fantasy](https://i.imgur.com/D40ymWX.png)
![Kirby's Adventure](https://i.imgur.com/SwNf1VF.png)
![Duck Hunt](https://i.imgur.com/nuQ9tGI.png)
![Castlevania](https://i.imgur.com/xjy94Mc.png)

## Controls
only configurable through config.json for now

### Port 1
* A: X
* B: Z
* Start: enter
* Select: right shift
* Directional buttons: arrow keys

### Port 2
* Zapper trigger: left mouse button

## Hotkeys
* Quit: escape
* Screenshot: F9
* Toggle fullscreen: F11
* Toggle mute: M
* Toggle paused: P
* Next frame: S
* Start/stop recording button input: R
* Start/stop button playback: A
* Save state: F5
* Load state: F6

## Mappers working
0. NROM
1. MMC1
2. UxROM
3. CNROM
4. MMC3

## Credits and Thanks
* Blargg's NES APU library (Scary stuff if you don't know digital audio): http://blargg.8bitalley.com/libs/audio.html#Nes_Snd_Emu
* JSON for Modern C++ (beautiful): https://github.com/nlohmann/json
* SDL_FontCache (efficient rendering of text in SDL): https://github.com/grimfang4/SDL_FontCache
* Nesdev (great documentation): https://wiki.nesdev.com/w/index.php/Nesdev
* LaiNES (helped me when I got stuck on the PPU): https://github.com/AndreaOrru/LaiNES