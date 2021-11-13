FX2LIBDIR=./../external/fx2lib/
BASENAME = bulkloop
SOURCES=bulkloop.c
A51_SOURCES=dscr.a51
PID=0x1004
ADDITIONAL_FLAGS=-g

include $(FX2LIBDIR)lib/fx2.mk


test: test.cpp
	g++ ${ADDITIONAL_FLAGS} -o test test.cpp -lusb-1.0
