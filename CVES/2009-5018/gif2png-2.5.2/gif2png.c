/*
 * gif2png.c
 * Copyright (C) 1995 Alexander Lehmann
 * Bug fixes and enhancements by Greg Roelofs, 5 September 1999.
 * Production release packaging by Eric S. Raymond <esr@thyrsus.com>.
 * For conditions of distribution and use, see the file COPYING.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>	/* for isatty() */

#if !defined(TRUE)
#define FALSE	0
#define TRUE	1
#endif

#include "gif2png.h"

/* Define png_jmpbuf() in case we are using a pre-1.0.6 version of libpng */
#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) (png_ptr->jmpbuf)
#endif

struct GIFelement first={NULL};
struct GIFelement *current;
int delete = FALSE;
int optimize = FALSE;
int histogram = FALSE;
int interlaced = PNG_INTERLACE_LAST;
int progress = FALSE;
int transparent_ok = FALSE;
int verbose = FALSE;
int recover = FALSE;
int webconvert = FALSE;
int software_chunk = TRUE;
int filtermode = FALSE;
int matte = FALSE;
int gamma_srgb = FALSE;
GifColor matte_color;
long numgifs = 0L;
long numpngs = 0L;


int interlace_line(int height, int line)
/* return the actual line # used to store an interlaced line */
{
    int res;

    if ((line & 7) == 0) {
	return line >> 3;
    }
    res = (height+7) >> 3;
    if ((line & 7) == 4) {
	return res+((line-4) >> 3);
    }
    res += (height+3) >> 3;
    if ((line & 3) == 2) {
	return res + ((line-2) >> 2);
    }
    return res + ((height+1) >> 2) + ((line-1) >> 1);
}

int inv_interlace_line(int height, int line)
/* inverse function of above, used for recovery of interlaced images */
{
    if ((line << 3)<height) {
	return line << 3;
    }
    line -= (height+7) >> 3;
    if ((line << 3) + 4 < height) {
	return (line << 3) + 4;
    }
    line -= (height+3) >> 3;
    if ((line << 2) + 2 < height) {
	return (line << 2) + 2;
    }
    line -= (height + 1) >> 2;
    return (line << 1) + 1;
}

/* 
 * Do some adjustments of the color palette.
 *
 * We won't check for duplicate colors, this is probably only a very
 * rare case, but we check if all used colors are grays.  Some GIF
 * writers (e.g. some versions of ppmtogif) leave the unused entries
 * uninitialized, which breaks the detection of grayscale images
 * (e.g. in QPV).
 *
 * If the image is grayscale we will use PNG color type 0, except when
 * the transparency color is also appearing as a visible color. In
 * this case we write a paletted image.  Another solution would be
 * gray+alpha, but that would probably increase the image size too
 * much.
 *
 * If there are only few gray levels (<=16), we try to create a 4-bit
 * or less grayscale file, but if the gray levels do not fit into the
 * necessary grid, we write a paletted image--e.g., if the image
 * contains black, white and 50% gray, the grayscale image would
 * require 8-bit, but the paletted image only 2-bit. Even with
 * filtering, the 8-bit file will be larger.
 */

int is_gray(png_color c)
{
    /* check if r==g==b, moved to function to work around NeXTstep 3.3 cc bug.
     * life could be so easy if at least the basic tools worked as expected.
     */
    return c.red == c.green && c.green == c.blue;
}

