// SPDX-License-Identifier: GPL-2.0-only
/*
 * Boot console on the top screen, registered from init_early so CON_PRINTBUFFER
 * replays everything printk buffered up to that point. console_init() takes it
 * back down again when the VT console registers, which is well before fbcon can
 * draw; boot with keep_bootcon to cover that gap, at the cost of this console
 * and fbcon writing over each other.
 */

#include <linux/console.h>
#include <linux/font.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/sizes.h>

#include <asm/mach/map.h>

#include <mach/platform.h>

#define FB_PHYS		0x18000000
#define FB_VIRT		0xf0200000 /* its own pmd: __pmd_populate writes both halves */
#define FB_DEBUG_LL	0xf0000000
#define SCREEN_W	400
#define SCREEN_H	240
#define FB_STRIDE	(SCREEN_H * 3)
#define COLS		(SCREEN_W / 8)
#define ROWS		(SCREEN_H / 8)

static const struct font_desc *ctr_font;
static unsigned int ctr_col, ctr_row;

static struct map_desc ctr_fb_desc[] __initdata = {
	{
		.virtual	= FB_VIRT,
		.pfn		= __phys_to_pfn(FB_PHYS),
		.length		= SZ_1M,
		.type		= MT_DEVICE,
	},
#ifdef CONFIG_DEBUG_LL
	{
		.virtual	= FB_DEBUG_LL,
		.pfn		= __phys_to_pfn(FB_PHYS),
		.length		= SZ_1M,
		.type		= MT_DEVICE,
	},
#endif
};

/* The screen is scanned out rotated, so x picks the line and y runs backwards */
static void ctr_plot(unsigned int x, unsigned int y, u8 value)
{
	u8 *pixel = (u8 *)FB_VIRT + x * FB_STRIDE + (SCREEN_H - y - 1) * 3;

	pixel[0] = pixel[1] = pixel[2] = value;
}

static void ctr_clear(void)
{
	memset_io((void __iomem *)FB_VIRT, 0, SCREEN_W * FB_STRIDE);
	ctr_col = ctr_row = 0;
}

static void ctr_draw_char(char c)
{
	const u8 *glyph = ctr_font->data + (u8)c * 8;
	unsigned int i, j;

	for (i = 0; i < 8; i++, glyph++)
		for (j = 0; j < 8; j++)
			ctr_plot(ctr_col * 8 + j, ctr_row * 8 + i,
				 (*glyph & (0x80 >> j)) ? 0xff : 0x00);
}

static void ctr_putc(char c)
{
	if (c != '\n' && c != '\r') {
		ctr_draw_char(c);
		if (++ctr_col < COLS)
			return;
	}

	if (c == '\r') {
		ctr_col = 0;
		return;
	}

	/* Wipe the screen instead of scrolling, so the newest lines are the only ones */
	ctr_col = 0;
	if (++ctr_row >= ROWS)
		ctr_clear();
}

static void ctr_console_write(struct console *con, const char *s, unsigned int count)
{
	while (count--)
		ctr_putc(*s++);
}

static struct console ctr_console = {
	.name	= "ctrfb",
	.write	= ctr_console_write,
	.flags	= CON_PRINTBUFFER | CON_BOOT,
	.index	= -1,
};

void __init ctr_map_io(void)
{
	/* devicemaps_init skips debug_ll_io_init() for machines with map_io */
	iotable_init(ctr_fb_desc, ARRAY_SIZE(ctr_fb_desc));
}

void __init ctr_early_console_init(void)
{
	ctr_font = find_font("VGA8x8");
	if (!ctr_font)
		return;

	ctr_clear();
	register_console(&ctr_console);
}
