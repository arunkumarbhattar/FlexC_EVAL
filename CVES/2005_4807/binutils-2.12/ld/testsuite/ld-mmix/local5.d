#source: greg-4.s
#source: greg-4.s
#source: local2.s
#source: local1.s
#source: regext1.s
#source: start.s
#ld: -m elf64mmix
#readelf:  -Ssx1 -x5

# Like local1, but with two checks for a local register.

There are 9 section headers, starting at offset 0x118:

Section Headers:
  \[Nr\] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  \[ 0\]                   NULL             0+  0+
       0+  0+           0     0     0
  \[ 1\] \.text             PROGBITS         0+  0+b0
       0+c  0+  AX       0     0     4
  \[ 2\] \.data             PROGBITS         2000000000000000  0+bc
       0+  0+  WA       0     0     1
  \[ 3\] \.sbss             PROGBITS         2000000000000000  0+bc
       0+  0+   W       0     0     1
  \[ 4\] \.bss              NOBITS           2000000000000000  0+bc
       0+  0+  WA       0     0     1
  \[ 5\] \.MMIX\.reg_content PROGBITS         0+7e8  0+bc
       0+10  0+   W       0     0     1
  \[ 6\] \.shstrtab         STRTAB           0+  0+cc
       0+45  0+           0     0     1
  \[ 7\] \.symtab           SYMTAB           0+  0+358
       0+198  0+18           8     b     8
  \[ 8\] \.strtab           STRTAB           0+  0+4f0
       0+32  0+           0     0     1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\)
  I \(info\), L \(link order\), G \(group\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)

Symbol table '\.symtab' contains 17 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0+     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0+     0 SECTION LOCAL  DEFAULT    1 
     2: 2000000000000000     0 SECTION LOCAL  DEFAULT    2 
     3: 2000000000000000     0 SECTION LOCAL  DEFAULT    3 
     4: 2000000000000000     0 SECTION LOCAL  DEFAULT    4 
     5: 0+7e8     0 SECTION LOCAL  DEFAULT    5 
     6: 0+     0 SECTION LOCAL  DEFAULT    6 
     7: 0+     0 SECTION LOCAL  DEFAULT    7 
     8: 0+     0 SECTION LOCAL  DEFAULT    8 
     9: 0+fd     0 NOTYPE  LOCAL  DEFAULT  PRC lsym
    10: 0+fe     0 NOTYPE  LOCAL  DEFAULT  PRC lsym
    11: 0+fc     0 NOTYPE  GLOBAL DEFAULT  PRC ext1
    12: 0+8     0 NOTYPE  GLOBAL DEFAULT    1 _start
    13: 2000000000000000     0 OBJECT  GLOBAL DEFAULT  ABS __bss_start
    14: 2000000000000000     0 OBJECT  GLOBAL DEFAULT  ABS _edata
    15: 2000000000000000     0 OBJECT  GLOBAL DEFAULT  ABS _end
    16: 0+8     0 OBJECT  GLOBAL DEFAULT    1 _start\.

Hex dump of section '\.text':
  0x0+ fd020202 fd030201 e3fd0001          .*

Hex dump of section '\.MMIX\.reg_contents':
  0x0+7e8 00000000 0000004e 00000000 0000004e .*
