#source: locdo.s -globalize-symbols
#source: start.s
#ld: -m elf64mmix
#objdump: -str

.*:     file format elf64-mmix

SYMBOL TABLE:
0+ l    d  \.text	0+ 
2000000000000008 l    d  \.data	0+ 
2000000000000010 l    d  \.sbss	0+ 
2000000000000010 l    d  \.bss	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
2000000000000008 g       \*ABS\*	0+ __\.MMIX\.start\.\.data
2000000000000008 g       \.data	0+ od
0+ g       \.text	0+ _start
2000000000000010 g     O \*ABS\*	0+ __bss_start
2000000000000000 g       \*ABS\*	0+ Data_Segment
2000000000000010 g     O \*ABS\*	0+ _edata
2000000000000010 g     O \*ABS\*	0+ _end
0+ g     O \.text	0+ _start\.

Contents of section \.text:
 0000 e3fd0001                             .*
Contents of section \.data:
 0008 20000000 00000008                    .*
Contents of section \.sbss:
