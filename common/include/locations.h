#ifndef LOCATIONS_H
#define LOCATIONS_H

#define LOAD_CRT0_LOCATION 0x06840000 // LCDC_BANK_C

#define SDENGINE_LOCATION      0x037C0000
//#define TEMP_MEM 0x02FFE000 // __DSiHeader

#define NDS_HEADER         0x023FFE00
#define NDS_HEADER_SDK5    0x02FFFE00 // __NDSHeader

#define ARM9_START_ADDRESS_LOCATION      (NDS_HEADER + 0x1F4) //0x023FFFF4
#define ARM9_START_ADDRESS_SDK5_LOCATION (NDS_HEADER_SDK5 + 0x1F4) //0x02FFFFF4

#define RAM_DISK_LOCATION      0x0C400000

#endif // LOCATIONS_H
