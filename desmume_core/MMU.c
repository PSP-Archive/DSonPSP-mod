/*  Copyright (C) 2006 yopyop
    yopyop156@ifrance.com
    yopyop156.ifrance.com

	Copyright (C) 2007 shash

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public Licensend
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/



//#define RENDER3D

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "GPU.h"

#include "debug.h"
#include "NDSSystem.h"
#include "cflash.h"
#include "cp15.h"
#include "wifi.h"
#include "registers.h"
#include "render3D.h"
#include "gfx3d.h"
#ifdef __psp__
#include "ROMReader.h"
#endif

#define ROM_MASK 3

#define LOG(...)

/*
 *
 */

static const int save_types[7][2] = {
        {MC_TYPE_AUTODETECT,1},
        {MC_TYPE_EEPROM1,MC_SIZE_4KBITS},
        {MC_TYPE_EEPROM2,MC_SIZE_64KBITS},
        {MC_TYPE_EEPROM2,MC_SIZE_512KBITS},
        {MC_TYPE_FRAM,MC_SIZE_256KBITS},
        {MC_TYPE_FLASH,MC_SIZE_2MBITS},
		{MC_TYPE_FLASH,MC_SIZE_4MBITS}
};

#define EARLY_MEMORY_ACCESS 1

#define INTERNAL_DTCM_READ 1
#define INTERNAL_DTCM_WRITE 1
//ITCM implementation
#define INTERNAL_ITCM_READ 1
#define INTERNAL_ITCM_WRITE 1

char szRomPath[512];
char szRomBaseName[512];

#define DUP2(x)  x, x
#define DUP4(x)  x, x, x, x
#define DUP8(x)  x, x, x, x,  x, x, x, x
#define DUP16(x) x, x, x, x,  x, x, x, x,  x, x, x, x,  x, x, x, x

MMU_struct MMU;

u8 * MMU_ARM9_MEM_MAP[256]={
/* 0X*/	DUP16(ARM9Mem.ARM9_ITCM), 
/* 1X*/	DUP16(ARM9Mem.ARM9_ITCM), 
/* 2X*/	DUP16(ARM9Mem.MAIN_MEM),
/* 3X*/	DUP16(MMU.SWIRAM),
/* 4X*/	DUP16(ARM9Mem.ARM9_REG),
/* 5X*/	DUP16(ARM9Mem.ARM9_VMEM),
/* 6X*/	DUP2(ARM9Mem.ARM9_ABG), 
		DUP2(ARM9Mem.ARM9_BBG),
		DUP2(ARM9Mem.ARM9_AOBJ),
		DUP2(ARM9Mem.ARM9_BOBJ),
		DUP8(ARM9Mem.ARM9_LCD),
/* 7X*/	DUP16(ARM9Mem.ARM9_OAM),
/* 8X*/	DUP16(NULL),
/* 9X*/	DUP16(NULL),
/* AX*/	DUP16(MMU.CART_RAM),
/* BX*/	DUP16(MMU.UNUSED_RAM),
/* CX*/	DUP16(MMU.UNUSED_RAM),
/* DX*/	DUP16(MMU.UNUSED_RAM),
/* EX*/	DUP16(MMU.UNUSED_RAM),
/* FX*/	DUP16(ARM9Mem.ARM9_BIOS)
};
	   
u32 MMU_ARM9_MEM_MASK[256]={
/* 0X*/	DUP16(0x00007FFF), 
/* 1X*/	DUP16(0x00007FFF),
/* 2X*/	DUP16(0x003FFFFF),
/* 3X*/	DUP16(0x00007FFF),
/* 4X*/	DUP16(0x0003FFFF),
/* 5X*/	DUP16(0x000007FF),
/* 6X*/	DUP2(0x0007FFFF), 
		DUP2(0x0001FFFF),
		DUP2(0x0003FFFF),
		DUP2(0x0001FFFF),
		DUP8(0x000FFFFF),
/* 7X*/	DUP16(0x000007FF),
/* 8X*/	DUP16(ROM_MASK),
/* 9X*/	DUP16(ROM_MASK),
/* AX*/	DUP16(0x0000FFFF),
/* BX*/	DUP16(0x00000003),
/* CX*/	DUP16(0x00000003),
/* DX*/	DUP16(0x00000003),
/* EX*/	DUP16(0x00000003),
/* FX*/	DUP16(0x00007FFF)
};

u8 * MMU_ARM7_MEM_MAP[256]={
/* 0X*/	DUP16(MMU.ARM7_BIOS), 
/* 1X*/	DUP16(MMU.UNUSED_RAM), 
/* 2X*/	DUP16(ARM9Mem.MAIN_MEM),
/* 3X*/	DUP8(MMU.SWIRAM),
		DUP8(MMU.ARM7_ERAM),
/* 4X*/	DUP8(MMU.ARM7_REG),
		DUP8(MMU.ARM7_WIRAM),
/* 5X*/	DUP16(MMU.UNUSED_RAM),
/* 6X*/	DUP16(ARM9Mem.ARM9_ABG), 
/* 7X*/	DUP16(MMU.UNUSED_RAM),
/* 8X*/	DUP16(NULL),
/* 9X*/	DUP16(NULL),
/* AX*/	DUP16(MMU.CART_RAM),
/* BX*/	DUP16(MMU.UNUSED_RAM),
/* CX*/	DUP16(MMU.UNUSED_RAM),
/* DX*/	DUP16(MMU.UNUSED_RAM),
/* EX*/	DUP16(MMU.UNUSED_RAM),
/* FX*/	DUP16(MMU.UNUSED_RAM)
};

u32 MMU_ARM7_MEM_MASK[256]={
/* 0X*/	DUP16(0x00003FFF), 
/* 1X*/	DUP16(0x00000003),
/* 2X*/	DUP16(0x003FFFFF),
/* 3X*/	DUP8(0x00007FFF),
		DUP8(0x0000FFFF),
/* 4X*/	DUP8(0x00FFFFFF),
		DUP8(0x0000FFFF),
/* 5X*/	DUP16(0x00000003),
/* 6X*/	DUP16(0x0003FFFF), 
/* 7X*/	DUP16(0x00000003),
/* 8X*/	DUP16(ROM_MASK),
/* 9X*/	DUP16(ROM_MASK),
/* AX*/	DUP16(0x0000FFFF),
/* BX*/	DUP16(0x00000003),
/* CX*/	DUP16(0x00000003),
/* DX*/	DUP16(0x00000003),
/* EX*/	DUP16(0x00000003),
/* FX*/	DUP16(0x00000003)
};

u32 MMU_ARM9_WAIT16[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 1, 1, 1, 1, 1,
};

u32 MMU_ARM9_WAIT32[16]={
	1, 1, 1, 1, 1, 2, 2, 1, 8, 8, 5, 1, 1, 1, 1, 1,
};

u32 MMU_ARM7_WAIT16[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 1, 1, 1, 1, 1,
};

u32 MMU_ARM7_WAIT32[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 8, 8, 5, 1, 1, 1, 1, 1,
};

void MMU_Init(void) {
	int i;

	LOG("MMU init\n");

	memset(&MMU, 0, sizeof(MMU_struct));

	MMU.CART_ROM = MMU.UNUSED_RAM;

        for(i = 0x80; i<0xA0; ++i)
        {
           MMU_ARM9_MEM_MAP[i] = MMU.CART_ROM;
           MMU_ARM7_MEM_MAP[i] = MMU.CART_ROM;
        }

	MMU.MMU_MEM[0] = MMU_ARM9_MEM_MAP;
	MMU.MMU_MEM[1] = MMU_ARM7_MEM_MAP;
	MMU.MMU_MASK[0]= MMU_ARM9_MEM_MASK;
	MMU.MMU_MASK[1] = MMU_ARM7_MEM_MASK;


//	MMU.ITCMRegion = 0x00800000;

	MMU.DTCMRegion = 0x027C0000;
	MMU.ITCMRegion = 0x00000000;


	MMU.MMU_WAIT16[0] = MMU_ARM9_WAIT16;
	MMU.MMU_WAIT16[1] = MMU_ARM7_WAIT16;
	MMU.MMU_WAIT32[0] = MMU_ARM9_WAIT32;
	MMU.MMU_WAIT32[1] = MMU_ARM7_WAIT32;

	
	IPC_FIFOclear();


	mc_init(&MMU.fw, MC_TYPE_FLASH);  /* init fw device */
	mc_alloc(&MMU.fw, NDS_FW_SIZE_V1);
	MMU.fw.fp = NULL;

	// Init Backup Memory device, this should really be done when the rom is loaded
	mc_init(&MMU.bupmem, MC_TYPE_AUTODETECT);
	mc_alloc(&MMU.bupmem, 1);
	MMU.bupmem.fp = NULL;
} 

void MMU_DeInit(void) {
	LOG("MMU deinit\n");
    if (MMU.fw.fp)
       fclose(MMU.fw.fp);
    mc_free(&MMU.fw);      
    if (MMU.bupmem.fp)
       fclose(MMU.bupmem.fp);
    mc_free(&MMU.bupmem);
}

//Card rom & ram

u16 SPI_CNT = 0;
u16 SPI_CMD = 0;
u16 AUX_SPI_CNT = 0;
u16 AUX_SPI_CMD = 0;

u32 rom_mask = 0;

