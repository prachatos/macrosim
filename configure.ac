# -*- Autoconf -*-
#
#

# Set Version of SST forzasim_macro Library
AC_INIT([SST forzasim_macro Library], [-dev], [dean.chester@cornelisnetworks.com])

AC_PREREQ([2.59])
AC_COPYRIGHT([Copyright Dean Chester, 2023])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE([1.9.6 foreign dist-bzip2 subdir-objects no-define tar-pax])

# If Automake supports silent rules, enable them.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_HEADER([src/sst_forzasim_macro_config.h])

m4_include([config/sst_forzasim_macro_include.m4])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_C99
AC_C_INLINE
AC_PROG_MAKE_SET

AC_PROG_CXX
AC_CACHE_SAVE

AC_PROG_CPP
AC_CANONICAL_HOST

AC_CACHE_SAVE
AC_HEADER_TIME

LT_CONFIG_LTDL_DIR([src/libltdl])
LT_INIT([shared disable-static dlopen])
LTDL_INIT([recursive])

AC_HEADER_TIME
AC_CACHE_SAVE

SST_CORE_CHECK_INSTALL()

SST_CHECK_OSX()

SST_FORZASIM_MACRO_CONFIG_OUTPUT()

# Compile flags come from SST-Core, add or remove extra warnings
# depending on the use-picky flag
SST_CHECK_PICKY
AS_IF([test "x$use_picky" = "xyes"],
      [WARNFLAGS="-Wall -Wextra"],
      [WARNFLAGS=""])
CFLAGS="$CFLAGS $WARNFLAGS"
CXXFLAGS="$CXXFLAGS $WARNFLAGS"

AC_MSG_CHECKING([for SST-forzasim_macro Git Branch, Head SHA and Commit Count])
if test -d ".git" ; then
    SSTFORZASIM_MACRO_GIT_BRANCH=`git rev-parse --abbrev-ref HEAD`
    AC_MSG_RESULT([${SSTFORZASIM_MACRO_GIT_BRANCH}])
    SSTFORZASIM_MACRO_GIT_HEADSHA=`git rev-parse HEAD`
    AC_MSG_RESULT([${SSTFORZASIM_MACRO_GIT_HEADSHA}])
    SSTFORZASIM_MACRO_GIT_COMMITCOUNT=`git rev-list HEAD | wc -l | sed -e 's/^ *//g'`
    AC_MSG_RESULT([${SSTFORZASIM_MACRO_GIT_COMMITCOUNT}])
else
    SSTFORZASIM_MACRO_GIT_BRANCH="N/A"
    AC_MSG_RESULT([${SSTFORZASIM_MACRO_GIT_BRANCH}])
    SSTFORZASIM_MACRO_GIT_HEADSHA="${PACKAGE_VERSION}"
    AC_MSG_RESULT([${SSTFORZASIM_MACRO_GIT_HEADSHA}])
    SSTFORZASIM_MACRO_GIT_COMMITCOUNT="0"
    AC_MSG_RESULT([${SSTFORZASIM_MACRO_GIT_COMMITCOUNT}])
fi

AC_SUBST(SSTFORZASIM_MACRO_GIT_BRANCH)
AC_DEFINE_UNQUOTED([SSTFORZASIM_MACRO_GIT_BRANCH], ["$SSTFORZASIM_MACRO_GIT_BRANCH"], [SST-forzasim_macro Git Branch])

AC_SUBST(SSTFORZASIM_MACRO_GIT_HEADSHA)
AC_DEFINE_UNQUOTED([SSTFORZASIM_MACRO_GIT_HEADSHA], ["$SSTFORZASIM_MACRO_GIT_HEADSHA"], [SST-forzasim_macro Git Head SHA])

AC_SUBST(SSTFORZASIM_MACRO_GIT_COMMITCOUNT)
AC_DEFINE_UNQUOTED([SSTFORZASIM_MACRO_GIT_COMMITCOUNT], ["$SSTFORZASIM_MACRO_GIT_COMMITCOUNT"], [SST-forzasim_macro Git Commit Count])

