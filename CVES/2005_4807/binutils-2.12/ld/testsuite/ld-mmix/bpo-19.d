#source: start.s
#source: bpo-9.s
#as: -linker-allocated-gregs
#ld: -m elf64mmix
#objdump: -st

# 223 (max) linker-allocated GREGs, four relocs merged for each register
# allocated.

.*:     file format elf64-mmix

SYMBOL TABLE:
0+ l    d  \.text	0+ 
2000000000000000 l    d  \.data	0+ 
2000000000000000 l    d  \.sbss	0+ 
2000000000000000 l    d  \.bss	0+ 
0+100 l    d  \.MMIX\.reg_contents	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
0+37c l       \*ABS\*	0+ i
0+ g       \.text	0+ _start
2000000000000000 g     O \*ABS\*	0+ __bss_start
2000000000000000 g     O \*ABS\*	0+ _edata
2000000000000000 g     O \*ABS\*	0+ _end
0+ g     O \.text	0+ _start\.

Contents of section \.text:
 0000 e3fd0001 230b2000 230b2040 230b2080  .*
 0010 230b20c0 230b2100 230b2140 230b2180  .*
 0020 230b21c0 230b2200 230b2240 230b2280  .*
#...
 0dd0 230bfcc0 230bfd00 230bfd40 230bfd80  .*
 0de0 230bfdc0 230bfe00 230bfe40 230bfe80  .*
 0df0 230bfec0                             .*
Contents of section \.data:
Contents of section \.sbss:
Contents of section \.MMIX\.reg_contents:
 0100 00000000 00000000 00000000 00000100  .*
 0110 00000000 00000200 00000000 00000300  .*
#...
 07d0 00000000 0000da00 00000000 0000db00  .*
 07e0 00000000 0000dc00 00000000 0000dd00  .*
 07f0 00000000 0000de00                    .*
