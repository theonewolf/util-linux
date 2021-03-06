/*
 * Copyright (C) 2012 Ondrej Oprala <ooprala@redhat.com>
 *
 * This file may be distributed under the terms of the
 * GNU Lesser General Public License.
 */
#include <c.h>
#include <assert.h>
#include <sys/stat.h>

#include "colors.h"
#include "xalloc.h"
#include "pathnames.h"

static int ul_color_term_ok;

int colors_init(int mode, const char *name)
{
	int atty = -1;

	if (mode == UL_COLORMODE_UNDEF && (atty = isatty(STDOUT_FILENO))) {
		char path[PATH_MAX];

		snprintf(path, sizeof(path), "%s%s%s",
				_PATH_TERMCOLORS_DIR, name, ".enable");

		if (access(path, F_OK) == 0)
			mode = UL_COLORMODE_AUTO;
		else {
			snprintf(path, sizeof(path), "%s%s%s",
				_PATH_TERMCOLORS_DIR, name, ".disable");

			if (access(_PATH_TERMCOLORS_DISABLE, F_OK) == 0 ||
			    access(path, F_OK) == 0)
				mode = UL_COLORMODE_NEVER;
			else
				mode = UL_COLORMODE_AUTO;
		}
	}

	switch (mode) {
	case UL_COLORMODE_AUTO:
		ul_color_term_ok = atty == -1 ? isatty(STDOUT_FILENO) : atty;
		break;
	case UL_COLORMODE_ALWAYS:
		ul_color_term_ok = 1;
		break;
	case UL_COLORMODE_NEVER:
	default:
		ul_color_term_ok = 0;
	}
	return ul_color_term_ok;
}

int colors_wanted(void)
{
	return ul_color_term_ok;
}

void color_fenable(const char *color_scheme, FILE *f)
{
	if (ul_color_term_ok && color_scheme)
		fputs(color_scheme, f);
}

void color_fdisable(FILE *f)
{
	if (ul_color_term_ok)
		fputs(UL_COLOR_RESET, f);
}

int colormode_from_string(const char *str)
{
	size_t i;
	static const char *modes[] = {
		[UL_COLORMODE_AUTO]   = "auto",
		[UL_COLORMODE_NEVER]  = "never",
		[UL_COLORMODE_ALWAYS] = "always",
		[UL_COLORMODE_UNDEF] = ""
	};

	if (!str || !*str)
		return -EINVAL;

	assert(ARRAY_SIZE(modes) == __UL_NCOLORMODES);

	for (i = 0; i < ARRAY_SIZE(modes); i++) {
		if (strcasecmp(str, modes[i]) == 0)
			return i;
	}

	return -EINVAL;
}

int colormode_or_err(const char *str, const char *errmsg)
{
	const char *p = str && *str == '=' ? str + 1 : str;
	int colormode;

	colormode = colormode_from_string(p);
	if (colormode < 0)
		errx(EXIT_FAILURE, "%s: '%s'", errmsg, p);

	return colormode;
}

struct colorscheme {
	const char *name, *scheme;
};

static int cmp_colorscheme_name(const void *a0, const void *b0)
{
	struct colorscheme *a = (struct colorscheme *) a0,
			   *b = (struct colorscheme *) b0;
	return strcmp(a->name, b->name);
}

const char *colorscheme_from_string(const char *str)
{
	static const struct colorscheme basic_schemes[] = {
		{ "black",	UL_COLOR_BLACK           },
		{ "blue",	UL_COLOR_BLUE            },
		{ "brown",	UL_COLOR_BROWN           },
		{ "cyan",	UL_COLOR_CYAN            },
		{ "darkgray",	UL_COLOR_DARK_GRAY       },
		{ "gray",	UL_COLOR_GRAY            },
		{ "green",	UL_COLOR_GREEN           },
		{ "lightblue",	UL_COLOR_BOLD_BLUE       },
		{ "lightcyan",	UL_COLOR_BOLD_CYAN       },
		{ "lightgray,",	UL_COLOR_GRAY            },
		{ "lightgreen", UL_COLOR_BOLD_GREEN      },
		{ "lightmagenta", UL_COLOR_BOLD_MAGENTA  },
		{ "lightred",	UL_COLOR_BOLD_RED        },
		{ "magenta",	UL_COLOR_MAGENTA         },
		{ "red",	UL_COLOR_RED             },
		{ "yellow",	UL_COLOR_BOLD_YELLOW     },
	};
	struct colorscheme key = { .name = str }, *res;
	if (!str)
		return NULL;

	res = bsearch(&key, basic_schemes, ARRAY_SIZE(basic_schemes),
				sizeof(struct colorscheme),
				cmp_colorscheme_name);
	return res ? res->scheme : NULL;
}

#ifdef TEST_PROGRAM
# include <getopt.h>
int main(int argc, char *argv[])
{
	static const struct option longopts[] = {
		{ "colors", optional_argument, 0, 'c' },
		{ "scheme", required_argument, 0, 's' },
		{ NULL, 0, 0, 0 }
	};
	int c, mode = UL_COLORMODE_NEVER;	/* default */
	const char *scheme = "red";

	while ((c = getopt_long(argc, argv, "c::s:", longopts, NULL)) != -1) {
		switch (c) {
		case 'c':
			mode = UL_COLORMODE_AUTO;
			if (optarg)
				mode = colormode_or_err(optarg, "unsupported color mode");
			break;
		case 's':
			scheme = optarg;
			break;
		}
	}

	colors_init(mode, program_invocation_short_name);
	color_enable(colorscheme_from_string(scheme));

	printf("Hello World!");
	color_disable();
	return EXIT_SUCCESS;
}
#endif

