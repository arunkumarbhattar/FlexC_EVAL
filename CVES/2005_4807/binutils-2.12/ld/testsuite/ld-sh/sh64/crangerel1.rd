There are 10 section headers, starting at offset 0xb0:

Section Headers:
  \[Nr\] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            00000000 000000 000000 00      0   0  0
  \[ 1\] \.text             PROGBITS        00000000 000034 000000 00  AX  0   0  1
  \[ 2\] \.text\.mixed       PROGBITS        00000000 000034 000018 00 AXp  0   0  4
  \[ 3\] \.data             PROGBITS        00000000 00004c 000000 00  WA  0   0  1
  \[ 4\] \.bss              NOBITS          00000000 00004c 000000 00  WA  0   0  1
  \[ 5\] \.cranges          PROGBITS        00000000 00004c 00001e 00   W  0   0  1
  \[ 6\] \.rela\.cranges     RELA            00000000 000240 000024 0c      8   5  4
  \[ 7\] \.shstrtab         STRTAB          00000000 00006a 000046 00      0   0  1
  \[ 8\] \.symtab           SYMTAB          00000000 000264 0000c0 10      9   b  4
  \[ 9\] \.strtab           STRTAB          00000000 000324 000013 00      0   0  1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\)
  I \(info\), L \(link order\), G \(group\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)

Relocation section '\.rela\.cranges' at offset 0x240 contains 3 entries:
[ ]*Offset[ ]+Info[ ]+Type[ ]+Symbol's Value[ ]+Symbol's Name[ ]+Addend
0*00000000  0+0201 R_SH_DIR32            00000000  \.text\.mixed               \+ 0
0*0000000a  0+0201 R_SH_DIR32            00000000  \.text\.mixed               \+ 0
0*00000014  0+0201 R_SH_DIR32            00000000  \.text\.mixed               \+ 0

Symbol table '\.symtab' contains 12 entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
     0: 00000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 00000000     0 SECTION LOCAL  DEFAULT    1 
     2: 00000000     0 SECTION LOCAL  DEFAULT    2 
     3: 00000000     0 SECTION LOCAL  DEFAULT    3 
     4: 00000000     0 SECTION LOCAL  DEFAULT    4 
     5: 00000000     0 SECTION LOCAL  DEFAULT    5 
     6: 00000000     0 SECTION LOCAL  DEFAULT    6 
     7: 00000000     0 SECTION LOCAL  DEFAULT    7 
     8: 00000000     0 SECTION LOCAL  DEFAULT    8 
     9: 00000000     0 SECTION LOCAL  DEFAULT    9 
    10: 00000000     0 NOTYPE  LOCAL  DEFAULT    2 start2
    11: 00000000     0 NOTYPE  GLOBAL DEFAULT    2 diversion2

Hex dump of section '\.text\.mixed':
  0x00000000 6ff0fff0 6ff0fff0 6ff0fff0 0000002a .*
  0x00000010 0000002b 00090009                   .*

Hex dump of section '\.cranges':
  0x00000000 00000000 0000000c 00030000 000c0000 .*
  0x00000010 00080001 00000014 00000004 0002     .*
