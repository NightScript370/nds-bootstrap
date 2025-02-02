/*
    NitroHax -- Cheat tool for the Nintendo DS
    Copyright (C) 2008  Michael "Chishm" Chisholm

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <nds/ndstypes.h>
#include <nds/system.h>
#include <nds/dma.h>
#include <nds/arm9/video.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/timers.h>
#include <nds/memory.h> // tNDSHeader
#include "tonccpy.h"
#include "nds_header.h"
#include "cardengine.h"
#include "locations.h"
#include "cardengine_header_arm9.h"

#define extendedMemory BIT(1)
#define eSdk2 BIT(2)
#define dsiMode BIT(3)
#define enableExceptionHandler BIT(4)
#define isSdk5 BIT(5)
#define overlaysInRam BIT(6)
#define slowSoftReset BIT(10)
#define softResetMb BIT(13)
#define cloneboot BIT(14)

extern cardengineArm9* volatile ce9;

extern vu32* volatile sharedAddr;

extern tNDSHeader* ndsHeader;

extern bool flagsSet;
extern bool igmReset;

void SetBrightness(u8 screen, s8 bright) {
	u16 mode = 1 << 14;

	if (bright < 0) {
		mode = 2 << 14;
		bright = -bright;
	}
	if (bright > 31) {
		bright = 31;
	}
	*(u16*)(0x0400006C + (0x1000 * screen)) = bright + mode;
}

// Alternative to swiWaitForVBlank()
void waitFrames(int count) {
	for (int i = 0; i < count; i++) {
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}
}

/*void sleepMs(int ms) {
	if (REG_IME == 0 || REG_IF == 0) {
		return;
	}

	if(ce9->patches->sleepRef) {
		volatile void (*sleepRef)(int ms) = (volatile void*)ce9->patches->sleepRef;
		(*sleepRef)(ms);
	} else if(ce9->thumbPatches->sleepRef) {
		extern void callSleepThumb(int ms);
		callSleepThumb(ms);
	}
}

static void waitForArm7(void) {
	IPC_SendSync(0x4);
	while (sharedAddr[3] != (vu32)0);
}*/

bool IPC_SYNC_hooked = false;
void hookIPC_SYNC(void) {
    if (!IPC_SYNC_hooked) {
		if (!(ce9->valueBits & isSdk5)) {
			u32* vblankHandler = ce9->irqTable;
			ce9->intr_vblank_orig_return = *vblankHandler;
			*vblankHandler = ce9->patches->vblankHandlerRef;
		}
        u32* ipcSyncHandler = ce9->irqTable + 16;
		ce9->intr_ipc_orig_return = *ipcSyncHandler;
        *ipcSyncHandler = ce9->patches->ipcSyncHandlerRef;
        IPC_SYNC_hooked = true;
    }
}

void enableIPC_SYNC(void) {
	if (IPC_SYNC_hooked && !(REG_IE & IRQ_IPC_SYNC)) {
		REG_IE |= IRQ_IPC_SYNC;
	}
}


//static void clearIcache (void) {
      // Seems to have no effect
      // disable interrupt
      /*int oldIME = enterCriticalSection();
      IC_InvalidateAll();
      // restore interrupt
      leaveCriticalSection(oldIME);*/
//}

void initMBKARM9_dsiMode(void) {
	*(vu32*)REG_MBK1 = *(u32*)0x02FFE180;
	*(vu32*)REG_MBK2 = *(u32*)0x02FFE184;
	*(vu32*)REG_MBK3 = *(u32*)0x02FFE188;
	*(vu32*)REG_MBK4 = *(u32*)0x02FFE18C;
	*(vu32*)REG_MBK5 = *(u32*)0x02FFE190;
	REG_MBK6 = *(u32*)0x02FFE194;
	REG_MBK7 = *(u32*)0x02FFE198;
	REG_MBK8 = *(u32*)0x02FFE19C;
	REG_MBK9 = *(u32*)0x02FFE1AC;
}

void __attribute__((target("arm"))) resetMpu(void) {
	asm("LDR R0,=#0x12078\n\tmcr p15, 0, r0, C1,C0,0");
}

