/* This file defines generic ELF types, structures, and macros.
   Copyright (C) 1999, 2000, 2001, 2002, 2004, 2005 Red Hat, Inc.
   This file is part of Red Hat elfutils.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   In addition, as a special exception, Red Hat, Inc. gives You the
   additional right to link the code of Red Hat elfutils with code licensed
   under any Open Source Initiative certified open source license
   (http://www.opensource.org/licenses/index.php) which requires the
   distribution of source code with any binary distribution and to
   distribute linked combinations of the two.  Non-GPL Code permitted under
   this exception must only link to the code of Red Hat elfutils through
   those well defined interfaces identified in the file named EXCEPTION
   found in the source code files (the "Approved Interfaces").  The files
   of Non-GPL Code may instantiate templates or use macros or inline
   functions from the Approved Interfaces without causing the resulting
   work to be covered by the GNU General Public License.  Only Red Hat,
   Inc. may make changes or additions to the list of Approved Interfaces.
   Red Hat's grant of this exception is conditioned upon your not adding
   any new exceptions.  If you wish to add a new Approved Interface or
   exception, please contact Red Hat.  You must obey the GNU General Public
   License in all respects for all of the Red Hat elfutils code and other
   code used in conjunction with Red Hat elfutils except the Non-GPL Code
   covered by this exception.  If you modify this file, you may extend this
   exception to your version of the file, but you are not obligated to do
   so.  If you do not wish to provide this exception without modification,
   you must delete this exception statement from your version and license
   this file solely under the GPL without exception.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#ifndef _GELF_H
#define	_GELF_H 1

#include <libelf.h>


#ifdef __cplusplus
extern "C" {
#endif

/* Class independent type definitions.  Correctly speaking this is not
   true.  We assume that 64-bit binaries are the largest class and
   therefore all other classes can be represented without loss.  */

/* Type for a 16-bit quantity.  */
typedef Elf64_Half GElf_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef Elf64_Word GElf_Word;
typedef	Elf64_Sword GElf_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef Elf64_Xword GElf_Xword;
typedef	Elf64_Sxword GElf_Sxword;

/* Type of addresses.  */
typedef Elf64_Addr GElf_Addr;

/* Type of file offsets.  */
typedef Elf64_Off GElf_Off;


/* The ELF file header.  This appears at the start of every ELF file.  */
typedef Elf64_Ehdr GElf_Ehdr;

/* Section header.  */
typedef Elf64_Shdr GElf_Shdr;

/* Section index.  */
/* XXX This should probably be a larger type in preparation of times when
   regular section indices can be larger.  */
typedef Elf64_Section GElf_Section;

/* Symbol table entry.  */
typedef Elf64_Sym GElf_Sym;

/* The syminfo section if available contains additional information about
   every dynamic symbol.  */
typedef Elf64_Syminfo GElf_Syminfo;

/* Relocation table entry without addend (in section of type SHT_REL).  */
typedef Elf64_Rel GElf_Rel;

/* Relocation table entry with addend (in section of type SHT_RELA).  */
typedef Elf64_Rela GElf_Rela;

/* Program segment header.  */
typedef Elf64_Phdr GElf_Phdr;

/* Dynamic section entry.  */
typedef Elf64_Dyn GElf_Dyn;


/* Version definition sections.  */
typedef Elf64_Verdef GElf_Verdef;

/* Auxialiary version information.  */
typedef Elf64_Verdaux GElf_Verdaux;

/* Version dependency section.  */
typedef Elf64_Verneed GElf_Verneed;

/* Auxiliary needed version information.  */
typedef Elf64_Vernaux GElf_Vernaux;


/* Type for version symbol information.  */
typedef Elf64_Versym GElf_Versym;


/* Auxiliary vector.  */
typedef Elf64_auxv_t GElf_auxv_t;


/* Note section contents.  */
typedef Elf64_Nhdr GElf_Nhdr;


/* Move structure.  */
typedef Elf64_Move GElf_Move;


/* Library list structure.  */
typedef Elf64_Lib GElf_Lib;


/* How to extract and insert information held in the st_info field.  */

#define GELF_ST_BIND(val)		ELF64_ST_BIND (val)
#define GELF_ST_TYPE(val)		ELF64_ST_TYPE (val)
#define GELF_ST_INFO(bind, type)	ELF64_ST_INFO (bind, type)

/* How to extract information held in the st_other field.  */

#define GELF_ST_VISIBILITY(val)		ELF64_ST_VISIBILITY (val)


/* How to extract and insert information held in the r_info field.  */

#define GELF_R_SYM(info)		ELF64_R_SYM (info)
#define GELF_R_TYPE(info)		ELF64_R_TYPE (info)
#define GELF_R_INFO(sym, type)		ELF64_R_INFO (sym, type)


/* How to extract and insert information held in the m_info field.  */
#define GELF_M_SYM(info)		ELF64_M_SYM (info)
#define GELF_M_SIZE(info)		ELF64_M_SIZE (info)
#define GELF_M_INFO(sym, size)		ELF64_M_INFO (sym, size)


/* Get class of the file associated with ELF.  */
extern int gelf_getclass (Elf *__elf);


/* Return size of array of COUNT elemeents of the type denoted by TYPE
   in the external representation.  The binary class is taken from ELF.
   The result is based on version VERSION of the ELF standard.  */
extern size_t gelf_fsize (Elf *__elf, Elf_Type __type, size_t __count,
			  unsigned int __version);

/* Retrieve object file header.  */
extern GElf_Ehdr *gelf_getehdr (Elf *__elf, GElf_Ehdr *__dest);

/* Update the ELF header.  */
extern int gelf_update_ehdr (Elf *__elf, GElf_Ehdr *__src);

/* Create new ELF header if none exists.  */
extern unsigned long int gelf_newehdr (Elf *__elf, int __class);

/* Get section at OFFSET.  */
extern Elf_Scn *gelf_offscn (Elf *__elf, GElf_Off __offset);

/* Retrieve section header.  */
extern GElf_Shdr *gelf_getshdr (Elf_Scn *__scn, GElf_Shdr *__dst);

/* Update section header.  */
extern int gelf_update_shdr (Elf_Scn *__scn, GElf_Shdr *__src);

/* Retrieve program header table entry.  */
extern GElf_Phdr *gelf_getphdr (Elf *__elf, int __ndx, GElf_Phdr *__dst);

/* Update the program header.  */
extern int gelf_update_phdr (Elf *__elf, int __ndx, GElf_Phdr *__src);

/* Create new program header with PHNUM entries.  */
extern unsigned long int gelf_newphdr (Elf *__elf, size_t __phnum);


/* Convert data structure from the representation in the file represented
   by ELF to their memory representation.  */
extern Elf_Data *gelf_xlatetom (Elf *__elf, Elf_Data *__dest,
				const Elf_Data *__src, unsigned int __encode);

/* Convert data structure from to the representation in memory
   represented by ELF file representation.  */
extern Elf_Data *gelf_xlatetof (Elf *__elf, Elf_Data *__dest,
				const Elf_Data *__src, unsigned int __encode);


/* Retrieve REL relocation info at the given index.  */
extern GElf_Rel *gelf_getrel (Elf_Data *__data, int __ndx, GElf_Rel *__dst);

/* Retrieve RELA relocation info at the given index.  */
extern GElf_Rela *gelf_getrela (Elf_Data *__data, int __ndx, GElf_Rela *__dst);

/* Update REL relocation information at given index.  */
extern int gelf_update_rel (Elf_Data *__dst, int __ndx, GElf_Rel *__src);

/* Update RELA relocation information at given index.  */
extern int gelf_update_rela (Elf_Data *__dst, int __ndx, GElf_Rela *__src);


/* Retrieve symbol information from the symbol table at the given index.  */
extern GElf_Sym *gelf_getsym (Elf_Data *__data, int __ndx, GElf_Sym *__dst);

/* Update symbol information in the symbol table at the given index.  */
extern int gelf_update_sym (Elf_Data *__data, int __ndx, GElf_Sym *__src);


