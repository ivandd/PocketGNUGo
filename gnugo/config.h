/* This is the Visual C version of config.h */
/* Replace the distributed config.h with this file */

#define HAVE_VISUAL_C 1

/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if major, minor, and makedev are declared in <mkdev.h>.  */
/* #undef MAJOR_IN_MKDEV */

/* Define if major, minor, and makedev are declared in <sysmacros.h>.  */
/* #undef MAJOR_IN_SYSMACROS */

/* Define to use ansi escape sequences for color debugging */
#undef ANSI_COLOR

/* Define to use curses for color debugging */
#undef CURSES

/* Define as 1 to use the grid optimisation, or 2 to run it in self-test mode */
#define GRID_OPT 1

/* The concatenation of the strings "GNU ", and PACKAGE.  */
#define GNU_PACKAGE "GNU gnugo"

/* Define as 1 to use hashing */
#define HASHING 1

/* Define if the preprocessor recognizes __FUNCTION__ */
/* #undef HAVE___FUNCTION__ */

/* Define if you have the alarm function.  */
#undef HAVE_ALARM 

/* Define if you have the gettimeofday function.  */
#undef HAVE_GETTIMEOFDAY 

/* Define if you have the random function.  */
/* #undef HAVE_RANDOM */

/* Define if you have the setlinebuf function.  */
/* #undef HAVE_SETLINEBUF */

/* Define if you have the snprintf function.  */
/* #undef HAVE_SNPRINTF */

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/time.h> header file.  */
#undef HAVE_SYS_TIME_H

/* Define if you have the <unistd.h> header file.  */
#undef HAVE_UNISTD_H

/* Define if you have the glib library (-lglib).  */
#undef HAVE_LIBGLIB

/* Name of package */
#define PACKAGE "gnugo"

/* Version number of package */
#define VERSION "2.6"

#define abort() exit(1)