AC_DEFUN(LCD_DRIVERS_SELECT, [
AC_CHECKING(for which drivers to compile)

AC_ARG_ENABLE(drivers,
  	[  --enable-drivers=<list> compile driver for LCDs in <list>.]
	[                  drivers may be separated with commas.]
  	[                  Possible choices are:]
 	[                    mtxorb,cfontz,curses,text,lb216,]
 	[                    hd44780,joy,irman,lircin,bayrad,glk,]
 	[                    stv5730,sed1330,sed1520,svgalib,lcdm001,t6963]
	[                  \"all\" compiles all drivers],
  	drivers="$enableval", 
  	drivers=[lcdm001,mtxorb,cfontz,curses,text,lb216,bayrad,glk])

if test "$drivers" = "all"; then
	drivers=[mtxorb,cfontz,curses,text,lb216,hd44780,joy,irman,lircin,bayrad,glk,stv5730,sed1330,sed1520,svgalib,lcdm001,t6963]
fi

  	drivers=`echo $drivers | sed 's/,/ /g'`
  	for driver in $drivers
  	do

    case "$driver" in
        lcdm001)
			DRIVERS="$DRIVERS lcdm001.o"
			AC_DEFINE(LCDM001_DRV)
			actdrivers=["$actdrivers lcdm001"]
			;;
        mtxorb)
			DRIVERS="$DRIVERS MtxOrb.o"
			AC_DEFINE(MTXORB_DRV)
			actdrivers=["$actdrivers mtxorb"]
			;;
		glk)
			DRIVERS="$DRIVERS glk.o glkproto.o"
			AC_DEFINE(GLK_DRV)
			actdrivers=["$actdrivers glk"]
			;;
		bayrad)
			DRIVERS="$DRIVERS bayrad.o"
			AC_DEFINE(BAYRAD_DRV)
			actdrivers=["$actdrivers bayrad"]
			;;
		cfontz)
			DRIVERS="$DRIVERS CFontz.o"
			AC_DEFINE(CFONTZ_DRV)
			actdrivers=["$actdrivers cfontz"]
			;;
		sli)	
			DRIVERS="$DRIVERS wirz-sli.o"
			AC_DEFINE(SLI_DRV)
			actdrivers=["$actdrivers sli"]
			;;
		curses)
			AC_CHECK_HEADERS(ncurses.h curses.h)
 			AC_CHECK_LIB(ncurses, main, 
 				AC_CHECK_HEADER(ncurses.h,
 					dnl We have ncurses.h and libncurses, add driver.
	 				LIBCURSES="-lncurses"
 					DRIVERS="$DRIVERS curses_drv.o"
 					AC_DEFINE(CURSES_DRV)
 					actdrivers=["$actdrivers curses"]
 				,
dnl				else
					AC_MSG_WARN([Could not find ncurses.h]),
				)
 			,
dnl			else
 				AC_CHECK_LIB(curses, main, 
 					AC_CHECK_HEADER(curses.h,
 						dnl We have curses.h and libcurses, add driver.
 						LIBCURSES="-lcurses"
 						DRIVERS="$DRIVERS curses_drv.o"
 						AC_DEFINE(CURSES_DRV)
 						actdrivers=["$actdrivers curses"]
 					,
dnl					else
						AC_MSG_WARN([Could not find curses.h]),
					)
 				,
dnl				else
 					AC_MSG_WARN([The curses driver needs the curses (or ncurses) library.]),
 				)
 			)
 			
 			AC_CURSES_ACS_ARRAY
 			
 			AC_CACHE_CHECK([for redrawwin() in curses], ac_cv_curses_redrawwin,
			[oldlibs="$LIBS"
			 LIBS="$LIBS $LIBCURSES"
			 AC_TRY_LINK_FUNC(redrawwin, ac_cv_curses_redrawwin=yes, ac_cv_curses_redrawwin=no)
			 LIBS="$oldlibs"
			])
			if test "$ac_cv_curses_redrawwin" = yes; then
				AC_DEFINE(CURSES_HAS_REDRAWWIN)
			fi
 						
			AC_CACHE_CHECK([for wcolor_set() in curses], ac_cv_curses_wcolor_set,
			[oldlibs="$LIBS"
			 LIBS="$LIBS $LIBCURSES"
			 AC_TRY_LINK_FUNC(wcolor_set, ac_cv_curses_wcolor_set=yes, ac_cv_curses_wcolor_set=no)
			 LIBS="$oldlibs"
			])
			if test "$ac_cv_curses_wcolor_set" = yes; then
				AC_DEFINE(CURSES_HAS_WCOLOR_SET)
			fi
			;;
		text)
			DRIVERS="$DRIVERS text.o"
			AC_DEFINE(TEXT_DRV)
			actdrivers=["$actdrivers text"]
			;;
		lb216)
			DRIVERS="$DRIVERS lb216.o"
			AC_DEFINE(LB216_DRV)
			actdrivers=["$actdrivers lb216"]
			;;
		hd44780)
			DRIVERS="$DRIVERS hd44780.o hd44780-4bit.o hd44780-ext8bit.o lcd_sem.o hd44780-serialLpt.o hd44780-winamp.o"
			AC_DEFINE(HD44780_DRV)
			actdrivers=["$actdrivers hd44780"]
			;;
		joy)	
			AC_CHECK_HEADER(linux/joystick.h,
				DRIVERS="$DRIVERS joy.o"
				AC_DEFINE(JOY_DRV)
				actdrivers=["$actdrivers joy"]
				,
dnl				else
 				AC_MSG_WARN([The joy driver needs header file linux/joystick.h.])
 			)
			;;
		irman)
 			AC_CHECK_LIB(irman, main, 
 				LIBIRMAN="-lirman"
 				DRIVERS="$DRIVERS irmanin.o"
 				AC_DEFINE(IRMANIN_DRV)
 				actdrivers=["$actdrivers irman"]
 				,
dnl				else
 				AC_MSG_WARN([The irman driver needs the irman library.])
 			)
			;;
		lircin)
			AC_CHECK_LIB(lirc_client, main, 
				LIBLIRC_CLIENT="-llirc_client"
				DRIVERS="$DRIVERS lircin.o"
				AC_DEFINE(LIRCIN_DRV)
				actdrivers=["$actdrivers lircin"]
				,
dnl				else
				AC_MSG_WARN([The lirc driver needs the lirc client library])
			)
			;;
		sed1330)
			DRIVERS="$DRIVERS sed1330.o"
			AC_DEFINE(SED1330_DRV)
			actdrivers=["$actdrivers sed1330"]
			;;
		sed1520)
			DRIVERS="$DRIVERS sed1520.o"
			AC_DEFINE(SED1520_DRV)
			actdrivers=["$actdrivers sed1520"]
			;;
		stv5730)
			DRIVERS="$DRIVERS stv5730.o"
			AC_DEFINE(STV5730_DRV)
			actdrivers=["$actdrivers stv5730"]
			;;
		svgalib)
			AC_CHECK_LIB(vga, main, 
				LIBSVGA="-lvga -lvgagl"
				DRIVERS="$DRIVERS svgalib_drv.o"
				AC_DEFINE(SVGALIB_DRV)
				actdrivers=["$actdrivers svgalib"]
				,
dnl				else
				AC_MSG_WARN([The svgalib driver needs the vga library]))
			;;
		t6963)
			DRIVERS="$DRIVERS t6963.o"
			AC_DEFINE(T6963_DRV)
			actdrivers=["$actdrivers t6963"]
			;;
	*) 	
			AC_MSG_ERROR([Unknown driver $driver])
			;;
  		esac
  	done

