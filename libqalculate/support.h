#ifndef SUPPORT_H
#define SUPPORT_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (GETTEXT_PACKAGE, String)
#  undef _n
#  define _n(String, String_plural, n) dngettext (GETTEXT_PACKAGE, String, String_plural, n)
#  undef _c
#  define _c(Context, String) (strcmp(dgettext (GETTEXT_PACKAGE, Context "\004" String), Context "\004" String) == 0 ? String : dgettext (GETTEXT_PACKAGE, Context "\004" String))
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define _c(Context, String) (String)
#  define _n(String, String_plural, n) (String)
#  define N_(String) (String)
#endif

#endif
