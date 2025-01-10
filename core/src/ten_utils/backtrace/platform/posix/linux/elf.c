//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "ten_utils/ten_config.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "include_internal/ten_utils/backtrace/platform/posix/config.h"

#ifdef HAVE_DL_ITERATE_PHDR
#ifdef HAVE_LINK_H
#define __USE_GNU
#include <link.h>
#undef __USE_GNU
#endif
#endif

#include "include_internal/ten_utils/backtrace/backtrace.h"
#include "include_internal/ten_utils/backtrace/platform/posix/internal.h"
#include "include_internal/ten_utils/backtrace/platform/posix/linux/crc32.h"
#include "include_internal/ten_utils/backtrace/platform/posix/linux/debugfile.h"
#include "include_internal/ten_utils/backtrace/platform/posix/linux/uncompress.h"
#include "include_internal/ten_utils/backtrace/platform/posix/linux/view.h"
#include "include_internal/ten_utils/backtrace/platform/posix/linux/zlib.h"
#include "ten_utils/io/mmap.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/atomic_ptr.h"
#include "ten_utils/lib/file.h"

#ifndef S_ISLNK
#ifndef S_IFLNK
#define S_IFLNK 0120000
#endif
#ifndef S_IFMT
#define S_IFMT 0170000
#endif
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif

#ifndef HAVE_DL_ITERATE_PHDR

/* Dummy version of dl_iterate_phdr for systems that don't have it.  */

#define dl_phdr_info x_dl_phdr_info
#define dl_iterate_phdr x_dl_iterate_phdr

struct dl_phdr_info {
  uintptr_t dlpi_addr;
  const char *dlpi_name;
};

static int dl_iterate_phdr(int (*callback)(struct dl_phdr_info *, size_t,
                                           void *),
                           void *data) {
  return 0;
}

#endif /* ! defined (HAVE_DL_ITERATE_PHDR) */

/* The configure script must tell us whether we are 32-bit or 64-bit
   ELF.  We could make this code test and support either possibility,
   but there is no point.  This code only works for the currently
   running executable, which means that we know the ELF mode at
   configure time.  */

#if BACKTRACE_ELF_SIZE != 32 && BACKTRACE_ELF_SIZE != 64
#error "Unknown BACKTRACE_ELF_SIZE"
#endif

/* <link.h> might #include <elf.h> which might define our constants
   with slightly different values.  Undefine them to be safe.  */

#undef EI_NIDENT
#undef EI_MAG0
#undef EI_MAG1
#undef EI_MAG2
#undef EI_MAG3
#undef EI_CLASS
#undef EI_DATA
#undef EI_VERSION
#undef ELF_MAG0
#undef ELF_MAG1
#undef ELF_MAG2
#undef ELF_MAG3
#undef ELFCLASS32
#undef ELFCLASS64
#undef ELFDATA2LSB
#undef ELFDATA2MSB
#undef EV_CURRENT
#undef ET_DYN
#undef EM_PPC64
#undef EF_PPC64_ABI
#undef SHN_LORESERVE
#undef SHN_XINDEX
#undef SHN_UNDEF
#undef SHT_PROGBITS
#undef SHT_SYMTAB
#undef SHT_STRTAB
#undef SHT_DYNSYM
#undef SHF_COMPRESSED
#undef STT_OBJECT
#undef STT_FUNC
#undef NT_GNU_BUILD_ID
#undef ELFCOMPRESS_ZLIB
#undef ELFCOMPRESS_ZSTD

/* Basic types.  */

typedef uint16_t b_elf_half; /* Elf_Half.  */
typedef uint32_t b_elf_word; /* Elf_Word.  */
typedef int32_t b_elf_sword; /* Elf_Sword.  */

#if BACKTRACE_ELF_SIZE == 32

typedef uint32_t b_elf_addr; /* Elf_Addr.  */
typedef uint32_t b_elf_off;  /* Elf_Off.  */

typedef uint32_t b_elf_wxword; /* 32-bit Elf_Word, 64-bit ELF_Xword.  */

#else

typedef uint64_t b_elf_addr;  /* Elf_Addr.  */
typedef uint64_t b_elf_off;   /* Elf_Off.  */
typedef uint64_t b_elf_xword; /* Elf_Xword.  */
typedef int64_t b_elf_sxword; /* Elf_Sxword.  */

typedef uint64_t b_elf_wxword; /* 32-bit Elf_Word, 64-bit ELF_Xword.  */

#endif

/* Data structures and associated constants.  */

#define EI_NIDENT 16

typedef struct {
  unsigned char e_ident[EI_NIDENT]; /* ELF "magic number" */
  b_elf_half e_type;                /* Identifies object file type */
  b_elf_half e_machine;             /* Specifies required architecture */
  b_elf_word e_version;             /* Identifies object file version */
  b_elf_addr e_entry;               /* Entry point virtual address */
  b_elf_off e_phoff;                /* Program header table file offset */
  b_elf_off e_shoff;                /* Section header table file offset */
  b_elf_word e_flags;               /* Processor-specific flags */
  b_elf_half e_ehsize;              /* ELF header size in bytes */
  b_elf_half e_phentsize;           /* Program header table entry size */
  b_elf_half e_phnum;               /* Program header table entry count */
  b_elf_half e_shentsize;           /* Section header table entry size */
  b_elf_half e_shnum;               /* Section header table entry count */
  b_elf_half e_shstrndx;            /* Section header string table index */
} b_elf_ehdr;                       /* Elf_Ehdr.  */

#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EV_CURRENT 1

#define ET_DYN 3

#define EM_PPC64 21
#define EF_PPC64_ABI 3

typedef struct {
  b_elf_word sh_name;        /* Section name, index in string tbl */
  b_elf_word sh_type;        /* Type of section */
  b_elf_wxword sh_flags;     /* Miscellaneous section attributes */
  b_elf_addr sh_addr;        /* Section virtual addr at execution */
  b_elf_off sh_offset;       /* Section file offset */
  b_elf_wxword sh_size;      /* Size of section in bytes */
  b_elf_word sh_link;        /* Index of another section */
  b_elf_word sh_info;        /* Additional section information */
  b_elf_wxword sh_addralign; /* Section alignment */
  b_elf_wxword sh_entsize;   /* Entry size if section holds table */
} b_elf_shdr;                /* Elf_Shdr.  */

#define SHN_UNDEF 0x0000     /* Undefined section */
#define SHN_LORESERVE 0xFF00 /* Begin range of reserved indices */
#define SHN_XINDEX 0xFFFF    /* Section index is held elsewhere */

#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_DYNSYM 11

#define SHF_COMPRESSED 0x800

#if BACKTRACE_ELF_SIZE == 32

typedef struct {
  b_elf_word st_name;     /* Symbol name, index in string tbl */
  b_elf_addr st_value;    /* Symbol value */
  b_elf_word st_size;     /* Symbol size */
  unsigned char st_info;  /* Symbol binding and type */
  unsigned char st_other; /* Visibility and other data */
  b_elf_half st_shndx;    /* Symbol section index */
} b_elf_sym;              /* Elf_Sym.  */

#else /* BACKTRACE_ELF_SIZE != 32 */

typedef struct {
  b_elf_word st_name;     /* Symbol name, index in string tbl */
  unsigned char st_info;  /* Symbol binding and type */
  unsigned char st_other; /* Visibility and other data */
  b_elf_half st_shndx;    /* Symbol section index */
  b_elf_addr st_value;    /* Symbol value */
  b_elf_xword st_size;    /* Symbol size */
} b_elf_sym;              /* Elf_Sym.  */

#endif /* BACKTRACE_ELF_SIZE != 32 */

#define STT_OBJECT 1
#define STT_FUNC 2

typedef struct {
  uint32_t namesz;
  uint32_t descsz;
  uint32_t type;
  char name[1];
} b_elf_note;

#define NT_GNU_BUILD_ID 3

#if BACKTRACE_ELF_SIZE == 32

typedef struct {
  b_elf_word ch_type;      /* Compresstion algorithm */
  b_elf_word ch_size;      /* Uncompressed size */
  b_elf_word ch_addralign; /* Alignment for uncompressed data */
} b_elf_chdr;              /* Elf_Chdr */

#else /* BACKTRACE_ELF_SIZE != 32 */

typedef struct {
  b_elf_word ch_type;       /* Compression algorithm */
  b_elf_word ch_reserved;   /* Reserved */
  b_elf_xword ch_size;      /* Uncompressed size */
  b_elf_xword ch_addralign; /* Alignment for uncompressed data */
} b_elf_chdr;               /* Elf_Chdr */

#endif /* BACKTRACE_ELF_SIZE != 32 */

#define ELFCOMPRESS_ZLIB 1
#define ELFCOMPRESS_ZSTD 2

/* Names of sections, indexed by enum dwarf_section in internal.h.  */

static const char *const dwarf_section_names[DEBUG_MAX] = {
    ".debug_info",        ".debug_line",     ".debug_abbrev",
    ".debug_ranges",      ".debug_str",      ".debug_addr",
    ".debug_str_offsets", ".debug_line_str", ".debug_rnglists"};

/* Information we gather for the sections we care about.  */

struct debug_section_info {
  /* Section file offset.  */
  off_t offset;
  /* Section size.  */
  size_t size;
  /* Section contents, after read from file.  */
  const unsigned char *data;
  /* Whether the SHF_COMPRESSED flag is set for the section.  */
  int compressed;
};

/* Information we keep for an ELF symbol.  */

struct elf_symbol {
  /* The name of the symbol.  */
  const char *name;
  /* The address of the symbol.  */
  uintptr_t address;
  /* The size of the symbol.  */
  size_t size;
};

/* Information to pass to elf_syminfo.  */

struct elf_syminfo_data {
  /* Symbols for the next module.  */
  struct elf_syminfo_data *next;
  /* The ELF symbols, sorted by address.  */
  struct elf_symbol *symbols;
  /* The number of symbols.  */
  size_t count;
};

/* Information about PowerPC64 ELFv1 .opd section.  */

struct elf_ppc64_opd_data {
  /* Address of the .opd section.  */
  b_elf_addr addr;
  /* Section data.  */
  const char *data;
  /* Size of the .opd section.  */
  size_t size;
  /* Corresponding section view.  */
  struct elf_view view;
};

/* A dummy callback function used when we can't find a symbol
   table.  */

static void elf_nosyms(ten_backtrace_t *self, uintptr_t addr,
                       ten_backtrace_dump_syminfo_func_t dump_file_line_cb,
                       ten_backtrace_error_func_t error_cb, void *data) {
  error_cb(self, "no symbol table in ELF executable", -1, data);
}

/**
 * @brief A callback function used when we can't find any debug info.
 */
static int elf_nodebug(ten_backtrace_t *self_, uintptr_t pc,
                       ten_backtrace_dump_file_line_func_t callback,
                       ten_backtrace_error_func_t error_cb, void *data) {
  ten_backtrace_posix_t *self = (ten_backtrace_posix_t *)self_;
  assert(self && "Invalid argument.");

  if (self->get_syminfo != NULL && self->get_syminfo != elf_nosyms) {
    struct backtrace_call_full bt_data;

    // Fetch symbol information so that we can least get the function name.

    bt_data.dump_file_line_cb = callback;
    bt_data.error_cb = error_cb;
    bt_data.data = data;
    bt_data.ret = 0;

    self->get_syminfo((ten_backtrace_t *)self, pc,
                      backtrace_dump_syminfo_to_dump_file_line_cb,
                      backtrace_dump_syminfo_to_dump_file_line_error_cb,
                      &bt_data);

    return bt_data.ret;
  }

  error_cb(self_, "no debug info in ELF executable", -1, data);
  return 0;
}

/* Compare struct elf_symbol for qsort.  */

static int elf_symbol_compare(const void *v1, const void *v2) {
  const struct elf_symbol *e1 = (const struct elf_symbol *)v1;
  const struct elf_symbol *e2 = (const struct elf_symbol *)v2;

  if (e1->address < e2->address) {
    return -1;
  } else if (e1->address > e2->address) {
    return 1;
  } else {
    return 0;
  }
}

/* Compare an ADDR against an elf_symbol for bsearch.  We allocate one
   extra entry in the array so that this can look safely at the next
   entry.  */

static int elf_symbol_search(const void *vkey, const void *ventry) {
  const uintptr_t *key = (const uintptr_t *)vkey;
  const struct elf_symbol *entry = (const struct elf_symbol *)ventry;
  uintptr_t addr;

  addr = *key;
  if (addr < entry->address)
    return -1;
  else if (addr >= entry->address + entry->size)
    return 1;
  else
    return 0;
}

/* Initialize the symbol table info for elf_syminfo.  */

static int elf_initialize_syminfo(ten_backtrace_t *self, uintptr_t base_address,
                                  const unsigned char *symtab_data,
                                  size_t symtab_size,
                                  const unsigned char *strtab,
                                  size_t strtab_size,
                                  ten_backtrace_error_func_t error_cb,
                                  void *data, struct elf_syminfo_data *sdata,
                                  struct elf_ppc64_opd_data *opd) {
  size_t sym_count;
  const b_elf_sym *sym;
  size_t elf_symbol_count;
  size_t elf_symbol_size;
  struct elf_symbol *elf_symbols;
  size_t i;
  unsigned int j;

  sym_count = symtab_size / sizeof(b_elf_sym);

  /* We only care about function symbols.  Count them.  */
  sym = (const b_elf_sym *)symtab_data;
  elf_symbol_count = 0;
  for (i = 0; i < sym_count; ++i, ++sym) {
    int info;

    info = sym->st_info & 0xf;
    if ((info == STT_FUNC || info == STT_OBJECT) && sym->st_shndx != SHN_UNDEF)
      ++elf_symbol_count;
  }

  // Some shared library might not have any exported symbols, ex: the RTC
  // plugins.
  if (elf_symbol_count == 0) {
    return 1;
  }

  elf_symbol_size = elf_symbol_count * sizeof(struct elf_symbol);
  elf_symbols =
      (struct elf_symbol *)ten_malloc_without_backtrace(elf_symbol_size);
  assert(elf_symbols && "Failed to allocate memory.");
  if (elf_symbols == NULL) {
    return 0;
  }

  sym = (const b_elf_sym *)symtab_data;
  j = 0;
  for (i = 0; i < sym_count; ++i, ++sym) {
    int info = sym->st_info & 0xf;
    if (info != STT_FUNC && info != STT_OBJECT) {
      continue;
    }

    if (sym->st_shndx == SHN_UNDEF) {
      continue;
    }

    if (sym->st_name >= strtab_size) {
      error_cb(self, "symbol string index out of range", 0, data);
      ten_free_without_backtrace(elf_symbols);
      return 0;
    }
    elf_symbols[j].name = (const char *)strtab + sym->st_name;
    /* Special case PowerPC64 ELFv1 symbols in .opd section, if the symbol
       is a function descriptor, read the actual code address from the
       descriptor.  */
    if (opd && sym->st_value >= opd->addr &&
        sym->st_value < opd->addr + opd->size)
      elf_symbols[j].address =
          *(const b_elf_addr *)(opd->data + (sym->st_value - opd->addr));
    else
      elf_symbols[j].address = sym->st_value;
    elf_symbols[j].address += base_address;
    elf_symbols[j].size = sym->st_size;
    ++j;
  }

  backtrace_qsort(elf_symbols, elf_symbol_count, sizeof(struct elf_symbol),
                  elf_symbol_compare);

  sdata->next = NULL;
  sdata->symbols = elf_symbols;
  sdata->count = elf_symbol_count;

  return 1;
}

/* Add EDATA to the list in STATE.  */

static void elf_add_syminfo_data(ten_backtrace_t *self_,
                                 struct elf_syminfo_data *edata) {
  ten_backtrace_posix_t *self = (ten_backtrace_posix_t *)self_;
  assert(self && "Invalid argument.");

  while (1) {
    struct elf_syminfo_data **pp =
        (struct elf_syminfo_data **)(void *)&self->get_syminfo_data;

    while (1) {
      struct elf_syminfo_data *p = ten_atomic_ptr_load((void *)pp);
      if (p == NULL) {
        break;
      }

      pp = &p->next;
    }

    if (__sync_bool_compare_and_swap(pp, NULL, edata)) {
      break;
    }
  }
}

/* Return the symbol name and value for an ADDR.  */

static void elf_syminfo(ten_backtrace_t *self_, uintptr_t addr,
                        ten_backtrace_dump_syminfo_func_t callback,
                        ten_backtrace_error_func_t error_cb, void *data) {
  ten_backtrace_posix_t *self = (ten_backtrace_posix_t *)self_;
  assert(self && "Invalid argument.");

  struct elf_symbol *sym = NULL;

  struct elf_syminfo_data **pp =
      (struct elf_syminfo_data **)(void *)&self->get_syminfo_data;
  while (1) {
    struct elf_syminfo_data *edata = ten_atomic_ptr_load((void *)pp);
    if (edata == NULL) {
      break;
    }

    sym = ((struct elf_symbol *)bsearch(&addr, edata->symbols, edata->count,
                                        sizeof(struct elf_symbol),
                                        elf_symbol_search));
    if (sym != NULL) {
      break;
    }

    pp = &edata->next;
  }

  if (sym == NULL) {
    callback(self_, addr, NULL, 0, 0, data);
  } else {
    callback(self_, addr, sym->name, sym->address, sym->size, data);
  }
}

/* *PVAL is the current value being read from the stream, and *PBITS
   is the number of valid bits.  Ensure that *PVAL holds at least 15
   bits by reading additional bits from *PPIN, up to PINEND, as
   needed.  Updates *PPIN, *PVAL and *PBITS.  Returns 1 on success, 0
   on error.  */
