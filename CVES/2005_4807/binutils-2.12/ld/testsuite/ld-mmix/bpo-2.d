#source: start.s
#source: greg-1.s
#source: bpo-1.s
#as: -linker-allocated-gregs
#ld: -m elf64mmix
#objdump: -st

# Just a simple linker-allocated GREG plus one explicit GREG.

.*:     file format elf64-mmix

SYMBOL TABLE:
0+ l    d  \.text	0+ 
2000000000000000 l    d  \.data	0+ 
2000000000000000 l    d  \.sbss	0+ 
2000000000000000 l    d  \.bss	0+ 
0+7e8 l    d  \.MMIX\.reg_contents	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
0+4 l       \.text	0+ x
0+ g       \.text	0+ _start
0+fe g       \*REG\*	0+ areg
2000000000000000 g     O \*ABS\*	0+ __bss_start
2000000000000000 g     O \*ABS\*	0+ _edata
2000000000000000 g     O \*ABS\*	0+ _end
0+ g     O \.text	0+ _start\.

Contents of section \.text:
 0000 e3fd0001 232afd00                    .*
Contents of section \.data:
Contents of section \.sbss:
Contents of section \.MMIX\.reg_contents:
 07e8 00000000 0000002e 00007048 860f3a38  .*