/* Retrieve symbol information and separate section index from the
   symbol table at the given index.  */
extern GElf_Sym *gelf_getsymshndx (Elf_Data *__symdata, Elf_Data *__shndxdata,
				   int __ndx, GElf_Sym *__sym,
				   Elf32_Word *__xshndx);

/* Update symbol information and separate section index in the symbol
   table at the given index.  */
extern int gelf_update_symshndx (Elf_Data *__symdata, Elf_Data *__shndxdata,
				 int __ndx, GElf_Sym *__sym,
				 Elf32_Word __xshndx);


/* Retrieve additional symbol information from the symbol table at the
   given index.  */
extern GElf_Syminfo *gelf_getsyminfo (Elf_Data *__data, int __ndx,
				      GElf_Syminfo *__dst);

/* Update additional symbol information in the symbol table at the
   given index.  */
extern int gelf_update_syminfo (Elf_Data *__data, int __ndx,
				GElf_Syminfo *__src);


/* Get information from dynamic table at the given index.  */
extern GElf_Dyn *gelf_getdyn (Elf_Data *__data, int __ndx, GElf_Dyn *__dst);

/* Update information in dynamic table at the given index.  */
extern int gelf_update_dyn (Elf_Data *__dst, int __ndx, GElf_Dyn *__src);


/* Get move structure at the given index.  */
extern GElf_Move *gelf_getmove (Elf_Data *__data, int __ndx, GElf_Move *__dst);

/* Update move structure at the given index.  */
extern int gelf_update_move (Elf_Data *__data, int __ndx,
			     GElf_Move *__src);


/* Get library from table at the given index.  */
extern GElf_Lib *gelf_getlib (Elf_Data *__data, int __ndx, GElf_Lib *__dst);

/* Update library in table at the given index.  */
extern int gelf_update_lib (Elf_Data *__data, int __ndx, GElf_Lib *__src);



/* Retrieve symbol version information at given index.  */
extern GElf_Versym *gelf_getversym (Elf_Data *__data, int __ndx,
				    GElf_Versym *__dst);

/* Update symbol version information.  */
extern int gelf_update_versym (Elf_Data *__data, int __ndx,
			       GElf_Versym *__src);


/* Retrieve required symbol version information at given offset.  */
extern GElf_Verneed *gelf_getverneed (Elf_Data *__data, int __offset,
				      GElf_Verneed *__dst);

/* Update required symbol version information.  */
extern int gelf_update_verneed (Elf_Data *__data, int __offset,
				GElf_Verneed *__src);

/* Retrieve additional required symbol version information at given offset.  */
extern GElf_Vernaux *gelf_getvernaux (Elf_Data *__data, int __offset,
				      GElf_Vernaux *__dst);

/* Update additional required symbol version information.  */
extern int gelf_update_vernaux (Elf_Data *__data, int __offset,
				GElf_Vernaux *__src);


/* Retrieve symbol version definition information at given offset.  */
extern GElf_Verdef *gelf_getverdef (Elf_Data *__data, int __offset,
				    GElf_Verdef *__dst);

/* Update symbol version definition information.  */
extern int gelf_update_verdef (Elf_Data *__data, int __offset,
			       GElf_Verdef *__src);

/* Retrieve additional symbol version definition information at given
   offset.  */
extern GElf_Verdaux *gelf_getverdaux (Elf_Data *__data, int __offset,
				      GElf_Verdaux *__dst);

/* Update additional symbol version definition information.  */
extern int gelf_update_verdaux (Elf_Data *__data, int __offset,
				GElf_Verdaux *__src);


/* Retrieve uninterpreted chunk of the file contents.  */
extern char *gelf_rawchunk (Elf *__elf, GElf_Off __offset, GElf_Word __size);

/* Release uninterpreted chunk of the file contents.  */
extern void gelf_freechunk (Elf *__elf, char *__ptr);


/* Compute simple checksum from permanent parts of the ELF file.  */
extern long int gelf_checksum (Elf *__elf);

#ifdef __cplusplus
}
#endif

#endif	/* gelf.h */