int writefile(struct GIFelement *s,struct GIFelement *e, FILE *fp, int lastimg)
{
    int i;
    struct GIFimagestruct *img = e->imagestruct;
    unsigned long *count = img->color_count;
    GifColor *colors = img->colors;
    int colors_used = 0;
    byte remap[MAXCMSIZE];
    int low_prec;
    png_struct *png_ptr = xalloc(sizeof (png_struct));
    png_info *info_ptr = xalloc(sizeof (png_info));
    int p;
    int gray_bitdepth;
    png_color pal_rgb[MAXCMSIZE], *pltep;
    png_byte pal_trans[MAXCMSIZE];
    png_color_16 color16trans, color16back;
    byte buffer[24]; /* used for gIFt and gIFg */
    int j;
    png_uint_16 histogr[MAXCMSIZE];
    unsigned long hist_maxvalue;
    int passcount;
    int errtype, errorcount = 0;
    png_text software;
    png_text comment;

    /* these volatile declarations prevent gcc warnings ("variable might be
     *  clobbered by `longjmp' or `vfork'") */
    volatile int gray = TRUE;
    volatile int last_color = 0;
    volatile int remapping = FALSE;
    volatile int bitdepth;

    /* ignore the transparency value if no pixel in the image uses it */
    if (img->trans != -1 && !count[img->trans])
	img->trans = -1;

    if (optimize) {
	/* zero out unused color table entries and use zlib level 9 */
	for (i = 0; i < MAXCMSIZE; i++)
	    if(!count[i])
		colors[i].red = colors[i].green = colors[i].blue = 0;
	/*
	 * The way that last_color is computed ensures that trailing zeroed-out
	 * entries will be dropped.
	 * FIXME: someday, actually compact unused colors out of the table
	 * and remap pixels appropriately.
	 */
    } else if (!recover) {
	/* report unused colors so user can choose to reconvert */
	int unused = 0;

	for (i = 0; i < MAXCMSIZE; i++)
	    if (!count[i])
		unused++;

	if (unused)
	{
	    fprintf(stderr, 
		    "gif2png: %d unused colors; convert with -O to remove\n",
		    unused);
	    errorcount++;
	}
    }

    for (i = 0; i < MAXCMSIZE; i++)
	if (count[i]) {
	    gray &= is_gray(colors[i]);
	    colors_used++;
	    if (last_color < i) 
		last_color = i;
	}

    if (gray) {
	if (verbose > 1)
	    fprintf(stderr, "gif2png: input image is grayscale\n");

	if (img->trans != -1) {
	    for (i = 0; i < MAXCMSIZE; i++) {
		if (i!=img->trans && colors[i].red == colors[img->trans].red) {
		    gray = FALSE;
		    if (verbose > 1)
			fprintf(stderr,
				"gif2png: transparent color is repeated in visible colors, using palette\n");
		    break;
		}
	    }
	}
    }

    bitdepth = 8;
    if (last_color<16) bitdepth = 4;
    if (last_color<4) bitdepth = 2;
    if (last_color<2) bitdepth = 1;

    if (gray) {
	remapping = TRUE;
	for (i = 0; i < MAXCMSIZE; i++)
	    remap[i] = colors[i].red;

	gray_bitdepth = 8;

	/* try to adjust to 4-bit precision grayscale */

	low_prec = TRUE;
	for (i = 0; i < MAXCMSIZE; i++) {
	    if ((remap[i]&0xf)*0x11 != remap[i]) {
		low_prec = FALSE;
		break;
	    }
	}
	if (low_prec) {
	    for (i = 0; i < MAXCMSIZE; i++) {
		remap[i] &= 0xf;
	    }
	    gray_bitdepth = 4;
	}

	/* try to adjust to 2-bit precision grayscale */

	if (low_prec) {
	    for (i = 0; i < MAXCMSIZE; i++) {
		if ((remap[i]&3)*5 != remap[i]) {
		    low_prec = FALSE;
		    break;
		}
	    }
	}

	if (low_prec) {
	    for (i = 0; i < MAXCMSIZE; i++) {
		remap[i] &= 3;
	    }
	    gray_bitdepth = 2;
	}

	/* try to adjust to 1-bit precision grayscale */

	if (low_prec) {
	    for (i = 0; i < MAXCMSIZE; i++) {
		if ((remap[i]&1)*3 != remap[i]) {
		    low_prec = FALSE;
		    break;
		}
	    }
	}
	if (low_prec) {
	    for (i = 0; i < MAXCMSIZE; i++) {
		remap[i] &= 1;
	    }
	    gray_bitdepth=1;
	}

	if (bitdepth<gray_bitdepth) {
	    gray = FALSE;		/* write palette file */
	    remapping = FALSE;
	} else {
	    bitdepth = gray_bitdepth;
	}
    }

    if (verbose > 1)
	fprintf(stderr, "gif2png: %d colors used, highest color %d, %s, bitdepth %d\n",
		colors_used, last_color, gray ? "gray":"palette", bitdepth);

    if (verbose > 2)
	for (i = 0; i <= last_color; i++) {
	    fprintf(stderr, "(%3d, %3d, %3d)\n", 
		    colors[i].red, colors[i].blue, colors[i].green);  
	}

    if (setjmp(png_jmpbuf(png_ptr))) {
	fprintf(stderr, "gif2png:  libpng returns fatal error condition\n");
	free(png_ptr);
	free(info_ptr);
	return 1;
    }


    /* Create and initialize the png_struct with the desired error handler
     * functions.  If you want to use the default stderr and longjump method,
     * you can supply NULL for the last three parameters.  We also check that
     * the library version is compatible with the one used at compile time,
     * in case we are using dynamically linked libraries.  REQUIRED.
     */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
				      (void *)NULL, NULL, NULL);

    if (png_ptr == NULL)
	return(2);

    /* Allocate/initialize the image information data.  REQUIRED */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
	png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
	return(2);
    }

    /* if errtype is not 1, this was generated by fatal() */ 
    if ((errtype = setjmp(png_jmpbuf(png_ptr)))) {
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return errtype;
    }

    /* use standard C streams */
    png_init_io(png_ptr, fp);

    if(optimize)
       png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

    if (gamma_srgb)
    {
          png_set_gAMA(png_ptr, info_ptr, 1./2.2);
#if (PNG_LIBPNG_VER > 96)
	  png_set_sRGB(png_ptr, info_ptr, PNG_sRGB_INTENT_PERCEPTUAL);
#endif
    }

    /* set basic image properties: width/height, type, bit depth, interlace */
    if (interlaced == PNG_INTERLACE_LAST)
	interlaced = current->imagestruct->interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE;

    png_set_IHDR(png_ptr, info_ptr, 
		 current->imagestruct->width, current->imagestruct->height,
		 bitdepth, 
		 gray ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_PALETTE,
		 interlaced, 
		 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    /* if the GIF specified an aspect ratio, turn it into a pHYs chunk */
    if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49)
	png_set_pHYs(png_ptr, info_ptr, 
		     GifScreen.AspectRatio+15, 64, PNG_RESOLUTION_UNKNOWN);

    /* if the GIF specified an image offset, turn it into a oFFs chunk */
    if (img->offset_x>0 && img->offset_y>0)
	png_set_oFFs(png_ptr, info_ptr, 
		     img->offset_x, img->offset_y, PNG_OFFSET_PIXEL);

    if (GifScreen.Background > 0) { /* no background for palette entry 0 */
	/* if the background color doesn't appear in local palette, we just
	   leave it out, if the pic is part of an animation, at least some will
	   use the global palette */

	if (gray) {
	    if (is_gray(GifScreen.ColorMap[GifScreen.Background])) {
			color16back.gray = remap[GifScreen.Background];
		png_set_bKGD(png_ptr, info_ptr, &color16back);
	    }
	} else {
	    for (i = 0; i < MAXCMSIZE; i++) {
		if (GifScreen.ColorMap[GifScreen.Background].red == colors[i].red &&
		    GifScreen.ColorMap[GifScreen.Background].green == colors[i].green &&
		    GifScreen.ColorMap[GifScreen.Background].blue == colors[i].blue) {
		    if (last_color < i) 
			last_color = i;
		    color16back.index = i;
		    png_set_bKGD(png_ptr, info_ptr, &color16back);
		    break;
		}
	    }
	}
    }

    /* transparency handling */
    if (img->trans != -1) {
	if (gray) {
	    if (verbose > 2)
		fprintf(stderr, "gif2png: handling grayscale transparency\n");
	    color16trans.gray = remap[img->trans];
	    png_set_tRNS(png_ptr, info_ptr, NULL, 0, &color16trans);
	} else {
	    if (verbose > 2)
		fprintf(stderr, "gif2png: handling palette transparency\n");
	    /* GRR NOTE: may be inconsistent with background-color logic (above) */
	    remapping = TRUE;
	    for (i = 0; i < MAXCMSIZE; i++)
		remap[i] = i;
	    remap[0] = img->trans;
	    remap[img->trans] = 0;
	    pal_trans[0] = 0;
	    png_set_tRNS(png_ptr, info_ptr, pal_trans, 1, NULL);
	}
    }

    /* generate the PNG palette */
    if (!gray) {
	if (remapping) {
	    if (verbose > 2)
		fprintf(stderr, "Remapping palette...\n");
	    for (i = 0; i <= last_color; i++) {
		pal_rgb[i].red   = current->imagestruct->colors[remap[i]].red;
		pal_rgb[i].green = current->imagestruct->colors[remap[i]].green;
		pal_rgb[i].blue  = current->imagestruct->colors[remap[i]].blue;
	    }
	    pltep = pal_rgb;
	} else {
	    if (verbose > 2)
		fprintf(stderr, "Palette copied from GIF colors...\n");
	    pltep = current->imagestruct->colors;
	}
	png_set_PLTE(png_ptr, info_ptr, pltep, last_color+1);

	if (verbose > 2) {
	    fprintf(stderr,"PNG palette generated with %d colors:\n",last_color+1);
	    for (i = 0; i <= last_color; i++) {
		fprintf(stderr, "(%3d, %3d, %3d)\n", 
			pltep[i].red, pltep[i].blue, pltep[i].green);  
	    }
	}
    }

    /* generate histogram chunk for paletted images only */
    if (histogram && !gray) {
	hist_maxvalue = 0;
	for (i = 0; i < MAXCMSIZE; i++)
	    if (count[i]>hist_maxvalue)
		hist_maxvalue = count[i];
	if (hist_maxvalue <= 65535) {
	    /* no scaling necessary */
	    for (i = 0; i < MAXCMSIZE; i++)
		histogr[i] = (png_uint_16)count[i];
	} else {
	    for (i = 0; i < MAXCMSIZE; i++)
		if (count[i]) {
		    histogr[i] = (png_uint_16)
#ifdef __GO32__
			/* avoid using fpu instructions on djgpp, so we don't need emu387 */
			(((long long)count[i]*65535)/hist_maxvalue);
#else
		    ((double)count[i]*65535.0/hist_maxvalue);
#endif
		    if (histogr[i] == 0)
			histogr[i] = 1;
		} else {
		    histogr[i] = 0;
		}
	}
	png_set_hIST(png_ptr, info_ptr, histogr);
    }

    /* transcribe software chunk if necessary */
    if (software_chunk) {
	software.compression = PNG_TEXT_COMPRESSION_NONE;
	software.key = "Software";
	software.text = (char*)version;
	software.text_length = strlen(software.text);

	png_set_text(png_ptr, info_ptr, &software, 1);
    }

    png_write_info(png_ptr, info_ptr);

    if (bitdepth<8)
	png_set_packing(png_ptr);

    /* loop over elements until we reach the image or the last element if
       this is the last image in a GIF */

    while(lastimg ? s != NULL : s != e->next) {
	byte *data;
	switch((byte)s->GIFtype) {
	case GIFimage:
	    passcount = png_set_interlace_handling(png_ptr);
	    for (p = 0; p<passcount;p++)
		for (i = 0; i<current->imagestruct->height; i++) {
		    if (progress) {
			if (passcount>1)
			    fprintf(stderr, "%d/%d ", p+1, passcount);
			fprintf(stderr, "%2d%%\r",
				(int)(((long)i*100)/current->imagestruct->height));
			fflush(stderr);
		    }
		    data = current->data + 
				       (long) (img->interlace ?
							interlace_line(current->imagestruct->height,i) : i) * current->imagestruct->width;
		    /* we have to remap once */
		    if (remapping && p == 0)
		    {
			for (j = 0; j < img->width; j++) {
			    data[j] = remap[data[j]];
			}
		    }
		    png_write_row(png_ptr, data);
		}
	    break;

	case GIFcomment:
	    j = s->size;
	    /* some GIF encoders produce trailing garbage and NULs */
	    if (j > 0 && ((s->data[j-1] == '\0') || (s->data[j-1] & 0x80)))
		--j;
	    comment.key = "Comment";
	    if (j<500) {
		comment.compression = PNG_TEXT_COMPRESSION_NONE;
	    } else {
		comment.compression = PNG_TEXT_COMPRESSION_zTXt;
	    }
 	    comment.text = (png_charp)s->data;
 	    comment.text_length = strlen(comment.text);
 	    png_set_text(png_ptr, info_ptr, &comment, 1);
	    break;

	case GIFapplication:
	    png_write_chunk_start(png_ptr, (unsigned char *)"gIFx", s->size);
	    png_write_chunk_data(png_ptr, s->data, s->size);
	    png_write_chunk_end(png_ptr);
	    break;

/* Plain Text Extensions are not supported because they represent a separate
   subimage, and PNG is explicitly a single-image format.  PNG's gIFt chunk
   has been officially deprecated.
*/
#if 0
	case GIFplaintext:
	    /* gIFt is 12 bytes longer than GCE, due to 32-bit ints and
	       rgb colors */
	    png_write_chunk_start(png_ptr, "gIFt", s->size+12);
	    memset(buffer, 0, 24);
	    buffer[2] = s->data[1];
	    buffer[3] = s->data[0];
	    buffer[6] = s->data[3];
	    buffer[7] = s->data[2];
	    buffer[10] = s->data[5];
	    buffer[11] = s->data[4];
	    buffer[14] = s->data[7];
	    buffer[15] = s->data[6];
	    buffer[16] = s->data[8];
	    buffer[17] = s->data[9];
	    buffer[18] = GifScreen.ColorMap[s->data[10]].red;
	    buffer[19] = GifScreen.ColorMap[s->data[10]].green;
	    buffer[20] = GifScreen.ColorMap[s->data[10]].blue;
	    buffer[21] = GifScreen.ColorMap[s->data[11]].red;
	    buffer[22] = GifScreen.ColorMap[s->data[11]].green;
	    buffer[23] = GifScreen.ColorMap[s->data[11]].blue;
	    png_write_chunk_s->data(png_ptr, buffer, 24);
	    png_write_chunk_s->data(png_ptr, s->data+12, s->size-12);
	    png_write_chunk_end(png_ptr);
	    break;
#endif

	case GIFgraphicctl:
	    buffer[0] = (s->data[0] >> 2) & 7;
	    buffer[1] = (s->data[0] >> 1) & 1;
	    buffer[2] = s->data[2];
	    buffer[3] = s->data[1];

	    png_write_chunk(png_ptr, (unsigned char *)"gIFg", buffer, 4);
	    break;

	default:
	    fprintf(stderr, "gif2png internal error: encountered unused element type %c\n", s->GIFtype);
	    break;
	}
	s = s->next;
    }
    png_write_end(png_ptr, info_ptr);

    /* clean up after the write, and free any memory allocated */
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return errorcount;
}