actdrivers=`echo $actdrivers | sed 's/ /,/g'`
AC_MSG_RESULT([Will compile drivers: $actdrivers])

AC_SUBST(LIBCURSES)
AC_SUBST(LIBIRMAN)
AC_SUBST(LIBLIRC_CLIENT)
AC_SUBST(LIBSVGA)
AC_SUBST(DRIVERS)
])


dnl
dnl Curses test to check if we use _acs_char* or acs_map*
dnl
AC_DEFUN(AC_CURSES_ACS_ARRAY, [
	AC_CACHE_CHECK([for acs_map in curses.h], ac_cv_curses_acs_map, 
	[AC_TRY_COMPILE([#include <curses.h>], [ char map = acs_map['p'] ], ac_cv_curses_acs_map=yes, ac_cv_curses_acs_map=no)])
	
	if test "$ac_cv_curses_acs_map" = yes
	then
		AC_DEFINE(CURSES_HAS_ACS_MAP)
	else
	
		AC_CACHE_CHECK([for _acs_char in curses.h], ac_cv_curses__acs_char, 
		[AC_TRY_COMPILE([#include <curses.h>], [ char map = _acs_char['p'] ], ac_cv_curses__acs_char=yes, ac_cv_curses__acs_char=no)])

		if test "$ac_cv_curses__acs_char" = yes
		then
			AC_DEFINE(CURSES_HAS__ACS_CHAR)
		fi
		
	fi
])

dnl
dnl Find out where is the mounted filesystem table
dnl

AC_DEFUN([AC_FIND_MTAB_FILE], [
	AC_CACHE_CHECK([for your mounted filesystem table], ac_cv_mtab_file, [
		dnl Linux
		if test -e "/etc/mtab"; then
			ac_cv_mtab_file=/etc/mtab
		else 
			dnl Solaris
			if test -e "/etc/mnttab"; then
				ac_cv_mtab_file=/etc/mnttab
			else
				dnl BSD
				if test -e "/etc/fstab"; then
					ac_cv_mtab_file=/etc/fstab
				fi
			fi
		fi
	])
	if test "$ac_cv_mtab_file"; then
		AC_DEFINE_UNQUOTED([MTAB_FILE], ["$ac_cv_mtab_file"], [Location of your mounted filesystem table file])
	fi
])

dnl
dnl Filesystem information detection
dnl
dnl To get information about the disk, mount points, etc.
dnl
dnl This code is stolen from mc-4.5.41, which in turn has stolen it
dnl from GNU fileutils-3.12.
dnl

AC_DEFUN(AC_GET_FS_INFO, [
    AC_CHECK_HEADERS(fcntl.h sys/dustat.h sys/param.h sys/statfs.h sys/fstyp.h)
    AC_CHECK_HEADERS(mnttab.h mntent.h utime.h sys/statvfs.h sys/vfs.h)
    AC_CHECK_HEADERS(sys/mount.h sys/filsys.h sys/fs_types.h)
    AC_CHECK_FUNCS(getmntinfo)

    AC_CHECKING(how to get filesystem space usage)
    space=no

    # Here we'll compromise a little (and perform only the link test)
    # since it seems there are no variants of the statvfs function.
    if test $space = no; then
      # SVR4
      AC_CHECK_FUNCS(statvfs)
      if test $ac_cv_func_statvfs = yes; then
        space=yes
        AC_DEFINE(STAT_STATVFS)
      fi
    fi

    if test $space = no; then
      # DEC Alpha running OSF/1
      AC_MSG_CHECKING([for 3-argument statfs function (DEC OSF/1)])
      AC_CACHE_VAL(fu_cv_sys_stat_statfs3_osf1,
      [AC_TRY_RUN([
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
      main ()
      {
        struct statfs fsd;
        fsd.f_fsize = 0;
        exit (statfs (".", &fsd, sizeof (struct statfs)));
      }],
      fu_cv_sys_stat_statfs3_osf1=yes,
      fu_cv_sys_stat_statfs3_osf1=no,
      fu_cv_sys_stat_statfs3_osf1=no)])
      AC_MSG_RESULT($fu_cv_sys_stat_statfs3_osf1)
      if test $fu_cv_sys_stat_statfs3_osf1 = yes; then
        space=yes
        AC_DEFINE(STAT_STATFS3_OSF1)
      fi
    fi

    if test $space = no; then
    # AIX
      AC_MSG_CHECKING([for two-argument statfs with statfs.bsize member (AIX, 4.3BSD)])
      AC_CACHE_VAL(fu_cv_sys_stat_statfs2_bsize,
      [AC_TRY_RUN([
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
      main ()
      {
      struct statfs fsd;
      fsd.f_bsize = 0;
      exit (statfs (".", &fsd));
      }],
      fu_cv_sys_stat_statfs2_bsize=yes,
      fu_cv_sys_stat_statfs2_bsize=no,
      fu_cv_sys_stat_statfs2_bsize=no)])
      AC_MSG_RESULT($fu_cv_sys_stat_statfs2_bsize)
      if test $fu_cv_sys_stat_statfs2_bsize = yes; then
        space=yes
        AC_DEFINE(STAT_STATFS2_BSIZE)
      fi
    fi

    if test $space = no; then
    # SVR3
      AC_MSG_CHECKING([for four-argument statfs (AIX-3.2.5, SVR3)])
      AC_CACHE_VAL(fu_cv_sys_stat_statfs4,
      [AC_TRY_RUN([#include <sys/types.h>
#include <sys/statfs.h>
      main ()
      {
      struct statfs fsd;
      exit (statfs (".", &fsd, sizeof fsd, 0));
      }],
        fu_cv_sys_stat_statfs4=yes,
        fu_cv_sys_stat_statfs4=no,
        fu_cv_sys_stat_statfs4=no)])
      AC_MSG_RESULT($fu_cv_sys_stat_statfs4)
      if test $fu_cv_sys_stat_statfs4 = yes; then
        space=yes
        AC_DEFINE(STAT_STATFS4)
      fi
    fi
    if test $space = no; then
    # 4.4BSD and NetBSD
      AC_MSG_CHECKING([for two-argument statfs with statfs.fsize dnl
    member (4.4BSD and NetBSD)])
      AC_CACHE_VAL(fu_cv_sys_stat_statfs2_fsize,
      [AC_TRY_RUN([#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
      main ()
      {
      struct statfs fsd;
      fsd.f_fsize = 0;
      exit (statfs (".", &fsd));
      }],
      fu_cv_sys_stat_statfs2_fsize=yes,
      fu_cv_sys_stat_statfs2_fsize=no,
      fu_cv_sys_stat_statfs2_fsize=no)])
      AC_MSG_RESULT($fu_cv_sys_stat_statfs2_fsize)
      if test $fu_cv_sys_stat_statfs2_fsize = yes; then
        space=yes
        AC_DEFINE(STAT_STATFS2_FSIZE)
      fi
    fi

    if test $space = no; then
      # Ultrix
      AC_MSG_CHECKING([for two-argument statfs with struct fs_data (Ultrix)])
      AC_CACHE_VAL(fu_cv_sys_stat_fs_data,
      [AC_TRY_RUN([
#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_FS_TYPES_H
#include <sys/fs_types.h>
#endif
      main ()
      {
      struct fs_data fsd;
      /* Ultrix's statfs returns 1 for success,
         0 for not mounted, -1 for failure.  */
      exit (statfs (".", &fsd) != 1);
      }],
      fu_cv_sys_stat_fs_data=yes,
      fu_cv_sys_stat_fs_data=no,
      fu_cv_sys_stat_fs_data=no)])
      AC_MSG_RESULT($fu_cv_sys_stat_fs_data)
      if test $fu_cv_sys_stat_fs_data = yes; then
        space=yes
        AC_DEFINE(STAT_STATFS2_FS_DATA)
      fi
    fi

    dnl Not supported
    dnl if test $space = no; then
    dnl # SVR2
    dnl AC_TRY_CPP([#include <sys/filsys.h>],
    dnl   AC_DEFINE(STAT_READ_FILSYS) space=yes)
    dnl fi
])

dnl 1.1 (2001/07/26) -- Miscellaneous @ ac-archive-0.5.32 
dnl Warren Young <warren@etr-usa.com> 
dnl This macro checks for the SysV IPC header files. It only checks 
dnl that you can compile a program with them, not whether the system 
dnl actually implements working SysV IPC. 
dnl http://ac-archive.sourceforge.net/Miscellaneous/etr_sysv_ipc.html
AC_DEFUN([ETR_SYSV_IPC],
[
AC_CACHE_CHECK([for System V IPC headers], ac_cv_sysv_ipc, [
        AC_TRY_COMPILE(
                [
                        #include <sys/types.h>
                        #include <sys/ipc.h>
                        #include <sys/msg.h>
                        #include <sys/sem.h>
                        #include <sys/shm.h>
                ],, ac_cv_sysv_ipc=yes, ac_cv_sysv_ipc=no)
])

        if test x"$ac_cv_sysv_ipc" = "xyes"
        then
                AC_DEFINE(HAVE_SYSV_IPC, 1, [ Define if you have System V IPC ])
        fi
]) dnl ETR_SYSV_IPC

dnl 1.1 (2001/07/26) -- Miscellaneous @ ac-archive-0.5.32 
dnl Warren Young <warren@etr-usa.com> 
dnl This macro checks to see if sys/sem.h defines union semun. Some 
dnl systems do, some systems don't. Your code must be able to deal with 
dnl this possibility; if HAVE_STRUCT_SEMUM isn't defined for a given system, 
dnl you have to define this structure before you can call functions 
dnl like semctl(). 
dnl You should call ETR_SYSV_IPC before this macro, to separate the check 
dnl for System V IPC headers from the check for struct semun. 
dnl http://ac-archive.sourceforge.net/Miscellaneous/etr_struct_semun.html
AC_DEFUN([ETR_UNION_SEMUN],
[
AC_CACHE_CHECK([for union semun], ac_cv_union_semun, [
        AC_TRY_COMPILE(
                [
                        #include <sys/types.h>
                        #include <sys/ipc.h>
                        #include <sys/sem.h>
                ],
                [ union semun s ],
                ac_cv_union_semun=yes,
                ac_cv_union_semun=no)
])

        if test x"$ac_cv_union_semun" = "xyes"
        then
                AC_DEFINE(HAVE_UNION_SEMUN, 1,
                        [ Define if your system's sys/sem.h file defines union semun ])
        fi
]) dnl ETR_UNION_SEMUN