u32 DMASrc[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
u32 DMADst[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};

void MMU_clearMem()
{
	int i;
	
	memset(ARM9Mem.ARM9_ABG,  0, 0x080000);
	memset(ARM9Mem.ARM9_AOBJ, 0, 0x040000);
	memset(ARM9Mem.ARM9_BBG,  0, 0x020000);
	memset(ARM9Mem.ARM9_BOBJ, 0, 0x020000);
	memset(ARM9Mem.ARM9_DTCM, 0, 0x4000);
	memset(ARM9Mem.ARM9_ITCM, 0, 0x8000);
	memset(ARM9Mem.ARM9_LCD,  0, 0x0A4000);
	memset(ARM9Mem.ARM9_OAM,  0, 0x0800);
	memset(ARM9Mem.ARM9_REG,  0, sizeof(ARM9Mem.ARM9_REG));
	memset(ARM9Mem.ARM9_VMEM, 0, 0x0800);
	memset(ARM9Mem.MAIN_MEM,  0, 0x400000);

	memset(ARM9Mem.blank_memory,  0, 0x020000);
	
	memset(MMU.ARM7_ERAM,     0, 0x010000);
	memset(MMU.ARM7_REG,      0, 0x010000);
	
	
    IPC_FIFOclear();


//	MMU.DTCMRegion = 0;
//	MMU.ITCMRegion = 0x00800000;

	MMU.DTCMRegion = 0x027C0000;
	MMU.ITCMRegion = 0x00000000;

	
	memset(MMU.timer,         0, sizeof(u16) * 2 * 4);
	memset(MMU.timerMODE,     0, sizeof(s32) * 2 * 4);
	memset(MMU.timerON,       0, sizeof(u32) * 2 * 4);
	memset(MMU.timerRUN,      0, sizeof(u32) * 2 * 4);
	memset(MMU.timerReload,   0, sizeof(u16) * 2 * 4);
	
	memset(MMU.reg_IME,       0, sizeof(u32) * 2);
	memset(MMU.reg_IE,        0, sizeof(u32) * 2);
	memset(MMU.reg_IF,        0, sizeof(u32) * 2);
	
	memset(MMU.DMAStartTime,  0, sizeof(u32) * 2 * 4);
	memset(MMU.DMACycle,      0, sizeof(s32) * 2 * 4);
	memset(MMU.DMACrt,        0, sizeof(u32) * 2 * 4);
	memset(MMU.DMAing,        0, sizeof(BOOL) * 2 * 4);
	
	memset(MMU.dscard,        0, sizeof(nds_dscard) * 2);
	
	MainScreen.offset = 192;
	SubScreen.offset  = 0;

        /* setup the texture slot pointers */
#if 0
        ARM9Mem.textureSlotAddr[0] = ARM9Mem.blank_memory;
        ARM9Mem.textureSlotAddr[1] = ARM9Mem.blank_memory;
        ARM9Mem.textureSlotAddr[2] = ARM9Mem.blank_memory;
        ARM9Mem.textureSlotAddr[3] = ARM9Mem.blank_memory;
#else
        ARM9Mem.textureSlotAddr[0] = &ARM9Mem.ARM9_LCD[0x20000 * 0];
        ARM9Mem.textureSlotAddr[1] = &ARM9Mem.ARM9_LCD[0x20000 * 1];
        ARM9Mem.textureSlotAddr[2] = &ARM9Mem.ARM9_LCD[0x20000 * 2];
        ARM9Mem.textureSlotAddr[3] = &ARM9Mem.ARM9_LCD[0x20000 * 3];
#endif
}

/* the VRAM blocks keep their content even when not blended in */
/* to ensure that we write the content back to the LCD ram */
/* FIXME: VRAM Bank E,F,G,H,I missing */
void MMU_VRAMWriteBackToLCD(u8 block)
{
	u8 *destination;
	u8 *source;
	u32 size ;
	u8 VRAMBankCnt;
	#if 1
		return ;
	#endif
	destination = 0 ;
	source = 0;
	VRAMBankCnt = MMU_read8(ARMCPU_ARM9,REG_VRAMCNTA+block) ;
	switch (block)
	{
		case 0: // Bank A
			destination = ARM9Mem.ARM9_LCD ;
			size = 0x20000 ;
			break ;
		case 1: // Bank B
			destination = ARM9Mem.ARM9_LCD + 0x20000 ;
			size = 0x20000 ;
			break ;
		case 2: // Bank C
			destination = ARM9Mem.ARM9_LCD + 0x40000 ;
			size = 0x20000 ;
			break ;
		case 3: // Bank D
			destination = ARM9Mem.ARM9_LCD + 0x60000 ;
			size = 0x20000 ;
			break ;
		case 4: // Bank E
			destination = ARM9Mem.ARM9_LCD + 0x80000 ;
			size = 0x10000 ;
			break ;
		case 5: // Bank F
			destination = ARM9Mem.ARM9_LCD + 0x90000 ;
			size = 0x4000 ;
			break ;
		case 6: // Bank G
			destination = ARM9Mem.ARM9_LCD + 0x94000 ;
			size = 0x4000 ;
			break ;
		case 8: // Bank H
			destination = ARM9Mem.ARM9_LCD + 0x98000 ;
			size = 0x8000 ;
			break ;
		case 9: // Bank I
			destination = ARM9Mem.ARM9_LCD + 0xA0000 ;
			size = 0x4000 ;
			break ;
		default:
			return ;
	}
	switch (VRAMBankCnt & 7) {
		case 0:
			/* vram is allready stored at LCD, we dont need to write it back */
			MMU.vScreen = 1;
			break ;
		case 1:
	switch(block){
	case 0:
	case 1:
	case 2:
	case 3:
		/* banks are in use for BG at ABG + ofs * 0x20000 */
				source = ARM9Mem.ARM9_ABG + ((VRAMBankCnt >> 3) & 3) * 0x20000 ;
		break ;
	case 4:
		/* bank E is in use at ABG */ 
		source = ARM9Mem.ARM9_ABG ;
		break;
	case 5:
	case 6:
		/* banks are in use for BG at ABG + (0x4000*OFS.0)+(0x10000*OFS.1)*/
		source = ARM9Mem.ARM9_ABG + (((VRAMBankCnt >> 3) & 1) * 0x4000) + (((VRAMBankCnt >> 2) & 1) * 0x10000) ;
		break;
	case 8:
		/* bank H is in use at BBG */ 
		source = ARM9Mem.ARM9_BBG ;
		break ;
	case 9:
		/* bank I is in use at BBG */ 
		source = ARM9Mem.ARM9_BBG + 0x8000 ;
		break;
	default: return ;
	}
			break ;
		case 2:
			if (block < 2)
			{
				/* banks A,B are in use for OBJ at AOBJ + ofs * 0x20000 */
				source = ARM9Mem.ARM9_AOBJ + ((VRAMBankCnt >> 3) & 1) * 0x20000 ;
			} else return ;
			break ;
		case 4:
	switch(block){
	case 2:
		/* bank C is in use at BBG */ 
		source = ARM9Mem.ARM9_BBG ;
		break ;
	case 3:
		/* bank D is in use at BOBJ */ 
		source = ARM9Mem.ARM9_BOBJ ;
		break ;
	default: return ;
	}
			break ;
		default:
			return ;
	}
	if (!destination) return ;
	if (!source) return ;
	memcpy(destination,source,size) ;
}

void MMU_VRAMReloadFromLCD(u8 block,u8 VRAMBankCnt)
{
	u8 *destination;
	u8 *source;
	u32 size;
	#if 1
		return ;
	#endif
	destination = 0;
	source = 0;
	size = 0;
	switch (block)
	{
		case 0: // Bank A
			source = ARM9Mem.ARM9_LCD ;
			size = 0x20000 ;
			break ;
		case 1: // Bank B
			source = ARM9Mem.ARM9_LCD + 0x20000 ;
			size = 0x20000 ;
			break ;
		case 2: // Bank C
			source = ARM9Mem.ARM9_LCD + 0x40000 ;
			size = 0x20000 ;
			break ;
		case 3: // Bank D
			source = ARM9Mem.ARM9_LCD + 0x60000 ;
			size = 0x20000 ;
			break ;
		case 4: // Bank E
			source = ARM9Mem.ARM9_LCD + 0x80000 ;
			size = 0x10000 ;
			break ;
		case 5: // Bank F
			source = ARM9Mem.ARM9_LCD + 0x90000 ;
			size = 0x4000 ;
			break ;
		case 6: // Bank G
			source = ARM9Mem.ARM9_LCD + 0x94000 ;
			size = 0x4000 ;
			break ;
		case 8: // Bank H
			source = ARM9Mem.ARM9_LCD + 0x98000 ;
			size = 0x8000 ;
			break ;
		case 9: // Bank I
			source = ARM9Mem.ARM9_LCD + 0xA0000 ;
			size = 0x4000 ;
			break ;
		default:
			return ;
	}
	switch (VRAMBankCnt & 7) {
		case 0:
			/* vram is allready stored at LCD, we dont need to write it back */
			MMU.vScreen = 1;
			break ;
		case 1:
			if (block < 4)
			{
				/* banks are in use for BG at ABG + ofs * 0x20000 */
				destination = ARM9Mem.ARM9_ABG + ((VRAMBankCnt >> 3) & 3) * 0x20000 ;
			} else return ;
			break ;
		case 2:
				switch(block){
	case 0:
	case 1:
	case 2:
	case 3:
		/* banks are in use for BG at ABG + ofs * 0x20000 */
				destination = ARM9Mem.ARM9_ABG + ((VRAMBankCnt >> 3) & 3) * 0x20000 ;
		break ;
	case 4:
		/* bank E is in use at ABG */ 
		destination = ARM9Mem.ARM9_ABG ;
		break;
	case 5:
	case 6:
		/* banks are in use for BG at ABG + (0x4000*OFS.0)+(0x10000*OFS.1)*/
		destination = ARM9Mem.ARM9_ABG + (((VRAMBankCnt >> 3) & 1) * 0x4000) + (((VRAMBankCnt >> 2) & 1) * 0x10000) ;
		break;
	case 8:
		/* bank H is in use at BBG */ 
		destination = ARM9Mem.ARM9_BBG ;
		break ;
	case 9:
		/* bank I is in use at BBG */ 
		destination = ARM9Mem.ARM9_BBG + 0x8000 ;
		break;
	default: return ;
	}
			break ;
		case 4:
	switch(block){
	case 2:
		/* bank C is in use at BBG */ 
		destination = ARM9Mem.ARM9_BBG ;
		break ;
	case 3:
		/* bank D is in use at BOBJ */ 
		destination = ARM9Mem.ARM9_BOBJ ;
		break ;
	default: return ;
	}
			break ;
		default:
			return ;
	}
	if (!destination) return ;
	if (!source) return ;
	memcpy(destination,source,size) ;
}

void MMU_setRom(u8 * rom, u32 mask)
{
	unsigned int i;
	MMU.CART_ROM = rom;
	
	for(i = 0x80; i<0xA0; ++i)
	{
		MMU_ARM9_MEM_MAP[i] = rom;
		MMU_ARM7_MEM_MAP[i] = rom;
		MMU_ARM9_MEM_MASK[i] = mask;
		MMU_ARM7_MEM_MASK[i] = mask;
	}
	rom_mask = mask;
}

void MMU_unsetRom()
{
	unsigned int i;
	MMU.CART_ROM=MMU.UNUSED_RAM;
	
	for(i = 0x80; i<0xA0; ++i)
	{
		MMU_ARM9_MEM_MAP[i] = MMU.UNUSED_RAM;
		MMU_ARM7_MEM_MAP[i] = MMU.UNUSED_RAM;
		MMU_ARM9_MEM_MASK[i] = ROM_MASK;
		MMU_ARM7_MEM_MASK[i] = ROM_MASK;
	}
	rom_mask = ROM_MASK;
}


static void MMU_IPCSync(u8 proc, u32 val)
{
	//INFO("IPC%s sync 0x%08X\n", proc?"7":"9", val);
	u32 IPCSYNC_local = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0x180) & 0xFFFF;
	u32 IPCSYNC_remote = T1ReadLong(MMU.MMU_MEM[proc^1][0x40], 0x180);

	IPCSYNC_local = (IPCSYNC_local&0x6000)|(val&0xf00)|(IPCSYNC_local&0xf);
	IPCSYNC_remote =(IPCSYNC_remote&0x6f00)|((val>>8)&0xf);

	T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x180, IPCSYNC_local);
	T1WriteLong(MMU.MMU_MEM[proc^1][0x40], 0x180, IPCSYNC_remote);

//	if ((val & 0x2000) && (IPCSYNC_remote & 0x4000))
//		NDS_makeInt(proc^1, 17);
}

char txt[80];	

#define NEXT(n, i)  (((n) + (i)/(n)) >> 1)

unsigned int isqrt(int number) {
  unsigned int n  = 1;
  unsigned int n1 = NEXT(n, number);

  while(abs(n1 - n) > 1) {
    n  = n1;
    n1 = NEXT(n, number);
  }
  while((n1*n1) > number) {
    n1 -= 1;
  }
  return n1;
}


static void execsqrt(u32 proc) {
	u32 ret;
	u16 cnt = T1ReadWord(MMU.MMU_MEM[proc][0x40], 0x2B0);
	switch(cnt&1)
	{
	case 0: {
		u32 v = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0x2B8);
		ret = isqrt(v);
		break;
		}
	case 1: {
		u64 v = T1ReadQuad(MMU.MMU_MEM[proc][0x40], 0x2B8);
		ret = isqrt(v);
		break;
		}
	}
	T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x2B4, 0);
	T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x2B0, cnt | 0x8000);

	MMU.sqrtCycles = (nds.cycles + 26);
	MMU.sqrtResult = ret;
	MMU.sqrtCnt = (cnt & 0x7FFF);
	MMU.sqrtRunning = TRUE;
}


static void execdiv(u32 proc) {
	u16 cnt = T1ReadWord(MMU.MMU_MEM[proc][0x40], 0x280);
	s64 num,den;
	s64 res,mod;
	switch(cnt&3)
	{
	case 0:
		num = (s64) (s32) T1ReadLong(MMU.MMU_MEM[proc][0x40], 0x290);
		den = (s64) (s32) T1ReadLong(MMU.MMU_MEM[proc][0x40], 0x298);
		MMU.divCycles = (nds.cycles + 36);
	break;
	case 3: //gbatek says this is same as mode 1
	case 1:
		num = (s64) T1ReadQuad(MMU.MMU_MEM[proc][0x40], 0x290);
		den = (s64) (s32) T1ReadLong(MMU.MMU_MEM[proc][0x40], 0x298);
		MMU.divCycles = (nds.cycles + 68);
	break;
	case 2:
		num = (s64) T1ReadQuad(MMU.MMU_MEM[proc][0x40], 0x290);
		den = (s64) T1ReadQuad(MMU.MMU_MEM[proc][0x40], 0x298);
		MMU.divCycles = (nds.cycles + 68);
		break;
	}

	if(den==0)
	{
		res = ((num < 0) ? 1 : -1);
		mod = num;
		cnt |= 0x4000;
		cnt &= 0x7FFF;
	}
	else
	{
		res = num / den;
		mod = num % den;
		cnt &= 0x3FFF;
	}

	DIVLOG("DIV %08X%08X / %08X%08X = %08X%08X\r\n", (u32)(num>>32), (u32)num, 
							(u32)(den>>32), (u32)den, 
							(u32)(res>>32), (u32)res);

	T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x2A0, 0);
	T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x2A4, 0);
	T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x2A8, 0);
	T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x2AC, 0);
	T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x280, ((cnt & 0xBFFF) | 0x8000));

	MMU.divResult = res;
	MMU.divMod = mod;
	MMU.divCnt = (cnt & 0x7FFF);
	MMU.divRunning = TRUE;
}



u8 FASTCALL MMU_read8(u32 proc, u32 adr)
{
#ifdef INTERNAL_DTCM_READ
	if((proc==ARMCPU_ARM9)&((adr&(~0x3FFF))==MMU.DTCMRegion))
	{
		return ARM9Mem.ARM9_DTCM[adr&0x3FFF];
	}
#endif
	
#ifdef INTERNAL_ITCM_READ
	if(proc==ARMCPU_ARM9 && adr<0x02000000){
		return T1ReadByte(ARM9Mem.ARM9_ITCM, adr&0x7fff);
	}
#endif


	if(adr>=0x02400000 && adr<0x03000000) {
		//int zzz=9;
	}


	// CFlash reading, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000))
		return (unsigned char)cflash_read(adr);

#ifdef EXPERIMENTAL_WIFI
	/* wifi mac access */
	if ((proc==ARMCPU_ARM7) && (adr>=0x04800000)&&(adr<0x05000000))
	{
		if (adr & 1)
			return (WIFI_read16(&wifiMac,adr) >> 8) & 0xFF;
		else
			return WIFI_read16(&wifiMac,adr) & 0xFF;
	}
#endif

	if(0x08000000 <= adr && adr < 0x0A000000) {
		/* ROM  FIX Security */
		return 0;
	}

        return MMU.MMU_MEM[proc][(adr>>20)&0xFF][adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF]];
}



u16 FASTCALL MMU_read16(u32 proc, u32 adr)
{    
#ifdef INTERNAL_DTCM_READ
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == MMU.DTCMRegion))
	{
		/* Returns data from DTCM (ARM9 only) */
		return T1ReadWord(ARM9Mem.ARM9_DTCM, adr & 0x3FFF);
	}
#endif
#ifdef INTERNAL_ITCM_READ
	if(proc==ARMCPU_ARM9 && adr<0x02000000){
		return T1ReadWord(ARM9Mem.ARM9_ITCM, adr&0x7fff);
	}
#endif

	if(adr>=0x02400000 && adr<0x03000000) {
		//int zzz=9;
	}

	
	// CFlash reading, Mic
	if ((adr>=0x08800000)&&(adr<0x09900000))
	   return (unsigned short)cflash_read(adr);