int
MatteGIF(GifColor matte)
/* convert transparent pixels to a matte of given color */
{
    int icount = 0;

    /* apply matte color to image-local transparency values */
    for (current = &first; current ; current = current->next) {
	if (current->imagestruct) {
	    if (current->imagestruct->trans == -1)
		fprintf(stderr, 
			"gif2png: no transparency color in image %d, matte argument ignored\n",
			icount);
	    else {
		if (verbose) {
		    fprintf(stderr, 
			"gif2png: transparent value in image %d is %d\n",  
			icount, current->imagestruct->trans);
		}

		current->imagestruct->colors[current->imagestruct->trans] = matte;
		current->imagestruct->trans = -1;
	    }
	}
    }
    return icount;
}

int processfilter(void)
{
    int num_pics;
    struct GIFelement *start;

    current = &first;
    num_pics = ReadGIF (stdin);
    fclose(stdin);

    if (num_pics != 1) {
	fprintf(stderr, "gif2png: %d images in -f (filter) mode\n", num_pics);
	return 1;
    }

    /* eliminate use of transparency, if that is called for */
    if (matte)
	MatteGIF(matte_color);

    start = NULL;
    for (current = first.next; current ; current = current->next) {
	if (start == NULL) start = current;
	if (current->GIFtype == GIFimage) {
	    (void)writefile(start, current, stdout, TRUE);
	    start = NULL;
	    ++numpngs;
	}
    }
    free_mem();
    return 0;
}

