/* New for v2.0: readline support -- daw */

#include <config.h>

#include <stdio.h>

#include "rlvt100.proto"

/*
* i tried to put all the stuff that is terminal-specific in one file,
* just in case we decide to support other terminals someday or something.
* [forget switching to curses, it and readline do *not* get along.]
* 
* there is still some terminal-specific stuff in other files, though.
* (highlight.c springs to mind)
*/

#define ESCAPE 27

/* don't forget to do a 'fflush(stdout);' after each of these commands! */

void save_pos(void)
{
	printf("%c7", ESCAPE); 
}

void restore_pos(void)
{
	printf("%c8", ESCAPE); 
}

void goto_rowcol(int row, int col)
{
	printf("%c[%d;%df", ESCAPE, row, col);
}

void erase_screen(void)
{
	printf("%c[2J", ESCAPE);
}

void erase_toeol(void)
{
	printf("%c[K", ESCAPE);
}

void reset(void)
{
	printf("%cc", ESCAPE);
}

void scroll_region(int top, int bottom)
{
	printf("%c[%d;%dr", ESCAPE, top, bottom);
}
