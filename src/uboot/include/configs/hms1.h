/*
 * (C) Copyright 2004 STMicroelectronics.
 *
 * Andy Sturges <andy.sturges@st.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * High Level Configuration Options
 * (easy to change)
 */

#define CONFIG_SH4    1	              /* This is an SH4 CPU	        */
#define CONFIG_CPU_SUBTYPE_SH4_2XX    /* its an SH4-202                 */

#define INPUT_CLOCK_RATE 27

#define P_CLOCK_RATE     66000000     /* clock rate for CSP             */

/*-----------------------------------------------------------------------
 * Start addresses for the final memory configuration
 * Assume we run out of uncached memory for the moment
 */

#define CFG_SDRAM_BASE		0x84000000      /* SDRAM in P1 region         */
#define CFG_SDRAM_SIZE		0x08000000
#define CFG_FLASH_BASE		0xA0000000
#define CFG_RESET_ADDRESS	0xA0000000

#define CFG_MONITOR_LEN		0x00040000	/* Reserve 256 kB for Monitor */
#define CFG_MONITOR_BASE        0xA0000000
#define CFG_MALLOC_LEN		(1 << 20)	/* Reserve 1MB kB for malloc */
#define CFG_BOOTPARAMS_LEN	(128 << 10)
#define CFG_GBL_DATA_SIZE	1024		/* Global data structures */

#define CFG_MEMTEST_START   	CFG_SDRAM_BASE
#define CFG_MEMTEST_END     	(CFG_SDRAM_BASE + CFG_SDRAM_SIZE - (2 << 20))

#define CONFIG_BAUDRATE		115200
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

#define XSTR(s) STR(s)
#define STR(s) #s

#define BOARD stb7100mboard

#define	CONFIG_EXTRA_ENV_SETTINGS "\0"


#if 0
		"board=" XSTR(BOARD) "\0" \
		"monitor_base=" XSTR(CFG_MONITOR_BASE) "\0" \
		"monitor_len=" XSTR(CFG_MONITOR_LEN) "\0" \
		"monitor_sec=1:0\0" \
		"load_addr=" XSTR(CFG_LOAD_ADDR) "\0" \
		"unprot=" \
		  "protect off $monitor_sec\0" \
		"update=" \
		  "erase $monitor_sec;" \
		  "cp.b $load_addr $monitor_base $monitor_len;" \
		  "protect on $monitor_sec\0"
#endif

#define CONFIG_COMMANDS	(CONFIG_CMD_DFL | \
			 CFG_CMD_ASKENV  | \
			 CFG_CMD_NFS | CFG_CMD_PING | CFG_CMD_DHCP |\
			 CFG_CMD_IDE | CFG_CMD_EXT2 | \
			 CFG_CMD_JFFS2)

/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#include <cmd_confdefs.h>

/*--------------------------------------------------------------
 * Serial console info
 */

#define CFG_SH_ASC_SERIAL 1

#define CONFIG_CONS_INDEX 0
#define CONFIG_SH_ASC_BASE 0xb8032000ul /* UART2 */

/*---------------------------------------------------------------
 * Ethernet driver config
 */

/*
 * There are 2 options for ethernet:
 *    The onboard SMC91111
 *    The DB641 STEM card - this has two ethernet devices Port0 and Port1
 */

/* Config for SMC91111 */

#if 0
#define CONFIG_DRIVER_SMC91111 1
#define	CONFIG_SMC91111_BASE	0xa3e00300ul
#endif

/* Config for SMC9118 STEM card */

#if 1
#define CONFIG_DRIVER_SMC911X
#if 1
/* PORT 0 */
#define	CONFIG_SMC911X_BASE	0xA1000000ul
#else
/* PORT 1 */
#define	CONFIG_SMC911X_BASE	0xA1800000ul
#endif
#endif

/*  If this board does not have eeprom for ethernet address so allow the user
 *  to set it in the environment
 */

/* #define CONFIG_ENV_OVERWRITE */


/*---------------------------------------------------------------
 * IDE driver config
 */

/* Choose one of the the following */

#define CONFIG_SH_STB7100_IDE
/* #define CONFIG_SH_STB7100_SATA */

#ifdef CONFIG_SH_STB7100_IDE

#define	CFG_PIO_MODE	4

#define CFG_IDE_MAXBUS       1
#define CFG_IDE_MAXDEVICE    1

#define CFG_ATA_BASE_ADDR    0xA2800000
#define CFG_ATA_IDE0_OFFSET  0x00200000
#define CFG_ATA_REG_OFFSET   0
#define CFG_ATA_DATA_OFFSET  0
#define CFG_ATA_STRIDE       0x00020000
#define CFG_ATA_ALT_OFFSET   -0x0100000

#define CONFIG_DOS_PARTITION

#endif

#ifdef CONFIG_SH_STB7100_SATA

#define	CFG_PIO_MODE	4

#define CFG_IDE_MAXBUS       1
#define CFG_IDE_MAXDEVICE    1

#define CFG_ATA_BASE_ADDR    0xB9209800
#define CFG_ATA_IDE0_OFFSET  0x0
#define CFG_ATA_REG_OFFSET   0x0
#define CFG_ATA_DATA_OFFSET  0x0
#define CFG_ATA_STRIDE       0x4
#define CFG_ATA_ALT_OFFSET   0x8

// #define CFG_64BIT_VSPRINTF
#define CONFIG_LBA48
#define CONFIG_DOS_PARTITION

#endif

/*----------------------------------------------------------------------
 * jffs2 support
 */

#if (CONFIG_COMMANDS &  CFG_CMD_JFFS2)

#define CFG_JFFS_CUSTOM_PART
#define CFG_JFFS_SINGLE_PART	1

#define CFG_JFFS2_FIRST_SECTOR 18  /* u-boot, env, kernel  */
#define CFG_JFFS2_FIRST_BANK 0
#define CFG_JFFS2_NUM_BANKS 1

#endif

/*-----------------------------------------------------------------------
 * Miscellaneous configurable options
 */

#define CFG_HUSH_PARSER         1
#define	CFG_LONGHELP		1		/* undef to save memory		*/
#define CFG_PROMPT		"HMS1> "	/* Monitor Command Prompt	*/
#define CFG_PROMPT_HUSH_PS2     "> "
#define CFG_CBSIZE		1024
#define CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size	*/
#define CFG_MAXARGS		16		/* max number of command args	*/
#define CFG_HZ			(P_CLOCK_RATE/1024)		/* HZ for timer ticks		*/
#define CFG_LOAD_ADDR		CFG_SDRAM_BASE	/* default load address		*/
#define CFG_BOOTMAPSZ           (16 << 20)      /* initial linux memory size 	*/
#define CONFIG_BOOTDELAY	10		/* default delay before executing bootcmd */
#define CONFIG_ZERO_BOOTDELAY_CHECK

#define CONFIG_AUTO_COMPLETE
#define CONFIG_COMMAND_HISTORY
#define CONFIG_COMMAND_EDIT

/*-----------------------------------------------------------------------
 * FLASH organization
 */

/* stb7100 mboard organised as 8MB flash with 128k blocks */
#define CFG_FLASH_CFI_DRIVER
#define CFG_FLASH_CFI
#define CFG_FLASH_PROTECTION    1	/* use hardware flash protection        */
#define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks		*/
#define CFG_MAX_FLASH_SECT	64    	/* max number of sectors on one chip 	*/


/*-----------------------------------------------------------------------
 * NVRAM organization
 */

/* Address and size of Primary Environment Sector	*/

#define	CFG_ENV_IS_IN_FLASH	1
#define CFG_ENV_OFFSET          0x40000
#define CFG_ENV_ADDR            (CFG_FLASH_BASE + CFG_ENV_OFFSET)
#define CFG_ENV_SIZE            0x20000
#define CFG_ENV_SECT_SIZE       0x20000

#endif	/* __CONFIG_H */