#ifdef EXPERIMENTAL_WIFI
	/* wifi mac access */
	if ((proc==ARMCPU_ARM7) && (adr>=0x04800000)&&(adr<0x05000000))
		return WIFI_read16(&wifiMac,adr) ;
#endif

	adr &= 0x0FFFFFFF;

	if (adr >> 24 == 4)
	{
		/* Address is an IO register */
		switch(adr)
		{
			case 0x04000604:
				return (gfx3d_GetNumPolys()&2047);
			case 0x04000606:
				return (gfx3d_GetNumVertex()&8191);
			// ============================================= 3D end
	
			case REG_IPCFIFORECV :               /* TODO (clear): ??? */
				//execute = FALSE;
				return 1;
				
			case REG_IME :
				return (u16)MMU.reg_IME[proc];
				
			case REG_IE :
				return (u16)MMU.reg_IE[proc];
			case REG_IE + 2 :
				return (u16)(MMU.reg_IE[proc]>>16);
				
			case REG_IF :
				return (u16)MMU.reg_IF[proc];
			case REG_IF + 2 :
				return (u16)(MMU.reg_IF[proc]>>16);
				
			case REG_TM0CNTL :
			case REG_TM1CNTL :
			case REG_TM2CNTL :
			case REG_TM3CNTL :
				return MMU.timer[proc][(adr&0xF)>>2];
			
			case 0x04000630 :
				LOG("vect res\r\n");	/* TODO (clear): ??? */
				return 0;
                        case REG_POSTFLG :
				return 1;
			default :
				break;
		}
	}
	
	if(0x08000000 <= adr && adr < 0x0A000000) {
		/* ROM PSP Security */
		return 0;
	}

	/* Returns data from memory */
	return T1ReadWord(MMU.MMU_MEM[proc][(adr >> 20) & 0xFF], adr & MMU.MMU_MASK[proc][(adr >> 20) & 0xFF]); 
}
	 
u32 FASTCALL MMU_read32(u32 proc, u32 adr)
{
#ifdef INTERNAL_DTCM_READ
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == MMU.DTCMRegion))
	{
		/* Returns data from DTCM (ARM9 only) */
		return T1ReadLong(ARM9Mem.ARM9_DTCM, adr & 0x3FFF);
	}
#endif
#ifdef INTERNAL_ITCM_READ
	if(proc==ARMCPU_ARM9 && adr<0x02000000){
		return T1ReadLong(ARM9Mem.ARM9_ITCM, adr&0x7fff);
	}
#endif

	if(adr>=0x02400000 && adr<0x03000000) {
		//int zzz=9;
	}

	
	// CFlash reading, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000))
	   return (unsigned long)cflash_read(adr);
		
	adr &= 0x0FFFFFFF;

	if((adr >> 24) == 4)
	{
		/* Adress is an IO register */
		switch(adr)
		{
			// This is hacked due to the only current 3D core
/*			case 0x04000600:
            {
                u32 fifonum = IPCFIFO+proc;

				u32 gxstat =	(MMU.fifos[fifonum].empty<<26) | 
								(1<<25) | 
								(MMU.fifos[fifonum].full<<24) | 
								2;

				LOG ("GXSTAT: 0x%X", gxstat);

				return	gxstat;
            }
*/
			case 0x04000640:
			case 0x04000644:
			case 0x04000648:
			case 0x0400064C:
			case 0x04000650:
			case 0x04000654:
			case 0x04000658:
			case 0x0400065C:
			case 0x04000660:
			case 0x04000664:
			case 0x04000668:
			case 0x0400066C:
			case 0x04000670:
			case 0x04000674:
			case 0x04000678:
			case 0x0400067C:
			{
				//LOG("4000640h..67Fh - CLIPMTX_RESULT - Read Current Clip Coordinates Matrix (R)");
				return gfx3d_GetClipMatrix ((adr-0x04000640)/4);
			}
			case 0x04000680:
			case 0x04000684:
			case 0x04000688:
			case 0x0400068C:
			case 0x04000690:
			case 0x04000694:
			case 0x04000698:
			case 0x0400069C:
			case 0x040006A0:
			{
				//LOG("4000680h..6A3h - VECMTX_RESULT - Read Current Directional Vector Matrix (R)");
				return gfx3d_GetDirectionalMatrix ((adr-0x04000680)/4);
			}

			case 0x4000604:
			{
				return (gfx3d_GetNumPolys()&2047) & ((gfx3d_GetNumVertex()&8191) << 16);
				//LOG ("read32 - RAM_COUNT -> 0x%X", ((u32 *)(MMU.MMU_MEM[proc][(adr>>20)&0xFF]))[(adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF])>>2]);
			}
			
			case REG_IME :
				return MMU.reg_IME[proc];
			case REG_IE :
				return MMU.reg_IE[proc];
			case REG_IF :
				return MMU.reg_IF[proc];
			case REG_IPCFIFORECV :
			return IPC_FIFOrecv(proc);
                        case REG_TM0CNTL :
                        case REG_TM1CNTL :
                        case REG_TM2CNTL :
                        case REG_TM3CNTL :
			{
				u32 val = T1ReadWord(MMU.MMU_MEM[proc][0x40], (adr + 2) & 0xFFF);
				return MMU.timer[proc][(adr&0xF)>>2] | (val<<16);
			}	
            case REG_GCDATAIN:
			{
                                u32 val;

                                if(!MMU.dscard[proc].adress) return 0;
				
#ifdef __psp__
								{
									ROMReader_struct * reader;
									FILE * file;
									char * noext;
									extern char* gpszROMFilename;

									static u32 page_address = -1;
									static u32 page[2048 / 4];

									if(page_address != (MMU.dscard[proc].adress & ~0x7FF)) {
										page_address = MMU.dscard[proc].adress & ~0x7FF;
										noext = strdup( gpszROMFilename );
										reader = ROMReaderInit( &noext );
										
										file = (FILE*)reader->Init( gpszROMFilename );
										reader->Seek( file, page_address, SEEK_SET );

										reader->Read( file, &page, 2048 );
										reader->DeInit( file );
										free( noext );
									}
								val = page[(MMU.dscard[proc].adress & 0x7FF) / 4];
								}
#else
                                val = T1ReadLong(MMU.CART_ROM, MMU.dscard[proc].adress);
#endif

				MMU.dscard[proc].adress += 4;	/* increment adress */
				
				MMU.dscard[proc].transfer_count--;	/* update transfer counter */
				if(MMU.dscard[proc].transfer_count) /* if transfer is not ended */
				{
					return val;	/* return data */
				}
				else	/* transfer is done */
                                {                                                       
                                        T1WriteLong(MMU.MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, T1ReadLong(MMU.MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff) & ~(0x00800000 | 0x80000000));
					/* = 0x7f7fffff */
					
					/* if needed, throw irq for the end of transfer */
                                        if(T1ReadWord(MMU.MMU_MEM[proc][(REG_AUXSPICNT >> 20) & 0xff], REG_AUXSPICNT & 0xfff) & 0x4000)
					{
                                                if(proc == ARMCPU_ARM7) NDS_makeARM7Int(19); 
                                                else NDS_makeARM9Int(19);
					}
					
					return val;
				}
			}

			default :
				break;
		}
	}
	
#ifdef __psp__
	if(0x08000000 <= adr && adr < 0x0A000000) {
		/* ROM PSP Security */
		return 0;
	}
#endif
	/* Returns data from memory */
	return T1ReadLong(MMU.MMU_MEM[proc][(adr >> 20) & 0xFF], adr & MMU.MMU_MASK[proc][(adr >> 20) & 0xFF]);
}
	
void FASTCALL MMU_write8(u32 proc, u32 adr, u8 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == MMU.DTCMRegion))
	{
		/* Writes data in DTCM (ARM9 only) */
		ARM9Mem.ARM9_DTCM[adr&0x3FFF] = val;
		return ;
	}
