# tools/Makefile — standalone build for soh_export

ENABLE_SOH_EXPORT ?= 1

ifeq ($(ENABLE_SOH_EXPORT),1)
  SOH_EXPORT_OBJ = soh_export.o
  SOH_EXPORT_BIN = soh_export
  CJSON_CFLAGS = $(shell pkg-config --cflags cjson 2>/dev/null || echo -I/usr/include/cjson)
  CJSON_LIBS   = $(shell pkg-config --libs cjson 2>/dev/null || echo -lcjson)
else
  SOH_EXPORT_OBJ =
  SOH_EXPORT_BIN =
  CJSON_CFLAGS =
  CJSON_LIBS   =
endif

all: $(SOH_EXPORT_BIN)

$(SOH_EXPORT_BIN): $(SOH_EXPORT_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(CJSON_LIBS)

soh_export.o: soh_export.c
	$(CC) $(CFLAGS) $(CJSON_CFLAGS) -c -o $@ $<

clean:
	rm -f $(SOH_EXPORT_OBJ) $(SOH_EXPORT_BIN)

.PHONY: all clean
