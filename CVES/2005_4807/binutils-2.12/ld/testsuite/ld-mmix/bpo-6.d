#source: start.s
#source: bpo-4.s
#as: -linker-allocated-gregs
#ld: -m elf64mmix
#objdump: -st

# 223 (max) linker-allocated GREGs.

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
0+df l       \*ABS\*	0+ i
0+ g       \.text	0+ _start
2000000000000000 g     O \*ABS\*	0+ __bss_start
2000000000000000 g     O \*ABS\*	0+ _edata
2000000000000000 g     O \*ABS\*	0+ _end
0+ g     O \.text	0+ _start\.

Contents of section \.text:
 0000 e3fd0001 230b2000 230b2100 230b2200  .*
 0010 230b2300 230b2400 230b2500 230b2600  .*
#...
 0360 230bf700 230bf800 230bf900 230bfa00  .*
 0370 230bfb00 230bfc00 230bfd00 230bfe00  .*
Contents of section \.data:
Contents of section \.sbss:
Contents of section \.MMIX\.reg_contents:
 0100 00000000 00000000 00000000 00000100  .*
 0110 00000000 00000200 00000000 00000300  .*
#...
 07d0 00000000 0000da00 00000000 0000db00  .*
 07e0 00000000 0000dc00 00000000 0000dd00  .*
 07f0 00000000 0000de00                    .*
