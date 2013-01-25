AM_CFLAGS = -I$(top_srcdir)/include --include=config.h

check_PROGRAMS = cjson

LDADD = $(top_builddir)/src/libcjson.la -lec -lecx_libc -lecx_Judy -lJudy -lccstreams -lecx_ccstreams -lpthread
