
AC_DEFUN([SST_CHECK_PICKY], [
  use_picky="no"

  AC_ARG_ENABLE([picky-warnings], [AS_HELP_STRING([--enable-picky-warnings],
      [Enable the use of compiler flags -Wall -Wextra within SST forzasim_macro.])])

  AS_IF([test "x$enable_picky_warnings" = "xyes"], [use_picky="yes"])
 
])