int processfile(char *fname, FILE *fp)
{
    char outname[1025]; /* MAXPATHLEN comes under too many names */
    int num_pics;
    struct GIFelement *start;
    int i, suppress_delete = FALSE;
    char *file_ext;

    if (fp == NULL) return 1;

    current = &first;

    num_pics = ReadGIF (fp);

    fclose(fp);

    if (num_pics >= 0 && verbose > 1)
	fprintf(stderr, "gif2png: number of images %d\n", num_pics);

    if (num_pics <= 0)
	return 1;

    if (webconvert)
	if (num_pics != 1)
	{
	    fprintf(stderr, "gif2png: %s is multi-image\n", fname);
	    return 0;
	}
	else if (!transparent_ok
		 && current->imagestruct && current->imagestruct->trans != -1)
	{
	    fprintf(stderr, "gif2png: %s has a transparency color\n", fname);
	    return 0;
	}
	else
	{
	    printf("%s\n", fname);
	    return 0;
	}

    /* eliminate use of transparency, if that is called for */
    if (matte)
	MatteGIF(matte_color);

    /* create output filename */

    strcpy(outname, fname);

    file_ext = outname+strlen(outname)-4;
    if (strcmp(file_ext, ".gif") != 0 && strcmp(file_ext, ".GIF") != 0 &&
	strcmp(file_ext, "_gif") != 0 && strcmp(file_ext, "_GIF") != 0) {
	/* try to derive basename */
	file_ext = outname+strlen(outname);
	while(file_ext >= outname) {
	    if (*file_ext == '.' || *file_ext == '/' || *file_ext == '\\') break;
	    file_ext--;
	}
	if (file_ext<outname || *file_ext != '.') {
	    /* as a last resort, just add .png to the filename */
	    file_ext = outname+strlen(outname);
	}
    }

    strcpy(file_ext, ".png"); /* images are named .png, .p01, .p02, ... */

    start = NULL;

    i = 0;

    for (current = first.next; current ; current = current->next) {
	if (start == NULL) start = current;
	if (current->GIFtype == GIFimage) {
	    i++;
	    if ((fp = fopen(outname, "wb")) == NULL) {
		perror(outname);
		return 1;
	    } else {
		suppress_delete |= writefile(start, current, fp, i == num_pics);
		fclose(fp);
		++numpngs;
		start = NULL;
		sprintf(file_ext, ".p%02d", i);
	    }
	}
    }

    free_mem();

    if (delete && !suppress_delete) {
	unlink(fname);
    }

    return 0;
}

