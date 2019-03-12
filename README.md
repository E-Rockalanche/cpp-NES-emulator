# C++ NES Emulator (name pending)

A catch-up style cycle acurate NES emulator written in c++

## Mappers working
0 - NROM
1 - MMC1
2 - UxROM

## Tests failing
cpu_dummy_writes_oam
cpu_dummy_writes_ppumem
cpu_interrupts_v2
instr_misc
instr_timing
nmi_sync
oam_read
ppu_sprite_hit
ppu_sprite_overflow
ppu_vbl_nmi
read_joy3 (requires mapper 3)
sprdma_and_dmc_dma
scanline