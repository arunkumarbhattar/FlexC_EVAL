#source: ldr1.s
#source: ldr2.s
#as: -little
#ld: -r -EL
#readelf: -r -x1 -x2
#target: sh*-*-elf sh*-*-linux*

# Make sure relocations against global and local symbols with relative and
# absolute 32-bit relocs don't come out wrong after ld -r.  Remember that
# SH uses partial_inplace (sort-of REL within RELA) with its confusion
# where and which addends to use and how.  A file linked -r must have the
# same layout as a plain assembly file: the addend is in the data only.

Relocation section '\.rela\.text' at offset 0x1b4 contains 1 entries:
 Offset     Info    Type            Symbol's Value  Symbol's Name          Addend
00000008  00000101 R_SH_DIR32            00000000  \.text                     \+ 0

Hex dump of section '\.text':
  0x00000000          0000000c 00090009 00090009 .*

Hex dump of section '\.rela\.text':
  0x00000000          00000000 00000101 00000008 .*
