AM_CFLAGS = -I$(top_srcdir)/include --include=config.h @CHECK_CFLAGS@

TESTS = jestr string number boolean null array pair object root
check_PROGRAMS = jestr string number boolean null array pair object root

LDADD = $(top_builddir)/src/libcjson.la -lec -lecx_libc -lecx_Judy -lJudy -lccstreams -lecx_ccstreams -lpthread @CHECK_LIBS@