#endif
#ifdef INTERNAL_ITCM_WRITE
	if(proc==ARMCPU_ARM9 && adr<0x02000000){
		T1WriteByte(ARM9Mem.ARM9_ITCM, adr&0x7fff, val);
		return;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000)) {
		cflash_write(adr,val);
		return;
	}

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteByte(adr, val);
              return;
           }
        }

	if (adr & 0xFF800000 == 0x04800000)
	{
		/* is wifi hardware, dont intermix with regular hardware registers */
		/* FIXME handle 8 bit writes */
		return ;
	}

	switch(adr)
	{
		case REG_DISPA_WIN0H: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_H1 (MainScreen.gpu, val);
			break ; 	 
		case REG_DISPA_WIN0H+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_H0 (MainScreen.gpu, val);
			break ; 	 
		case REG_DISPA_WIN1H: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_H1 (MainScreen.gpu,val);
			break ; 	 
		case REG_DISPA_WIN1H+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_H0 (MainScreen.gpu,val);
			break ; 	 

		case REG_DISPB_WIN0H: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_H1(SubScreen.gpu,val);
			break ; 	 
		case REG_DISPB_WIN0H+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_H0(SubScreen.gpu,val);
			break ; 	 
		case REG_DISPB_WIN1H: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_H1(SubScreen.gpu,val);
			break ; 	 
		case REG_DISPB_WIN1H+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_H0(SubScreen.gpu,val);
			break ;

		case REG_DISPA_WIN0V: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_V1(MainScreen.gpu,val) ; 	 
			break ; 	 
		case REG_DISPA_WIN0V+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_V0(MainScreen.gpu,val) ; 	 
			break ; 	 
		case REG_DISPA_WIN1V: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_V1(MainScreen.gpu,val) ; 	 
			break ; 	 
		case REG_DISPA_WIN1V+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_V0(MainScreen.gpu,val) ; 	 
			break ; 	 

		case REG_DISPB_WIN0V: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_V1(SubScreen.gpu,val) ;
			break ; 	 
		case REG_DISPB_WIN0V+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_V0(SubScreen.gpu,val) ;
			break ; 	 
		case REG_DISPB_WIN1V: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_V1(SubScreen.gpu,val) ;
			break ; 	 
		case REG_DISPB_WIN1V+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_V0(SubScreen.gpu,val) ;
			break ;

		case REG_DISPA_WININ: 	 
			if(proc == ARMCPU_ARM9) GPU_setWININ0(MainScreen.gpu,val) ; 	 
			break ; 	 
		case REG_DISPA_WININ+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWININ1(MainScreen.gpu,val) ; 	 
			break ; 	 
		case REG_DISPA_WINOUT: 	 
			if(proc == ARMCPU_ARM9) GPU_setWINOUT(MainScreen.gpu,val) ; 	 
			break ; 	 
		case REG_DISPA_WINOUT+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWINOBJ(MainScreen.gpu,val);
			break ; 	 

		case REG_DISPB_WININ: 	 
			if(proc == ARMCPU_ARM9) GPU_setWININ0(SubScreen.gpu,val) ; 	 
			break ; 	 
		case REG_DISPB_WININ+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWININ1(SubScreen.gpu,val) ; 	 
			break ; 	 
		case REG_DISPB_WINOUT: 	 
			if(proc == ARMCPU_ARM9) GPU_setWINOUT(SubScreen.gpu,val) ; 	 
			break ; 	 
		case REG_DISPB_WINOUT+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWINOBJ(SubScreen.gpu,val) ; 	 
			break ;


		case REG_DISPA_BLDCNT:
			if(proc == ARMCPU_ARM9) GPU_setBLDCNT_HIGH(MainScreen.gpu,val);
			break;
		case REG_DISPA_BLDCNT+1:
			if(proc == ARMCPU_ARM9) GPU_setBLDCNT_LOW (MainScreen.gpu,val);
			break;

		case REG_DISPB_BLDCNT: 	 
			if(proc == ARMCPU_ARM9) GPU_setBLDCNT_HIGH (SubScreen.gpu,val);
			break;
		case REG_DISPB_BLDCNT+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setBLDCNT_LOW (SubScreen.gpu,val);
			break;

		case REG_DISPA_BLDALPHA: 	 
			if(proc == ARMCPU_ARM9) GPU_setBLDALPHA_EVB(MainScreen.gpu,val) ; 	 
			break;
		case REG_DISPA_BLDALPHA+1:
			if(proc == ARMCPU_ARM9) GPU_setBLDALPHA_EVA(MainScreen.gpu,val) ; 	 
			break;

		case REG_DISPB_BLDALPHA:
			if(proc == ARMCPU_ARM9) GPU_setBLDALPHA_EVB(SubScreen.gpu,val) ; 	 
			break;
		case REG_DISPB_BLDALPHA+1:
			if(proc == ARMCPU_ARM9) GPU_setBLDALPHA_EVA(SubScreen.gpu,val);
			break;

		case REG_DISPA_BLDY: 	 
			if(proc == ARMCPU_ARM9) GPU_setBLDY_EVY(MainScreen.gpu,val) ; 	 
			break ; 	 
		case REG_DISPB_BLDY: 	 
			if(proc == ARMCPU_ARM9) GPU_setBLDY_EVY(SubScreen.gpu,val) ; 	 
			break;

		/* TODO: EEEK ! Controls for VRAMs A, B, C, D are missing ! */
		/* TODO: Not all mappings of VRAMs are handled... (especially BG and OBJ modes) */
		case REG_VRAMCNTA:
		case REG_VRAMCNTB:
		case REG_VRAMCNTC:
		case REG_VRAMCNTD:
			if(proc == ARMCPU_ARM9)
			{
                MMU_VRAMWriteBackToLCD(0) ;
                MMU_VRAMWriteBackToLCD(1) ;
                MMU_VRAMWriteBackToLCD(2) ;
                MMU_VRAMWriteBackToLCD(3) ;
				switch(val & 0x1F)
				{
				case 1 :
					MMU.vram_mode[adr-REG_VRAMCNTA] = 0; // BG-VRAM
					//MMU.vram_offset[0] = ARM9Mem.ARM9_ABG+(0x20000*0); // BG-VRAM
					break;
				case 1 | (1 << 3) :
					MMU.vram_mode[adr-REG_VRAMCNTA] = 1; // BG-VRAM
					//MMU.vram_offset[0] = ARM9Mem.ARM9_ABG+(0x20000*1); // BG-VRAM
					break;
				case 1 | (2 << 3) :
					MMU.vram_mode[adr-REG_VRAMCNTA] = 2; // BG-VRAM
					//MMU.vram_offset[0] = ARM9Mem.ARM9_ABG+(0x20000*2); // BG-VRAM
					break;
				case 1 | (3 << 3) :
					MMU.vram_mode[adr-REG_VRAMCNTA] = 3; // BG-VRAM
					//MMU.vram_offset[0] = ARM9Mem.ARM9_ABG+(0x20000*3); // BG-VRAM
					break;
				case 0: /* mapped to lcd */
                    MMU.vram_mode[adr-REG_VRAMCNTA] = 4 | (adr-REG_VRAMCNTA) ;
					break ;
				}
                                /*
                                 * FIXME: simply texture slot handling
                                 * This is a first stab and is not correct. It does
                                 * not handle a VRAM texture slot becoming
                                 * unconfigured.
                                 * Revisit all of VRAM control handling for future
                                 * release?
                                 */
                                if ( val & 0x80) {
                                  if ( (val & 0x7) == 3) {
                                    int slot_index = (val >> 3) & 0x3;

                                    ARM9Mem.textureSlotAddr[slot_index] =
                                      &ARM9Mem.ARM9_LCD[0x20000 * (adr - REG_VRAMCNTA)];
                                  }
                                }
                MMU_VRAMReloadFromLCD(adr-REG_VRAMCNTA,val) ;
			}
			break;
                case REG_VRAMCNTE :
			if(proc == ARMCPU_ARM9)
			{
                MMU_VRAMWriteBackToLCD((u8)REG_VRAMCNTE) ;
                                if((val & 7) == 5)
				{
					ARM9Mem.ExtPal[0][0] = ARM9Mem.ARM9_LCD + 0x80000;
					ARM9Mem.ExtPal[0][1] = ARM9Mem.ARM9_LCD + 0x82000;
					ARM9Mem.ExtPal[0][2] = ARM9Mem.ARM9_LCD + 0x84000;
					ARM9Mem.ExtPal[0][3] = ARM9Mem.ARM9_LCD + 0x86000;
				}
                                else if((val & 7) == 3)
				{
					ARM9Mem.texPalSlot[0] = ARM9Mem.ARM9_LCD + 0x80000;
					ARM9Mem.texPalSlot[1] = ARM9Mem.ARM9_LCD + 0x82000;
					ARM9Mem.texPalSlot[2] = ARM9Mem.ARM9_LCD + 0x84000;
					ARM9Mem.texPalSlot[3] = ARM9Mem.ARM9_LCD + 0x86000;
				}
                                else if((val & 7) == 4)
				{
					ARM9Mem.ExtPal[0][0] = ARM9Mem.ARM9_LCD + 0x80000;
					ARM9Mem.ExtPal[0][1] = ARM9Mem.ARM9_LCD + 0x82000;
					ARM9Mem.ExtPal[0][2] = ARM9Mem.ARM9_LCD + 0x84000;
					ARM9Mem.ExtPal[0][3] = ARM9Mem.ARM9_LCD + 0x86000;
				}
				
				MMU_VRAMReloadFromLCD(adr-REG_VRAMCNTE,val) ;
			}
			break;
		
                case REG_VRAMCNTF :
			if(proc == ARMCPU_ARM9)
			{
				switch(val & 0x1F)
				{
                                        case 4 :
						ARM9Mem.ExtPal[0][0] = ARM9Mem.ARM9_LCD + 0x90000;
						ARM9Mem.ExtPal[0][1] = ARM9Mem.ARM9_LCD + 0x92000;
						break;
						
                                        case 4 | (1 << 3) :
						ARM9Mem.ExtPal[0][2] = ARM9Mem.ARM9_LCD + 0x90000;
						ARM9Mem.ExtPal[0][3] = ARM9Mem.ARM9_LCD + 0x92000;
						break;
						
                                        case 3 :
						ARM9Mem.texPalSlot[0] = ARM9Mem.ARM9_LCD + 0x90000;
						break;
						
                                        case 3 | (1 << 3) :
						ARM9Mem.texPalSlot[1] = ARM9Mem.ARM9_LCD + 0x90000;
						break;
						
                                        case 3 | (2 << 3) :
						ARM9Mem.texPalSlot[2] = ARM9Mem.ARM9_LCD + 0x90000;
						break;
						
                                        case 3 | (3 << 3) :
						ARM9Mem.texPalSlot[3] = ARM9Mem.ARM9_LCD + 0x90000;
						break;
						
                                        case 5 :
                                        case 5 | (1 << 3) :
                                        case 5 | (2 << 3) :
                                        case 5 | (3 << 3) :
						ARM9Mem.ObjExtPal[0][0] = ARM9Mem.ARM9_LCD + 0x90000;
						ARM9Mem.ObjExtPal[0][1] = ARM9Mem.ARM9_LCD + 0x92000;
						break;
				}
		 	}
			break;
                case REG_VRAMCNTG :
			if(proc == ARMCPU_ARM9)
			{
		 		switch(val & 0x1F)
				{
                                        case 4 :
						ARM9Mem.ExtPal[0][0] = ARM9Mem.ARM9_LCD + 0x94000;
						ARM9Mem.ExtPal[0][1] = ARM9Mem.ARM9_LCD + 0x96000;
						break;
						
                                        case 4 | (1 << 3) :
						ARM9Mem.ExtPal[0][2] = ARM9Mem.ARM9_LCD + 0x94000;
						ARM9Mem.ExtPal[0][3] = ARM9Mem.ARM9_LCD + 0x96000;
						break;
						
                                        case 3 :
						ARM9Mem.texPalSlot[0] = ARM9Mem.ARM9_LCD + 0x94000;
						break;
						
                                        case 3 | (1 << 3) :
						ARM9Mem.texPalSlot[1] = ARM9Mem.ARM9_LCD + 0x94000;
						break;
						
                                        case 3 | (2 << 3) :
						ARM9Mem.texPalSlot[2] = ARM9Mem.ARM9_LCD + 0x94000;
						break;
						
                                        case 3 | (3 << 3) :
						ARM9Mem.texPalSlot[3] = ARM9Mem.ARM9_LCD + 0x94000;
						break;
						
                                        case 5 :
                                        case 5 | (1 << 3) :
                                        case 5 | (2 << 3) :
                                        case 5 | (3 << 3) :
						ARM9Mem.ObjExtPal[0][0] = ARM9Mem.ARM9_LCD + 0x94000;
						ARM9Mem.ObjExtPal[0][1] = ARM9Mem.ARM9_LCD + 0x96000;
						break;
				}
			}
			break;
			
                case REG_VRAMCNTH  :
			if(proc == ARMCPU_ARM9)
			{
                MMU_VRAMWriteBackToLCD((u8)REG_VRAMCNTH) ;
                
                                if((val & 7) == 2)
				{
					ARM9Mem.ExtPal[1][0] = ARM9Mem.ARM9_LCD + 0x98000;
					ARM9Mem.ExtPal[1][1] = ARM9Mem.ARM9_LCD + 0x9A000;
					ARM9Mem.ExtPal[1][2] = ARM9Mem.ARM9_LCD + 0x9C000;
					ARM9Mem.ExtPal[1][3] = ARM9Mem.ARM9_LCD + 0x9E000;
				}
				
				MMU_VRAMReloadFromLCD(adr-REG_VRAMCNTH,val) ;
			}
			break;
			
                case REG_VRAMCNTI  :
			if(proc == ARMCPU_ARM9)
			{
                MMU_VRAMWriteBackToLCD((u8)REG_VRAMCNTI) ;
                
                                if((val & 7) == 3)
				{
					ARM9Mem.ObjExtPal[1][0] = ARM9Mem.ARM9_LCD + 0xA0000;
					ARM9Mem.ObjExtPal[1][1] = ARM9Mem.ARM9_LCD + 0xA2000;
				}
				
				MMU_VRAMReloadFromLCD(adr-REG_VRAMCNTI,val) ;
			}
			break;

#ifdef LOG_CARD
		case 0x040001A0 : /* TODO (clear): ??? */
		case 0x040001A1 :
		case 0x040001A2 :
		case 0x040001A8 :
		case 0x040001A9 :
		case 0x040001AA :
		case 0x040001AB :
		case 0x040001AC :
		case 0x040001AD :
		case 0x040001AE :
		case 0x040001AF :
                    LOG("%08X : %02X\r\n", adr, val);
#endif
		
		default :
			break;
	}
	
	MMU.MMU_MEM[proc][(adr>>20)&0xFF][adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF]]=val;
}

u16 partie = 1;

void FASTCALL MMU_write16(u32 proc, u32 adr, u16 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == MMU.DTCMRegion))
	{
		/* Writes in DTCM (ARM9 only) */
		T1WriteWord(ARM9Mem.ARM9_DTCM, adr & 0x3FFF, val);
		return;
	}
#endif
#ifdef INTERNAL_ITCM_WRITE
	if(proc==ARMCPU_ARM9 && adr<0x02000000){
		T1WriteWord(ARM9Mem.ARM9_ITCM, adr&0x7fff, val);
		return;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x08800000)&&(adr<0x09900000))
	{
		cflash_write(adr,val);
		return;
	}

#ifdef EXPERIMENTAL_WIFI

	/* wifi mac access */
	if ((proc==ARMCPU_ARM7) && (adr>=0x04800000)&&(adr<0x05000000))
	{
		WIFI_write16(&wifiMac,adr,val) ;
		return ;
	}
#else
	if ((proc==ARMCPU_ARM7) && (adr>=0x04800000)&&(adr<0x05000000))
		return ;
