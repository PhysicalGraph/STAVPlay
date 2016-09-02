# for verbose output do "make V=1"
E=@echo
C=@
ifneq (${V}${VERBOSE},)
E=@true
C=
endif

BLDDIR = build
WGTDIR = ${BLDDIR}/wgt
CERT ?= stav

# tizen
TIZEN_SDK_ROOT ?= ~/tizen-sdk
TIZEN_SDK_DATA ?= ~/tizen-sdk-data
TIZEN = ${TIZEN_SDK_ROOT}/tools/ide/bin/tizen

# Pepper 42 for 2016 TVs
NACL_SDK_ROOT ?= ~/nacl_sdk/pepper_42

# Build flags
WARNINGS := -Wno-long-long -Wall -Werror
CXXFLAGS := -std=gnu++0x -g $(WARNINGS)

# Tools
GETOS := python $(NACL_SDK_ROOT)/tools/getos.py
OSHELPERS = python $(NACL_SDK_ROOT)/tools/oshelpers.py
OSNAME := $(shell $(GETOS))

PNACL_TC_PATH := $(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_pnacl)
PNACL_CC := $(PNACL_TC_PATH)/bin/pnacl-clang
PNACL_CXX := $(PNACL_TC_PATH)/bin/pnacl-clang++
PNACL_TRANSLATE := $(PNACL_TC_PATH)/bin/pnacl-translate
PNACL_FINALIZE := $(PNACL_TC_PATH)/bin/pnacl-finalize
CXXFLAGS += -I$(NACL_SDK_ROOT)/include -I third/include
LDFLAGS := -L$(NACL_SDK_ROOT)/lib/pnacl/Release -L third/lib
LIBS := -lpthread -lavformat -lavcodec -lswresample -lbz2 -lavutil -lm -lc++ -lssl -lcrypto -lz -lnacl_player -lnacl_io -lppapi -lppapi_cpp

SOURCES = \
src/convert_codecs.cc \
src/elementary_stream_packet.cc \
src/logger.cc \
src/message_receiver.cc \
src/message_sender.cc \
src/player_listeners.cc \
src/player_provider.cc \
src/rtsp_player_controller.cc \
src/stav_player.cc \

NEXES = \
${BLDDIR}/stavplay_i686.nexe \
${BLDDIR}/stavplay_x86-64.nexe \
${BLDDIR}/stavplay_armv7.nexe \

OBJS := $(patsubst %.cc, ${BLDDIR}/%.po, ${SOURCES})
DEPS := $(patsubst %, %.deps, ${OBJS})


all: stavplay.wgt

# first target is default. deps are targets. thus not first
-include ${DEPS}

${BLDDIR}/stavplay.pexe: ${OBJS}
	@mkdir -p $(dir $@)
	$E "LD $@"
	$C ${PNACL_CXX} -o $@ ${CXXFLAGS} ${LDFLAGS} $^ ${LIBS}

${BLDDIR}/%.po: %.cc
	@mkdir -p $(dir $@)
	$E "CC $@"
	$C ${PNACL_CXX} -o $@ -c $< ${CXXFLAGS} -MMD -MF $@.deps

${BLDDIR}/stavplay.nmf: ${NEXES}
	@mkdir -p $(dir $@)
	$E "NMF $@"
	$C python "$(NACL_SDK_ROOT)/tools/create_nmf.py" --objdump true -o $@ $^

${BLDDIR}/stavplay_%.nexe: ${BLDDIR}/stavplay.pexe
	@mkdir -p $(dir $@)
	$E "TRANSLATE $@"
	$C ${PNACL_TRANSLATE} --allow-llvm-bitcode-input -arch $* $^ -o $@

stavplay.wgt: ${BLDDIR}/stavplay.nmf
	@mkdir -p ${WGTDIR}
	$E "BUILD-WEB $@"
	$C ${TIZEN} build-web -e "build* scripts* src* third* makefile .gitignore README.md" -out ${WGTDIR}
	$C cp ${NEXES} ${BLDDIR}/stavplay.nmf ${WGTDIR}
	$C cd ${WGTDIR}; ${TIZEN} package -t wgt -s ${CERT} >/dev/null
	$C mv ${WGTDIR}/stavplay.wgt $@

clean:
	@test "" != "${BLDDIR}" && test . != ${BLDDIR}
	rm -rf ${BLDDIR}
	rm -f stavplay.wgt

.PHONY: all clean

# disable many built-in rules
.SUFFIXES:
%: %,v
%: RCS/%
%: RCS/%,v
%: s.%
%: SCCS/s.%