/* check if stdin is a terminal, if your compiler is lacking isatty or fileno
   (non-ANSI functions), just return 0
*/

int input_is_terminal(void)
{
    return isatty(fileno(stdin));
}

int main(int argc, char *argv[])
{
    FILE *fp;
    int i, background;
    int errors = 0;
    char name[1025];
    int ac;
    char *color;

    software_chunk = 1;

    ac = 1;

    for (ac = 1; ac<argc && argv[ac][0] == '-'; ac++)
    {
	for (i = 1;i<strlen(argv[ac]); i++)
	    switch(argv[ac][i]) {
	    case 'b':
		if (ac >= argc - 1)
		{
		    fputs("gif2png: missing background-matte argument\n",
			  stderr);
		    exit(1);
		}
		color = argv[++ac];
		if (*color == '#')
		    color++;
	        sscanf(color, "%x", &background);
		matte_color.red = (background >> 16) & 0xff;
		matte_color.green = (background >> 8) & 0xff; 
		matte_color.blue = background & 0xff;
		matte = TRUE;
		goto skiparg;

	    case 'd':
		delete = TRUE;
		break;

	    case 'f':
	    case '1': /* backward compatibility */
		filtermode = TRUE;
		break;

	    case 'g':
		gamma_srgb = TRUE;
		break;

	    case 'h':
		histogram = TRUE;
		break;

	    case 'i':
		interlaced = PNG_INTERLACE_ADAM7;
		break;

	    case 'n':
		interlaced = PNG_INTERLACE_NONE;
		break;

	    case 'p':
		progress = TRUE;
		break;

	    case 'r':
		recover = TRUE;
		break;

	    case 's':
		software_chunk = FALSE;
		break;

	    case 't':
		transparent_ok = TRUE;
		break;

	    case 'v':
		++verbose;
		break;

	    case 'w':
		webconvert = TRUE;
		break;

	    case 'O':
		optimize = TRUE;
		break;

	    default:
		fprintf(stderr, "gif2png: unknown option `-%c'\n", argv[ac][i]);
	    usage:
		fprintf(stderr,
	"\n"
	"usage: gif2png [ -bdfghinprstvwO ] [file[.gif]] ...\n"
	"\n"
	"   -b  replace background pels with given RRGGBB color (hex digits)\n"
	"   -d  delete source GIF files after successful conversion\n"
	"   -f  allow use as filter, die on multi-image GIF\n"
	"   -g  write gamma=(1.0/2.2) and sRGB chunks\n"
	"   -h  generate PNG histogram chunks into color output files\n"
	"   -i  force conversion to interlaced PNG files\n"
	"   -n  force conversion to noninterlaced PNG files\n"
	"   -p  display progress of PNG writing in %%\n"
	"   -r  try to recover corrupted GIF files\n"
	"   -s  do not write Software text chunk\n"
	"   -t  transparency OK, web probe mode accepts transparent GIFs\n"
	"   -v  verbose; display conversion statistics and debugging messages\n"
	"   -w  web probe, list images without animation or transparency\n"
	"   -O  optimize; remove unused color-table entries; use zlib level 9\n"
	"   -1  one-only; same as -f; provided for backward compatibility\n"
	"\n"
	"   If no source files are listed, stdin is converted to noname.png.\n"
	"\n"
	"   This is %s, %s\n", version, compile_info);
		return 1;
	    }
    skiparg:;
    }

    if (verbose > 1)
	fprintf(stderr, "gif2png: %s, %s\n", version, compile_info);

    /* loop over arguments or use stdin */
    if (argc == ac) {
	if (input_is_terminal()) {
	    goto usage;
	}
	if (verbose > 1)
	    fprintf(stderr, "stdin:\n");
	if (filtermode) {
	    if (processfilter() != 0) ++errors;
	} else {
	    processfile("noname.gif", stdin);
	    ++numgifs;
	}
    } else {
	for (i = ac;i<argc; i++) {
	    strcpy(name, argv[i]);
	    if ((fp = fopen(name, "rb")) == NULL) {
		/* retry with .gif appended */
		strcat(name, ".gif");
		fp = fopen(name,"rb");
	    }
	    if (fp == NULL) {
		perror(argv[i]);
		errors = 1;
	    } else {
		if (verbose > 1)
		    fprintf(stderr, "%s:\n", name);
		errors |= processfile(name, fp);
		/* fp is closed in processfile */
		++numgifs;
	    }
	}
    }

    if (verbose)
	fprintf(stderr, "Done (%s).  Converted %ld GIF%s into %ld PNG%s.\n",
		errors? "with one or more errors" : "no errors detected", 
		numgifs, (numgifs == 1) ? "" : "s", numpngs, (numpngs == 1)? "" : "s");

    return errors;
}
