#source: greg-1.s
#source: gregpsj1.s
#source: start.s
#source: a.s
#as: -x
#ld: -m elf64mmix
#objdump: -dt

# Like greg-3, but a different expanding insn.

.*:     file format elf64-mmix

SYMBOL TABLE:
0+ l    d  .text	0+ 
2000000000000000 l    d  .data	0+ 
2000000000000000 l    d  .sbss	0+ 
2000000000000000 l    d  .bss	0+ 
0+7f0 l    d  \.MMIX\.reg_contents	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
0+14 g       \.text	0+ _start
0+fe g       \*REG\*	0+ areg
2000000000000000 g     O \*ABS\*	0+ __bss_start
2000000000000000 g     O \*ABS\*	0+ _edata
2000000000000000 g     O \*ABS\*	0+ _end
0+14 g     O \.text	0+ _start\.
0+18 g       \.text	0+ a

Disassembly of section \.text:

0+ <_start-0x14>:
   0:	e3ff0018 	setl \$255,0x18
   4:	e6ff0000 	incml \$255,0x0
   8:	e5ff0000 	incmh \$255,0x0
   c:	e4ff0000 	inch \$255,0x0
  10:	bffeff00 	pushgo \$254,\$255,0

0+14 <_start>:
  14:	e3fd0001 	setl \$253,0x1

0+18 <a>:
  18:	e3fd0004 	setl \$253,0x4