#endif

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteWord(adr, val);
              return;
           }
        }

	if((adr >> 24) == 4)
	{
		/* Adress is an IO register */
		switch(adr)
		{
			case 0x0400035C:
			{
				((u16 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[0x35C>>1] = val;
				gfx3d_glFogOffset (val);
				return;
			}
			case 0x04000340:
			{
				((u16 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[0x340>>1] = val;
				gfx3d_glAlphaFunc(val);
				return;
			}
			case 0x04000060:
			{
				((u16 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[0x060>>1] = val;
				gfx3d_Control(val);
				return;
			}
			case 0x04000354:
			{
				((u16 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[0x354>>1] = val;
				gfx3d_glClearDepth(val);
				return;
			}
			case REG_DISPA_BLDCNT: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDCNT(MainScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPB_BLDCNT: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDCNT(SubScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPA_BLDALPHA: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDALPHA(MainScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPB_BLDALPHA: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDALPHA(SubScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPA_BLDY: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDY_EVY(MainScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPB_BLDY: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDY_EVY(SubScreen.gpu,val) ; 	 
				break;
			case REG_DISPA_MASTERBRIGHT:
				GPU_setMasterBrightness (MainScreen.gpu, val);
				break;
				/*
			case REG_DISPA_MOSAIC: 	 
				if(proc == ARMCPU_ARM9) GPU_setMOSAIC(MainScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPB_MOSAIC: 	 
				if(proc == ARMCPU_ARM9) GPU_setMOSAIC(SubScreen.gpu,val) ; 	 
				break ;
				*/

			case REG_DISPA_WIN0H: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN0_H (MainScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPA_WIN1H: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN1_H(MainScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPB_WIN0H: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN0_H(SubScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPB_WIN1H: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN1_H(SubScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPA_WIN0V: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN0_V(MainScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPA_WIN1V: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN1_V(MainScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPB_WIN0V: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN0_V(SubScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPB_WIN1V: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN1_V(SubScreen.gpu,val) ; 	 
				break ; 	 
			case REG_DISPA_WININ: 	 
				if(proc == ARMCPU_ARM9) GPU_setWININ(MainScreen.gpu, val) ; 	 
				break ; 	 
			case REG_DISPA_WINOUT: 	 
				if(proc == ARMCPU_ARM9) GPU_setWINOUT16(MainScreen.gpu, val) ; 	 
				break ; 	 
			case REG_DISPB_WININ: 	 
				if(proc == ARMCPU_ARM9) GPU_setWININ(SubScreen.gpu, val) ; 	 
				break ; 	 
			case REG_DISPB_WINOUT: 	 
				if(proc == ARMCPU_ARM9) GPU_setWINOUT16(SubScreen.gpu, val) ; 	 
				break ;

			case REG_DISPB_MASTERBRIGHT:
				GPU_setMasterBrightness (SubScreen.gpu, val);
				break;
			
            case REG_POWCNT1 :
				if(proc == ARMCPU_ARM9)
				{
					if(val & (1<<15))
					{
						LOG("Main core on top\n");
						MainScreen.offset = 0;
						SubScreen.offset = 192;
						//nds.swapScreen();
					}
					else
					{
						LOG("Main core on bottom (%04X)\n", val);
						MainScreen.offset = 192;
						SubScreen.offset = 0;
					}
				}
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0x304, val);
				return;
			case REG_EXMEMCNT:
			{
			if(proc == ARMCPU_ARM9) {
				u16 oldval = T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x204);
				T1WriteWord(MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x204, val);
				T1WriteWord(MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x204, (val & 0xFF80) | (oldval & 0x7F));
			}else{
				u16 oldval = T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x204);
				T1WriteWord(MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x204, (val & 0x7F) | (oldval & 0xFF80));
			}
			}
			return;
            case REG_AUXSPICNT:
			if(proc == ARMCPU_ARM7) {
				T1WriteWord(MMU.MMU_MEM[ARMCPU_ARM7][(REG_AUXSPICNT >> 20) & 0xff], REG_AUXSPICNT & 0xfff, val);
			    AUX_SPI_CNT = val;

				if (val == 0)
				   mc_reset_com(&MMU.bupmem);     // reset backup memory device communication
			    }				
				return;
				
			case REG_AUXSPIDATA:
			if(proc == ARMCPU_ARM7) {
				if(val!=0)
				   AUX_SPI_CMD = val & 0xFF;

				T1WriteWord(MMU.MMU_MEM[ARMCPU_ARM7][(REG_AUXSPIDATA >> 20) & 0xff], REG_AUXSPIDATA & 0xfff, bm_transfer(&MMU.bupmem, val));
			    }
				return;
		    case REG_RTC:
			if(proc == ARMCPU_ARM7){
				rtcWrite(val);
			}			
				break;
				
			case REG_SPICNT :
				{
	if(proc == ARMCPU_ARM7) {
							int reset_firmware = 1;

					if ( ((SPI_CNT >> 8) & 0x3) == 1) 
					{
						if ( ((val >> 8) & 0x3) == 1) 
						{
							if ( BIT11(SPI_CNT)) 
							{
								// select held
								reset_firmware = 0;
							}
						}
					}

						//MMU.fw.com == 0; // reset fw device communication
					if ( reset_firmware) 
					{
					  // reset fw device communication
					  mc_reset_com(&MMU.fw);
					}
					SPI_CNT = val;
					
					T1WriteWord(MMU.MMU_MEM[ARMCPU_ARM7][(REG_SPICNT >> 20) & 0xff], REG_SPICNT & 0xfff, val);
				}
			}
			return;
		
			case REG_SPIDATA :
				if(proc==ARMCPU_ARM7)
				{
                    u16 spicnt;

					if(val!=0)
					{
						SPI_CMD = val;
					}
			
                                        spicnt = T1ReadWord(MMU.MMU_MEM[proc][(REG_SPICNT >> 20) & 0xff], REG_SPICNT & 0xfff);
					
                                        switch((spicnt >> 8) & 0x3)
					{
                                                case 0 :
							break;
							
                                                case 1 : /* firmware memory device */
                                                        if(spicnt & 0x3 != 0)      /* check SPI baudrate (must be 4mhz) */
							{
								T1WriteWord(MMU.MMU_MEM[proc][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, 0);
								break;
							}
							T1WriteWord(MMU.MMU_MEM[proc][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, fw_transfer(&MMU.fw, val));

							return;
							
                                                case 2 :
							switch(SPI_CMD & 0x70)
							{
								case 0x00 :
									val = 0;
									break;
								case 0x10 :
									//execute = FALSE;
									if(SPI_CNT&(1<<11))
									{
										if(partie)
										{
											val = ((nds.touchY<<3)&0x7FF);
											partie = 0;
											//execute = FALSE;
											break;
										}
										val = (nds.touchY>>5);
                                                                                partie = 1;
										break;
									}
									val = ((nds.touchY<<3)&0x7FF);
									partie = 1;
									break;
								case 0x20 :
									val = 0;
									break;
								case 0x30 :
									val = 0;
									break;
								case 0x40 :
									val = 0;
									break;
								case 0x50 :
                                                                        if(spicnt & 0x800)
									{
										if(partie)
										{
											val = ((nds.touchX<<3)&0x7FF);
											partie = 0;
											break;
										}
										val = (nds.touchX>>5);
										partie = 1;
										break;
									}
									val = ((nds.touchX<<3)&0x7FF);
									partie = 1;
									break;
								case 0x60 :
									val = 0;
									break;
								case 0x70 :
									val = 0;
									break;
							}
							break;
							
                            case 3 :
							/* NOTICE: Device 3 of SPI is reserved (unused and unusable) */
							break;
					}
			
				   	T1WriteWord(MMU.MMU_MEM[proc][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, val);
				}
				
				return;
				
				/* NOTICE: Perhaps we have to use gbatek-like reg names instead of libnds-like ones ...*/
				
                        case REG_DISPA_BG0CNT :
				//GPULOG("MAIN BG0 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(MainScreen.gpu, 0, val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0x8, val);
				return;
                        case REG_DISPA_BG1CNT :
				//GPULOG("MAIN BG1 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(MainScreen.gpu, 1, val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0xA, val);
				return;
                        case REG_DISPA_BG2CNT :
				//GPULOG("MAIN BG2 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(MainScreen.gpu, 2, val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0xC, val);
				return;
                        case REG_DISPA_BG3CNT :
				//GPULOG("MAIN BG3 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(MainScreen.gpu, 3, val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0xE, val);
				return;
                        case REG_DISPB_BG0CNT :
				//GPULOG("SUB BG0 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(SubScreen.gpu, 0, val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0x1008, val);
				return;
                        case REG_DISPB_BG1CNT :
				//GPULOG("SUB BG1 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(SubScreen.gpu, 1, val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0x100A, val);
				return;
                        case REG_DISPB_BG2CNT :
				//GPULOG("SUB BG2 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(SubScreen.gpu, 2, val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0x100C, val);
				return;
                        case REG_DISPB_BG3CNT :
				//GPULOG("SUB BG3 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(SubScreen.gpu, 3, val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0x100E, val);
				return;
                        case REG_IME : {
			        u32 old_val = MMU.reg_IME[proc];
				u32 new_val = val & 1;
				MMU.reg_IME[proc] = new_val;
				T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x208, val);
				if ( new_val && old_val != new_val) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( MMU.reg_IE[proc] & MMU.reg_IF[proc]) {
				    NDS_ARM7.wIRQ = TRUE;
				    NDS_ARM7.waitIRQ = FALSE;
				  }
				}
				return;
			}
			case REG_VRAMCNTA:
				MMU_write8(proc,adr,val & 0xFF) ;
				MMU_write8(proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTC:
				MMU_write8(proc,adr,val & 0xFF) ;
				MMU_write8(proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTE:
				MMU_write8(proc,adr,val & 0xFF) ;
				MMU_write8(proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTG:
				MMU_write8(proc,adr,val & 0xFF) ;
				MMU_write8(proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTI:
				MMU_write8(proc,adr,val & 0xFF) ;
				return ;

			case REG_IE :
				MMU.reg_IE[proc] = (MMU.reg_IE[proc]&0xFFFF0000) | val;
				if ( MMU.reg_IME[proc]) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( MMU.reg_IE[proc] & MMU.reg_IF[proc]) {
				    NDS_ARM7.wIRQ = TRUE;
				    NDS_ARM7.waitIRQ = FALSE;
				  }
				}
				return;
			case REG_IE + 2 :
				//execute = FALSE;
				MMU.reg_IE[proc] = (MMU.reg_IE[proc]&0xFFFF) | (((u32)val)<<16);
				return;
				
			case REG_IF :
				//execute = FALSE;
				MMU.reg_IF[proc] &= (~((u32)val)); 
				return;
			case REG_IF + 2 :
				//execute = FALSE;
				MMU.reg_IF[proc] &= (~(((u32)val)<<16));
				return;
				
                case REG_IPCSYNC :
				         MMU_IPCSync(proc, val);
				
				return;
                        case REG_IPCFIFOCNT :
				        IPC_FIFOcnt(proc, val);				
				return;
                        case REG_TM0CNTL :
                        case REG_TM1CNTL :
                        case REG_TM2CNTL :
                        case REG_TM3CNTL :
				MMU.timerReload[proc][(adr>>2)&3] = val;
				return;
                        case REG_TM0CNTH :
                        case REG_TM1CNTH :
                        case REG_TM2CNTH :
                        case REG_TM3CNTH :
				{
				int timerIndex	= ((adr-2)>>2)&0x3;
				int mask		= ((val&0x80)>>7) << timerIndex;
				MMU.CheckTimers = (MMU.CheckTimers & (~mask)) | mask;

				if(val&0x80)
				MMU.timer[proc][timerIndex] = MMU.timerReload[ARMCPU_ARM9][((adr-2)>>2)&0x3];

				MMU.timerON[proc][((adr-2)>>2)&0x3] = val & 0x80;			

				switch(val&7)
				{
					case 0 :
						MMU.timerMODE[proc][timerIndex] = 0+1;//ARMCPU_ARM9;
						break;
					case 1 :
						MMU.timerMODE[proc][timerIndex] = 6+1;//ARMCPU_ARM9; 
						break;
					case 2 :
						MMU.timerMODE[proc][timerIndex] = 8+1;//ARMCPU_ARM9;
						break;
					case 3 :
						MMU.timerMODE[proc][timerIndex] = 10+1;//ARMCPU_ARM9;
						break;
					default :
						MMU.timerMODE[proc][timerIndex] = 0xFFFF;
						break;
				}

				if(!(val & 0x80))
					MMU.timerRUN[proc][timerIndex] = FALSE;

				T1WriteWord(MMU.MMU_MEM[proc][0x40], adr & 0xFFF, val);
				return;
			}

			case REG_DISPA_DISPCNT :
				{
					u32 v = (T1ReadLong(MMU.MMU_MEM[proc][0x40], 0) & 0xFFFF0000) | val;
					GPU_setVideoProp(MainScreen.gpu, v);
					T1WriteLong(MMU.MMU_MEM[proc][0x40], 0, v);
					return;
				}
			case REG_DISPA_DISPCNT+2 : 
				{
					u32 v = (T1ReadLong(MMU.MMU_MEM[proc][0x40], 0) & 0xFFFF) | ((u32) val << 16);
					GPU_setVideoProp(MainScreen.gpu, v);
					T1WriteLong(MMU.MMU_MEM[proc][0x40], 0, v);
				}
				return;
			case REG_DISPA_DISPCAPCNT :
				{
					u32 v = (T1ReadLong(MMU.MMU_MEM[proc][0x40], 0x64) & 0xFFFF0000) | val; 
					GPU_set_DISPCAPCNT(MainScreen.gpu,val);
					T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x64, v);
					return;
				}
			case REG_DISPA_DISPCAPCNT + 2:
				{
					u32 v = (T1ReadLong(MMU.MMU_MEM[proc][0x40], 0x64) & 0xFFFF) | val; 
					GPU_set_DISPCAPCNT(MainScreen.gpu,val);
					T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x64, v);
					return;
				}

			case REG_DISPB_DISPCNT :
				{
					u32 v = (T1ReadLong(MMU.MMU_MEM[proc][0x40], 0x1000) & 0xFFFF0000) | val;
					GPU_setVideoProp(SubScreen.gpu, v);
					T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x1000, v);
					return;
				}
			case REG_DISPB_DISPCNT+2 : 
				{
					//emu_halt();
					u32 v = (T1ReadLong(MMU.MMU_MEM[proc][0x40], 0x1000) & 0xFFFF) | ((u32) val << 16);
					GPU_setVideoProp(SubScreen.gpu, v);
					T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x1000, v);
					return;
				}
			
			//case 0x020D8460 :
			/*case 0x0235A904 :
				LOG("ECRIRE %d %04X\r\n", proc, val);
				execute = FALSE;*/
                                case REG_DMA0CNTH :
				{
                                u32 v;

				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma0 %04X\r\n", val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0xBA, val);
				DMASrc[proc][0] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xB0);
				DMADst[proc][0] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xB4);
                                v = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xB8);
				MMU.DMAStartTime[proc][0] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				MMU.DMACrt[proc][0] = v;
				if(MMU.DMAStartTime[proc][0] == 0)
					MMU_doDMA(proc, 0);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 0, DMASrc[proc][0], DMADst[proc][0], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
                case REG_DMA1CNTH :
				{
                                u32 v;
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma1 %04X\r\n", val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0xC6, val);
				DMASrc[proc][1] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xBC);
				DMASrc[proc][1] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xC0);
                                v = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xC4);
				MMU.DMAStartTime[proc][1] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				MMU.DMACrt[proc][1] = v;
				if(MMU.DMAStartTime[proc][1] == 0)
					MMU_doDMA(proc, 1);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 1, DMASrc[proc][1], DMADst[proc][1], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
                                case REG_DMA2CNTH :
				{
                                u32 v;
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma2 %04X\r\n", val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0xD2, val);
				DMASrc[proc][2] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xC8);
				DMASrc[proc][2] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xCC);
                                v = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xD0);
				MMU.DMAStartTime[proc][2] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				MMU.DMACrt[proc][2] = v;
				if(MMU.DMAStartTime[proc][2] == 0)
					MMU_doDMA(proc, 2);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 2, DMASrc[proc][2], DMADst[proc][2], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
                                case REG_DMA3CNTH :
				{
                                u32 v;
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma3 %04X\r\n", val);
				T1WriteWord(MMU.MMU_MEM[proc][0x40], 0xDE, val);
				DMASrc[proc][3] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xD4);
				DMASrc[proc][3] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xD8);
                                v = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xDC);
				MMU.DMAStartTime[proc][3] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				MMU.DMACrt[proc][3] = v;
		
				if(MMU.DMAStartTime[proc][3] == 0)
					MMU_doDMA(proc, 3);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 3, DMASrc[proc][3], DMADst[proc][3], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
                        //case REG_AUXSPICNT : execute = FALSE;
			default :
				T1WriteWord(MMU.MMU_MEM[proc][0x40], adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF], val); 
				return;
		}
	}
	T1WriteWord(MMU.MMU_MEM[proc][(adr>>20)&0xFF], adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF], val);
} 


void FASTCALL MMU_write32(u32 proc, u32 adr, u32 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc==ARMCPU_ARM9)&((adr&(~0x3FFF))==MMU.DTCMRegion))
	{
		T1WriteLong(ARM9Mem.ARM9_DTCM, adr & 0x3FFF, val);
		return ;
	}
#endif
#ifdef INTERNAL_ITCM_WRITE
	if(proc==ARMCPU_ARM9 && adr<0x02000000){
		T1WriteLong(ARM9Mem.ARM9_ITCM, adr&0x7fff, val);
		return;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000)) {
	   cflash_write(adr,val);
	   return;
	}

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteLong(adr, val);
              return;
           }
        }

		if (adr & 0xFF800000 == 0x04800000) {
		/* access to non regular hw registers */
		/* return to not overwrite valid data */
			return ;
		} ;


	if((adr>>24)==4)
	{
		if( (adr >= 0x04000330) && (adr < 0x04000340) )	//edge color table
		{
			((u32 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[(adr & 0xFFF) >> 2] = val;
			return;
		}

		if( (adr >= 0x04000360) && (adr < 0x04000380) )	//fog table
		{
			((u32 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[(adr & 0xFFF) >> 2] = val;
			return;
		}

		if( (adr >= 0x04000380) && (adr <= 0x40003BC) )	//toon table
		{
			((u32 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[(adr & 0xFFF) >> 2] = val;
			gfx3d_UpdateToonTable(&((MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[(0x380)]);
			return;
		}

		if ( (adr >= 0x04000400) && (adr < 0x04000440) )
		{
			gfx3d_sendCommandToFIFO(val);
			return;
		}

		if ( (adr >= 0x04000440) && (adr < 0x04000600) )
		{
			gfx3d_sendCommand(adr, val);
			return;
		}

		switch(adr)
		{
			// Alpha test reference value - Parameters:1
			case 0x04000340:
			{
				((u32 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[0x340>>2] = val;
				gfx3d_glAlphaFunc(val);
				return;
			}
			// Clear background color setup - Parameters:2
			case 0x04000350:
			{
				((u32 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[0x350>>2] = val;
				gfx3d_glClearColor(val);
				return;
			}
			// Clear background depth setup - Parameters:2
			case 0x04000354:
			{
				((u32 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[0x354>>2] = val;
				gfx3d_glClearDepth(val);
				return;
			}
			// Fog Color - Parameters:4b
			case 0x04000358:
			{
				((u32 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[0x358>>2] = val;
				gfx3d_glFogColor(val);
				return;
			}
			case 0x0400035C:
			{
				((u32 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[0x35C>>2] = val;
				gfx3d_glFogOffset(val);
				return;
			}
			case 0x04000600:
			//	((u32 *)(MMU.MMU_MEM[ARMCPU_ARM9][0x40]))[0x600>>2] = val;
			//GFX_FIFOcnt(val);
			return;
			case REG_DISPA_WININ: 	 
			{
	            if(proc == ARMCPU_ARM9) 	 
	            { 	 
	                    GPU_setWININ	(MainScreen.gpu, val & 0xFFFF) ; 	 
	                    GPU_setWINOUT16	(MainScreen.gpu, (val >> 16) & 0xFFFF) ; 	 
	            } 	 
	            break;
			}
			case REG_DISPB_WININ:
			{
	            if(proc == ARMCPU_ARM9) 	 
	            { 	 
	                    GPU_setWININ	(SubScreen.gpu, val & 0xFFFF) ; 	 
	                    GPU_setWINOUT16	(SubScreen.gpu, (val >> 16) & 0xFFFF) ; 	 
	            } 	 
	            break;
			}

			case REG_DISPA_BLDCNT:
			{
				if (proc == ARMCPU_ARM9) 	 
				{ 	 
					GPU_setBLDCNT   (MainScreen.gpu,val&0xffff);
					GPU_setBLDALPHA (MainScreen.gpu,val>>16);
				}
				break;
			}
			case REG_DISPB_BLDCNT:
			{
				if (proc == ARMCPU_ARM9) 	 
				{ 	 
					GPU_setBLDCNT   (SubScreen.gpu,val&0xffff);
					GPU_setBLDALPHA (SubScreen.gpu,val>>16);
				}
				break;
			}
/*
 // Commented out, as this doesn't use the plug-in system, neither works
			case cmd_3D_MTX_MODE        // 0x04000440 :
				if (proc == ARMCPU_ARM9) gl_MTX_MODE(val);
				return;
			case cmd_3D_MTX_PUSH        // 0x04000444  :
			case cmd_3D_MTX_POP         // 0x04000448  :
			case cmd_3D_MTX_STORE       // 0x0400044C  :
			case cmd_3D_MTX_RESTORE     // 0x04000450  :
				if (proc == ARMCPU_ARM9) gl_print_cmd(adr);
				return;
			case cmd_3D_MTX_IDENTITY    // 0x04000454  :
				if (proc == ARMCPU_ARM9) gl_MTX_IDENTITY();
				return;
			case cmd_3D_MTX_LOAD_4x4    // 0x04000458  :
				if (proc == ARMCPU_ARM9) gl_MTX_LOAD_4x4(val);
				return;
			case cmd_3D_MTX_LOAD_4x3    // 0x0400045C  :
				if (proc == ARMCPU_ARM9) gl_MTX_LOAD_4x3(val);
				return;
			case cmd_3D_MTX_MULT_4x4    // 0x04000460  :
				if (proc == ARMCPU_ARM9) gl_MTX_MULT_4x4(val);
				return;
			case cmd_3D_MTX_MULT_4x3    // 0x04000464  :
				if (proc == ARMCPU_ARM9) gl_MTX_MULT_4x3(val);
				return;
			case cmd_3D_MTX_MULT_3x3    // 0x04000468  :
				if (proc == ARMCPU_ARM9) gl_MTX_MULT_3x3(val);
				return;
			case cmd_3D_MTX_SCALE       // 0x0400046C  :
			case cmd_3D_MTX_TRANS       // 0x04000470  :
			case cmd_3D_COLOR           // 0x04000480  :
			case cmd_3D_NORMA           // 0x04000484  :
				if (proc == ARMCPU_ARM9) gl_print_cmd(adr);
				return;
			case cmd_3D_TEXCOORD        // 0x04000488  :
				if (proc == ARMCPU_ARM9) gl_TEXCOORD(val);
				return;
			case cmd_3D_VTX_16          // 0x0400048C  :
				if (proc == ARMCPU_ARM9) gl_VTX_16(val);
				return;
			case cmd_3D_VTX_10          // 0x04000490  :
				if (proc == ARMCPU_ARM9) gl_VTX_10(val);
				return;
			case cmd_3D_VTX_XY          // 0x04000494  :
				if (proc == ARMCPU_ARM9) gl_VTX_XY(val);
				return;
			case cmd_3D_VTX_XZ          // 0x04000498  :
				if (proc == ARMCPU_ARM9) gl_VTX_XZ(val);
				return;
			case cmd_3D_VTX_YZ          // 0x0400049C  :
				if (proc == ARMCPU_ARM9) gl_VTX_YZ(val);
				return;
			case cmd_3D_VTX_DIFF        // 0x040004A0  :
				if (proc == ARMCPU_ARM9) gl_VTX_DIFF(val);
				return;
			case cmd_3D_POLYGON_ATTR    // 0x040004A4  :
			case cmd_3D_TEXIMAGE_PARAM  // 0x040004A8  :
			case cmd_3D_PLTT_BASE       // 0x040004AC  :
			case cmd_3D_DIF_AMB         // 0x040004C0  :
			case cmd_3D_SPE_EMI         // 0x040004C4  :
			case cmd_3D_LIGHT_VECTOR    // 0x040004C8  :
			case cmd_3D_LIGHT_COLOR     // 0x040004CC  :
			case cmd_3D_SHININESS       // 0x040004D0  :
				if (proc == ARMCPU_ARM9) gl_print_cmd(adr);
				return;
			case cmd_3D_BEGIN_VTXS      // 0x04000500  :
				if (proc == ARMCPU_ARM9) gl_VTX_begin(val);
				return;
			case cmd_3D_END_VTXS        // 0x04000504  :
				if (proc == ARMCPU_ARM9) gl_VTX_end();
				return;
			case cmd_3D_SWAP_BUFFERS    // 0x04000540  :
			case cmd_3D_VIEWPORT        // 0x04000580  :
			case cmd_3D_BOX_TEST        // 0x040005C0  :
			case cmd_3D_POS_TEST        // 0x040005C4  :
			case cmd_3D_VEC_TEST        // 0x040005C8  :
				if (proc == ARMCPU_ARM9) gl_print_cmd(adr);
				return;
*/
                        case REG_DISPA_DISPCNT :
								if(proc == ARMCPU_ARM9) GPU_setVideoProp(MainScreen.gpu, val);
				
				//GPULOG("MAIN INIT 32B %08X\r\n", val);
				T1WriteLong(MMU.MMU_MEM[proc][0x40], 0, val);
				return;
				
                        case REG_DISPB_DISPCNT : 
				if (proc == ARMCPU_ARM9) GPU_setVideoProp(SubScreen.gpu, val);
				//GPULOG("SUB INIT 32B %08X\r\n", val);
				T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x1000, val);
				return;
			case REG_VRAMCNTA:
			case REG_VRAMCNTE:
				MMU_write8(proc,adr,val & 0xFF) ;
				MMU_write8(proc,adr+1,val >> 8) ;
				MMU_write8(proc,adr+2,val >> 16) ;
				MMU_write8(proc,adr+3,val >> 24) ;
				return ;
			case REG_VRAMCNTI:
				MMU_write8(proc,adr,val & 0xFF) ;
				return ;

                        case REG_IME : {
			        u32 old_val = MMU.reg_IME[proc];
				u32 new_val = val & 1;
				MMU.reg_IME[proc] = new_val;
				T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x208, val);
				if ( new_val && old_val != new_val) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( MMU.reg_IE[proc] & MMU.reg_IF[proc]) {
				    NDS_ARM7.wIRQ = TRUE;
				    NDS_ARM7.waitIRQ = FALSE;
				  }
				}
				return;
			}
				
			case REG_IE :
				MMU.reg_IE[proc] = val;
				if ( MMU.reg_IME[proc]) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( MMU.reg_IE[proc] & MMU.reg_IF[proc]) {
				    NDS_ARM7.wIRQ = TRUE;
				    NDS_ARM7.waitIRQ = FALSE;
				  }
				}
				return;
			
			case REG_IF :
				MMU.reg_IF[proc] &= (~val); 
				return;
                        case REG_TM0CNTL :
                        case REG_TM1CNTL :
                        case REG_TM2CNTL :
                        case REG_TM3CNTL :
{
				int timerIndex = (adr>>2)&0x3;
				int mask = ((val & 0x800000)>>(16+7)) << timerIndex;
				MMU.CheckTimers = (MMU.CheckTimers & (~mask)) | mask;

				MMU.timerReload[proc][timerIndex] = (u16)val;
				if(val&0x800000)
					MMU.timer[proc][timerIndex] = MMU.timerReload[proc][(adr>>2)&0x3];

				MMU.timerON[proc][timerIndex] = val & 0x800000;
				switch((val>>16)&7)
				{
					case 0 :
					MMU.timerMODE[proc][timerIndex] = 0+1;//ARMCPU_ARM9;
					break;
					case 1 :
					MMU.timerMODE[proc][timerIndex] = 6+1;//ARMCPU_ARM9;
					break;
					case 2 :
					MMU.timerMODE[proc][timerIndex] = 8+1;//ARMCPU_ARM9;
					break;
					case 3 :
					MMU.timerMODE[proc][timerIndex] = 10+1;//ARMCPU_ARM9;
					break;
					default :
					MMU.timerMODE[proc][timerIndex] = 0xFFFF;
					break;
				}
				if(!(val & 0x800000))
					MMU.timerRUN[proc][timerIndex] = FALSE;

				T1WriteLong(MMU.MMU_MEM[proc][0x40], adr & 0xFFF, val);
				return;
			}
			     case REG_DIVDENOM :
				{
					T1WriteLong(MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x298, val);
					execdiv(proc);
					return;
				}
			    case REG_DIVDENOM+4 :
				{
					T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x29C, val);
					execdiv(proc);
					return;
				}

			case REG_SQRTPARAM :
			{
				T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x2B8, val);
				execsqrt(proc);
				return;
			}
			case REG_SQRTPARAM+4 :
			{
				T1WriteLong(MMU.MMU_MEM[proc][0x40], 0x2BC, val);
				execsqrt(proc);
				return;
			}
                 case REG_IPCSYNC :
							MMU_IPCSync(proc, val);
				return;
                 		case REG_IPCFIFOSEND :
				            IPC_FIFOsend(proc, val);
				return;
			case REG_DMA0CNTL :
				//LOG("32 bit dma0 %04X\r\n", val);
				DMASrc[proc][0] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xB0);
				DMADst[proc][0] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xB4);
				MMU.DMAStartTime[proc][0] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				MMU.DMACrt[proc][0] = val;
				T1WriteLong(MMU.MMU_MEM[proc][0x40], 0xB8, val);
				if( MMU.DMAStartTime[proc][0] == 0 ||
					MMU.DMAStartTime[proc][0] == 7)		// Start Immediately
					MMU_doDMA(proc, 0);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 0, DMASrc[proc][0], DMADst[proc][0], 0, ((MMU.DMACrt[proc][0]>>27)&7));
				}
				#endif
				//execute = FALSE;
				return;
			case REG_DMA1CNTL:
				//LOG("32 bit dma1 %04X\r\n", val);
				DMASrc[proc][1] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xBC);
				DMADst[proc][1] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xC0);
				MMU.DMAStartTime[proc][1] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				MMU.DMACrt[proc][1] = val;
				T1WriteLong(MMU.MMU_MEM[proc][0x40], 0xC4, val);
				if(MMU.DMAStartTime[proc][1] == 0 ||
					MMU.DMAStartTime[proc][1] == 7)		// Start Immediately
					MMU_doDMA(proc, 1);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 1, DMASrc[proc][1], DMADst[proc][1], 0, ((MMU.DMACrt[proc][1]>>27)&7));
				}
				#endif
				return;
			case REG_DMA2CNTL :
				//LOG("32 bit dma2 %04X\r\n", val);
				DMASrc[proc][2] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xC8);
				DMADst[proc][2] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xCC);
				MMU.DMAStartTime[proc][2] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				MMU.DMACrt[proc][2] = val;
				T1WriteLong(MMU.MMU_MEM[proc][0x40], 0xD0, val);
				if(MMU.DMAStartTime[proc][2] == 0 ||
					MMU.DMAStartTime[proc][2] == 7)		// Start Immediately
					MMU_doDMA(proc, 2);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 2, DMASrc[proc][2], DMADst[proc][2], 0, ((MMU.DMACrt[proc][2]>>27)&7));
				}
				#endif
				return;
			case 0x040000DC :
				//LOG("32 bit dma3 %04X\r\n", val);
				DMASrc[proc][3] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xD4);
				DMADst[proc][3] = T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xD8);
				MMU.DMAStartTime[proc][3] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				MMU.DMACrt[proc][3] = val;
				T1WriteLong(MMU.MMU_MEM[proc][0x40], 0xDC, val);
				if(	MMU.DMAStartTime[proc][3] == 0 ||
					MMU.DMAStartTime[proc][3] == 7)		// Start Immediately
					MMU_doDMA(proc, 3);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 3, DMASrc[proc][3], DMADst[proc][3], 0, ((MMU.DMACrt[proc][3]>>27)&7));
				}
				#endif
				return;
                case REG_GCROMCTRL :
				{
					int i;

                    if(MEM_8(MMU.MMU_MEM[proc], REG_GCCMDOUT) == 0xB7)
					{
                        MMU.dscard[proc].adress = (MEM_8(MMU.MMU_MEM[proc], REG_GCCMDOUT+1) << 24) | (MEM_8(MMU.MMU_MEM[proc], REG_GCCMDOUT+2) << 16) | (MEM_8(MMU.MMU_MEM[proc], REG_GCCMDOUT+3) << 8) | (MEM_8(MMU.MMU_MEM[proc], REG_GCCMDOUT+4));
						MMU.dscard[proc].transfer_count = 0x80;// * ((val>>24)&7));
					}
                     else if (MEM_8(MMU.MMU_MEM[proc], REG_GCCMDOUT) == 0xB8)
                     {
                            // Get ROM chip ID
                            val |= 0x800000; // Data-Word Status
                            T1WriteLong(MMU.MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
                            MMU.dscard[proc].adress = 0;
                    }
					else
					{
                     LOG("CARD command: %02X\n", MEM_8(MMU.MMU_MEM[proc], REG_GCCMDOUT));
					}
			
				    switch(MEM_8(MMU.MMU_MEM[ARMCPU_ARM9], REG_GCCMDOUT))
					{
						/* Dummy */
						case 0x9F:
							{
								MMU.dscard[ARMCPU_ARM9].adress = 0;
								MMU.dscard[ARMCPU_ARM9].transfer_count = 0x800;
							}
							break;

						/* Data read */
						case 0x00:
						case 0xB7:
							{
								MMU.dscard[ARMCPU_ARM9].adress = (MEM_8(MMU.MMU_MEM[ARMCPU_ARM9], REG_GCCMDOUT+1) << 24) | (MEM_8(MMU.MMU_MEM[ARMCPU_ARM9], REG_GCCMDOUT+2) << 16) | (MEM_8(MMU.MMU_MEM[ARMCPU_ARM9], REG_GCCMDOUT+3) << 8) | (MEM_8(MMU.MMU_MEM[ARMCPU_ARM9], REG_GCCMDOUT+4));
								MMU.dscard[ARMCPU_ARM9].transfer_count = 0x80;
							}
							break;

						/* Get ROM chip ID */
						case 0x90:
						case 0xB8:
							{
								MMU.dscard[ARMCPU_ARM9].adress = 0;
								MMU.dscard[ARMCPU_ARM9].transfer_count = 1;
							}
							break;

						default:
							{
								//LOG("CARD command: %02X\n", MEM_8(MMU.MMU_MEM[ARMCPU_ARM9], REG_GCCMDOUT));
								MMU.dscard[ARMCPU_ARM9].adress = 0;
								MMU.dscard[ARMCPU_ARM9].transfer_count = 0;
							}
							break;
					}

					if(MMU.dscard[ARMCPU_ARM9].transfer_count == 0)
					{
						val &= 0x7F7FFFFF;
						T1WriteLong(MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x1A4, val);
						return;
					}
					
                    val |= 0x00800000;
                    T1WriteLong(MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x1A4, val);
					//CARDLOG("%08X : %08X %08X\r\n", adr, val, adresse[proc]);
//                    val |= 0x00800000;
					
					if(MMU.dscard[proc].adress == 0)
					{
                        val &= ~0x80000000; 
                        T1WriteLong(MMU.MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
						return;
					}
                    T1WriteLong(MMU.MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
										
					/* launch DMA if start flag was set to "DS Cart" */
					
					if(MMU.DMAStartTime[ARMCPU_ARM9][0] == 5)
					{
						MMU_doDMA(ARMCPU_ARM9, 0);
						return;
					}
					if(MMU.DMAStartTime[ARMCPU_ARM9][1] == 5)
					{
						MMU_doDMA(ARMCPU_ARM9, 1);
						return;
					}
					if(MMU.DMAStartTime[ARMCPU_ARM9][2] == 5)
					{
						MMU_doDMA(ARMCPU_ARM9, 2);
						return;
					}
					if(MMU.DMAStartTime[ARMCPU_ARM9][3] == 5)
					{
						MMU_doDMA(ARMCPU_ARM9, 3);
						return;
					}
				}
				return;					
										

			case REG_DISPA_DISPCAPCNT :
				//INFO("MMU write32: REG_DISPA_DISPCAPCNT 0x%X\n", val);
				GPU_set_DISPCAPCNT(MainScreen.gpu,val);
				T1WriteLong(ARM9Mem.ARM9_REG, 0x64, val);
				return;
				
			case REG_DISPA_BG0CNT :
				GPU_setBGProp(MainScreen.gpu, 0, (val&0xFFFF));
				GPU_setBGProp(MainScreen.gpu, 1, (val>>16));
				//if((val>>16)==0x400) emu_halt();
				T1WriteLong(ARM9Mem.ARM9_REG, 8, val);
				return;
			case REG_DISPA_BG2CNT :
					GPU_setBGProp(MainScreen.gpu, 2, (val&0xFFFF));
					GPU_setBGProp(MainScreen.gpu, 3, (val>>16));
					T1WriteLong(ARM9Mem.ARM9_REG, 0xC, val);
				return;
			case REG_DISPB_BG0CNT :
					GPU_setBGProp(SubScreen.gpu, 0, (val&0xFFFF));
					GPU_setBGProp(SubScreen.gpu, 1, (val>>16));
					T1WriteLong(ARM9Mem.ARM9_REG, 0x1008, val);
				return;
			case REG_DISPB_BG2CNT :
					GPU_setBGProp(SubScreen.gpu, 2, (val&0xFFFF));
					GPU_setBGProp(SubScreen.gpu, 3, (val>>16));
					T1WriteLong(ARM9Mem.ARM9_REG, 0x100C, val);
				return;
			case REG_DISPA_DISPMMEMFIFO:
			{
				// NOTE: right now, the capture unit is not taken into account,
				//       I don't know is it should be handled here or 
			
				//FIFOAdd(MMU.fifos + MAIN_MEMORY_DISP_FIFO, val);
			//	break;
			}
			//case 0x21FDFF0 :  if(val==0) execute = FALSE;
			//case 0x21FDFB0 :  if(val==0) execute = FALSE;
			default :
				T1WriteLong(MMU.MMU_MEM[proc][0x40], adr & MMU.MMU_MASK[proc][(adr>>20)&0xFF], val);
				return;
		}
	}
	T1WriteLong(MMU.MMU_MEM[proc][(adr>>20)&0xFF], adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF], val);
}


void FASTCALL MMU_doDMA(u32 proc, u32 num)
{
	u32 src = DMASrc[proc][num];
	u32 dst = DMADst[proc][num];
        u32 taille;

	if(src==dst)
	{
		T1WriteLong(MMU.MMU_MEM[proc][0x40], 0xB8 + (0xC*num), T1ReadLong(MMU.MMU_MEM[proc][0x40], 0xB8 + (0xC*num)) & 0x7FFFFFFF);
		return;
	}
	
	if((!(MMU.DMACrt[proc][num]&(1<<31)))&&(!(MMU.DMACrt[proc][num]&(1<<25))))
	{       /* not enabled and not to be repeated */
		MMU.DMAStartTime[proc][num] = 0;
		MMU.DMACycle[proc][num] = 0;
		//MMU.DMAing[proc][num] = FALSE;
		return;
	}
	
	
	/* word count */
	taille = (MMU.DMACrt[proc][num]&0xFFFF);
	
	// If we are in "Main memory display" mode just copy an entire 
	// screen (256x192 pixels). 
	//    Reference:  http://nocash.emubase.de/gbatek.htm#dsvideocaptureandmainmemorydisplaymode
	//       (under DISP_MMEM_FIFO)
	if ((MMU.DMAStartTime[proc][num]==4) &&		// Must be in main memory display mode
		(taille==4) &&							// Word must be 4
		(((MMU.DMACrt[proc][num]>>26)&1) == 1))	// Transfer mode must be 32bit wide
		taille = 24576; //256*192/2;
	
	if(MMU.DMAStartTime[proc][num] == 5)
		taille *= 0x80;
	
	MMU.DMACycle[proc][num] = taille + nds.cycles;
	MMU.DMAing[proc][num] = TRUE;
	MMU.CheckDMAs |= (1<<(num+(proc<<2)));
	
	DMALOG("proc %d, dma %d src %08X dst %08X start %d taille %d repeat %s %08X\r\n",
		proc, num, src, dst, MMU.DMAStartTime[proc][num], taille,
		(MMU.DMACrt[proc][num]&(1<<25))?"on":"off",MMU.DMACrt[proc][num]);
	
	if(!(MMU.DMACrt[proc][num]&(1<<25)))
		MMU.DMAStartTime[proc][num] = 0;
	
	// transfer
	{
		u32 i=0;
		// 32 bit or 16 bit transfer ?
		int sz = ((MMU.DMACrt[proc][num]>>26)&1)? 4 : 2; 
		int dstinc,srcinc;
		int u=(MMU.DMACrt[proc][num]>>21);
		switch(u & 0x3) {
			case 0 :  dstinc =  sz; break;
			case 1 :  dstinc = -sz; break;
			case 2 :  dstinc =   0; break;
			case 3 :  dstinc =  sz; break; //reload
			default:
				return;
		}
		switch((u >> 2)&0x3) {
			case 0 :  srcinc =  sz; break;
			case 1 :  srcinc = -sz; break;
			case 2 :  srcinc =   0; break;
			case 3 :  // reserved
				return;
			default:
				return;
		}

		#define DMA_LOOP(PROC) \
		if ((MMU.DMACrt[proc][num]>>26)&1) \
			for(; i < taille; ++i) \
			{ \
				MMU_writeWord(proc, dst, MMU_readWord(proc, src)); \
				dst += dstinc; \
				src += srcinc; \
			} \
		else \
			for(; i < taille; ++i) \
			{ \
				MMU_write16(proc, dst, MMU_readHWord(proc, src)); \
				dst += dstinc; \
				src += srcinc; \
			} \

		if(proc == ARMCPU_ARM9) { DMA_LOOP(ARMCPU_ARM9); }
		else  { DMA_LOOP(ARMCPU_ARM7); }
		

		//write back the addresses
		DMASrc[proc][num] = src;
		if((u & 0x3)!=3) //but dont write back dst if we were supposed to reload
			DMADst[proc][num] = dst;
	}
}