CC_VERSION=`$CC --version | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n/%/g' | awk -F'%' '{print $1}'`
AC_SUBST(CC_VERSION)

SST_CHECK_PTRACE_SET_TRACER()

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/sst/Makefile
  src/sst/forzasim_macro/Makefile
])

AC_OUTPUT

echo ""
echo "-------------------------------------------------------"
echo "SST forzasim_macro Configuration completed."
echo "-------------------------------------------------------"
echo "Build Environment:"
echo ""
if test "x$SST_PREVIEW_BUILD" = "xyes"; then
echo "-- PREVIEW BUILD --"
echo ""
fi
printf "%38s : %s\n" "SST-forzasim_macro Version" "$PACKAGE_VERSION"
if test "x$SSTFORZASIM_MACRO_GIT_HEADSHA" != "x$PACKAGE_VERSION"; then
printf "%38s : %s\n" "Git Branch" "$SSTFORZASIM_MACRO_GIT_BRANCH"
printf "%38s : %s\n" "Git HEAD SHA" "$SSTFORZASIM_MACRO_GIT_HEADSHA"
printf "%38s : %s\n" "Branch Commit Count" "$SSTFORZASIM_MACRO_GIT_COMMITCOUNT"
else
printf "%38s : %s\n" "Version" "SST-forzasim_macro $PACKAGE_VERSION"
fi
printf "%38s : %s\n" "SST-forzasim_macro Prefix" "$prefix"
printf "%38s : %s\n" "SST-Core Prefix" "$SST_PREFIX"

printf "%38s : %s\n" "Preprocessor" "$CPP"
printf "%38s : %s\n" "C++ Compiler" "$CXX"
printf "%38s : %s\n" "C Compiler" "$CC"
printf "%38s : %s\n" "Compiler Version" "$CC_VERSION"
printf "%38s : %s\n" "Linker" "$LD"
printf "%38s : %s\n" "Preprocessor Options" "$CPPFLAGS"
printf "%38s : %s\n" "C Compiler Options" "$CFLAGS"
printf "%38s : %s\n" "C++ Compiler Options" "$CXXFLAGS"

if test "x$found_cxx1y" = "xyes"; then
printf "%38s : %s\n" "C++ Standard Compliance" "C++1Y/14"
else
printf "%38s : %s\n" "C++ Standard Compliance" "C++11"
fi

printf "%38s : %s\n" "Linker Options" "$LDFLAGS"
printf "%38s : %s\n" "Boost Library Preprocessor" "$BOOST_CPPFLAGS"
printf "%38s : %s\n" "Boost Library Linker Options" "$BOOST_LDFLAGS"
echo ""
echo "-------------------------------------------------------"
echo "Configuration Information (Found Dependencies):"
echo ""

prefix="sst_check_"
suffix=".m4"

list=`ls $srcdir/config/sst_check*.m4`

for depfilepath in $list; do

    # Get the basename of the file
    depfilename=`basename $depfilepath`

    # Strip off the .m4 from the filename
    depfile=${depfilename%$suffix}

    # Extract the subname from the depfile string name
    depsubname=$depfile
    depsubname=${depsubname#$prefix}

    happytest=${depfile}_happy

    if test "x${!happytest}" = "xyes" ; then
        printf "%38s : YES\n" $depsubname
    else
        printf "%38s : No\n" $depsubname
    fi

done

echo ""
echo "-------------------------------------------------------"
echo "Configuration Information (Make will build the following forzasim_macro):"
echo ""

for e in $dist_forzasim_macro_libraries
do
    echo "$active_forzasim_macro_libraries" | grep "$e" >/dev/null
    if test $? -eq 0 ; then
        if test -e "./src/sst/forzasim_macro/$e/.deprecated" ; then
            msg="YES - DEPRECATED"
        else
            msg="YES"
        fi
    else
        msg="no"
    fi
    printf "%38s : $msg\n" "$e"
done

echo ""
echo "-------------------------------------------------------"
echo ""