void reset(u32 param, u32 tid2) {
	u32 resetParam = ((ce9->valueBits & isSdk5) ? RESET_PARAM_SDK5 : RESET_PARAM);
	if (ndsHeader->unitCode == 0 || (*(u32*)0x02FFE234 != 0x00030004 && *(u32*)0x02FFE234 != 0x00030005)) {
		*(u32*)resetParam = param;
	}
	bool returnToLoader = (param == 0xFFFFFFFF && ndsHeader->unitCode > 0 && (ce9->valueBits & dsiMode));
	if (returnToLoader || *(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) { // If DSiWare...
		if (returnToLoader || (param != *(u32*)0x02FFE230 && tid2 != *(u32*)0x02FFE234)) {
			/*if (ce9->consoleModel < 2) {
				// Make screens white
				SetBrightness(0, 31);
				SetBrightness(1, 31);
				waitFrames(5);	// Wait for DSi screens to stabilize
			}
			enterCriticalSection();
			cacheFlush();*/
			sysSetCardOwner(false);	// Give Slot-1 access to arm7
			sharedAddr[3] = 0x54495845;
			//while (1);
		} else {
			sysSetCardOwner(false);	// Give Slot-1 access to arm7
			sharedAddr[3] = 0x52534554;
		}
	} else if (ce9->valueBits & slowSoftReset) {
		if (ce9->consoleModel < 2) {
			// Make screens white
			SetBrightness(0, 31);
			SetBrightness(1, 31);
			waitFrames(5);	// Wait for DSi screens to stabilize
		}
		enterCriticalSection();
		if (!igmReset && (ce9->valueBits & softResetMb)) {
			*(u32*)resetParam = 0;
			*(u32*)(resetParam+8) = 0x44414F4C; // 'LOAD'
		}
		cacheFlush();
		sharedAddr[3] = 0x52534554;
		while (1);
	} else {
		if (ce9->valueBits & dsiMode) {
			sysSetCardOwner(false);	// Give Slot-1 access to arm7
		}
		if (*(u32*)(resetParam+0xC) > 0) {
			sharedAddr[1] = ce9->valueBits;
		}
		if (!igmReset && (ce9->valueBits & softResetMb)) {
			*(u32*)resetParam = 0;
			*(u32*)(resetParam+8) = 0x44414F4C; // 'LOAD'
		}
		sharedAddr[3] = 0x52534554;
	}

 	register int i, reg;

	REG_IME = 0;
	REG_IE = 0;
	REG_IF = ~0;

	cacheFlush();
	resetMpu();

	if (igmReset) {
		igmReset = false;

		if (ce9->intr_vblank_orig_return && (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005)) {
			*(u32*)0x02FFC230 = *(u32*)0x02FFE230;
			*(u32*)0x02FFC234 = *(u32*)0x02FFE234;
		}
	} else {
		toncset((u8*)getDtcmBase()+0x3E00, 0, 0x200);

		if (ce9->intr_vblank_orig_return && (*(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005)) {
			*(u32*)0x02FFC230 = 0;
			*(u32*)0x02FFC234 = 0;
		}
	}

	// Clear out ARM9 DMA channels
	for (i = 0; i < 4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	for (i = 0; i < 4; i++) {
		for(reg=0; reg<0x1c; reg+=4)*((vu32*)(0x04004104 + ((i*0x1c)+reg))) = 0;//Reset NDMA.
	}

	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	flagsSet = false;
	IPC_SYNC_hooked = false;

	if (*(u32*)(resetParam+0xC) > 0) {
		u32 newIrqTable = sharedAddr[2];
		ce9->valueBits = sharedAddr[1];
		ce9->irqTable = (u32*)newIrqTable;
		ce9->cardStruct0 = sharedAddr[4];
		sharedAddr[4] = 0;
	}

	if (returnToLoader || *(u32*)0x02FFE234 == 0x00030004 || *(u32*)0x02FFE234 == 0x00030005) { // If DSiWare...
		REG_DISPSTAT = 0;
		REG_DISPCNT = 0;
		REG_DISPCNT_SUB = 0;

		toncset((u16*)0x04000000, 0, 0x56);
		toncset((u16*)0x04001000, 0, 0x56);

		VRAM_A_CR = 0x80;
		VRAM_B_CR = 0x80;
		VRAM_C_CR = 0x80;
		VRAM_D_CR = 0x80;
		VRAM_E_CR = 0x80;
		VRAM_F_CR = 0x80;
		VRAM_G_CR = 0x80;
		VRAM_H_CR = 0x80;
		VRAM_I_CR = 0x80;

		toncset16(BG_PALETTE, 0, 256); // Clear palettes
		toncset16(BG_PALETTE_SUB, 0, 256);
		toncset(VRAM, 0, 0xC0000); // Clear VRAM

		VRAM_A_CR = 0;
		VRAM_B_CR = 0;
		VRAM_C_CR = 0;
		VRAM_D_CR = 0;
		VRAM_E_CR = 0;
		VRAM_F_CR = 0;
		VRAM_G_CR = 0;
		VRAM_H_CR = 0;
		VRAM_I_CR = 0;
	}

	while (sharedAddr[0] != 0x44414F4C) { // 'LOAD'
		while (REG_VCOUNT != 191);
		while (REG_VCOUNT == 191);
	}

	if (returnToLoader && ndsHeader->unitCode > 0 && sharedAddr[3] == 0x54495845) {
		initMBKARM9_dsiMode();
		REG_SCFG_EXT = 0x8307F100;
		REG_SCFG_CLK = 0x87;
		REG_SCFG_RST = 1;
	}

	if (ndsHeader->unitCode > 0 && (ce9->valueBits & dsiMode)) {
		sysSetCardOwner(true);	// Give Slot-1 access back to arm9
	}

	sharedAddr[0] = 0x544F4F42; // 'BOOT'
	sharedAddr[3] = 0;
	while (REG_VCOUNT != 191);
	while (REG_VCOUNT == 191);

	// Start ARM9
	ndsCodeStart(ndsHeader->arm9executeAddress);
}
