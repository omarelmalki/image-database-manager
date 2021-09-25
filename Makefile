LIBMONGOOSEDIR = libmongoose/
## don't forget to export LD_LIBRARY_PATH pointing to it
# export LD_LIBRARY_PATH="${PWD}"/libmongoose
# export DYLD_FALLBACK_LIBRARY_PATH="${PWD}"/libmongoose

VIPS_CFLAGS += $$(pkg-config vips --cflags)
VIPS_LIBS   += $$(pkg-config vips --libs) -lm
# JSON_LIBS += -ljson-c
# JSON_CFLAGS += $$(pkg-config --cflags json-c)
JSON_LIBS  += $$(pkg-config --libs json-c)

.PHONY: clean new newlibs style \
feedback feedback-VM-CO clone-ssh clean-fake-ssh \
submit1 submit2 submit lib

CFLAGS += -std=c11 -Wall -pedantic -g

# a bit more checks if you'd like to (uncomment)
CFLAGS += -Wextra -Wfloat-equal -Wshadow                         \
-Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings \
-Wconversion -Wunreachable-code 

# specific for testing
# CHECK_TARGETS += tests/unit-test-dedup
#  LDLIBS += -fsanitize=address #-O1 -fno-omit-frame-pointer
#  CFLAGS += -fsanitize=address -g

#  LDFLAGS += -fsanitize=address -static-libsan


# specific to MacOs (to use openssl)
LDLIBS += -L/usr/local/opt/openssl@1.1/lib 
CFLAGS += -I/usr/local/opt/openssl@1.1/include 

TARGETS := imgStoreMgr lib imgStore_server # the main executable is imgStoreMgr

all:: $(TARGETS)

lib: $(LIBMONGOOSEDIR)/libmongoose.so

$(LIBMONGOOSEDIR)/libmongoose.so: $(LIBMONGOOSEDIR)/mongoose.c  $(LIBMONGOOSEDIR)/mongoose.h
	make -C $(LIBMONGOOSEDIR)

OBJS := error.o imgst_create.o imgst_delete.o imgst_list.o tools.o util.o image_content.o dedup.o imgst_insert.o imgst_read.o imgst_gbcollect.o
RUBS = $(OBJS) core

imgStore_server: LDLIBS += -lssl -lcrypto $(VIPS_LIBS) $(JSON_LIBS) -lmongoose
imgStore_server: LDFLAGS += -L$(LIBMONGOOSEDIR)
imgStore_server: imgStore_server.o $(OBJS)


imgStoreMgr: LDLIBS += -lssl -lcrypto $(VIPS_LIBS) $(JSON_LIBS) # openssl needed for tools.o and imgst_insert
imgStoreMgr: imgStoreMgr.o $(OBJS)

imgStore_server.o: CFLAGS += -I $(LIBMONGOOSEDIR) $(VIPS_CFLAGS)
imgStore_server.o: imgStore_server.c imgStore.h error.h
image_content.o: CFLAGS += $(VIPS_CFLAGS)
imgStoreMgr.o: CFLAGS += $(VIPS_CFLAGS)
imgst_read.o: CFLAGS += $(VIPS_CFLAGS)
imgst_list.o: CFLAGS += $(JSON_CFLAGS)

# tests/unit-test-cmd_args.o: tests/unit-test-cmd_args.c tests/tests.h \
#   error.h imgStore.h
# tests/unit-test-cmd_args: tests/unit-test-cmd_args.o $(OBJS)

tests/unit-test-dedup.o: tests/unit-test-dedup.c tests/tests.h \
  error.h imgStore.h 
tests/unit-test-dedup.o: CFLAGS += $(VIPS_CFLAGS)
tests/unit-test-dedup: tests/unit-test-dedup.o $(OBJS)
tests/unit-test-dedup: LDLIBS += -lssl -lcrypto $(VIPS_LIBS)
# copied from command gcc -MM *.c
error.o: error.c
imgStoreMgr.o: imgStoreMgr.c util.h imgStore.h error.h
imgst_create.o: imgst_create.c imgStore.h error.h
imgst_delete.o: imgst_delete.c imgStore.h error.h
imgst_list.o: imgst_list.c imgStore.h error.h
tools.o: tools.c imgStore.h error.h
util.o: util.c
image_content.o: image_content.c image_content.h imgStore.h error.h
dedup.o: dedup.c dedup.h imgStore.h error.h
imgst_insert.o: imgst_insert.c imgStore.h error.h image_content.h dedup.h
imgst_read.o: imgst_read.c imgStore.h error.h
imgst_gbcollect.o: imgst_gbcollect.c imgStore.h image_content.h error.h


# ----------------------------------------------------------------------
# This part is to make your life easier. See handouts how to make use of it.

## ======================================================================
## Tests

# target to run black-box tests
check:: all
	@if ls tests/*.*.sh 1> /dev/null 2>&1; then \
	    for file in tests/*.*.sh; do [ -x $$file ] || echo "Launching $$file"; ./$$file || exit 1; done; \
	fi

# all those libs are required on Debian, adapt to your box
$(CHECK_TARGETS): LDLIBS += -lcheck -lm -pthread $(VIPS_LIBS)


check:: CFLAGS += -I. 
check:: $(CHECK_TARGETS)
	export LD_LIBRARY_PATH=.; $(foreach target,$(CHECK_TARGETS),./$(target) &&) true

clean::
	-@/bin/rm -f *.o *~ $(CHECK_TARGETS)

new: clean all

static-check:
	CCC_CC=$(CC) scan-build -analyze-headers --status-bugs -maxloop 64 make -j1 new

style:
	astyle -n -o -A8 -xt0 *.[ch]