#ifdef MMU_ENABLE_ACL

INLINE void check_access(u32 adr, u32 access) {
	/* every other mode: sys */
	access |= 1;
	if ((NDS_ARM9.CPSR.val & 0x1F) == 0x10) {
		/* is user mode access */
		access ^= 1 ;
	}
	if (armcp15_isAccessAllowed((armcp15_t *)NDS_ARM9.coproc[15],adr,access)==FALSE) {
		execute = FALSE ;
	}
}
INLINE void check_access_write(u32 adr) {
	u32 access = CP15_ACCESS_WRITE;
	check_access(adr, access)
}

u8 FASTCALL MMU_read8_acl(u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(u32 adr, u32 access);
	return MMU_read8(proc,adr);
}
u16 FASTCALL MMU_read16_acl(u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(u32 adr, u32 access);
	return MMU_read16(proc,adr);
}
u32 FASTCALL MMU_read32_acl(u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(u32 adr, u32 access);
	return MMU_read32(proc,adr);
}

void FASTCALL MMU_write8_acl(u32 proc, u32 adr, u8 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(adr);
	MMU_write8(proc,adr,val);
}
void FASTCALL MMU_write16_acl(u32 proc, u32 adr, u16 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(adr);
	MMU_write16(proc,adr,val) ;
}
void FASTCALL MMU_write32_acl(u32 proc, u32 adr, u32 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(adr);
	MMU_write32(proc,adr,val) ;
}
#endif