int elf_fetch_bits(const unsigned char **ppin, const unsigned char *pinend,
                   uint64_t *pval, unsigned int *pbits) {
  unsigned int bits;
  const unsigned char *pin;
  uint64_t val;
  uint32_t next;

  bits = *pbits;
  if (bits >= 15) return 1;
  pin = *ppin;
  val = *pval;

  if (unlikely(pinend - pin < 4)) {
    elf_uncompress_failed();
    return 0;
  }

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
    defined(__ORDER_BIG_ENDIAN__) &&                               \
    (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ ||                     \
     __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  /* We've ensured that PIN is aligned.  */
  next = *(const uint32_t *)pin;

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  next = __builtin_bswap32(next);
#endif
#else
  next = pin[0] | (pin[1] << 8) | (pin[2] << 16) | (pin[3] << 24);
#endif

  val |= (uint64_t)next << bits;
  bits += 32;
  pin += 4;

  /* We will need the next four bytes soon.  */
  __builtin_prefetch(pin, 0, 0);

  *ppin = pin;
  *pval = val;
  *pbits = bits;
  return 1;
}

/* This is like elf_fetch_bits, but it fetchs the bits backward, and ensures at
   least 16 bits.  This is for zstd.  */

static int elf_fetch_bits_backward(const unsigned char **ppin,
                                   const unsigned char *pinend, uint64_t *pval,
                                   unsigned int *pbits) {
  unsigned int bits;
  const unsigned char *pin;
  uint64_t val;
  uint32_t next;

  bits = *pbits;
  if (bits >= 16) return 1;
  pin = *ppin;
  val = *pval;

  if (unlikely(pin <= pinend)) {
    if (bits == 0) {
      elf_uncompress_failed();
      return 0;
    }
    return 1;
  }

  pin -= 4;

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
    defined(__ORDER_BIG_ENDIAN__) &&                               \
    (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ ||                     \
     __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  /* We've ensured that PIN is aligned.  */
  next = *(const uint32_t *)pin;

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  next = __builtin_bswap32(next);
#endif
#else
  next = pin[0] | (pin[1] << 8) | (pin[2] << 16) | (pin[3] << 24);
#endif

  val <<= 32;
  val |= next;
  bits += 32;

  if (unlikely(pin < pinend)) {
    val >>= (pinend - pin) * 8;
    bits -= (pinend - pin) * 8;
  }

  *ppin = pin;
  *pval = val;
  *pbits = bits;
  return 1;
}

/* Initialize backward fetching when the bitstream starts with a 1 bit in the
   last byte in memory (which is the first one that we read).  This is used by
   zstd decompression.  Returns 1 on success, 0 on error.  */

static int elf_fetch_backward_init(const unsigned char **ppin,
                                   const unsigned char *pinend, uint64_t *pval,
                                   unsigned int *pbits) {
  const unsigned char *pin;
  unsigned int stream_start;
  uint64_t val;
  unsigned int bits;

  pin = *ppin;
  stream_start = (unsigned int)*pin;
  if (unlikely(stream_start == 0)) {
    elf_uncompress_failed();
    return 0;
  }
  val = 0;
  bits = 0;

  /* Align to a 32-bit boundary.  */
  while ((((uintptr_t)pin) & 3) != 0) {
    val <<= 8;
    val |= (uint64_t)*pin;
    bits += 8;
    --pin;
  }

  val <<= 8;
  val |= (uint64_t)*pin;
  bits += 8;

  *ppin = pin;
  *pval = val;
  *pbits = bits;
  if (!elf_fetch_bits_backward(ppin, pinend, pval, pbits)) return 0;

  *pbits -= __builtin_clz(stream_start) - (sizeof(unsigned int) - 1) * 8 + 1;

  if (!elf_fetch_bits_backward(ppin, pinend, pval, pbits)) return 0;

  return 1;
}

/* For working memory during zstd compression, we need
   - a literal length FSE table: 512 64-bit values == 4096 bytes
   - a match length FSE table: 512 64-bit values == 4096 bytes
   - a offset FSE table: 256 64-bit values == 2048 bytes
   - a Huffman tree: 2048 uint16_t values == 4096 bytes
   - scratch space, one of
     - to build an FSE table: 512 uint16_t values == 1024 bytes
     - to build a Huffman tree: 512 uint16_t + 256 uint32_t == 2048 bytes
*/

#define ZSTD_TABLE_SIZE                                   \
  (2 * 512 * sizeof(struct elf_zstd_fse_baseline_entry) + \
   256 * sizeof(struct elf_zstd_fse_baseline_entry) +     \
   2048 * sizeof(uint16_t) + 512 * sizeof(uint16_t) + 256 * sizeof(uint32_t))

#define ZSTD_TABLE_LITERAL_FSE_OFFSET (0)

#define ZSTD_TABLE_MATCH_FSE_OFFSET \
  (512 * sizeof(struct elf_zstd_fse_baseline_entry))

#define ZSTD_TABLE_OFFSET_FSE_OFFSET \
  (ZSTD_TABLE_MATCH_FSE_OFFSET +     \
   512 * sizeof(struct elf_zstd_fse_baseline_entry))

#define ZSTD_TABLE_HUFFMAN_OFFSET \
  (ZSTD_TABLE_OFFSET_FSE_OFFSET + \
   256 * sizeof(struct elf_zstd_fse_baseline_entry))

#define ZSTD_TABLE_WORK_OFFSET \
  (ZSTD_TABLE_HUFFMAN_OFFSET + 2048 * sizeof(uint16_t))

/* An entry in a zstd FSE table.  */

struct elf_zstd_fse_entry {
  /* The value that this FSE entry represents.  */
  unsigned char symbol;
  /* The number of bits to read to determine the next state.  */
  unsigned char bits;
  /* Add the bits to this base to get the next state.  */
  uint16_t base;
};

static int elf_zstd_build_fse(const int16_t *, int, uint16_t *, int,
                              struct elf_zstd_fse_entry *);

/* Read a zstd FSE table and build the decoding table in *TABLE, updating *PPIN
   as it reads.  ZDEBUG_TABLE is scratch space; it must be enough for 512
   uint16_t values (1024 bytes).  MAXIDX is the maximum number of symbols
   permitted. *TABLE_BITS is the maximum number of bits for symbols in the
   table: the size of *TABLE is at least 1 << *TABLE_BITS.  This updates
   *TABLE_BITS to the actual number of bits.  Returns 1 on success, 0 on
   error.  */

static int elf_zstd_read_fse(const unsigned char **ppin,
                             const unsigned char *pinend,
                             uint16_t *zdebug_table, int maxidx,
                             struct elf_zstd_fse_entry *table,
                             int *table_bits) {
  const unsigned char *pin;
  int16_t *norm;
  uint16_t *next;
  uint64_t val;
  unsigned int bits;
  int accuracy_log;
  uint32_t remaining;
  uint32_t threshold;
  int bits_needed;
  int idx;
  int prev0;

  pin = *ppin;

  norm = (int16_t *)zdebug_table;
  next = zdebug_table + 256;

  if (unlikely(pin + 3 >= pinend)) {
    elf_uncompress_failed();
    return 0;
  }

  /* Align PIN to a 32-bit boundary.  */

  val = 0;
  bits = 0;
  while ((((uintptr_t)pin) & 3) != 0) {
    val |= (uint64_t)*pin << bits;
    bits += 8;
    ++pin;
  }

  if (!elf_fetch_bits(&pin, pinend, &val, &bits)) return 0;

  accuracy_log = (val & 0xf) + 5;
  if (accuracy_log > *table_bits) {
    elf_uncompress_failed();
    return 0;
  }
  *table_bits = accuracy_log;
  val >>= 4;
  bits -= 4;

  /* This code is mostly copied from the reference implementation.  */

  /* The number of remaining probabilities, plus 1.  This sets the number of
     bits that need to be read for the next value.  */
  remaining = (1 << accuracy_log) + 1;

  /* The current difference between small and large values, which depends on
     the number of remaining values.  Small values use one less bit.  */
  threshold = 1 << accuracy_log;

  /* The number of bits used to compute threshold.  */
  bits_needed = accuracy_log + 1;

  /* The next character value.  */
  idx = 0;

  /* Whether the last count was 0.  */
  prev0 = 0;

  while (remaining > 1 && idx <= maxidx) {
    uint32_t max;
    int32_t count;

    if (!elf_fetch_bits(&pin, pinend, &val, &bits)) return 0;

    if (prev0) {
      int zidx;

      /* Previous count was 0, so there is a 2-bit repeat flag.  If the
         2-bit flag is 0b11, it adds 3 and then there is another repeat
         flag.  */
      zidx = idx;
      while ((val & 0xfff) == 0xfff) {
        zidx += 3 * 6;
        val >>= 12;
        bits -= 12;
        if (!elf_fetch_bits(&pin, pinend, &val, &bits)) return 0;
      }
      while ((val & 3) == 3) {
        zidx += 3;
        val >>= 2;
        bits -= 2;
        if (!elf_fetch_bits(&pin, pinend, &val, &bits)) return 0;
      }
      /* We have at least 13 bits here, don't need to fetch.  */
      zidx += val & 3;
      val >>= 2;
      bits -= 2;

      if (unlikely(zidx > maxidx)) {
        elf_uncompress_failed();
        return 0;
      }

      for (; idx < zidx; idx++) norm[idx] = 0;

      prev0 = 0;
      continue;
    }

    max = (2 * threshold - 1) - remaining;
    if ((val & (threshold - 1)) < max) {
      /* A small value.  */
      count = (int32_t)((uint32_t)val & (threshold - 1));
      val >>= bits_needed - 1;
      bits -= bits_needed - 1;
    } else {
      /* A large value.  */
      count = (int32_t)((uint32_t)val & (2 * threshold - 1));
      if (count >= (int32_t)threshold) count -= (int32_t)max;
      val >>= bits_needed;
      bits -= bits_needed;
    }

    count--;
    if (count >= 0)
      remaining -= count;
    else
      remaining--;
    if (unlikely(idx >= 256)) {
      elf_uncompress_failed();
      return 0;
    }
    norm[idx] = (int16_t)count;
    ++idx;

    prev0 = count == 0;

    while (remaining < threshold) {
      bits_needed--;
      threshold >>= 1;
    }
  }

  if (unlikely(remaining != 1)) {
    elf_uncompress_failed();
    return 0;
  }

  /* If we've read ahead more than a byte, back up.  */
  while (bits >= 8) {
    --pin;
    bits -= 8;
  }

  *ppin = pin;

  for (; idx <= maxidx; idx++) norm[idx] = 0;

  return elf_zstd_build_fse(norm, idx, next, *table_bits, table);
}

/* Build the FSE decoding table from a list of probabilities.  This reads from
   NORM of length IDX, uses NEXT as scratch space, and writes to *TABLE, whose
   size is TABLE_BITS.  */

static int elf_zstd_build_fse(const int16_t *norm, int idx, uint16_t *next,
                              int table_bits,
                              struct elf_zstd_fse_entry *table) {
  int table_size;
  int high_threshold;
  int i;
  int pos;
  int step;
  int mask;

  table_size = 1 << table_bits;
  high_threshold = table_size - 1;
  for (i = 0; i < idx; i++) {
    int16_t n;

    n = norm[i];
    if (n >= 0)
      next[i] = (uint16_t)n;
    else {
      table[high_threshold].symbol = (unsigned char)i;
      high_threshold--;
      next[i] = 1;
    }
  }

  pos = 0;
  step = (table_size >> 1) + (table_size >> 3) + 3;
  mask = table_size - 1;
  for (i = 0; i < idx; i++) {
    int n;
    int j;

    n = (int)norm[i];
    for (j = 0; j < n; j++) {
      table[pos].symbol = (unsigned char)i;
      pos = (pos + step) & mask;
      while (unlikely(pos > high_threshold)) pos = (pos + step) & mask;
    }
  }
  if (unlikely(pos != 0)) {
    elf_uncompress_failed();
    return 0;
  }

  for (i = 0; i < table_size; i++) {
    unsigned char sym;
    uint16_t next_state;
    int high_bit;
    int bits;

    sym = table[i].symbol;
    next_state = next[sym];
    ++next[sym];

    if (next_state == 0) {
      elf_uncompress_failed();
      return 0;
    }
    high_bit = 31 - __builtin_clz(next_state);

    bits = table_bits - high_bit;
    table[i].bits = (unsigned char)bits;
    table[i].base = (uint16_t)((next_state << bits) - table_size);
  }

  return 1;
}

/* Encode the baseline and bits into a single 32-bit value.  */

#define ZSTD_ENCODE_BASELINE_BITS(baseline, basebits) \
  ((uint32_t)(baseline) | ((uint32_t)(basebits) << 24))

#define ZSTD_DECODE_BASELINE(baseline_basebits) \
  ((uint32_t)(baseline_basebits) & 0xffffff)

#define ZSTD_DECODE_BASEBITS(baseline_basebits) \
  ((uint32_t)(baseline_basebits) >> 24)

/* Given a literal length code, we need to read a number of bits and add that
   to a baseline.  For states 0 to 15 the baseline is the state and the number
   of bits is zero.  */

#define ZSTD_LITERAL_LENGTH_BASELINE_OFFSET (16)

static const uint32_t elf_zstd_literal_length_base[] = {
    ZSTD_ENCODE_BASELINE_BITS(16, 1),     ZSTD_ENCODE_BASELINE_BITS(18, 1),
    ZSTD_ENCODE_BASELINE_BITS(20, 1),     ZSTD_ENCODE_BASELINE_BITS(22, 1),
    ZSTD_ENCODE_BASELINE_BITS(24, 2),     ZSTD_ENCODE_BASELINE_BITS(28, 2),
    ZSTD_ENCODE_BASELINE_BITS(32, 3),     ZSTD_ENCODE_BASELINE_BITS(40, 3),
    ZSTD_ENCODE_BASELINE_BITS(48, 4),     ZSTD_ENCODE_BASELINE_BITS(64, 6),
    ZSTD_ENCODE_BASELINE_BITS(128, 7),    ZSTD_ENCODE_BASELINE_BITS(256, 8),
    ZSTD_ENCODE_BASELINE_BITS(512, 9),    ZSTD_ENCODE_BASELINE_BITS(1024, 10),
    ZSTD_ENCODE_BASELINE_BITS(2048, 11),  ZSTD_ENCODE_BASELINE_BITS(4096, 12),
    ZSTD_ENCODE_BASELINE_BITS(8192, 13),  ZSTD_ENCODE_BASELINE_BITS(16384, 14),
    ZSTD_ENCODE_BASELINE_BITS(32768, 15), ZSTD_ENCODE_BASELINE_BITS(65536, 16)};

/* The same applies to match length codes.  For states 0 to 31 the baseline is
   the state + 3 and the number of bits is zero.  */

#define ZSTD_MATCH_LENGTH_BASELINE_OFFSET (32)

static const uint32_t elf_zstd_match_length_base[] = {
    ZSTD_ENCODE_BASELINE_BITS(35, 1),     ZSTD_ENCODE_BASELINE_BITS(37, 1),
    ZSTD_ENCODE_BASELINE_BITS(39, 1),     ZSTD_ENCODE_BASELINE_BITS(41, 1),
    ZSTD_ENCODE_BASELINE_BITS(43, 2),     ZSTD_ENCODE_BASELINE_BITS(47, 2),
    ZSTD_ENCODE_BASELINE_BITS(51, 3),     ZSTD_ENCODE_BASELINE_BITS(59, 3),
    ZSTD_ENCODE_BASELINE_BITS(67, 4),     ZSTD_ENCODE_BASELINE_BITS(83, 4),
    ZSTD_ENCODE_BASELINE_BITS(99, 5),     ZSTD_ENCODE_BASELINE_BITS(131, 7),
    ZSTD_ENCODE_BASELINE_BITS(259, 8),    ZSTD_ENCODE_BASELINE_BITS(515, 9),
    ZSTD_ENCODE_BASELINE_BITS(1027, 10),  ZSTD_ENCODE_BASELINE_BITS(2051, 11),
    ZSTD_ENCODE_BASELINE_BITS(4099, 12),  ZSTD_ENCODE_BASELINE_BITS(8195, 13),
    ZSTD_ENCODE_BASELINE_BITS(16387, 14), ZSTD_ENCODE_BASELINE_BITS(32771, 15),
    ZSTD_ENCODE_BASELINE_BITS(65539, 16)};

/* An entry in an FSE table used for literal/match/length values.  For these we
   have to map the symbol to a baseline value, and we have to read zero or more
   bits and add that value to the baseline value.  Rather than look the values
   up in a separate table, we grow the FSE table so that we get better memory
   caching.  */

struct elf_zstd_fse_baseline_entry {
  /* The baseline for the value that this FSE entry represents..  */
  uint32_t baseline;
  /* The number of bits to read to add to the baseline.  */
  unsigned char basebits;
  /* The number of bits to read to determine the next state.  */
  unsigned char bits;
  /* Add the bits to this base to get the next state.  */
  uint16_t base;
};

/* Convert the literal length FSE table FSE_TABLE to an FSE baseline table at
   BASELINE_TABLE.  Note that FSE_TABLE and BASELINE_TABLE will overlap.  */

static int elf_zstd_make_literal_baseline_fse(
    const struct elf_zstd_fse_entry *fse_table, int table_bits,
    struct elf_zstd_fse_baseline_entry *baseline_table) {
  size_t count;
  const struct elf_zstd_fse_entry *pfse;
  struct elf_zstd_fse_baseline_entry *pbaseline;

  /* Convert backward to avoid overlap.  */

  count = 1U << table_bits;
  pfse = fse_table + count;
  pbaseline = baseline_table + count;
  while (pfse > fse_table) {
    unsigned char symbol;
    unsigned char bits;
    uint16_t base;

    --pfse;
    --pbaseline;
    symbol = pfse->symbol;
    bits = pfse->bits;
    base = pfse->base;
    if (symbol < ZSTD_LITERAL_LENGTH_BASELINE_OFFSET) {
      pbaseline->baseline = (uint32_t)symbol;
      pbaseline->basebits = 0;
    } else {
      unsigned int idx;
      uint32_t basebits;

      if (unlikely(symbol > 35)) {
        elf_uncompress_failed();
        return 0;
      }
      idx = symbol - ZSTD_LITERAL_LENGTH_BASELINE_OFFSET;
      basebits = elf_zstd_literal_length_base[idx];
      pbaseline->baseline = ZSTD_DECODE_BASELINE(basebits);
      pbaseline->basebits = ZSTD_DECODE_BASEBITS(basebits);
    }
    pbaseline->bits = bits;
    pbaseline->base = base;
  }

  return 1;
}

/* Convert the offset length FSE table FSE_TABLE to an FSE baseline table at
   BASELINE_TABLE.  Note that FSE_TABLE and BASELINE_TABLE will overlap.  */

static int elf_zstd_make_offset_baseline_fse(
    const struct elf_zstd_fse_entry *fse_table, int table_bits,
    struct elf_zstd_fse_baseline_entry *baseline_table) {
  size_t count;
  const struct elf_zstd_fse_entry *pfse;
  struct elf_zstd_fse_baseline_entry *pbaseline;

  /* Convert backward to avoid overlap.  */

  count = 1U << table_bits;
  pfse = fse_table + count;
  pbaseline = baseline_table + count;
  while (pfse > fse_table) {
    unsigned char symbol;
    unsigned char bits;
    uint16_t base;

    --pfse;
    --pbaseline;
    symbol = pfse->symbol;
    bits = pfse->bits;
    base = pfse->base;
    if (unlikely(symbol > 31)) {
      elf_uncompress_failed();
      return 0;
    }

    /* The simple way to write this is

         pbaseline->baseline = (uint32_t)1 << symbol;
         pbaseline->basebits = symbol;

       That will give us an offset value that corresponds to the one
       described in the RFC.  However, for offset values > 3, we have to
       subtract 3.  And for offset values 1, 2, 3 we use a repeated offset.
       The baseline is always a power of 2, and is never 0, so for these low
       values we will see one entry that is baseline 1, basebits 0, and one
       entry that is baseline 2, basebits 1.  All other entries will have
       baseline >= 4 and basebits >= 2.

       So we can check for RFC offset <= 3 by checking for basebits <= 1.
       And that means that we can subtract 3 here and not worry about doing
       it in the hot loop.  */

    pbaseline->baseline = (uint32_t)1 << symbol;
    if (symbol >= 2) pbaseline->baseline -= 3;
    pbaseline->basebits = symbol;
    pbaseline->bits = bits;
    pbaseline->base = base;
  }

  return 1;
}

/* Convert the match length FSE table FSE_TABLE to an FSE baseline table at
   BASELINE_TABLE.  Note that FSE_TABLE and BASELINE_TABLE will overlap.  */

static int elf_zstd_make_match_baseline_fse(
    const struct elf_zstd_fse_entry *fse_table, int table_bits,
    struct elf_zstd_fse_baseline_entry *baseline_table) {
  size_t count;
  const struct elf_zstd_fse_entry *pfse;
  struct elf_zstd_fse_baseline_entry *pbaseline;

  /* Convert backward to avoid overlap.  */

  count = 1U << table_bits;
  pfse = fse_table + count;
  pbaseline = baseline_table + count;
  while (pfse > fse_table) {
    unsigned char symbol;
    unsigned char bits;
    uint16_t base;

    --pfse;
    --pbaseline;
    symbol = pfse->symbol;
    bits = pfse->bits;
    base = pfse->base;
    if (symbol < ZSTD_MATCH_LENGTH_BASELINE_OFFSET) {
      pbaseline->baseline = (uint32_t)symbol + 3;
      pbaseline->basebits = 0;
    } else {
      unsigned int idx;
      uint32_t basebits;

      if (unlikely(symbol > 52)) {
        elf_uncompress_failed();
        return 0;
      }
      idx = symbol - ZSTD_MATCH_LENGTH_BASELINE_OFFSET;
      basebits = elf_zstd_match_length_base[idx];
      pbaseline->baseline = ZSTD_DECODE_BASELINE(basebits);
      pbaseline->basebits = ZSTD_DECODE_BASEBITS(basebits);
    }
    pbaseline->bits = bits;
    pbaseline->base = base;
  }

  return 1;
}

#ifdef BACKTRACE_GENERATE_ZSTD_FSE_TABLES

/* Used to generate the predefined FSE decoding tables for zstd.  */

#include <stdio.h>

/* These values are straight from RFC 8878.  */

static int16_t lit[36] = {4, 3, 2, 2, 2, 2, 2, 2, 2,  2,  2,  2,
                          2, 1, 1, 1, 2, 2, 2, 2, 2,  2,  2,  2,
                          2, 3, 2, 1, 1, 1, 1, 1, -1, -1, -1, -1};

static int16_t match[53] = {1, 4, 3, 2, 2,  2,  2,  2,  2,  1,  1, 1, 1, 1,
                            1, 1, 1, 1, 1,  1,  1,  1,  1,  1,  1, 1, 1, 1,
                            1, 1, 1, 1, 1,  1,  1,  1,  1,  1,  1, 1, 1, 1,
                            1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1};

static int16_t offset[29] = {1, 1, 1, 1, 1, 1, 2, 2, 2, 1,  1,  1,  1,  1, 1,
                             1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1};

static uint16_t next[256];

static void print_table(const struct elf_zstd_fse_baseline_entry *table,
                        size_t size) {
  size_t i;

  printf("{\n");
  for (i = 0; i < size; i += 3) {
    int j;

    printf(" ");
    for (j = 0; j < 3 && i + j < size; ++j)
      printf(" { %u, %d, %d, %d },", table[i + j].baseline,
             table[i + j].basebits, table[i + j].bits, table[i + j].base);
    printf("\n");
  }
  printf("};\n");
}

int main() {
  struct elf_zstd_fse_entry lit_table[64];
  struct elf_zstd_fse_baseline_entry lit_baseline[64];
  struct elf_zstd_fse_entry match_table[64];
  struct elf_zstd_fse_baseline_entry match_baseline[64];
  struct elf_zstd_fse_entry offset_table[32];
  struct elf_zstd_fse_baseline_entry offset_baseline[32];

  if (!elf_zstd_build_fse(lit, sizeof lit / sizeof lit[0], next, 6,
                          lit_table)) {
    fprintf(stderr, "elf_zstd_build_fse failed\n");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  if (!elf_zstd_make_literal_baseline_fse(lit_table, 6, lit_baseline)) {
    fprintf(stderr, "elf_zstd_make_literal_baseline_fse failed\n");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  printf(
      "static const struct elf_zstd_fse_baseline_entry "
      "elf_zstd_lit_table[64] =\n");
  print_table(lit_baseline, sizeof lit_baseline / sizeof lit_baseline[0]);
  printf("\n");

  if (!elf_zstd_build_fse(match, sizeof match / sizeof match[0], next, 6,
                          match_table)) {
    fprintf(stderr, "elf_zstd_build_fse failed\n");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  if (!elf_zstd_make_match_baseline_fse(match_table, 6, match_baseline)) {
    fprintf(stderr, "elf_zstd_make_match_baseline_fse failed\n");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  printf(
      "static const struct elf_zstd_fse_baseline_entry "
      "elf_zstd_match_table[64] =\n");
  print_table(match_baseline, sizeof match_baseline / sizeof match_baseline[0]);
  printf("\n");

  if (!elf_zstd_build_fse(offset, sizeof offset / sizeof offset[0], next, 5,
                          offset_table)) {
    fprintf(stderr, "elf_zstd_build_fse failed\n");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  if (!elf_zstd_make_offset_baseline_fse(offset_table, 5, offset_baseline)) {
    fprintf(stderr, "elf_zstd_make_offset_baseline_fse failed\n");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

  printf(
      "static const struct elf_zstd_fse_baseline_entry "
      "elf_zstd_offset_table[32] =\n");
  print_table(offset_baseline,
              sizeof offset_baseline / sizeof offset_baseline[0]);
  printf("\n");

  return 0;
}

#endif

/* The fixed tables generated by the #ifdef'ed out main function
   above.  */

static const struct elf_zstd_fse_baseline_entry elf_zstd_lit_table[64] = {
    {0, 0, 4, 0},      {0, 0, 4, 16},     {1, 0, 5, 32},     {3, 0, 5, 0},
    {4, 0, 5, 0},      {6, 0, 5, 0},      {7, 0, 5, 0},      {9, 0, 5, 0},
    {10, 0, 5, 0},     {12, 0, 5, 0},     {14, 0, 6, 0},     {16, 1, 5, 0},
    {20, 1, 5, 0},     {22, 1, 5, 0},     {28, 2, 5, 0},     {32, 3, 5, 0},
    {48, 4, 5, 0},     {64, 6, 5, 32},    {128, 7, 5, 0},    {256, 8, 6, 0},
    {1024, 10, 6, 0},  {4096, 12, 6, 0},  {0, 0, 4, 32},     {1, 0, 4, 0},
    {2, 0, 5, 0},      {4, 0, 5, 32},     {5, 0, 5, 0},      {7, 0, 5, 32},
    {8, 0, 5, 0},      {10, 0, 5, 32},    {11, 0, 5, 0},     {13, 0, 6, 0},
    {16, 1, 5, 32},    {18, 1, 5, 0},     {22, 1, 5, 32},    {24, 2, 5, 0},
    {32, 3, 5, 32},    {40, 3, 5, 0},     {64, 6, 4, 0},     {64, 6, 4, 16},
    {128, 7, 5, 32},   {512, 9, 6, 0},    {2048, 11, 6, 0},  {0, 0, 4, 48},
    {1, 0, 4, 16},     {2, 0, 5, 32},     {3, 0, 5, 32},     {5, 0, 5, 32},
    {6, 0, 5, 32},     {8, 0, 5, 32},     {9, 0, 5, 32},     {11, 0, 5, 32},
    {12, 0, 5, 32},    {15, 0, 6, 0},     {18, 1, 5, 32},    {20, 1, 5, 32},
    {24, 2, 5, 32},    {28, 2, 5, 32},    {40, 3, 5, 32},    {48, 4, 5, 32},
    {65536, 16, 6, 0}, {32768, 15, 6, 0}, {16384, 14, 6, 0}, {8192, 13, 6, 0},
};

static const struct elf_zstd_fse_baseline_entry elf_zstd_match_table[64] = {
    {3, 0, 6, 0},     {4, 0, 4, 0},      {5, 0, 5, 32},     {6, 0, 5, 0},
    {8, 0, 5, 0},     {9, 0, 5, 0},      {11, 0, 5, 0},     {13, 0, 6, 0},
    {16, 0, 6, 0},    {19, 0, 6, 0},     {22, 0, 6, 0},     {25, 0, 6, 0},
    {28, 0, 6, 0},    {31, 0, 6, 0},     {34, 0, 6, 0},     {37, 1, 6, 0},
    {41, 1, 6, 0},    {47, 2, 6, 0},     {59, 3, 6, 0},     {83, 4, 6, 0},
    {131, 7, 6, 0},   {515, 9, 6, 0},    {4, 0, 4, 16},     {5, 0, 4, 0},
    {6, 0, 5, 32},    {7, 0, 5, 0},      {9, 0, 5, 32},     {10, 0, 5, 0},
    {12, 0, 6, 0},    {15, 0, 6, 0},     {18, 0, 6, 0},     {21, 0, 6, 0},
    {24, 0, 6, 0},    {27, 0, 6, 0},     {30, 0, 6, 0},     {33, 0, 6, 0},
    {35, 1, 6, 0},    {39, 1, 6, 0},     {43, 2, 6, 0},     {51, 3, 6, 0},
    {67, 4, 6, 0},    {99, 5, 6, 0},     {259, 8, 6, 0},    {4, 0, 4, 32},
    {4, 0, 4, 48},    {5, 0, 4, 16},     {7, 0, 5, 32},     {8, 0, 5, 32},
    {10, 0, 5, 32},   {11, 0, 5, 32},    {14, 0, 6, 0},     {17, 0, 6, 0},
    {20, 0, 6, 0},    {23, 0, 6, 0},     {26, 0, 6, 0},     {29, 0, 6, 0},
    {32, 0, 6, 0},    {65539, 16, 6, 0}, {32771, 15, 6, 0}, {16387, 14, 6, 0},
    {8195, 13, 6, 0}, {4099, 12, 6, 0},  {2051, 11, 6, 0},  {1027, 10, 6, 0},
};

static const struct elf_zstd_fse_baseline_entry elf_zstd_offset_table[32] = {
    {1, 0, 5, 0},          {61, 6, 4, 0},         {509, 9, 5, 0},
    {32765, 15, 5, 0},     {2097149, 21, 5, 0},   {5, 3, 5, 0},
    {125, 7, 4, 0},        {4093, 12, 5, 0},      {262141, 18, 5, 0},
    {8388605, 23, 5, 0},   {29, 5, 5, 0},         {253, 8, 4, 0},
    {16381, 14, 5, 0},     {1048573, 20, 5, 0},   {1, 2, 5, 0},
    {125, 7, 4, 16},       {2045, 11, 5, 0},      {131069, 17, 5, 0},
    {4194301, 22, 5, 0},   {13, 4, 5, 0},         {253, 8, 4, 16},
    {8189, 13, 5, 0},      {524285, 19, 5, 0},    {2, 1, 5, 0},
    {61, 6, 4, 16},        {1021, 10, 5, 0},      {65533, 16, 5, 0},
    {268435453, 28, 5, 0}, {134217725, 27, 5, 0}, {67108861, 26, 5, 0},
    {33554429, 25, 5, 0},  {16777213, 24, 5, 0},
};

/* Read a zstd Huffman table and build the decoding table in *TABLE, reading
   and updating *PPIN.  This sets *PTABLE_BITS to the number of bits of the
   table, such that the table length is 1 << *TABLE_BITS.  ZDEBUG_TABLE is
   scratch space; it must be enough for 512 uint16_t values + 256 32-bit values
   (2048 bytes).  Returns 1 on success, 0 on error.  */

static int elf_zstd_read_huff(const unsigned char **ppin,
                              const unsigned char *pinend,
                              uint16_t *zdebug_table, uint16_t *table,
                              int *ptable_bits) {
  const unsigned char *pin;
  unsigned char hdr;
  unsigned char *weights;
  size_t count;
  uint32_t *weight_mark;
  size_t i;
  uint32_t weight_mask;
  size_t table_bits;

  pin = *ppin;
  if (unlikely(pin >= pinend)) {
    elf_uncompress_failed();
    return 0;
  }
  hdr = *pin;
  ++pin;

  weights = (unsigned char *)zdebug_table;

  if (hdr < 128) {
    /* Table is compressed using FSE.  */

    struct elf_zstd_fse_entry *fse_table;
    int fse_table_bits;
    uint16_t *scratch;
    const unsigned char *pfse;
    const unsigned char *pback;
    uint64_t val;
    unsigned int bits;
    unsigned int state1, state2;

    /* SCRATCH is used temporarily by elf_zstd_read_fse.  It overlaps
       WEIGHTS.  */
    scratch = zdebug_table;
    fse_table = (struct elf_zstd_fse_entry *)(scratch + 512);
    fse_table_bits = 6;

    pfse = pin;
    if (!elf_zstd_read_fse(&pfse, pinend, scratch, 255, fse_table,
                           &fse_table_bits))
      return 0;

    if (unlikely(pin + hdr > pinend)) {
      elf_uncompress_failed();
      return 0;
    }

    /* We no longer need SCRATCH.  Start recording weights.  We need up to
       256 bytes of weights and 64 bytes of rank counts, so it won't overlap
       FSE_TABLE.  */

    pback = pin + hdr - 1;

    if (!elf_fetch_backward_init(&pback, pfse, &val, &bits)) return 0;

    bits -= fse_table_bits;
    state1 = (val >> bits) & ((1U << fse_table_bits) - 1);
    bits -= fse_table_bits;
    state2 = (val >> bits) & ((1U << fse_table_bits) - 1);

    /* There are two independent FSE streams, tracked by STATE1 and STATE2.
       We decode them alternately.  */

    count = 0;
    while (1) {
      struct elf_zstd_fse_entry *pt;
      uint64_t v;

      pt = &fse_table[state1];

      if (unlikely(pin < pinend) && bits < pt->bits) {
        if (unlikely(count >= 254)) {
          elf_uncompress_failed();
          return 0;
        }
        weights[count] = (unsigned char)pt->symbol;
        weights[count + 1] = (unsigned char)fse_table[state2].symbol;
        count += 2;
        break;
      }

      if (unlikely(pt->bits == 0))
        v = 0;
      else {
        if (!elf_fetch_bits_backward(&pback, pfse, &val, &bits)) return 0;

        bits -= pt->bits;
        v = (val >> bits) & (((uint64_t)1 << pt->bits) - 1);
      }

      state1 = pt->base + v;

      if (unlikely(count >= 255)) {
        elf_uncompress_failed();
        return 0;
      }

      weights[count] = pt->symbol;
      ++count;

      pt = &fse_table[state2];

      if (unlikely(pin < pinend && bits < pt->bits)) {
        if (unlikely(count >= 254)) {
          elf_uncompress_failed();
          return 0;
        }
        weights[count] = (unsigned char)pt->symbol;
        weights[count + 1] = (unsigned char)fse_table[state1].symbol;
        count += 2;
        break;
      }

      if (unlikely(pt->bits == 0))
        v = 0;
      else {
        if (!elf_fetch_bits_backward(&pback, pfse, &val, &bits)) return 0;

        bits -= pt->bits;
        v = (val >> bits) & (((uint64_t)1 << pt->bits) - 1);
      }

      state2 = pt->base + v;

      if (unlikely(count >= 255)) {
        elf_uncompress_failed();
        return 0;
      }

      weights[count] = pt->symbol;
      ++count;
    }

    pin += hdr;
  } else {
    /* Table is not compressed.  Each weight is 4 bits.  */

    count = hdr - 127;
    if (unlikely(pin + ((count + 1) / 2) >= pinend)) {
      elf_uncompress_failed();
      return 0;
    }
    for (i = 0; i < count; i += 2) {
      unsigned char b;

      b = *pin;
      ++pin;
      weights[i] = b >> 4;
      weights[i + 1] = b & 0xf;
    }
  }

  weight_mark = (uint32_t *)(weights + 256);
  memset(weight_mark, 0, 13 * sizeof(uint32_t));
  weight_mask = 0;
  for (i = 0; i < count; ++i) {
    unsigned char w;

    w = weights[i];
    if (unlikely(w > 12)) {
      elf_uncompress_failed();
      return 0;
    }
    ++weight_mark[w];
    if (w > 0) weight_mask += 1U << (w - 1);
  }
  if (unlikely(weight_mask == 0)) {
    elf_uncompress_failed();
    return 0;
  }

  table_bits = 32 - __builtin_clz(weight_mask);
  if (unlikely(table_bits > 11)) {
    elf_uncompress_failed();
    return 0;
  }

  /* Work out the last weight value, which is omitted because the weights must
     sum to a power of two.  */
  {
    uint32_t left;
    uint32_t high_bit;

    left = ((uint32_t)1 << table_bits) - weight_mask;
    if (left == 0) {
      elf_uncompress_failed();
      return 0;
    }
    high_bit = 31 - __builtin_clz(left);
    if (((uint32_t)1 << high_bit) != left) {
      elf_uncompress_failed();
      return 0;
    }

    if (unlikely(count >= 256)) {
      elf_uncompress_failed();
      return 0;
    }

    weights[count] = high_bit + 1;
    ++count;
    ++weight_mark[high_bit + 1];
  }

  if (weight_mark[1] < 2 || (weight_mark[1] & 1) != 0) {
    elf_uncompress_failed();
    return 0;
  }

  /* Change WEIGHT_MARK from a count of weights to the index of the first
     symbol for that weight.  We shift the indexes to also store how many we
     have seen so far, below.  */
  {
    uint32_t next;

    next = 0;
    for (i = 0; i < table_bits; ++i) {
      uint32_t cur;

      cur = next;
      next += weight_mark[i + 1] << i;
      weight_mark[i + 1] = cur;
    }
  }

  for (i = 0; i < count; ++i) {
    unsigned char weight;
    uint32_t length;
    uint16_t tval;
    size_t start;
    uint32_t j;

    weight = weights[i];
    if (weight == 0) continue;

    length = 1U << (weight - 1);
    tval = (i << 8) | (table_bits + 1 - weight);
    start = weight_mark[weight];
    for (j = 0; j < length; ++j) table[start + j] = tval;
    weight_mark[weight] += length;
  }

  *ppin = pin;
  *ptable_bits = (int)table_bits;

  return 1;
}

/* Read and decompress the literals and store them ending at POUTEND.  This
   works because we are going to use all the literals in the output, so they
   must fit into the output buffer.  HUFFMAN_TABLE, and PHUFFMAN_TABLE_BITS
   store the Huffman table across calls.  SCRATCH is used to read a Huffman
   table.  Store the start of the decompressed literals in *PPLIT.  Update
   *PPIN.  Return 1 on success, 0 on error.  */

static int elf_zstd_read_literals(const unsigned char **ppin,
                                  const unsigned char *pinend,
                                  unsigned char *pout, unsigned char *poutend,
                                  uint16_t *scratch, uint16_t *huffman_table,
                                  int *phuffman_table_bits,
                                  unsigned char **pplit) {
  const unsigned char *pin;
  unsigned char *plit;
  unsigned char hdr;
  uint32_t regenerated_size;
  uint32_t compressed_size;
  int streams;
  uint32_t total_streams_size;
  unsigned int huffman_table_bits;
  uint64_t huffman_mask;

  pin = *ppin;
  if (unlikely(pin >= pinend)) {
    elf_uncompress_failed();
    return 0;
  }
  hdr = *pin;
  ++pin;

  if ((hdr & 3) == 0 || (hdr & 3) == 1) {
    int raw;

    /* Raw_Literals_Block or RLE_Literals_Block */

    raw = (hdr & 3) == 0;

    switch ((hdr >> 2) & 3) {
      case 0:
      case 2:
        regenerated_size = hdr >> 3;
        break;
      case 1:
        if (unlikely(pin >= pinend)) {
          elf_uncompress_failed();
          return 0;
        }
        regenerated_size = (hdr >> 4) + ((uint32_t)(*pin) << 4);
        ++pin;
        break;
      case 3:
        if (unlikely(pin + 1 >= pinend)) {
          elf_uncompress_failed();
          return 0;
        }
        regenerated_size =
            ((hdr >> 4) + ((uint32_t)*pin << 4) + ((uint32_t)pin[1] << 12));
        pin += 2;
        break;
      default:
        elf_uncompress_failed();
        return 0;
    }

    if (unlikely((size_t)(poutend - pout) < regenerated_size)) {
      elf_uncompress_failed();
      return 0;
    }

    plit = poutend - regenerated_size;

    if (raw) {
      if (unlikely(pin + regenerated_size >= pinend)) {
        elf_uncompress_failed();
        return 0;
      }
      memcpy(plit, pin, regenerated_size);
      pin += regenerated_size;
    } else {
      if (pin >= pinend) {
        elf_uncompress_failed();
        return 0;
      }
      memset(plit, *pin, regenerated_size);
      ++pin;
    }

    *ppin = pin;
    *pplit = plit;

    return 1;
  }

  /* Compressed_Literals_Block or Treeless_Literals_Block */

  switch ((hdr >> 2) & 3) {
    case 0:
    case 1:
      if (unlikely(pin + 1 >= pinend)) {
        elf_uncompress_failed();
        return 0;
      }
      regenerated_size = (hdr >> 4) | ((uint32_t)(*pin & 0x3f) << 4);
      compressed_size = (uint32_t)*pin >> 6 | ((uint32_t)pin[1] << 2);
      pin += 2;
      streams = ((hdr >> 2) & 3) == 0 ? 1 : 4;
      break;
    case 2:
      if (unlikely(pin + 2 >= pinend)) {
        elf_uncompress_failed();
        return 0;
      }
      regenerated_size = (((uint32_t)hdr >> 4) | ((uint32_t)*pin << 4) |
                          (((uint32_t)pin[1] & 3) << 12));
      compressed_size = (((uint32_t)pin[1] >> 2) | ((uint32_t)pin[2] << 6));
      pin += 3;
      streams = 4;
      break;
    case 3:
      if (unlikely(pin + 3 >= pinend)) {
        elf_uncompress_failed();
        return 0;
      }
      regenerated_size = (((uint32_t)hdr >> 4) | ((uint32_t)*pin << 4) |
                          (((uint32_t)pin[1] & 0x3f) << 12));
      compressed_size = (((uint32_t)pin[1] >> 6) | ((uint32_t)pin[2] << 2) |
                         ((uint32_t)pin[3] << 10));
      pin += 4;
      streams = 4;
      break;
    default:
      elf_uncompress_failed();
      return 0;
  }

  if (unlikely(pin + compressed_size > pinend)) {
    elf_uncompress_failed();
    return 0;
  }

  pinend = pin + compressed_size;
  *ppin = pinend;

  if (unlikely((size_t)(poutend - pout) < regenerated_size)) {
    elf_uncompress_failed();
    return 0;
  }

  plit = poutend - regenerated_size;

  *pplit = plit;

  total_streams_size = compressed_size;
  if ((hdr & 3) == 2) {
    const unsigned char *ptable;

    /* Compressed_Literals_Block.  Read Huffman tree.  */

    ptable = pin;
    if (!elf_zstd_read_huff(&ptable, pinend, scratch, huffman_table,
                            phuffman_table_bits))
      return 0;

    if (unlikely(total_streams_size < (size_t)(ptable - pin))) {
      elf_uncompress_failed();
      return 0;
    }

    total_streams_size -= ptable - pin;
    pin = ptable;
  } else {
    /* Treeless_Literals_Block.  Reuse previous Huffman tree.  */
    if (unlikely(*phuffman_table_bits == 0)) {
      elf_uncompress_failed();
      return 0;
    }
  }

  /* Decompress COMPRESSED_SIZE bytes of data at PIN using the huffman table,
     storing REGENERATED_SIZE bytes of decompressed data at PLIT.  */

  huffman_table_bits = (unsigned int)*phuffman_table_bits;
  huffman_mask = ((uint64_t)1 << huffman_table_bits) - 1;

  if (streams == 1) {
    const unsigned char *pback;
    const unsigned char *pbackend;
    uint64_t val;
    unsigned int bits;
    uint32_t i;

    pback = pin + total_streams_size - 1;
    pbackend = pin;
    if (!elf_fetch_backward_init(&pback, pbackend, &val, &bits)) return 0;

    /* This is one of the inner loops of the decompression algorithm, so we
       put some effort into optimization.  We can't get more than 64 bytes
       from a single call to elf_fetch_bits_backward, and we can't subtract
       more than 11 bits at a time.  */

    if (regenerated_size >= 64) {
      unsigned char *plitstart;
      unsigned char *plitstop;

      plitstart = plit;
      plitstop = plit + regenerated_size - 64;
      while (plit < plitstop) {
        uint16_t t;

        if (!elf_fetch_bits_backward(&pback, pbackend, &val, &bits)) return 0;

        if (bits < 16) break;

        while (bits >= 33) {
          t = huffman_table[(val >> (bits - huffman_table_bits)) &
                            huffman_mask];
          *plit = t >> 8;
          ++plit;
          bits -= t & 0xff;

          t = huffman_table[(val >> (bits - huffman_table_bits)) &
                            huffman_mask];
          *plit = t >> 8;
          ++plit;
          bits -= t & 0xff;

          t = huffman_table[(val >> (bits - huffman_table_bits)) &
                            huffman_mask];
          *plit = t >> 8;
          ++plit;
          bits -= t & 0xff;
        }

        while (bits > 11) {
          t = huffman_table[(val >> (bits - huffman_table_bits)) &
                            huffman_mask];
          *plit = t >> 8;
          ++plit;
          bits -= t & 0xff;
        }
      }

      regenerated_size -= plit - plitstart;
    }

    for (i = 0; i < regenerated_size; ++i) {
      uint16_t t;

      if (!elf_fetch_bits_backward(&pback, pbackend, &val, &bits)) return 0;

      if (unlikely(bits < huffman_table_bits)) {
        t = huffman_table[(val << (huffman_table_bits - bits)) & huffman_mask];
        if (unlikely(bits < (t & 0xff))) {
          elf_uncompress_failed();
          return 0;
        }
      } else
        t = huffman_table[(val >> (bits - huffman_table_bits)) & huffman_mask];

      *plit = t >> 8;
      ++plit;
      bits -= t & 0xff;
    }

    return 1;
  }

  {
    uint32_t stream_size1, stream_size2, stream_size3, stream_size4;
    uint32_t tot;
    const unsigned char *pback1, *pback2, *pback3, *pback4;
    const unsigned char *pbackend1, *pbackend2, *pbackend3, *pbackend4;
    uint64_t val1, val2, val3, val4;
    unsigned int bits1, bits2, bits3, bits4;
    unsigned char *plit1, *plit2, *plit3, *plit4;
    uint32_t regenerated_stream_size;
    uint32_t regenerated_stream_size4;
    uint16_t t1, t2, t3, t4;
    uint32_t i;
    uint32_t limit;

    /* Read jump table.  */
    if (unlikely(pin + 5 >= pinend)) {
      elf_uncompress_failed();
      return 0;
    }
    stream_size1 = (uint32_t)*pin | ((uint32_t)pin[1] << 8);
    pin += 2;
    stream_size2 = (uint32_t)*pin | ((uint32_t)pin[1] << 8);
    pin += 2;
    stream_size3 = (uint32_t)*pin | ((uint32_t)pin[1] << 8);
    pin += 2;
    tot = stream_size1 + stream_size2 + stream_size3;
    if (unlikely(tot > total_streams_size - 6)) {
      elf_uncompress_failed();
      return 0;
    }
    stream_size4 = total_streams_size - 6 - tot;

    pback1 = pin + stream_size1 - 1;
    pbackend1 = pin;

    pback2 = pback1 + stream_size2;
    pbackend2 = pback1 + 1;

    pback3 = pback2 + stream_size3;
    pbackend3 = pback2 + 1;

    pback4 = pback3 + stream_size4;
    pbackend4 = pback3 + 1;

    if (!elf_fetch_backward_init(&pback1, pbackend1, &val1, &bits1)) return 0;
    if (!elf_fetch_backward_init(&pback2, pbackend2, &val2, &bits2)) return 0;
    if (!elf_fetch_backward_init(&pback3, pbackend3, &val3, &bits3)) return 0;
    if (!elf_fetch_backward_init(&pback4, pbackend4, &val4, &bits4)) return 0;

    regenerated_stream_size = (regenerated_size + 3) / 4;

    plit1 = plit;
    plit2 = plit1 + regenerated_stream_size;
    plit3 = plit2 + regenerated_stream_size;
    plit4 = plit3 + regenerated_stream_size;

    regenerated_stream_size4 = regenerated_size - regenerated_stream_size * 3;

    /* We can't get more than 64 literal bytes from a single call to
       elf_fetch_bits_backward.  The fourth stream can be up to 3 bytes less,
       so use as the limit.  */

    limit = regenerated_stream_size4 <= 64 ? 0 : regenerated_stream_size4 - 64;
    i = 0;
    while (i < limit) {
      if (!elf_fetch_bits_backward(&pback1, pbackend1, &val1, &bits1)) return 0;
      if (!elf_fetch_bits_backward(&pback2, pbackend2, &val2, &bits2)) return 0;
      if (!elf_fetch_bits_backward(&pback3, pbackend3, &val3, &bits3)) return 0;
      if (!elf_fetch_bits_backward(&pback4, pbackend4, &val4, &bits4)) return 0;

      /* We can't subtract more than 11 bits at a time.  */

      do {
        t1 = huffman_table[(val1 >> (bits1 - huffman_table_bits)) &
                           huffman_mask];
        t2 = huffman_table[(val2 >> (bits2 - huffman_table_bits)) &
                           huffman_mask];
        t3 = huffman_table[(val3 >> (bits3 - huffman_table_bits)) &
                           huffman_mask];
        t4 = huffman_table[(val4 >> (bits4 - huffman_table_bits)) &
                           huffman_mask];

        *plit1 = t1 >> 8;
        ++plit1;
        bits1 -= t1 & 0xff;

        *plit2 = t2 >> 8;
        ++plit2;
        bits2 -= t2 & 0xff;

        *plit3 = t3 >> 8;
        ++plit3;
        bits3 -= t3 & 0xff;

        *plit4 = t4 >> 8;
        ++plit4;
        bits4 -= t4 & 0xff;

        ++i;
      } while (bits1 > 11 && bits2 > 11 && bits3 > 11 && bits4 > 11);
    }

    while (i < regenerated_stream_size) {
      int use4;

      use4 = i < regenerated_stream_size4;

      if (!elf_fetch_bits_backward(&pback1, pbackend1, &val1, &bits1)) {
        return 0;
      }

      if (!elf_fetch_bits_backward(&pback2, pbackend2, &val2, &bits2)) {
        return 0;
      }

      if (!elf_fetch_bits_backward(&pback3, pbackend3, &val3, &bits3)) {
        return 0;
      }

      if (use4) {
        if (!elf_fetch_bits_backward(&pback4, pbackend4, &val4, &bits4)) {
          return 0;
        }
      }

      if (unlikely(bits1 < huffman_table_bits)) {
        t1 = huffman_table[(val1 << (huffman_table_bits - bits1)) &
                           huffman_mask];
        if (unlikely(bits1 < (t1 & 0xff))) {
          elf_uncompress_failed();
          return 0;
        }
      } else
        t1 = huffman_table[(val1 >> (bits1 - huffman_table_bits)) &
                           huffman_mask];

      if (unlikely(bits2 < huffman_table_bits)) {
        t2 = huffman_table[(val2 << (huffman_table_bits - bits2)) &
                           huffman_mask];
        if (unlikely(bits2 < (t2 & 0xff))) {
          elf_uncompress_failed();
          return 0;
        }
      } else
        t2 = huffman_table[(val2 >> (bits2 - huffman_table_bits)) &
                           huffman_mask];

      if (unlikely(bits3 < huffman_table_bits)) {
        t3 = huffman_table[(val3 << (huffman_table_bits - bits3)) &
                           huffman_mask];
        if (unlikely(bits3 < (t3 & 0xff))) {
          elf_uncompress_failed();
          return 0;
        }
      } else
        t3 = huffman_table[(val3 >> (bits3 - huffman_table_bits)) &
                           huffman_mask];

      if (use4) {
        if (unlikely(bits4 < huffman_table_bits)) {
          t4 = huffman_table[(val4 << (huffman_table_bits - bits4)) &
                             huffman_mask];
          if (unlikely(bits4 < (t4 & 0xff))) {
            elf_uncompress_failed();
            return 0;
          }
        } else
          t4 = huffman_table[(val4 >> (bits4 - huffman_table_bits)) &
                             huffman_mask];

        *plit4 = t4 >> 8;
        ++plit4;
        bits4 -= t4 & 0xff;
      }

      *plit1 = t1 >> 8;
      ++plit1;
      bits1 -= t1 & 0xff;

      *plit2 = t2 >> 8;
      ++plit2;
      bits2 -= t2 & 0xff;

      *plit3 = t3 >> 8;
      ++plit3;
      bits3 -= t3 & 0xff;

      ++i;
    }
  }

  return 1;
}

/* The information used to decompress a sequence code, which can be a literal
   length, an offset, or a match length.  */

struct elf_zstd_seq_decode {
  const struct elf_zstd_fse_baseline_entry *table;
  int table_bits;
};

/* Unpack a sequence code compression mode.  */

static int elf_zstd_unpack_seq_decode(
    int mode, const unsigned char **ppin, const unsigned char *pinend,
    const struct elf_zstd_fse_baseline_entry *predef, int predef_bits,
    uint16_t *scratch, int maxidx, struct elf_zstd_fse_baseline_entry *table,
    int table_bits,
    int (*conv)(const struct elf_zstd_fse_entry *, int,
                struct elf_zstd_fse_baseline_entry *),
    struct elf_zstd_seq_decode *decode) {
  switch (mode) {
    case 0:
      decode->table = predef;
      decode->table_bits = predef_bits;
      break;

    case 1: {
      struct elf_zstd_fse_entry entry;

      if (unlikely(*ppin >= pinend)) {
        elf_uncompress_failed();
        return 0;
      }
      entry.symbol = **ppin;
      ++*ppin;
      entry.bits = 0;
      entry.base = 0;
      decode->table_bits = 0;
      if (!conv(&entry, 0, table)) return 0;
    } break;

    case 2: {
      struct elf_zstd_fse_entry *fse_table;

      /* We use the same space for the simple FSE table and the baseline
         table.  */
      fse_table = (struct elf_zstd_fse_entry *)table;
      decode->table_bits = table_bits;
      if (!elf_zstd_read_fse(ppin, pinend, scratch, maxidx, fse_table,
                             &decode->table_bits))
        return 0;
      if (!conv(fse_table, decode->table_bits, table)) return 0;
      decode->table = table;
    } break;

    case 3:
      if (unlikely(decode->table_bits == -1)) {
        elf_uncompress_failed();
        return 0;
      }
      break;

    default:
      elf_uncompress_failed();
      return 0;
  }

  return 1;
}

/* Decompress a zstd stream from PIN/SIN to POUT/SOUT.  Code based on RFC 8878.
   Return 1 on success, 0 on error.  */

static int elf_zstd_decompress(const unsigned char *pin, size_t sin,
                               unsigned char *zdebug_table, unsigned char *pout,
                               size_t sout) {
  const unsigned char *pinend;
  unsigned char *poutstart;
  unsigned char *poutend;
  struct elf_zstd_seq_decode literal_decode;
  struct elf_zstd_fse_baseline_entry *literal_fse_table;
  struct elf_zstd_seq_decode match_decode;
  struct elf_zstd_fse_baseline_entry *match_fse_table;
  struct elf_zstd_seq_decode offset_decode;
  struct elf_zstd_fse_baseline_entry *offset_fse_table;
  uint16_t *huffman_table;
  int huffman_table_bits;
  uint32_t repeated_offset1;
  uint32_t repeated_offset2;
  uint32_t repeated_offset3;
  uint16_t *scratch;
  unsigned char hdr;
  int has_checksum;
  uint64_t content_size;
  int last_block;

  pinend = pin + sin;
  poutstart = pout;
  poutend = pout + sout;

  literal_decode.table = NULL;
  literal_decode.table_bits = -1;
  literal_fse_table =
      ((struct elf_zstd_fse_baseline_entry *)(zdebug_table +
                                              ZSTD_TABLE_LITERAL_FSE_OFFSET));

  match_decode.table = NULL;
  match_decode.table_bits = -1;
  match_fse_table =
      ((struct elf_zstd_fse_baseline_entry *)(zdebug_table +
                                              ZSTD_TABLE_MATCH_FSE_OFFSET));

  offset_decode.table = NULL;
  offset_decode.table_bits = -1;
  offset_fse_table =
      ((struct elf_zstd_fse_baseline_entry *)(zdebug_table +
                                              ZSTD_TABLE_OFFSET_FSE_OFFSET));
  huffman_table = ((uint16_t *)(zdebug_table + ZSTD_TABLE_HUFFMAN_OFFSET));
  huffman_table_bits = 0;
  scratch = ((uint16_t *)(zdebug_table + ZSTD_TABLE_WORK_OFFSET));

  repeated_offset1 = 1;
  repeated_offset2 = 4;
  repeated_offset3 = 8;

  if (unlikely(sin < 4)) {
    elf_uncompress_failed();
    return 0;
  }

  /* These values are the zstd magic number.  */
  if (unlikely(pin[0] != 0x28 || pin[1] != 0xb5 || pin[2] != 0x2f ||
               pin[3] != 0xfd)) {
    elf_uncompress_failed();
    return 0;
  }

  pin += 4;

  if (unlikely(pin >= pinend)) {
    elf_uncompress_failed();
    return 0;
  }

  hdr = *pin++;

  /* We expect a single frame.  */
  if (unlikely((hdr & (1 << 5)) == 0)) {
    elf_uncompress_failed();
    return 0;
  }
  /* Reserved bit must be zero.  */
  if (unlikely((hdr & (1 << 3)) != 0)) {
    elf_uncompress_failed();
    return 0;
  }
  /* We do not expect a dictionary.  */
  if (unlikely((hdr & 3) != 0)) {
    elf_uncompress_failed();
    return 0;
  }
  has_checksum = (hdr & (1 << 2)) != 0;
  switch (hdr >> 6) {
    case 0:
      if (unlikely(pin >= pinend)) {
        elf_uncompress_failed();
        return 0;
      }
      content_size = (uint64_t)*pin++;
      break;
    case 1:
      if (unlikely(pin + 1 >= pinend)) {
        elf_uncompress_failed();
        return 0;
      }
      content_size = (((uint64_t)pin[0]) | (((uint64_t)pin[1]) << 8)) + 256;
      pin += 2;
      break;
    case 2:
      if (unlikely(pin + 3 >= pinend)) {
        elf_uncompress_failed();
        return 0;
      }
      content_size = ((uint64_t)pin[0] | (((uint64_t)pin[1]) << 8) |
                      (((uint64_t)pin[2]) << 16) | (((uint64_t)pin[3]) << 24));
      pin += 4;
      break;
    case 3:
      if (unlikely(pin + 7 >= pinend)) {
        elf_uncompress_failed();
        return 0;
      }
      content_size = ((uint64_t)pin[0] | (((uint64_t)pin[1]) << 8) |
                      (((uint64_t)pin[2]) << 16) | (((uint64_t)pin[3]) << 24) |
                      (((uint64_t)pin[4]) << 32) | (((uint64_t)pin[5]) << 40) |
                      (((uint64_t)pin[6]) << 48) | (((uint64_t)pin[7]) << 56));
      pin += 8;
      break;
    default:
      elf_uncompress_failed();
      return 0;
  }

  if (unlikely(content_size != (size_t)content_size ||
               (size_t)content_size != sout)) {
    elf_uncompress_failed();
    return 0;
  }

  last_block = 0;
  while (!last_block) {
    uint32_t block_hdr;
    int block_type;
    uint32_t block_size;

    if (unlikely(pin + 2 >= pinend)) {
      elf_uncompress_failed();
      return 0;
    }
    block_hdr = ((uint32_t)pin[0] | (((uint32_t)pin[1]) << 8) |
                 (((uint32_t)pin[2]) << 16));
    pin += 3;

    last_block = block_hdr & 1;
    block_type = (block_hdr >> 1) & 3;
    block_size = block_hdr >> 3;

    switch (block_type) {
      case 0:
        /* Raw_Block */
        if (unlikely((size_t)block_size > (size_t)(pinend - pin))) {
          elf_uncompress_failed();
          return 0;
        }
        if (unlikely((size_t)block_size > (size_t)(poutend - pout))) {
          elf_uncompress_failed();
          return 0;
        }
        memcpy(pout, pin, block_size);
        pout += block_size;
        pin += block_size;
        break;

      case 1:
        /* RLE_Block */
        if (unlikely(pin >= pinend)) {
          elf_uncompress_failed();
          return 0;
        }
        if (unlikely((size_t)block_size > (size_t)(poutend - pout))) {
          elf_uncompress_failed();
          return 0;
        }
        memset(pout, *pin, block_size);
        pout += block_size;
        pin++;
        break;

      case 2: {
        const unsigned char *pblockend;
        unsigned char *plitstack;
        unsigned char *plit;
        uint32_t literal_count;
        unsigned char seq_hdr;
        size_t seq_count;
        size_t seq;
        const unsigned char *pback;
        uint64_t val;
        unsigned int bits;
        unsigned int literal_state;
        unsigned int offset_state;
        unsigned int match_state;

        /* Compressed_Block */
        if (unlikely((size_t)block_size > (size_t)(pinend - pin))) {
          elf_uncompress_failed();
          return 0;
        }

        pblockend = pin + block_size;

        /* Read the literals into the end of the output space, and leave
           PLIT pointing at them.  */

        if (!elf_zstd_read_literals(&pin, pblockend, pout, poutend, scratch,
                                    huffman_table, &huffman_table_bits,
                                    &plitstack))
          return 0;
        plit = plitstack;
        literal_count = poutend - plit;

        seq_hdr = *pin;
        pin++;
        if (seq_hdr < 128)
          seq_count = seq_hdr;
        else if (seq_hdr < 255) {
          if (unlikely(pin >= pinend)) {
            elf_uncompress_failed();
            return 0;
          }
          seq_count = ((seq_hdr - 128) << 8) + *pin;
          pin++;
        } else {
          if (unlikely(pin + 1 >= pinend)) {
            elf_uncompress_failed();
            return 0;
          }
          seq_count = *pin + (pin[1] << 8) + 0x7f00;
          pin += 2;
        }

        if (seq_count > 0) {
          int (*pfn)(const struct elf_zstd_fse_entry *, int,
                     struct elf_zstd_fse_baseline_entry *);

          if (unlikely(pin >= pinend)) {
            elf_uncompress_failed();
            return 0;
          }
          seq_hdr = *pin;
          ++pin;

          pfn = elf_zstd_make_literal_baseline_fse;
          if (!elf_zstd_unpack_seq_decode(
                  (seq_hdr >> 6) & 3, &pin, pinend, &elf_zstd_lit_table[0], 6,
                  scratch, 35, literal_fse_table, 9, pfn, &literal_decode))
            return 0;

          pfn = elf_zstd_make_offset_baseline_fse;
          if (!elf_zstd_unpack_seq_decode(
                  (seq_hdr >> 4) & 3, &pin, pinend, &elf_zstd_offset_table[0],
                  5, scratch, 31, offset_fse_table, 8, pfn, &offset_decode))
            return 0;

          pfn = elf_zstd_make_match_baseline_fse;
          if (!elf_zstd_unpack_seq_decode(
                  (seq_hdr >> 2) & 3, &pin, pinend, &elf_zstd_match_table[0], 6,
                  scratch, 52, match_fse_table, 9, pfn, &match_decode))
            return 0;
        }

        pback = pblockend - 1;
        if (!elf_fetch_backward_init(&pback, pin, &val, &bits)) return 0;

        bits -= literal_decode.table_bits;
        literal_state =
            ((val >> bits) & ((1U << literal_decode.table_bits) - 1));

        if (!elf_fetch_bits_backward(&pback, pin, &val, &bits)) return 0;
        bits -= offset_decode.table_bits;
        offset_state = ((val >> bits) & ((1U << offset_decode.table_bits) - 1));

        if (!elf_fetch_bits_backward(&pback, pin, &val, &bits)) return 0;
        bits -= match_decode.table_bits;
        match_state = ((val >> bits) & ((1U << match_decode.table_bits) - 1));

        seq = 0;
        while (1) {
          const struct elf_zstd_fse_baseline_entry *pt;
          uint32_t offset_basebits;
          uint32_t offset_baseline;
          uint32_t offset_bits;
          uint32_t offset_base;
          uint32_t offset;
          uint32_t match_baseline;
          uint32_t match_bits;
          uint32_t match_base;
          uint32_t match;
          uint32_t literal_baseline;
          uint32_t literal_bits;
          uint32_t literal_base;
          uint32_t literal;
          uint32_t need;
          uint32_t add;

          pt = &offset_decode.table[offset_state];
          offset_basebits = pt->basebits;
          offset_baseline = pt->baseline;
          offset_bits = pt->bits;
          offset_base = pt->base;

          /* This case can be more than 16 bits, which is all that
             elf_fetch_bits_backward promises.  */
          need = offset_basebits;
          add = 0;
          if (unlikely(need > 16)) {
            if (!elf_fetch_bits_backward(&pback, pin, &val, &bits)) return 0;
            bits -= 16;
            add = (val >> bits) & ((1U << 16) - 1);
            need -= 16;
            add <<= need;
          }
          if (need > 0) {
            if (!elf_fetch_bits_backward(&pback, pin, &val, &bits)) return 0;
            bits -= need;
            add += (val >> bits) & ((1U << need) - 1);
          }

          offset = offset_baseline + add;

          pt = &match_decode.table[match_state];
          need = pt->basebits;
          match_baseline = pt->baseline;
          match_bits = pt->bits;
          match_base = pt->base;

          add = 0;
          if (need > 0) {
            if (!elf_fetch_bits_backward(&pback, pin, &val, &bits)) return 0;
            bits -= need;
            add = (val >> bits) & ((1U << need) - 1);
          }

          match = match_baseline + add;

          pt = &literal_decode.table[literal_state];
          need = pt->basebits;
          literal_baseline = pt->baseline;
          literal_bits = pt->bits;
          literal_base = pt->base;

          add = 0;
          if (need > 0) {
            if (!elf_fetch_bits_backward(&pback, pin, &val, &bits)) return 0;
            bits -= need;
            add = (val >> bits) & ((1U << need) - 1);
          }

          literal = literal_baseline + add;

          /* See the comment in elf_zstd_make_offset_baseline_fse.  */
          if (offset_basebits > 1) {
            repeated_offset3 = repeated_offset2;
            repeated_offset2 = repeated_offset1;
            repeated_offset1 = offset;
          } else {
            if (unlikely(literal == 0)) ++offset;
            switch (offset) {
              case 1:
                offset = repeated_offset1;
                break;
              case 2:
                offset = repeated_offset2;
                repeated_offset2 = repeated_offset1;
                repeated_offset1 = offset;
                break;
              case 3:
                offset = repeated_offset3;
                repeated_offset3 = repeated_offset2;
                repeated_offset2 = repeated_offset1;
                repeated_offset1 = offset;
                break;
              case 4:
                offset = repeated_offset1 - 1;
                repeated_offset3 = repeated_offset2;
                repeated_offset2 = repeated_offset1;
                repeated_offset1 = offset;
                break;
            }
          }

          ++seq;
          if (seq < seq_count) {
            uint32_t v;

            /* Update the three states.  */

            if (!elf_fetch_bits_backward(&pback, pin, &val, &bits)) return 0;

            need = literal_bits;
            bits -= need;
            v = (val >> bits) & (((uint32_t)1 << need) - 1);

            literal_state = literal_base + v;

            if (!elf_fetch_bits_backward(&pback, pin, &val, &bits)) return 0;

            need = match_bits;
            bits -= need;
            v = (val >> bits) & (((uint32_t)1 << need) - 1);

            match_state = match_base + v;

            if (!elf_fetch_bits_backward(&pback, pin, &val, &bits)) return 0;

            need = offset_bits;
            bits -= need;
            v = (val >> bits) & (((uint32_t)1 << need) - 1);

            offset_state = offset_base + v;
          }

          /* The next sequence is now in LITERAL, OFFSET, MATCH.  */

          /* Copy LITERAL bytes from the literals.  */

          if (unlikely((size_t)(poutend - pout) < literal)) {
            elf_uncompress_failed();
            return 0;
          }

          if (unlikely(literal_count < literal)) {
            elf_uncompress_failed();
            return 0;
          }

          literal_count -= literal;

          /* Often LITERAL is small, so handle small cases quickly.  */
          switch (literal) {
            case 8:
              *pout++ = *plit++;
              // FALLTHROUGH
            case 7:
              *pout++ = *plit++;
              // FALLTHROUGH
            case 6:
              *pout++ = *plit++;
              // FALLTHROUGH
            case 5:
              *pout++ = *plit++;
              // FALLTHROUGH
            case 4:
              *pout++ = *plit++;
              // FALLTHROUGH
            case 3:
              *pout++ = *plit++;
              // FALLTHROUGH
            case 2:
              *pout++ = *plit++;
              // FALLTHROUGH
            case 1:
              *pout++ = *plit++;
              break;

            case 0:
              break;

            default:
              if (unlikely((size_t)(plit - pout) < literal)) {
                uint32_t move;

                move = plit - pout;
                while (literal > move) {
                  memcpy(pout, plit, move);
                  pout += move;
                  plit += move;
                  literal -= move;
                }
              }

              memcpy(pout, plit, literal);
              pout += literal;
              plit += literal;
          }

          if (match > 0) {
            /* Copy MATCH bytes from the decoded output at OFFSET.  */

            if (unlikely((size_t)(poutend - pout) < match)) {
              elf_uncompress_failed();
              return 0;
            }

            if (unlikely((size_t)(pout - poutstart) < offset)) {
              elf_uncompress_failed();
              return 0;
            }

            if (offset >= match) {
              memcpy(pout, pout - offset, match);
              pout += match;
            } else {
              while (match > 0) {
                uint32_t copy;

                copy = match < offset ? match : offset;
                memcpy(pout, pout - offset, copy);
                match -= copy;
                pout += copy;
              }
            }
          }

          if (unlikely(seq >= seq_count)) {
            /* Copy remaining literals.  */
            if (literal_count > 0 && plit != pout) {
              if (unlikely((size_t)(poutend - pout) < literal_count)) {
                elf_uncompress_failed();
                return 0;
              }

              if ((size_t)(plit - pout) < literal_count) {
                uint32_t move;

                move = plit - pout;
                while (literal_count > move) {
                  memcpy(pout, plit, move);
                  pout += move;
                  plit += move;
                  literal_count -= move;
                }
              }

              memcpy(pout, plit, literal_count);
            }

            pout += literal_count;

            break;
          }
        }

        pin = pblockend;
      } break;

      case 3:
      default:
        elf_uncompress_failed();
        return 0;
    }
  }

  if (has_checksum) {
    if (unlikely(pin + 4 > pinend)) {
      elf_uncompress_failed();
      return 0;
    }

    /* We don't currently verify the checksum.  Currently running GNU ld with
       --compress-debug-sections=zstd does not seem to generate a
       checksum.  */

    pin += 4;
  }

  if (pin != pinend) {
    elf_uncompress_failed();
    return 0;
  }

  return 1;
}

#define ZDEBUG_TABLE_SIZE \
  (ZLIB_TABLE_SIZE > ZSTD_TABLE_SIZE ? ZLIB_TABLE_SIZE : ZSTD_TABLE_SIZE)

/* Uncompress the old compressed debug format, the one emitted by
   --compress-debug-sections=zlib-gnu.  The compressed data is in
   COMPRESSED / COMPRESSED_SIZE, and the function writes to
   *UNCOMPRESSED / *UNCOMPRESSED_SIZE.  ZDEBUG_TABLE is work space to
   hold Huffman tables.  Returns 0 on error, 1 on successful
   decompression or if something goes wrong.  In general we try to
   carry on, by returning 1, even if we can't decompress.  */

static int elf_uncompress_zdebug(ten_backtrace_t *self,
                                 const unsigned char *compressed,
                                 size_t compressed_size, uint16_t *zdebug_table,
                                 ten_backtrace_error_func_t error_cb,
                                 void *data, unsigned char **uncompressed,
                                 size_t *uncompressed_size) {
  size_t sz;
  size_t i;
  unsigned char *po;

  *uncompressed = NULL;
  *uncompressed_size = 0;

  /* The format starts with the four bytes ZLIB, followed by the 8
     byte length of the uncompressed data in big-endian order,
     followed by a zlib stream.  */

  if (compressed_size < 12 || memcmp(compressed, "ZLIB", 4) != 0) return 1;

  sz = 0;
  for (i = 0; i < 8; i++) sz = (sz << 8) | compressed[i + 4];

  if (*uncompressed != NULL && *uncompressed_size >= sz)
    po = *uncompressed;
  else {
    po = (unsigned char *)ten_malloc_without_backtrace(sz);
    if (po == NULL) {
      return 0;
    }
  }

  if (!elf_zlib_inflate_and_verify(compressed + 12, compressed_size - 12,
                                   zdebug_table, po, sz)) {
    return 1;
  }

  *uncompressed = po;
  *uncompressed_size = sz;

  return 1;
}

/* Uncompress the new compressed debug format, the official standard
   ELF approach emitted by --compress-debug-sections=zlib-gabi.  The
   compressed data is in COMPRESSED / COMPRESSED_SIZE, and the
   function writes to *UNCOMPRESSED / *UNCOMPRESSED_SIZE.
   ZDEBUG_TABLE is work space as for elf_uncompress_zdebug.  Returns 0
   on error, 1 on successful decompression or if something goes wrong.
   In general we try to carry on, by returning 1, even if we can't
   decompress.  */

static int elf_uncompress_chdr(ten_backtrace_t *self,
                               const unsigned char *compressed,
                               size_t compressed_size, uint16_t *zdebug_table,
                               ten_backtrace_error_func_t error_cb, void *data,
                               unsigned char **uncompressed,
                               size_t *uncompressed_size) {
  const b_elf_chdr *chdr;
  char *alc;
  size_t alc_len;
  unsigned char *po;

  *uncompressed = NULL;
  *uncompressed_size = 0;

  /* The format starts with an ELF compression header.  */
  if (compressed_size < sizeof(b_elf_chdr)) return 1;

  chdr = (const b_elf_chdr *)compressed;

  alc = NULL;
  alc_len = 0;
  if (*uncompressed != NULL && *uncompressed_size >= chdr->ch_size)
    po = *uncompressed;
  else {
    alc_len = chdr->ch_size;
    alc = ten_malloc_without_backtrace(alc_len);
    if (alc == NULL) {
      return 0;
    }
    po = (unsigned char *)alc;
  }

  switch (chdr->ch_type) {
    case ELFCOMPRESS_ZLIB:
      if (!elf_zlib_inflate_and_verify(compressed + sizeof(b_elf_chdr),
                                       compressed_size - sizeof(b_elf_chdr),
                                       zdebug_table, po, chdr->ch_size))
        goto skip;
      break;

    case ELFCOMPRESS_ZSTD:
      if (!elf_zstd_decompress(compressed + sizeof(b_elf_chdr),
                               compressed_size - sizeof(b_elf_chdr),
                               (unsigned char *)zdebug_table, po,
                               chdr->ch_size))
        goto skip;
      break;

    default:
      /* Unsupported compression algorithm.  */
      goto skip;
  }

  *uncompressed = po;
  *uncompressed_size = chdr->ch_size;

  return 1;

skip:
  if (alc != NULL && alc_len > 0) ten_free_without_backtrace(alc);
  return 1;
}

/* Number of LZMA states.  */
#define LZMA_STATES (12)

/* Number of LZMA position states.  The pb value of the property byte
   is the number of bits to include in these states, and the maximum
   value of pb is 4.  */
#define LZMA_POS_STATES (16)

/* Number of LZMA distance states.  These are used match distances
   with a short match length: up to 4 bytes.  */
#define LZMA_DIST_STATES (4)

/* Number of LZMA distance slots.  LZMA uses six bits to encode larger
   match lengths, so 1 << 6 possible probabilities.  */
#define LZMA_DIST_SLOTS (64)

/* LZMA distances 0 to 3 are encoded directly, larger values use a
   probability model.  */
#define LZMA_DIST_MODEL_START (4)

/* The LZMA probability model ends at 14.  */
#define LZMA_DIST_MODEL_END (14)

/* LZMA distance slots for distances less than 127.  */
#define LZMA_FULL_DISTANCES (128)

/* LZMA uses four alignment bits.  */
#define LZMA_ALIGN_SIZE (16)

/* LZMA match length is encoded with 4, 5, or 10 bits, some of which
   are already known.  */
#define LZMA_LEN_LOW_SYMBOLS (8)
#define LZMA_LEN_MID_SYMBOLS (8)
#define LZMA_LEN_HIGH_SYMBOLS (256)

/* LZMA literal encoding.  */
#define LZMA_LITERAL_CODERS_MAX (16)
#define LZMA_LITERAL_CODER_SIZE (0x300)

/* LZMA is based on a large set of probabilities, each managed
   independently.  Each probability is an 11 bit number that we store
   in a uint16_t.  We use a single large array of probabilities.  */

/* Lengths of entries in the LZMA probabilities array.  The names used
   here are copied from the Linux kernel implementation.  */

#define LZMA_PROB_IS_MATCH_LEN (LZMA_STATES * LZMA_POS_STATES)
#define LZMA_PROB_IS_REP_LEN LZMA_STATES
#define LZMA_PROB_IS_REP0_LEN LZMA_STATES
#define LZMA_PROB_IS_REP1_LEN LZMA_STATES
#define LZMA_PROB_IS_REP2_LEN LZMA_STATES
#define LZMA_PROB_IS_REP0_LONG_LEN (LZMA_STATES * LZMA_POS_STATES)
#define LZMA_PROB_DIST_SLOT_LEN (LZMA_DIST_STATES * LZMA_DIST_SLOTS)
#define LZMA_PROB_DIST_SPECIAL_LEN (LZMA_FULL_DISTANCES - LZMA_DIST_MODEL_END)
#define LZMA_PROB_DIST_ALIGN_LEN LZMA_ALIGN_SIZE
#define LZMA_PROB_MATCH_LEN_CHOICE_LEN 1
#define LZMA_PROB_MATCH_LEN_CHOICE2_LEN 1
#define LZMA_PROB_MATCH_LEN_LOW_LEN (LZMA_POS_STATES * LZMA_LEN_LOW_SYMBOLS)
#define LZMA_PROB_MATCH_LEN_MID_LEN (LZMA_POS_STATES * LZMA_LEN_MID_SYMBOLS)
#define LZMA_PROB_MATCH_LEN_HIGH_LEN LZMA_LEN_HIGH_SYMBOLS
#define LZMA_PROB_REP_LEN_CHOICE_LEN 1
#define LZMA_PROB_REP_LEN_CHOICE2_LEN 1
#define LZMA_PROB_REP_LEN_LOW_LEN (LZMA_POS_STATES * LZMA_LEN_LOW_SYMBOLS)
#define LZMA_PROB_REP_LEN_MID_LEN (LZMA_POS_STATES * LZMA_LEN_MID_SYMBOLS)
#define LZMA_PROB_REP_LEN_HIGH_LEN LZMA_LEN_HIGH_SYMBOLS
#define LZMA_PROB_LITERAL_LEN \
  (LZMA_LITERAL_CODERS_MAX * LZMA_LITERAL_CODER_SIZE)

/* Offsets into the LZMA probabilities array.  This is mechanically
   generated from the above lengths.  */

#define LZMA_PROB_IS_MATCH_OFFSET 0
#define LZMA_PROB_IS_REP_OFFSET \
  (LZMA_PROB_IS_MATCH_OFFSET + LZMA_PROB_IS_MATCH_LEN)
#define LZMA_PROB_IS_REP0_OFFSET \
  (LZMA_PROB_IS_REP_OFFSET + LZMA_PROB_IS_REP_LEN)
#define LZMA_PROB_IS_REP1_OFFSET \
  (LZMA_PROB_IS_REP0_OFFSET + LZMA_PROB_IS_REP0_LEN)
#define LZMA_PROB_IS_REP2_OFFSET \
  (LZMA_PROB_IS_REP1_OFFSET + LZMA_PROB_IS_REP1_LEN)
#define LZMA_PROB_IS_REP0_LONG_OFFSET \
  (LZMA_PROB_IS_REP2_OFFSET + LZMA_PROB_IS_REP2_LEN)
#define LZMA_PROB_DIST_SLOT_OFFSET \
  (LZMA_PROB_IS_REP0_LONG_OFFSET + LZMA_PROB_IS_REP0_LONG_LEN)
#define LZMA_PROB_DIST_SPECIAL_OFFSET \
  (LZMA_PROB_DIST_SLOT_OFFSET + LZMA_PROB_DIST_SLOT_LEN)
#define LZMA_PROB_DIST_ALIGN_OFFSET \
  (LZMA_PROB_DIST_SPECIAL_OFFSET + LZMA_PROB_DIST_SPECIAL_LEN)
#define LZMA_PROB_MATCH_LEN_CHOICE_OFFSET \
  (LZMA_PROB_DIST_ALIGN_OFFSET + LZMA_PROB_DIST_ALIGN_LEN)
#define LZMA_PROB_MATCH_LEN_CHOICE2_OFFSET \
  (LZMA_PROB_MATCH_LEN_CHOICE_OFFSET + LZMA_PROB_MATCH_LEN_CHOICE_LEN)
#define LZMA_PROB_MATCH_LEN_LOW_OFFSET \
  (LZMA_PROB_MATCH_LEN_CHOICE2_OFFSET + LZMA_PROB_MATCH_LEN_CHOICE2_LEN)
#define LZMA_PROB_MATCH_LEN_MID_OFFSET \
  (LZMA_PROB_MATCH_LEN_LOW_OFFSET + LZMA_PROB_MATCH_LEN_LOW_LEN)
#define LZMA_PROB_MATCH_LEN_HIGH_OFFSET \
  (LZMA_PROB_MATCH_LEN_MID_OFFSET + LZMA_PROB_MATCH_LEN_MID_LEN)
#define LZMA_PROB_REP_LEN_CHOICE_OFFSET \
  (LZMA_PROB_MATCH_LEN_HIGH_OFFSET + LZMA_PROB_MATCH_LEN_HIGH_LEN)
#define LZMA_PROB_REP_LEN_CHOICE2_OFFSET \
  (LZMA_PROB_REP_LEN_CHOICE_OFFSET + LZMA_PROB_REP_LEN_CHOICE_LEN)
#define LZMA_PROB_REP_LEN_LOW_OFFSET \
  (LZMA_PROB_REP_LEN_CHOICE2_OFFSET + LZMA_PROB_REP_LEN_CHOICE2_LEN)
#define LZMA_PROB_REP_LEN_MID_OFFSET \
  (LZMA_PROB_REP_LEN_LOW_OFFSET + LZMA_PROB_REP_LEN_LOW_LEN)
#define LZMA_PROB_REP_LEN_HIGH_OFFSET \
  (LZMA_PROB_REP_LEN_MID_OFFSET + LZMA_PROB_REP_LEN_MID_LEN)
#define LZMA_PROB_LITERAL_OFFSET \
  (LZMA_PROB_REP_LEN_HIGH_OFFSET + LZMA_PROB_REP_LEN_HIGH_LEN)

#define LZMA_PROB_TOTAL_COUNT (LZMA_PROB_LITERAL_OFFSET + LZMA_PROB_LITERAL_LEN)

/* Check that the number of LZMA probabilities is the same as the
   Linux kernel implementation.  */

#if LZMA_PROB_TOTAL_COUNT != 1846 + (1 << 4) * 0x300
#error Wrong number of LZMA probabilities
#endif

/* Expressions for the offset in the LZMA probabilities array of a
   specific probability.  */

#define LZMA_IS_MATCH(state, pos) \
  (LZMA_PROB_IS_MATCH_OFFSET + (state) * LZMA_POS_STATES + (pos))
#define LZMA_IS_REP(state) (LZMA_PROB_IS_REP_OFFSET + (state))
#define LZMA_IS_REP0(state) (LZMA_PROB_IS_REP0_OFFSET + (state))
#define LZMA_IS_REP1(state) (LZMA_PROB_IS_REP1_OFFSET + (state))
#define LZMA_IS_REP2(state) (LZMA_PROB_IS_REP2_OFFSET + (state))
#define LZMA_IS_REP0_LONG(state, pos) \
  (LZMA_PROB_IS_REP0_LONG_OFFSET + (state) * LZMA_POS_STATES + (pos))
#define LZMA_DIST_SLOT(dist, slot) \
  (LZMA_PROB_DIST_SLOT_OFFSET + (dist) * LZMA_DIST_SLOTS + (slot))
#define LZMA_DIST_SPECIAL(dist) (LZMA_PROB_DIST_SPECIAL_OFFSET + (dist))
#define LZMA_DIST_ALIGN(dist) (LZMA_PROB_DIST_ALIGN_OFFSET + (dist))
#define LZMA_MATCH_LEN_CHOICE LZMA_PROB_MATCH_LEN_CHOICE_OFFSET
#define LZMA_MATCH_LEN_CHOICE2 LZMA_PROB_MATCH_LEN_CHOICE2_OFFSET
#define LZMA_MATCH_LEN_LOW(pos, sym) \
  (LZMA_PROB_MATCH_LEN_LOW_OFFSET + (pos) * LZMA_LEN_LOW_SYMBOLS + (sym))
#define LZMA_MATCH_LEN_MID(pos, sym) \
  (LZMA_PROB_MATCH_LEN_MID_OFFSET + (pos) * LZMA_LEN_MID_SYMBOLS + (sym))
#define LZMA_MATCH_LEN_HIGH(sym) (LZMA_PROB_MATCH_LEN_HIGH_OFFSET + (sym))
#define LZMA_REP_LEN_CHOICE LZMA_PROB_REP_LEN_CHOICE_OFFSET
#define LZMA_REP_LEN_CHOICE2 LZMA_PROB_REP_LEN_CHOICE2_OFFSET
#define LZMA_REP_LEN_LOW(pos, sym) \
  (LZMA_PROB_REP_LEN_LOW_OFFSET + (pos) * LZMA_LEN_LOW_SYMBOLS + (sym))
#define LZMA_REP_LEN_MID(pos, sym) \
  (LZMA_PROB_REP_LEN_MID_OFFSET + (pos) * LZMA_LEN_MID_SYMBOLS + (sym))
#define LZMA_REP_LEN_HIGH(sym) (LZMA_PROB_REP_LEN_HIGH_OFFSET + (sym))
#define LZMA_LITERAL(code, size) \
  (LZMA_PROB_LITERAL_OFFSET + (code) * LZMA_LITERAL_CODER_SIZE + (size))

/* Read an LZMA varint from BUF, reading and updating *POFFSET,
   setting *VAL.  Returns 0 on error, 1 on success.  */

static int elf_lzma_varint(const unsigned char *compressed,
                           size_t compressed_size, size_t *poffset,
                           uint64_t *val) {
  size_t off;
  int i;
  uint64_t v;
  unsigned char b;

  off = *poffset;
  i = 0;
  v = 0;
  while (1) {
    if (unlikely(off >= compressed_size)) {
      elf_uncompress_failed();
      return 0;
    }
    b = compressed[off];
    v |= (b & 0x7f) << (i * 7);
    ++off;
    if ((b & 0x80) == 0) {
      *poffset = off;
      *val = v;
      return 1;
    }
    ++i;
    if (unlikely(i >= 9)) {
      elf_uncompress_failed();
      return 0;
    }
  }
}

/* Normalize the LZMA range decoder, pulling in an extra input byte if
   needed.  */

static void elf_lzma_range_normalize(const unsigned char *compressed,
                                     size_t compressed_size, size_t *poffset,
                                     uint32_t *prange, uint32_t *pcode) {
  if (*prange < (1U << 24)) {
    if (unlikely(*poffset >= compressed_size)) {
      /* We assume this will be caught elsewhere.  */
      elf_uncompress_failed();
      return;
    }
    *prange <<= 8;
    *pcode <<= 8;
    *pcode += compressed[*poffset];
    ++*poffset;
  }
}

/* Read and return a single bit from the LZMA stream, reading and
   updating *PROB.  Each bit comes from the range coder.  */

static int elf_lzma_bit(const unsigned char *compressed, size_t compressed_size,
                        uint16_t *prob, size_t *poffset, uint32_t *prange,
                        uint32_t *pcode) {
  uint32_t bound;

  elf_lzma_range_normalize(compressed, compressed_size, poffset, prange, pcode);
  bound = (*prange >> 11) * (uint32_t)*prob;
  if (*pcode < bound) {
    *prange = bound;
    *prob += ((1U << 11) - *prob) >> 5;
    return 0;
  } else {
    *prange -= bound;
    *pcode -= bound;
    *prob -= *prob >> 5;
    return 1;
  }
}

/* Read an integer of size BITS from the LZMA stream, most significant
   bit first.  The bits are predicted using PROBS.  */

static uint32_t elf_lzma_integer(const unsigned char *compressed,
                                 size_t compressed_size, uint16_t *probs,
                                 uint32_t bits, size_t *poffset,
                                 uint32_t *prange, uint32_t *pcode) {
  uint32_t sym;
  uint32_t i;

  sym = 1;
  for (i = 0; i < bits; i++) {
    int bit;

    bit = elf_lzma_bit(compressed, compressed_size, probs + sym, poffset,
                       prange, pcode);
    sym <<= 1;
    sym += bit;
  }
  return sym - (1 << bits);
}

/* Read an integer of size BITS from the LZMA stream, least
   significant bit first.  The bits are predicted using PROBS.  */

static uint32_t elf_lzma_reverse_integer(const unsigned char *compressed,
                                         size_t compressed_size,
                                         uint16_t *probs, uint32_t bits,
                                         size_t *poffset, uint32_t *prange,
                                         uint32_t *pcode) {
  uint32_t sym;
  uint32_t val;
  uint32_t i;

  sym = 1;
  val = 0;
  for (i = 0; i < bits; i++) {
    int bit;

    bit = elf_lzma_bit(compressed, compressed_size, probs + sym, poffset,
                       prange, pcode);
    sym <<= 1;
    sym += bit;
    val += bit << i;
  }
  return val;
}

/* Read a length from the LZMA stream.  IS_REP picks either LZMA_MATCH
   or LZMA_REP probabilities.  */

static uint32_t elf_lzma_len(const unsigned char *compressed,
                             size_t compressed_size, uint16_t *probs,
                             int is_rep, unsigned int pos_state,
                             size_t *poffset, uint32_t *prange,
                             uint32_t *pcode) {
  uint16_t *probs_choice;
  uint16_t *probs_sym;
  uint32_t bits;
  uint32_t len;

  probs_choice = probs + (is_rep ? LZMA_REP_LEN_CHOICE : LZMA_MATCH_LEN_CHOICE);
  if (elf_lzma_bit(compressed, compressed_size, probs_choice, poffset, prange,
                   pcode)) {
    probs_choice =
        probs + (is_rep ? LZMA_REP_LEN_CHOICE2 : LZMA_MATCH_LEN_CHOICE2);
    if (elf_lzma_bit(compressed, compressed_size, probs_choice, poffset, prange,
                     pcode)) {
      probs_sym =
          probs + (is_rep ? LZMA_REP_LEN_HIGH(0) : LZMA_MATCH_LEN_HIGH(0));
      bits = 8;
      len = 2 + 8 + 8;
    } else {
      probs_sym = probs + (is_rep ? LZMA_REP_LEN_MID(pos_state, 0)
                                  : LZMA_MATCH_LEN_MID(pos_state, 0));
      bits = 3;
      len = 2 + 8;
    }
  } else {
    probs_sym = probs + (is_rep ? LZMA_REP_LEN_LOW(pos_state, 0)
                                : LZMA_MATCH_LEN_LOW(pos_state, 0));
    bits = 3;
    len = 2;
  }

  len += elf_lzma_integer(compressed, compressed_size, probs_sym, bits, poffset,
                          prange, pcode);
  return len;
}

/* Uncompress one LZMA block from a minidebug file.  The compressed
   data is at COMPRESSED + *POFFSET.  Update *POFFSET.  Store the data
   into the memory at UNCOMPRESSED, size UNCOMPRESSED_SIZE.  CHECK is
   the stream flag from the xz header.  Return 1 on successful
   decompression.  */

static int elf_uncompress_lzma_block(const unsigned char *compressed,
                                     size_t compressed_size,
                                     unsigned char check, uint16_t *probs,
                                     unsigned char *uncompressed,
                                     size_t uncompressed_size,
                                     size_t *poffset) {
  size_t off;
  size_t block_header_offset;
  size_t block_header_size;
  unsigned char block_flags;
  uint64_t header_compressed_size;
  uint64_t header_uncompressed_size;
  unsigned char lzma2_properties;
  uint32_t computed_crc;
  uint32_t stream_crc;
  size_t uncompressed_offset;
  size_t dict_start_offset;
  unsigned int lc;
  unsigned int lp;
  unsigned int pb;
  uint32_t range;
  uint32_t code;
  uint32_t lstate;
  uint32_t dist[4];

  off = *poffset;
  block_header_offset = off;

  /* Block header size is a single byte.  */
  if (unlikely(off >= compressed_size)) {
    elf_uncompress_failed();
    return 0;
  }
  block_header_size = (compressed[off] + 1) * 4;
  if (unlikely(off + block_header_size > compressed_size)) {
    elf_uncompress_failed();
    return 0;
  }

  /* Block flags.  */
  block_flags = compressed[off + 1];
  if (unlikely((block_flags & 0x3c) != 0)) {
    elf_uncompress_failed();
    return 0;
  }

  off += 2;

  /* Optional compressed size.  */
  header_compressed_size = 0;
  if ((block_flags & 0x40) != 0) {
    *poffset = off;
    if (!elf_lzma_varint(compressed, compressed_size, poffset,
                         &header_compressed_size))
      return 0;
    off = *poffset;
  }

  /* Optional uncompressed size.  */
  header_uncompressed_size = 0;
  if ((block_flags & 0x80) != 0) {
    *poffset = off;
    if (!elf_lzma_varint(compressed, compressed_size, poffset,
                         &header_uncompressed_size))
      return 0;
    off = *poffset;
  }

  /* The recipe for creating a minidebug file is to run the xz program
     with no arguments, so we expect exactly one filter: lzma2.  */

  if (unlikely((block_flags & 0x3) != 0)) {
    elf_uncompress_failed();
    return 0;
  }

  if (unlikely(off + 2 >= block_header_offset + block_header_size)) {
    elf_uncompress_failed();
    return 0;
  }

  /* The filter ID for LZMA2 is 0x21.  */
  if (unlikely(compressed[off] != 0x21)) {
    elf_uncompress_failed();
    return 0;
  }
  ++off;

  /* The size of the filter properties for LZMA2 is 1.  */
  if (unlikely(compressed[off] != 1)) {
    elf_uncompress_failed();
    return 0;
  }
  ++off;

  lzma2_properties = compressed[off];
  ++off;

  if (unlikely(lzma2_properties > 40)) {
    elf_uncompress_failed();
    return 0;
  }

  /* The properties describe the dictionary size, but we don't care
     what that is.  */

  /* Block header padding.  */
  if (unlikely(off + 4 > compressed_size)) {
    elf_uncompress_failed();
    return 0;
  }

  off = (off + 3) & ~(size_t)3;

  if (unlikely(off + 4 > compressed_size)) {
    elf_uncompress_failed();
    return 0;
  }

  /* Block header CRC.  */
  computed_crc =
      elf_crc32(0, compressed + block_header_offset, block_header_size - 4);
  stream_crc = (compressed[off] | (compressed[off + 1] << 8) |
                (compressed[off + 2] << 16) | (compressed[off + 3] << 24));
  if (unlikely(computed_crc != stream_crc)) {
    elf_uncompress_failed();
    return 0;
  }
  off += 4;

  /* Read a sequence of LZMA2 packets.  */

  uncompressed_offset = 0;
  dict_start_offset = 0;
  lc = 0;
  lp = 0;
  pb = 0;
  lstate = 0;
  while (off < compressed_size) {
    unsigned char control;

    range = 0xffffffff;
    code = 0;

    control = compressed[off];
    ++off;
    if (unlikely(control == 0)) {
      /* End of packets.  */
      break;
    }

    if (control == 1 || control >= 0xe0) {
      /* Reset dictionary to empty.  */
      dict_start_offset = uncompressed_offset;
    }

    if (control < 0x80) {
      size_t chunk_size;

      /* The only valid values here are 1 or 2.  A 1 means to
         reset the dictionary (done above).  Then we see an
         uncompressed chunk.  */

      if (unlikely(control > 2)) {
        elf_uncompress_failed();
        return 0;
      }

      /* An uncompressed chunk is a two byte size followed by
         data.  */

      if (unlikely(off + 2 > compressed_size)) {
        elf_uncompress_failed();
        return 0;
      }

      chunk_size = compressed[off] << 8;
      chunk_size += compressed[off + 1];
      ++chunk_size;

      off += 2;

      if (unlikely(off + chunk_size > compressed_size)) {
        elf_uncompress_failed();
        return 0;
      }
      if (unlikely(uncompressed_offset + chunk_size > uncompressed_size)) {
        elf_uncompress_failed();
        return 0;
      }

      memcpy(uncompressed + uncompressed_offset, compressed + off, chunk_size);
      uncompressed_offset += chunk_size;
      off += chunk_size;
    } else {
      size_t uncompressed_chunk_start;
      size_t uncompressed_chunk_size;
      size_t compressed_chunk_size;
      size_t limit;

      /* An LZMA chunk.  This starts with an uncompressed size and
         a compressed size.  */

      if (unlikely(off + 4 >= compressed_size)) {
        elf_uncompress_failed();
        return 0;
      }

      uncompressed_chunk_start = uncompressed_offset;

      uncompressed_chunk_size = (control & 0x1f) << 16;
      uncompressed_chunk_size += compressed[off] << 8;
      uncompressed_chunk_size += compressed[off + 1];
      ++uncompressed_chunk_size;

      compressed_chunk_size = compressed[off + 2] << 8;
      compressed_chunk_size += compressed[off + 3];
      ++compressed_chunk_size;

      off += 4;

      /* Bit 7 (0x80) is set.
         Bits 6 and 5 (0x40 and 0x20) are as follows:
         0: don't reset anything
         1: reset state
         2: reset state, read properties
         3: reset state, read properties, reset dictionary (done above) */

      if (control >= 0xc0) {
        unsigned char props;

        /* Bit 6 is set, read properties.  */

        if (unlikely(off >= compressed_size)) {
          elf_uncompress_failed();
          return 0;
        }
        props = compressed[off];
        ++off;
        if (unlikely(props > (4 * 5 + 4) * 9 + 8)) {
          elf_uncompress_failed();
          return 0;
        }
        pb = 0;
        while (props >= 9 * 5) {
          props -= 9 * 5;
          ++pb;
        }
        lp = 0;
        while (props > 9) {
          props -= 9;
          ++lp;
        }
        lc = props;
        if (unlikely(lc + lp > 4)) {
          elf_uncompress_failed();
          return 0;
        }
      }

      if (control >= 0xa0) {
        size_t i;

        /* Bit 5 or 6 is set, reset LZMA state.  */

        lstate = 0;
        memset(&dist, 0, sizeof dist);
        for (i = 0; i < LZMA_PROB_TOTAL_COUNT; i++) probs[i] = 1 << 10;
        range = 0xffffffff;
        code = 0;
      }

      /* Read the range code.  */

      if (unlikely(off + 5 > compressed_size)) {
        elf_uncompress_failed();
        return 0;
      }

      /* The byte at compressed[off] is ignored for some
         reason.  */

      code = ((compressed[off + 1] << 24) + (compressed[off + 2] << 16) +
              (compressed[off + 3] << 8) + compressed[off + 4]);
      off += 5;

      /* This is the main LZMA decode loop.  */

      limit = off + compressed_chunk_size;
      *poffset = off;
      while (*poffset < limit) {
        unsigned int pos_state;

        if (unlikely(uncompressed_offset ==
                     (uncompressed_chunk_start + uncompressed_chunk_size))) {
          /* We've decompressed all the expected bytes.  */
          break;
        }

        pos_state =
            ((uncompressed_offset - dict_start_offset) & ((1 << pb) - 1));

        if (elf_lzma_bit(compressed, compressed_size,
                         probs + LZMA_IS_MATCH(lstate, pos_state), poffset,
                         &range, &code)) {
          uint32_t len;

          if (elf_lzma_bit(compressed, compressed_size,
                           probs + LZMA_IS_REP(lstate), poffset, &range,
                           &code)) {
            int short_rep;
            uint32_t next_dist;

            /* Repeated match.  */

            short_rep = 0;
            if (elf_lzma_bit(compressed, compressed_size,
                             probs + LZMA_IS_REP0(lstate), poffset, &range,
                             &code)) {
              if (elf_lzma_bit(compressed, compressed_size,
                               probs + LZMA_IS_REP1(lstate), poffset, &range,
                               &code)) {
                if (elf_lzma_bit(compressed, compressed_size,
                                 probs + LZMA_IS_REP2(lstate), poffset, &range,
                                 &code)) {
                  next_dist = dist[3];
                  dist[3] = dist[2];
                } else {
                  next_dist = dist[2];
                }
                dist[2] = dist[1];
              } else {
                next_dist = dist[1];
              }

              dist[1] = dist[0];
              dist[0] = next_dist;
            } else {
              if (!elf_lzma_bit(compressed, compressed_size,
                                (probs + LZMA_IS_REP0_LONG(lstate, pos_state)),
                                poffset, &range, &code))
                short_rep = 1;
            }

            if (lstate < 7)
              lstate = short_rep ? 9 : 8;
            else
              lstate = 11;

            if (short_rep)
              len = 1;
            else
              len = elf_lzma_len(compressed, compressed_size, probs, 1,
                                 pos_state, poffset, &range, &code);
          } else {
            uint32_t dist_state;
            uint32_t dist_slot;
            uint16_t *probs_dist;

            /* Match.  */

            if (lstate < 7)
              lstate = 7;
            else
              lstate = 10;
            dist[3] = dist[2];
            dist[2] = dist[1];
            dist[1] = dist[0];
            len = elf_lzma_len(compressed, compressed_size, probs, 0, pos_state,
                               poffset, &range, &code);

            if (len < 4 + 2)
              dist_state = len - 2;
            else
              dist_state = 3;
            probs_dist = probs + LZMA_DIST_SLOT(dist_state, 0);
            dist_slot = elf_lzma_integer(compressed, compressed_size,
                                         probs_dist, 6, poffset, &range, &code);
            if (dist_slot < LZMA_DIST_MODEL_START)
              dist[0] = dist_slot;
            else {
              uint32_t limit;

              limit = (dist_slot >> 1) - 1;
              dist[0] = 2 + (dist_slot & 1);
              if (dist_slot < LZMA_DIST_MODEL_END) {
                dist[0] <<= limit;
                probs_dist =
                    (probs + LZMA_DIST_SPECIAL(dist[0] - dist_slot - 1));
                dist[0] += elf_lzma_reverse_integer(compressed, compressed_size,
                                                    probs_dist, limit, poffset,
                                                    &range, &code);
              } else {
                uint32_t dist0;
                uint32_t i;

                dist0 = dist[0];
                for (i = 0; i < limit - 4; i++) {
                  uint32_t mask;

                  elf_lzma_range_normalize(compressed, compressed_size, poffset,
                                           &range, &code);
                  range >>= 1;
                  code -= range;
                  mask = -(code >> 31);
                  code += range & mask;
                  dist0 <<= 1;
                  dist0 += mask + 1;
                }
                dist0 <<= 4;
                probs_dist = probs + LZMA_DIST_ALIGN(0);
                dist0 += elf_lzma_reverse_integer(compressed, compressed_size,
                                                  probs_dist, 4, poffset,
                                                  &range, &code);
                dist[0] = dist0;
              }
            }
          }

          if (unlikely(uncompressed_offset - dict_start_offset < dist[0] + 1)) {
            elf_uncompress_failed();
            return 0;
          }
          if (unlikely(uncompressed_offset + len > uncompressed_size)) {
            elf_uncompress_failed();
            return 0;
          }

          if (dist[0] == 0) {
            /* A common case, meaning repeat the last
               character LEN times.  */
            memset(uncompressed + uncompressed_offset,
                   uncompressed[uncompressed_offset - 1], len);
            uncompressed_offset += len;
          } else if (dist[0] + 1 >= len) {
            memcpy(uncompressed + uncompressed_offset,
                   uncompressed + uncompressed_offset - dist[0] - 1, len);
            uncompressed_offset += len;
          } else {
            while (len > 0) {
              uint32_t copy;

              copy = len < dist[0] + 1 ? len : dist[0] + 1;
              memcpy(uncompressed + uncompressed_offset,
                     (uncompressed + uncompressed_offset - dist[0] - 1), copy);
              len -= copy;
              uncompressed_offset += copy;
            }
          }
        } else {
          unsigned char prev;
          unsigned char low;
          size_t high;
          uint16_t *lit_probs;
          unsigned int sym;

          /* Literal value.  */

          if (uncompressed_offset > 0)
            prev = uncompressed[uncompressed_offset - 1];
          else
            prev = 0;
          low = prev >> (8 - lc);
          high = (((uncompressed_offset - dict_start_offset) & ((1 << lp) - 1))
                  << lc);
          lit_probs = probs + LZMA_LITERAL(low + high, 0);
          if (lstate < 7)
            sym = elf_lzma_integer(compressed, compressed_size, lit_probs, 8,
                                   poffset, &range, &code);
          else {
            unsigned int match;
            unsigned int bit;
            unsigned int match_bit;
            unsigned int idx;

            sym = 1;
            if (uncompressed_offset >= dist[0] + 1)
              match = uncompressed[uncompressed_offset - dist[0] - 1];
            else
              match = 0;
            match <<= 1;
            bit = 0x100;
            do {
              match_bit = match & bit;
              match <<= 1;
              idx = bit + match_bit + sym;
              sym <<= 1;
              if (elf_lzma_bit(compressed, compressed_size, lit_probs + idx,
                               poffset, &range, &code)) {
                ++sym;
                bit &= match_bit;
              } else {
                bit &= ~match_bit;
              }
            } while (sym < 0x100);
          }

          if (unlikely(uncompressed_offset >= uncompressed_size)) {
            elf_uncompress_failed();
            return 0;
          }

          uncompressed[uncompressed_offset] = (unsigned char)sym;
          ++uncompressed_offset;
          if (lstate <= 3)
            lstate = 0;
          else if (lstate <= 9)
            lstate -= 3;
          else
            lstate -= 6;
        }
      }

      elf_lzma_range_normalize(compressed, compressed_size, poffset, &range,
                               &code);

      off = *poffset;
    }
  }

  /* We have reached the end of the block.  Pad to four byte
     boundary.  */
  off = (off + 3) & ~(size_t)3;
  if (unlikely(off > compressed_size)) {
    elf_uncompress_failed();
    return 0;
  }

  switch (check) {
    case 0:
      /* No check.  */
      break;

    case 1:
      /* CRC32 */
      if (unlikely(off + 4 > compressed_size)) {
        elf_uncompress_failed();
        return 0;
      }
      computed_crc = elf_crc32(0, uncompressed, uncompressed_offset);
      stream_crc = (compressed[off] | (compressed[off + 1] << 8) |
                    (compressed[off + 2] << 16) | (compressed[off + 3] << 24));
      if (computed_crc != stream_crc) {
        elf_uncompress_failed();
        return 0;
      }
      off += 4;
      break;

    case 4:
      /* CRC64.  We don't bother computing a CRC64 checksum.  */
      if (unlikely(off + 8 > compressed_size)) {
        elf_uncompress_failed();
        return 0;
      }
      off += 8;
      break;

    case 10:
      /* SHA.  We don't bother computing a SHA checksum.  */
      if (unlikely(off + 32 > compressed_size)) {
        elf_uncompress_failed();
        return 0;
      }
      off += 32;
      break;

    default:
      elf_uncompress_failed();
      return 0;
  }

  *poffset = off;

  return 1;
}

/* Uncompress LZMA data found in a minidebug file.  The minidebug
   format is described at
   https://sourceware.org/gdb/current/onlinedocs/gdb/MiniDebugInfo.html.
   Returns 0 on error, 1 on successful decompression.  For this
   function we return 0 on failure to decompress, as the calling code
   will carry on in that case.  */

static int elf_uncompress_lzma(ten_backtrace_t *self,
                               const unsigned char *compressed,
                               size_t compressed_size,
                               ten_backtrace_error_func_t error_cb, void *data,
                               unsigned char **uncompressed,
                               size_t *uncompressed_size) {
  size_t header_size;
  size_t footer_size;
  unsigned char check;
  uint32_t computed_crc;
  uint32_t stream_crc;
  size_t offset;
  size_t index_size;
  size_t footer_offset;
  size_t index_offset;
  uint64_t index_compressed_size;
  uint64_t index_uncompressed_size;
  unsigned char *mem;
  uint16_t *probs;
  size_t compressed_block_size;

  /* The format starts with a stream header and ends with a stream
     footer.  */
  header_size = 12;
  footer_size = 12;
  if (unlikely(compressed_size < header_size + footer_size)) {
    elf_uncompress_failed();
    return 0;
  }

  /* The stream header starts with a magic string.  */
  if (unlikely(memcmp(compressed,
                      "\375"
                      "7zXZ\0",
                      6) != 0)) {
    elf_uncompress_failed();
    return 0;
  }

  /* Next come stream flags.  The first byte is zero, the second byte
     is the check.  */
  if (unlikely(compressed[6] != 0)) {
    elf_uncompress_failed();
    return 0;
  }
  check = compressed[7];
  if (unlikely((check & 0xf8) != 0)) {
    elf_uncompress_failed();
    return 0;
  }

  /* Next comes a CRC of the stream flags.  */
  computed_crc = elf_crc32(0, compressed + 6, 2);
  stream_crc = (compressed[8] | (compressed[9] << 8) | (compressed[10] << 16) |
                (compressed[11] << 24));
  if (unlikely(computed_crc != stream_crc)) {
    elf_uncompress_failed();
    return 0;
  }

  /* Now that we've parsed the header, parse the footer, so that we
     can get the uncompressed size.  */

  /* The footer ends with two magic bytes.  */

  offset = compressed_size;
  if (unlikely(memcmp(compressed + offset - 2, "YZ", 2) != 0)) {
    elf_uncompress_failed();
    return 0;
  }
  offset -= 2;

  /* Before that are the stream flags, which should be the same as the
     flags in the header.  */
  if (unlikely(compressed[offset - 2] != 0 ||
               compressed[offset - 1] != check)) {
    elf_uncompress_failed();
    return 0;
  }
  offset -= 2;

  /* Before that is the size of the index field, which precedes the
     footer.  */
  index_size =
      (compressed[offset - 4] | (compressed[offset - 3] << 8) |
       (compressed[offset - 2] << 16) | (compressed[offset - 1] << 24));
  index_size = (index_size + 1) * 4;
  offset -= 4;

  /* Before that is a footer CRC.  */
  computed_crc = elf_crc32(0, compressed + offset, 6);
  stream_crc =
      (compressed[offset - 4] | (compressed[offset - 3] << 8) |
       (compressed[offset - 2] << 16) | (compressed[offset - 1] << 24));
  if (unlikely(computed_crc != stream_crc)) {
    elf_uncompress_failed();
    return 0;
  }
  offset -= 4;

  /* The index comes just before the footer.  */
  if (unlikely(offset < index_size + header_size)) {
    elf_uncompress_failed();
    return 0;
  }

  footer_offset = offset;
  offset -= index_size;
  index_offset = offset;

  /* The index starts with a zero byte.  */
  if (unlikely(compressed[offset] != 0)) {
    elf_uncompress_failed();
    return 0;
  }
  ++offset;

  /* Next is the number of blocks.  We expect zero blocks for an empty
     stream, and otherwise a single block.  */
  if (unlikely(compressed[offset] == 0)) {
    *uncompressed = NULL;
    *uncompressed_size = 0;
    return 1;
  }
  if (unlikely(compressed[offset] != 1)) {
    elf_uncompress_failed();
    return 0;
  }
  ++offset;

  /* Next is the compressed size and the uncompressed size.  */
  if (!elf_lzma_varint(compressed, compressed_size, &offset,
                       &index_compressed_size))
    return 0;
  if (!elf_lzma_varint(compressed, compressed_size, &offset,
                       &index_uncompressed_size))
    return 0;

  /* Pad to a four byte boundary.  */
  offset = (offset + 3) & ~(size_t)3;

  /* Next is a CRC of the index.  */
  computed_crc = elf_crc32(0, compressed + index_offset, offset - index_offset);
  stream_crc =
      (compressed[offset] | (compressed[offset + 1] << 8) |
       (compressed[offset + 2] << 16) | (compressed[offset + 3] << 24));
  if (unlikely(computed_crc != stream_crc)) {
    elf_uncompress_failed();
    return 0;
  }
  offset += 4;

  /* We should now be back at the footer.  */
  if (unlikely(offset != footer_offset)) {
    elf_uncompress_failed();
    return 0;
  }

  /* Allocate space to hold the uncompressed data.  If we succeed in
     uncompressing the LZMA data, we never free this memory.  */
  mem = (unsigned char *)ten_malloc_without_backtrace(index_uncompressed_size);
  if (unlikely(mem == NULL)) {
    return 0;
  }

  *uncompressed = mem;
  *uncompressed_size = index_uncompressed_size;

  /* Allocate space for probabilities.  */
  probs = (uint16_t *)ten_malloc_without_backtrace(LZMA_PROB_TOTAL_COUNT *
                                                   sizeof(uint16_t));
  if (unlikely(probs == NULL)) {
    ten_free_without_backtrace(mem);
    return 0;
  }

  /* Uncompress the block, which follows the header.  */
  offset = 12;
  if (!elf_uncompress_lzma_block(compressed, compressed_size, check, probs, mem,
                                 index_uncompressed_size, &offset)) {
    ten_free_without_backtrace(mem);
    return 0;
  }

  compressed_block_size = offset - 12;
  if (unlikely(compressed_block_size !=
               ((index_compressed_size + 3) & ~(size_t)3))) {
    elf_uncompress_failed();
    ten_free_without_backtrace(mem);
    return 0;
  }

  offset = (offset + 3) & ~(size_t)3;
  if (unlikely(offset != index_offset)) {
    elf_uncompress_failed();
    ten_free_without_backtrace(mem);
    return 0;
  }

  return 1;
}

/**
 * @brief Add the backtrace data for one ELF file.
 *
 * @return 1 on success, 0 on failure (in both cases descriptor is closed) or -1
 * if exe is non-zero and the ELF file is ET_DYN, which tells the caller that
 * elf_add will need to be called on the descriptor again after base_address is
 * determined.
 */
static int elf_add(ten_backtrace_t *self, const char *filename, int descriptor,
                   const unsigned char *memory, size_t memory_size,
                   uintptr_t base_address, ten_backtrace_error_func_t error_cb,
                   void *data, ten_backtrace_get_file_line_func_t *fileline_fn,
                   int *found_sym, int *found_dwarf,
                   struct dwarf_data **fileline_entry, int exe, int debuginfo,
                   const char *with_buildid_data, uint32_t with_buildid_size) {
  struct elf_view ehdr_view;
  b_elf_ehdr ehdr;
  off_t shoff;
  unsigned int shnum;
  unsigned int shstrndx;
  struct elf_view shdrs_view;
  int shdrs_view_valid;
  const b_elf_shdr *shdrs;
  const b_elf_shdr *shstrhdr;
  size_t shstr_size;
  off_t shstr_off;
  struct elf_view names_view;
  int names_view_valid;
  const char *names;
  unsigned int symtab_shndx;
  unsigned int dynsym_shndx;
  unsigned int i;
  struct debug_section_info sections[DEBUG_MAX];
  struct debug_section_info zsections[DEBUG_MAX];
  struct elf_view symtab_view;
  int symtab_view_valid;
  struct elf_view strtab_view;
  int strtab_view_valid;
  struct elf_view buildid_view;
  int buildid_view_valid;
  const char *buildid_data;
  uint32_t buildid_size;
  struct elf_view debuglink_view;
  int debuglink_view_valid;
  const char *debuglink_name;
  uint32_t debuglink_crc;
  struct elf_view debugaltlink_view;
  int debugaltlink_view_valid;
  const char *debugaltlink_name;
  const char *debugaltlink_buildid_data;
  uint32_t debugaltlink_buildid_size;
  struct elf_view gnu_debugdata_view;
  int gnu_debugdata_view_valid;
  size_t gnu_debugdata_size;
  unsigned char *gnu_debugdata_uncompressed;
  size_t gnu_debugdata_uncompressed_size;
  off_t min_offset;
  off_t max_offset;
  off_t debug_size;
  struct elf_view debug_view;
  int debug_view_valid;
  unsigned int using_debug_view;
  uint16_t *zdebug_table;
  struct elf_view split_debug_view[DEBUG_MAX];
  unsigned char split_debug_view_valid[DEBUG_MAX];
  struct elf_ppc64_opd_data opd_data, *opd;
  struct dwarf_sections dwarf_sections;

  if (!debuginfo) {
    *found_sym = 0;
    *found_dwarf = 0;
  }

  shdrs_view_valid = 0;
  names_view_valid = 0;
  symtab_view_valid = 0;
  strtab_view_valid = 0;
  buildid_view_valid = 0;
  buildid_data = NULL;
  buildid_size = 0;
  debuglink_view_valid = 0;
  debuglink_name = NULL;
  debuglink_crc = 0;
  debugaltlink_view_valid = 0;
  debugaltlink_name = NULL;
  debugaltlink_buildid_data = NULL;
  debugaltlink_buildid_size = 0;
  gnu_debugdata_view_valid = 0;
  gnu_debugdata_size = 0;
  debug_view_valid = 0;
  memset(&split_debug_view_valid[0], 0, sizeof split_debug_view_valid);
  opd = NULL;

  if (!elf_get_view(self, descriptor, memory, memory_size, 0, sizeof ehdr,
                    error_cb, data, &ehdr_view)) {
    goto fail;
  }

  memcpy(&ehdr, ehdr_view.view.data, sizeof ehdr);

  elf_release_view(self, &ehdr_view, error_cb, data);

  if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
      ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3) {
    error_cb(self, "executable file is not ELF", 0, data);
    goto fail;
  }
  if (ehdr.e_ident[EI_VERSION] != EV_CURRENT) {
    error_cb(self, "executable file is unrecognized ELF version", 0, data);
    goto fail;
  }

#if BACKTRACE_ELF_SIZE == 32
#define BACKTRACE_ELFCLASS ELFCLASS32
#else
#define BACKTRACE_ELFCLASS ELFCLASS64
#endif

  if (ehdr.e_ident[EI_CLASS] != BACKTRACE_ELFCLASS) {
    error_cb(self, "executable file is unexpected ELF class", 0, data);
    goto fail;
  }

  if (ehdr.e_ident[EI_DATA] != ELFDATA2LSB &&
      ehdr.e_ident[EI_DATA] != ELFDATA2MSB) {
    error_cb(self, "executable file has unknown endianness", 0, data);
    goto fail;
  }

  /* If the executable is ET_DYN, it is either a PIE, or we are running
     directly a shared library with .interp.  We need to wait for
     dl_iterate_phdr in that case to determine the actual base_address.  */
  if (exe && ehdr.e_type == ET_DYN) return -1;

  shoff = ehdr.e_shoff;
  shnum = ehdr.e_shnum;
  shstrndx = ehdr.e_shstrndx;

  if ((shnum == 0 || shstrndx == SHN_XINDEX) && shoff != 0) {
    struct elf_view shdr_view;
    const b_elf_shdr *shdr;

    if (!elf_get_view(self, descriptor, memory, memory_size, shoff, sizeof shdr,
                      error_cb, data, &shdr_view))
      goto fail;

    shdr = (const b_elf_shdr *)shdr_view.view.data;

    if (shnum == 0) shnum = shdr->sh_size;

    if (shstrndx == SHN_XINDEX) {
      shstrndx = shdr->sh_link;

      /* Versions of the GNU binutils between 2.12 and 2.18 did
         not handle objects with more than SHN_LORESERVE sections
         correctly.  All large section indexes were offset by
         0x100.  There is more information at
         http://sourceware.org/bugzilla/show_bug.cgi?id-5900 .
         Fortunately these object files are easy to detect, as the
         GNU binutils always put the section header string table
         near the end of the list of sections.  Thus if the
         section header string table index is larger than the
         number of sections, then we know we have to subtract
         0x100 to get the real section index.  */
      if (shstrndx >= shnum && shstrndx >= SHN_LORESERVE + 0x100)
        shstrndx -= 0x100;
    }

    elf_release_view(self, &shdr_view, error_cb, data);
  }

  if (shnum == 0 || shstrndx == 0) goto fail;

  /* To translate PC to file/line when using DWARF, we need to find
     the .debug_info and .debug_line sections.  */

  /* Read the section headers, skipping the first one.  */

  if (!elf_get_view(
          self, descriptor, memory, memory_size, shoff + sizeof(b_elf_shdr),
          (shnum - 1) * sizeof(b_elf_shdr), error_cb, data, &shdrs_view))
    goto fail;
  shdrs_view_valid = 1;
  shdrs = (const b_elf_shdr *)shdrs_view.view.data;

  /* Read the section names.  */

  shstrhdr = &shdrs[shstrndx - 1];
  shstr_size = shstrhdr->sh_size;
  shstr_off = shstrhdr->sh_offset;

  if (!elf_get_view(self, descriptor, memory, memory_size, shstr_off,
                    shstrhdr->sh_size, error_cb, data, &names_view))
    goto fail;
  names_view_valid = 1;
  names = (const char *)names_view.view.data;

  symtab_shndx = 0;
  dynsym_shndx = 0;

  memset(sections, 0, sizeof sections);
  memset(zsections, 0, sizeof zsections);

  /* Look for the symbol table.  */
  for (i = 1; i < shnum; ++i) {
    const b_elf_shdr *shdr;
    unsigned int sh_name;
    const char *name;
    int j;

    shdr = &shdrs[i - 1];

    if (shdr->sh_type == SHT_SYMTAB)
      symtab_shndx = i;
    else if (shdr->sh_type == SHT_DYNSYM)
      dynsym_shndx = i;

    sh_name = shdr->sh_name;
    if (sh_name >= shstr_size) {
      error_cb(self, "ELF section name out of range", 0, data);
      goto fail;
    }

    name = names + sh_name;

    for (j = 0; j < (int)DEBUG_MAX; ++j) {
      if (strcmp(name, dwarf_section_names[j]) == 0) {
        sections[j].offset = shdr->sh_offset;
        sections[j].size = shdr->sh_size;
        sections[j].compressed = (shdr->sh_flags & SHF_COMPRESSED) != 0;
        break;
      }
    }

    if (name[0] == '.' && name[1] == 'z') {
      for (j = 0; j < (int)DEBUG_MAX; ++j) {
        if (strcmp(name + 2, dwarf_section_names[j] + 1) == 0) {
          zsections[j].offset = shdr->sh_offset;
          zsections[j].size = shdr->sh_size;
          break;
        }
      }
    }

    /* Read the build ID if present.  This could check for any
       SHT_NOTE section with the right note name and type, but gdb
       looks for a specific section name.  */
    if ((!debuginfo || with_buildid_data != NULL) && !buildid_view_valid &&
        strcmp(name, ".note.gnu.build-id") == 0) {
      const b_elf_note *note;

      if (!elf_get_view(self, descriptor, memory, memory_size, shdr->sh_offset,
                        shdr->sh_size, error_cb, data, &buildid_view))
        goto fail;

      buildid_view_valid = 1;
      note = (const b_elf_note *)buildid_view.view.data;
      if (note->type == NT_GNU_BUILD_ID && note->namesz == 4 &&
          strncmp(note->name, "GNU", 4) == 0 &&
          shdr->sh_size <= 12 + ((note->namesz + 3) & ~3) + note->descsz) {
        buildid_data = &note->name[0] + ((note->namesz + 3) & ~3);
        buildid_size = note->descsz;
      }

      if (with_buildid_size != 0) {
        if (buildid_size != with_buildid_size) goto fail;

        if (memcmp(buildid_data, with_buildid_data, buildid_size) != 0)
          goto fail;
      }
    }

    /* Read the debuglink file if present.  */
    if (!debuginfo && !debuglink_view_valid &&
        strcmp(name, ".gnu_debuglink") == 0) {
      const char *debuglink_data;
      size_t crc_offset;

      if (!elf_get_view(self, descriptor, memory, memory_size, shdr->sh_offset,
                        shdr->sh_size, error_cb, data, &debuglink_view))
        goto fail;

      debuglink_view_valid = 1;
      debuglink_data = (const char *)debuglink_view.view.data;
      crc_offset = strnlen(debuglink_data, shdr->sh_size);
      crc_offset = (crc_offset + 3) & ~3;
      if (crc_offset + 4 <= shdr->sh_size) {
        debuglink_name = debuglink_data;
        debuglink_crc = *(const uint32_t *)(debuglink_data + crc_offset);
      }
    }

    if (!debugaltlink_view_valid && strcmp(name, ".gnu_debugaltlink") == 0) {
      const char *debugaltlink_data;
      size_t debugaltlink_name_len;

      if (!elf_get_view(self, descriptor, memory, memory_size, shdr->sh_offset,
                        shdr->sh_size, error_cb, data, &debugaltlink_view))
        goto fail;

      debugaltlink_view_valid = 1;
      debugaltlink_data = (const char *)debugaltlink_view.view.data;
      debugaltlink_name = debugaltlink_data;
      debugaltlink_name_len = strnlen(debugaltlink_data, shdr->sh_size);
      if (debugaltlink_name_len < shdr->sh_size) {
        /* Include terminating zero.  */
        debugaltlink_name_len += 1;

        debugaltlink_buildid_data = debugaltlink_data + debugaltlink_name_len;
        debugaltlink_buildid_size = shdr->sh_size - debugaltlink_name_len;
      }
    }

    if (!gnu_debugdata_view_valid && strcmp(name, ".gnu_debugdata") == 0) {
      if (!elf_get_view(self, descriptor, memory, memory_size, shdr->sh_offset,
                        shdr->sh_size, error_cb, data, &gnu_debugdata_view))
        goto fail;

      gnu_debugdata_size = shdr->sh_size;
      gnu_debugdata_view_valid = 1;
    }

    /* Read the .opd section on PowerPC64 ELFv1.  */
    if (ehdr.e_machine == EM_PPC64 && (ehdr.e_flags & EF_PPC64_ABI) < 2 &&
        shdr->sh_type == SHT_PROGBITS && strcmp(name, ".opd") == 0) {
      if (!elf_get_view(self, descriptor, memory, memory_size, shdr->sh_offset,
                        shdr->sh_size, error_cb, data, &opd_data.view))
        goto fail;

      opd = &opd_data;
      opd->addr = shdr->sh_addr;
      opd->data = (const char *)opd_data.view.view.data;
      opd->size = shdr->sh_size;
    }
  }

  if (symtab_shndx == 0) symtab_shndx = dynsym_shndx;
  if (symtab_shndx != 0 && !debuginfo) {
    const b_elf_shdr *symtab_shdr;
    unsigned int strtab_shndx;
    const b_elf_shdr *strtab_shdr;
    struct elf_syminfo_data *sdata;

    symtab_shdr = &shdrs[symtab_shndx - 1];
    strtab_shndx = symtab_shdr->sh_link;
    if (strtab_shndx >= shnum) {
      error_cb(self, "ELF symbol table strtab link out of range", 0, data);
      goto fail;
    }
    strtab_shdr = &shdrs[strtab_shndx - 1];

    if (!elf_get_view(self, descriptor, memory, memory_size,
                      symtab_shdr->sh_offset, symtab_shdr->sh_size, error_cb,
                      data, &symtab_view))
      goto fail;
    symtab_view_valid = 1;

    if (!elf_get_view(self, descriptor, memory, memory_size,
                      strtab_shdr->sh_offset, strtab_shdr->sh_size, error_cb,
                      data, &strtab_view))
      goto fail;
    strtab_view_valid = 1;

    sdata =
        (struct elf_syminfo_data *)ten_malloc_without_backtrace(sizeof *sdata);
    if (sdata == NULL) {
      goto fail;
    }

    if (!elf_initialize_syminfo(self, base_address, symtab_view.view.data,
                                symtab_shdr->sh_size, strtab_view.view.data,
                                strtab_shdr->sh_size, error_cb, data, sdata,
                                opd)) {
      ten_free_without_backtrace(sdata);
      goto fail;
    }

    /* We no longer need the symbol table, but we hold on to the
       string table permanently.  */
    elf_release_view(self, &symtab_view, error_cb, data);
    symtab_view_valid = 0;
    strtab_view_valid = 0;

    *found_sym = 1;

    elf_add_syminfo_data(self, sdata);
  }

  elf_release_view(self, &shdrs_view, error_cb, data);
  shdrs_view_valid = 0;
  elf_release_view(self, &names_view, error_cb, data);
  names_view_valid = 0;

  /* If the debug info is in a separate file, read that one instead.  */

  if (buildid_data != NULL) {
    int d;

    d = elf_open_debug_file_by_build_id(self, buildid_data, buildid_size);
    if (d >= 0) {
      int ret;

      elf_release_view(self, &buildid_view, error_cb, data);
      if (debuglink_view_valid) {
        elf_release_view(self, &debuglink_view, error_cb, data);
      }
      if (debugaltlink_view_valid) {
        elf_release_view(self, &debugaltlink_view, error_cb, data);
      }

      ret = elf_add(self, "", d, NULL, 0, base_address, error_cb, data,
                    fileline_fn, found_sym, found_dwarf, NULL, 0, 1, NULL, 0);
      if (ret < 0) {
        ten_file_close(d);
      } else if (descriptor >= 0) {
        ten_file_close(descriptor);
      }

      return ret;
    }
  }

  if (buildid_view_valid) {
    elf_release_view(self, &buildid_view, error_cb, data);
    buildid_view_valid = 0;
  }

  if (opd) {
    elf_release_view(self, &opd->view, error_cb, data);
    opd = NULL;
  }

  if (debuglink_name != NULL) {
    int d;

    d = elf_open_debug_file_by_debug_link(self, filename, debuglink_name,
                                          debuglink_crc, error_cb, data);
    if (d >= 0) {
      int ret;

      elf_release_view(self, &debuglink_view, error_cb, data);

      if (debugaltlink_view_valid) {
        elf_release_view(self, &debugaltlink_view, error_cb, data);
      }

      ret = elf_add(self, "", d, NULL, 0, base_address, error_cb, data,
                    fileline_fn, found_sym, found_dwarf, NULL, 0, 1, NULL, 0);
      if (ret < 0) {
        ten_file_close(d);
      } else if (descriptor >= 0) {
        ten_file_close(descriptor);
      }

      return ret;
    }
  }

  if (debuglink_view_valid) {
    elf_release_view(self, &debuglink_view, error_cb, data);
    debuglink_view_valid = 0;
  }

  struct dwarf_data *fileline_altlink = NULL;
  if (debugaltlink_name != NULL) {
    int d;

    d = elf_open_debug_file_by_debug_link(self, filename, debugaltlink_name, 0,
                                          error_cb, data);
    if (d >= 0) {
      int ret;

      ret = elf_add(self, filename, d, NULL, 0, base_address, error_cb, data,
                    fileline_fn, found_sym, found_dwarf, &fileline_altlink, 0,
                    1, debugaltlink_buildid_data, debugaltlink_buildid_size);
      elf_release_view(self, &debugaltlink_view, error_cb, data);
      debugaltlink_view_valid = 0;
      if (ret < 0) {
        ten_file_close(d);
        return ret;
      }
    }
  }

  if (debugaltlink_view_valid) {
    elf_release_view(self, &debugaltlink_view, error_cb, data);
    debugaltlink_view_valid = 0;
  }

  if (gnu_debugdata_view_valid) {
    int ret;

    ret = elf_uncompress_lzma(
        self, ((const unsigned char *)gnu_debugdata_view.view.data),
        gnu_debugdata_size, error_cb, data, &gnu_debugdata_uncompressed,
        &gnu_debugdata_uncompressed_size);

    elf_release_view(self, &gnu_debugdata_view, error_cb, data);
    gnu_debugdata_view_valid = 0;

    if (ret) {
      ret =
          elf_add(self, filename, -1, gnu_debugdata_uncompressed,
                  gnu_debugdata_uncompressed_size, base_address, error_cb, data,
                  fileline_fn, found_sym, found_dwarf, NULL, 0, 0, NULL, 0);
      if (ret >= 0 && descriptor >= 0) {
        ten_file_close(descriptor);
      }
      return ret;
    }
  }

  /* Read all the debug sections in a single view, since they are
     probably adjacent in the file.  If any of sections are
     uncompressed, we never release this view.  */

  min_offset = 0;
  max_offset = 0;
  debug_size = 0;
  for (i = 0; i < (int)DEBUG_MAX; ++i) {
    off_t end;

    if (sections[i].size != 0) {
      if (min_offset == 0 || sections[i].offset < min_offset)
        min_offset = sections[i].offset;
      end = sections[i].offset + sections[i].size;
      if (end > max_offset) max_offset = end;
      debug_size += sections[i].size;
    }
    if (zsections[i].size != 0) {
      if (min_offset == 0 || zsections[i].offset < min_offset)
        min_offset = zsections[i].offset;
      end = zsections[i].offset + zsections[i].size;
      if (end > max_offset) max_offset = end;
      debug_size += zsections[i].size;
    }
  }
  if (min_offset == 0 || max_offset == 0) {
    if (descriptor >= 0) {
      if (!ten_file_close(descriptor)) {
        goto fail;
      }
    }
    return 1;
  }

  /* If the total debug section size is large, assume that there are
     gaps between the sections, and read them individually.  */

  if (max_offset - min_offset < 0x20000000 ||
      max_offset - min_offset < debug_size + 0x10000) {
    if (!elf_get_view(self, descriptor, memory, memory_size, min_offset,
                      max_offset - min_offset, error_cb, data, &debug_view))
      goto fail;
    debug_view_valid = 1;
  } else {
    memset(&split_debug_view[0], 0, sizeof split_debug_view);
    for (i = 0; i < (int)DEBUG_MAX; ++i) {
      struct debug_section_info *dsec;

      if (sections[i].size != 0)
        dsec = &sections[i];
      else if (zsections[i].size != 0)
        dsec = &zsections[i];
      else
        continue;

      if (!elf_get_view(self, descriptor, memory, memory_size, dsec->offset,
                        dsec->size, error_cb, data, &split_debug_view[i]))
        goto fail;
      split_debug_view_valid[i] = 1;

      if (sections[i].size != 0)
        sections[i].data =
            ((const unsigned char *)split_debug_view[i].view.data);
      else
        zsections[i].data =
            ((const unsigned char *)split_debug_view[i].view.data);
    }
  }

  /* We've read all we need from the executable.  */
  if (descriptor >= 0) {
    if (!ten_file_close(descriptor)) {
      goto fail;
    }
    descriptor = -1;
  }

  using_debug_view = 0;
  if (debug_view_valid) {
    for (i = 0; i < (int)DEBUG_MAX; ++i) {
      if (sections[i].size == 0)
        sections[i].data = NULL;
      else {
        sections[i].data = ((const unsigned char *)debug_view.view.data +
                            (sections[i].offset - min_offset));
        ++using_debug_view;
      }

      if (zsections[i].size == 0)
        zsections[i].data = NULL;
      else
        zsections[i].data = ((const unsigned char *)debug_view.view.data +
                             (zsections[i].offset - min_offset));
    }
  }

  /* Uncompress the old format (--compress-debug-sections=zlib-gnu).  */

  zdebug_table = NULL;
  for (i = 0; i < (int)DEBUG_MAX; ++i) {
    if (sections[i].size == 0 && zsections[i].size > 0) {
      unsigned char *uncompressed_data;
      size_t uncompressed_size;

      if (zdebug_table == NULL) {
        zdebug_table =
            (uint16_t *)ten_malloc_without_backtrace(ZLIB_TABLE_SIZE);
        if (zdebug_table == NULL) {
          goto fail;
        }
      }

      uncompressed_data = NULL;
      uncompressed_size = 0;
      if (!elf_uncompress_zdebug(self, zsections[i].data, zsections[i].size,
                                 zdebug_table, error_cb, data,
                                 &uncompressed_data, &uncompressed_size))
        goto fail;
      sections[i].data = uncompressed_data;
      sections[i].size = uncompressed_size;
      sections[i].compressed = 0;

      if (split_debug_view_valid[i]) {
        elf_release_view(self, &split_debug_view[i], error_cb, data);
        split_debug_view_valid[i] = 0;
      }
    }
  }

  if (zdebug_table != NULL) {
    ten_free_without_backtrace(zdebug_table);
    zdebug_table = NULL;
  }

  /* Uncompress the official ELF format
     (--compress-debug-sections=zlib-gabi, --compress-debug-sections=zstd).  */
  for (i = 0; i < (int)DEBUG_MAX; ++i) {
    unsigned char *uncompressed_data;
    size_t uncompressed_size;

    if (sections[i].size == 0 || !sections[i].compressed) {
      continue;
    }

    if (zdebug_table == NULL) {
      zdebug_table =
          (uint16_t *)ten_malloc_without_backtrace(ZDEBUG_TABLE_SIZE);
      if (zdebug_table == NULL) {
        goto fail;
      }
    }

    uncompressed_data = NULL;
    uncompressed_size = 0;
    if (!elf_uncompress_chdr(self, sections[i].data, sections[i].size,
                             zdebug_table, error_cb, data, &uncompressed_data,
                             &uncompressed_size)) {
      goto fail;
    }

    sections[i].data = uncompressed_data;
    sections[i].size = uncompressed_size;
    sections[i].compressed = 0;

    if (debug_view_valid) {
      --using_debug_view;
    } else if (split_debug_view_valid[i]) {
      elf_release_view(self, &split_debug_view[i], error_cb, data);
      split_debug_view_valid[i] = 0;
    }
  }

  if (zdebug_table != NULL) ten_free_without_backtrace(zdebug_table);

  if (debug_view_valid && using_debug_view == 0) {
    elf_release_view(self, &debug_view, error_cb, data);
    debug_view_valid = 0;
  }

  for (i = 0; i < (int)DEBUG_MAX; ++i) {
    dwarf_sections.data[i] = sections[i].data;
    dwarf_sections.size[i] = sections[i].size;
  }

  if (!backtrace_dwarf_add(self, base_address, &dwarf_sections,
                           ehdr.e_ident[EI_DATA] == ELFDATA2MSB,
                           fileline_altlink, error_cb, data, fileline_fn,
                           fileline_entry))
    goto fail;

  *found_dwarf = 1;

  return 1;

fail:
  if (shdrs_view_valid) {
    elf_release_view(self, &shdrs_view, error_cb, data);
  }
  if (names_view_valid) {
    elf_release_view(self, &names_view, error_cb, data);
  }
  if (symtab_view_valid) {
    elf_release_view(self, &symtab_view, error_cb, data);
  }
  if (strtab_view_valid) {
    elf_release_view(self, &strtab_view, error_cb, data);
  }
  if (debuglink_view_valid) {
    elf_release_view(self, &debuglink_view, error_cb, data);
  }
  if (debugaltlink_view_valid) {
    elf_release_view(self, &debugaltlink_view, error_cb, data);
  }
  if (gnu_debugdata_view_valid) {
    elf_release_view(self, &gnu_debugdata_view, error_cb, data);
  }
  if (buildid_view_valid) {
    elf_release_view(self, &buildid_view, error_cb, data);
  }
  if (debug_view_valid) {
    elf_release_view(self, &debug_view, error_cb, data);
  }
  for (i = 0; i < (int)DEBUG_MAX; ++i) {
    if (split_debug_view_valid[i]) {
      elf_release_view(self, &split_debug_view[i], error_cb, data);
    }
  }
  if (opd) {
    elf_release_view(self, &opd->view, error_cb, data);
  }
  if (descriptor >= 0) {
    ten_file_close(descriptor);
  }
  return 0;
}

/* Data passed to phdr_callback.  */

struct phdr_data {
  ten_backtrace_t *self;
  ten_backtrace_error_func_t error_cb;
  void *data;
  ten_backtrace_get_file_line_func_t *fileline_fn;
  int *found_sym;
  int *found_dwarf;
  const char *exe_filename;
  int exe_descriptor;
};

/* Callback passed to dl_iterate_phdr.  Load debug info from shared
   libraries.  */

static int
#ifdef __i386__
    __attribute__((__force_align_arg_pointer__))
#endif
    phdr_callback(struct dl_phdr_info *info, size_t size, void *pdata) {
  struct phdr_data *pd = (struct phdr_data *)pdata;
  const char *filename;
  int descriptor;
  bool does_not_exist = false;
  ten_backtrace_get_file_line_func_t elf_fileline_fn;
  int found_dwarf;

  /* There is not much we can do if we don't have the module name,
     unless executable is ET_DYN, where we expect the very first
     phdr_callback to be for the PIE.  */
  if (info->dlpi_name == NULL || info->dlpi_name[0] == '\0') {
    if (pd->exe_descriptor == -1) {
      return 0;
    }

    filename = pd->exe_filename;
    descriptor = pd->exe_descriptor;
    pd->exe_descriptor = -1;
  } else {
    if (pd->exe_descriptor != -1) {
      ten_file_close(pd->exe_descriptor);
      pd->exe_descriptor = -1;
    }

    filename = info->dlpi_name;
    descriptor = ten_file_open(info->dlpi_name, &does_not_exist);
    if (descriptor < 0) {
      return 0;
    }
  }

  if (elf_add(pd->self, filename, descriptor, NULL, 0, info->dlpi_addr,
              pd->error_cb, pd->data, &elf_fileline_fn, pd->found_sym,
              &found_dwarf, NULL, 0, 0, NULL, 0)) {
    if (found_dwarf) {
      *pd->found_dwarf = 1;
      *pd->fileline_fn = elf_fileline_fn;
    }
  }

  return 0;
}

/**
 * @brief Initialize the backtrace data we need from an ELF executable. At the
 * ELF level, all we need to do is find the debug info sections.
 */
int ten_backtrace_init_posix(ten_backtrace_t *self_, const char *filename,
                             int descriptor,
                             ten_backtrace_error_func_t error_cb, void *data,
                             ten_backtrace_get_file_line_func_t *fileline_fn) {
  ten_backtrace_posix_t *self = (ten_backtrace_posix_t *)self_;
  assert(self && "Invalid argument.");

  int ret;
  int found_sym;
  int found_dwarf;
  ten_backtrace_get_file_line_func_t elf_fileline_fn = elf_nodebug;
  struct phdr_data pd;

  ret =
      elf_add(self_, filename, descriptor, NULL, 0, 0, error_cb, data,
              &elf_fileline_fn, &found_sym, &found_dwarf, NULL, 1, 0, NULL, 0);
  if (!ret) {
    return 0;
  }

  pd.self = self_;
  pd.error_cb = error_cb;
  pd.data = data;
  pd.fileline_fn = &elf_fileline_fn;
  pd.found_sym = &found_sym;
  pd.found_dwarf = &found_dwarf;
  pd.exe_filename = filename;
  pd.exe_descriptor = ret < 0 ? descriptor : -1;

  dl_iterate_phdr(phdr_callback, (void *)&pd);

  if (found_sym) {
    ten_atomic_ptr_store((void *)&self->get_syminfo, elf_syminfo);
  } else {
    (void)__sync_bool_compare_and_swap(&self->get_syminfo, NULL, elf_nosyms);
  }

  *fileline_fn = (ten_backtrace_get_file_line_func_t)ten_atomic_ptr_load(
      (void *)&self->get_file_line);

  if (*fileline_fn == NULL || *fileline_fn == elf_nodebug) {
    *fileline_fn = elf_fileline_fn;
  }

  return 1;
}