#ifdef PROFILE_MEMORY_ACCESS

#define PROFILE_PREFETCH 0
#define PROFILE_READ 1
#define PROFILE_WRITE 2

struct mem_access_profile {
  u64 num_accesses;
  u32 address_mask;
  u32 masked_value;
};

#define PROFILE_NUM_MEM_ACCESS_PROFILES 4

static u64 profile_num_accesses[2][3];
static u64 profile_unknown_addresses[2][3];
static struct mem_access_profile
profile_memory_accesses[2][3][PROFILE_NUM_MEM_ACCESS_PROFILES];

static void
setup_profiling( void) {
  int i;

  for ( i = 0; i < 2; i++) {
    int access_type;

    for ( access_type = 0; access_type < 3; access_type++) {
      profile_num_accesses[i][access_type] = 0;
      profile_unknown_addresses[i][access_type] = 0;

      /*
       * Setup the access testing structures
       */
      profile_memory_accesses[i][access_type][0].address_mask = 0x0e000000;
      profile_memory_accesses[i][access_type][0].masked_value = 0x00000000;
      profile_memory_accesses[i][access_type][0].num_accesses = 0;

      /* main memory */
      profile_memory_accesses[i][access_type][1].address_mask = 0x0f000000;
      profile_memory_accesses[i][access_type][1].masked_value = 0x02000000;
      profile_memory_accesses[i][access_type][1].num_accesses = 0;

      /* shared memory */
      profile_memory_accesses[i][access_type][2].address_mask = 0x0f800000;
      profile_memory_accesses[i][access_type][2].masked_value = 0x03000000;
      profile_memory_accesses[i][access_type][2].num_accesses = 0;

      /* arm7 memory */
      profile_memory_accesses[i][access_type][3].address_mask = 0x0f800000;
      profile_memory_accesses[i][access_type][3].masked_value = 0x03800000;
      profile_memory_accesses[i][access_type][3].num_accesses = 0;
    }
  }
}

static void
profile_memory_access( int arm9, u32 adr, int access_type) {
  static int first = 1;
  int mem_profile;
  int address_found = 0;

  if ( first) {
    setup_profiling();
    first = 0;
  }

  profile_num_accesses[arm9][access_type] += 1;

  for ( mem_profile = 0;
        mem_profile < PROFILE_NUM_MEM_ACCESS_PROFILES &&
          !address_found;
        mem_profile++) {
    if ( (adr & profile_memory_accesses[arm9][access_type][mem_profile].address_mask) ==
         profile_memory_accesses[arm9][access_type][mem_profile].masked_value) {
      /*printf( "adr %08x mask %08x res %08x expected %08x\n",
              adr,
              profile_memory_accesses[arm9][access_type][mem_profile].address_mask,
              adr & profile_memory_accesses[arm9][access_type][mem_profile].address_mask,
              profile_memory_accesses[arm9][access_type][mem_profile].masked_value);*/
      address_found = 1;
      profile_memory_accesses[arm9][access_type][mem_profile].num_accesses += 1;
    }
  }

  if ( !address_found) {
    profile_unknown_addresses[arm9][access_type] += 1;
  }
}


static const char *access_type_strings[] = {
  "prefetch",
  "read    ",
  "write   "
};

void
print_memory_profiling( void) {
  int arm;

  printf("------ Memory access profile ------\n");

  for ( arm = 0; arm < 2; arm++) {
    int access_type;

    for ( access_type = 0; access_type < 3; access_type++) {
      int mem_profile;
      printf("ARM%c: num of %s %lld\n",
             arm ? '9' : '7',
             access_type_strings[access_type],
             profile_num_accesses[arm][access_type]);

      for ( mem_profile = 0;
            mem_profile < PROFILE_NUM_MEM_ACCESS_PROFILES;
            mem_profile++) {
        printf( "address %08x: %lld\n",
                profile_memory_accesses[arm][access_type][mem_profile].masked_value,
                profile_memory_accesses[arm][access_type][mem_profile].num_accesses);
      }
              
      printf( "unknown addresses %lld\n",
              profile_unknown_addresses[arm][access_type]);

      printf( "\n");
    }
  }

  printf("------ End of Memory access profile ------\n\n");
}
#else
void
print_memory_profiling( void) {
}
#endif /* End of PROFILE_MEMORY_ACCESS area */

static u16 FASTCALL
arm9_prefetch16( void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_PREFETCH);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == MMU.DTCMRegion)
    {
      /* Returns data from DTCM (ARM9 only) */
      return T1ReadWord(ARM9Mem.ARM9_DTCM, adr & 0x3FFF);
    }
  /* access to main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    return T1ReadWord( MMU.MMU_MEM[ARMCPU_ARM9][(adr >> 20) & 0xFF],
                       adr & MMU.MMU_MASK[ARMCPU_ARM9][(adr >> 20) & 0xFF]);
  }
#endif

  return MMU_read16( ARMCPU_ARM9, adr);
}
static u32 FASTCALL
arm9_prefetch32( void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_PREFETCH);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == MMU.DTCMRegion)
    {
      /* Returns data from DTCM (ARM9 only) */
      return T1ReadLong(ARM9Mem.ARM9_DTCM, adr & 0x3FFF);
    }
  /* access to main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    return T1ReadLong( MMU.MMU_MEM[ARMCPU_ARM9][(adr >> 20) & 0xFF],
                       adr & MMU.MMU_MASK[ARMCPU_ARM9][(adr >> 20) & 0xFF]);
  }
#endif

  return MMU_read32( ARMCPU_ARM9, adr);
}

static u8 FASTCALL
arm9_read8( void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_READ);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if( (adr&(~0x3FFF)) == MMU.DTCMRegion)
    {
      return ARM9Mem.ARM9_DTCM[adr&0x3FFF];
    }
  /* access to main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    return MMU.MMU_MEM[ARMCPU_ARM9][(adr >> 20) & 0xFF]
      [adr & MMU.MMU_MASK[ARMCPU_ARM9][(adr >> 20) & 0xFF]];
  }
#endif

  return MMU_read8( ARMCPU_ARM9, adr);
}
static u16 FASTCALL
arm9_read16( void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_READ);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == MMU.DTCMRegion)
    {
      /* Returns data from DTCM (ARM9 only) */
      return T1ReadWord(ARM9Mem.ARM9_DTCM, adr & 0x3FFF);
    }

  /* access to main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    return T1ReadWord( MMU.MMU_MEM[ARMCPU_ARM9][(adr >> 20) & 0xFF],
                       adr & MMU.MMU_MASK[ARMCPU_ARM9][(adr >> 20) & 0xFF]);
  }
#endif

  return MMU_read16( ARMCPU_ARM9, adr);
}
static u32 FASTCALL
arm9_read32( void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_READ);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == MMU.DTCMRegion)
    {
      /* Returns data from DTCM (ARM9 only) */
      return T1ReadLong(ARM9Mem.ARM9_DTCM, adr & 0x3FFF);
    }
  /* access to main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    return T1ReadLong( MMU.MMU_MEM[ARMCPU_ARM9][(adr >> 20) & 0xFF],
                       adr & MMU.MMU_MASK[ARMCPU_ARM9][(adr >> 20) & 0xFF]);
  }
#endif

  return MMU_read32( ARMCPU_ARM9, adr);
}


static void FASTCALL
arm9_write8(void *data, u32 adr, u8 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_WRITE);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if( (adr & ~0x3FFF) == MMU.DTCMRegion)
    {
      /* Writes data in DTCM (ARM9 only) */
      ARM9Mem.ARM9_DTCM[adr&0x3FFF] = val;
      return ;
    }
  /* main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    MMU.MMU_MEM[ARMCPU_ARM9][(adr>>20)&0xFF]
      [adr&MMU.MMU_MASK[ARMCPU_ARM9][(adr>>20)&0xFF]] = val;
    return;
  }
#endif

  MMU_write8( ARMCPU_ARM9, adr, val);
}
static void FASTCALL
arm9_write16(void *data, u32 adr, u16 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_WRITE);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == MMU.DTCMRegion)
    {
      /* Writes in DTCM (ARM9 only) */
      T1WriteWord(ARM9Mem.ARM9_DTCM, adr & 0x3FFF, val);
      return;
    }
  /* main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    T1WriteWord( MMU.MMU_MEM[ARMCPU_ARM9][(adr>>20)&0xFF],
                 adr&MMU.MMU_MASK[ARMCPU_ARM9][(adr>>20)&0xFF], val);
    return;
  }
#endif

  MMU_write16( ARMCPU_ARM9, adr, val);
}
static void FASTCALL
arm9_write32(void *data, u32 adr, u32 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_WRITE);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == MMU.DTCMRegion)
    {
      /* Writes in DTCM (ARM9 only) */
      T1WriteLong(ARM9Mem.ARM9_DTCM, adr & 0x3FFF, val);
      return;
    }
  /* main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    T1WriteLong( MMU.MMU_MEM[ARMCPU_ARM9][(adr>>20)&0xFF],
                 adr&MMU.MMU_MASK[ARMCPU_ARM9][(adr>>20)&0xFF], val);
    return;
  }
#endif

  MMU_write32( ARMCPU_ARM9, adr, val);
}




static u16 FASTCALL
arm7_prefetch16( void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_PREFETCH);
#endif

#ifdef EARLY_MEMORY_ACCESS
  /* ARM7 private memory */
  if ( (adr & 0x0f800000) == 0x03800000) {
    T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM7][(adr >> 20) & 0xFF],
               adr & MMU.MMU_MASK[ARMCPU_ARM7][(adr >> 20) & 0xFF]); 
  }
#endif

  return MMU_read16( ARMCPU_ARM7, adr);
}
static u32 FASTCALL
arm7_prefetch32( void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_PREFETCH);
#endif

#ifdef EARLY_MEMORY_ACCESS
  /* ARM7 private memory */
  if ( (adr & 0x0f800000) == 0x03800000) {
    T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM7][(adr >> 20) & 0xFF],
               adr & MMU.MMU_MASK[ARMCPU_ARM7][(adr >> 20) & 0xFF]); 
  }
#endif

  return MMU_read32( ARMCPU_ARM7, adr);
}

static u8 FASTCALL
arm7_read8( void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_READ);
#endif

  return MMU_read8( ARMCPU_ARM7, adr);
}
static u16 FASTCALL
arm7_read16( void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_READ);
#endif

  return MMU_read16( ARMCPU_ARM7, adr);
}
static u32 FASTCALL
arm7_read32( void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_READ);
#endif

  return MMU_read32( ARMCPU_ARM7, adr);
}


static void FASTCALL
arm7_write8(void *data, u32 adr, u8 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_WRITE);
#endif

  MMU_write8( ARMCPU_ARM7, adr, val);
}
static void FASTCALL
arm7_write16(void *data, u32 adr, u16 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_WRITE);
#endif

  MMU_write16( ARMCPU_ARM7, adr, val);
}
static void FASTCALL
arm7_write32(void *data, u32 adr, u32 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_WRITE);
#endif

  MMU_write32( ARMCPU_ARM7, adr, val);
}



/*
 * the base memory interfaces
 */
struct armcpu_memory_iface arm9_base_memory_iface = {
#ifdef __GNUC__
  .prefetch32 = arm9_prefetch32,
  .prefetch16 = arm9_prefetch16,

  .read8 = arm9_read8,
  .read16 = arm9_read16,
  .read32 = arm9_read32,

  .write8 = arm9_write8,
  .write16 = arm9_write16,
  .write32 = arm9_write32
#else
  arm9_prefetch32,
  arm9_prefetch16,

  arm9_read8,
  arm9_read16,
  arm9_read32,

  arm9_write8,
  arm9_write16,
  arm9_write32
#endif
};

struct armcpu_memory_iface arm7_base_memory_iface = {
#ifdef __GNUC__
  .prefetch32 = arm7_prefetch32,
  .prefetch16 = arm7_prefetch16,

  .read8 = arm7_read8,
  .read16 = arm7_read16,
  .read32 = arm7_read32,

  .write8 = arm7_write8,
  .write16 = arm7_write16,
  .write32 = arm7_write32
#else
  arm7_prefetch32,
  arm7_prefetch16,

  arm7_read8,
  arm7_read16,
  arm7_read32,

  arm7_write8,
  arm7_write16,
  arm7_write32
#endif
};

/*
 * The direct memory interface for the ARM9.
 * This avoids the ARM9 protection unit when accessing
 * memory.
 */
struct armcpu_memory_iface arm9_direct_memory_iface = {
#ifdef __GNUC__
  /* the prefetch is not used */
  .prefetch32 = NULL,
  .prefetch16 = NULL,

  .read8 = arm9_read8,
  .read16 = arm9_read16,
  .read32 = arm9_read32,

  .write8 = arm9_write8,
  .write16 = arm9_write16,
  .write32 = arm9_write32
#else
  NULL,
  NULL,

  arm9_read8,
  arm9_read16,
  arm9_read32,

  arm9_write8,
  arm9_write16,
  arm9_write32
#endif
};

void mmu_select_savetype(int type, int *bmemtype, u32 *bmemsize) {
        if (type<0 || type > 6)
			return;

        *bmemtype=save_types[type][0];
        *bmemsize=save_types[type][1];
        
		mc_realloc(&MMU.bupmem, *bmemtype, *bmemsize);
}
