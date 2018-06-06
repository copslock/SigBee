###############################################################################
# SET ARCH and tools when included in Makefile
#   Sets ARCH to tilegx
#   Sets VERSION to native release or cross MDE version
#   Sets the tools to native or cross versions
###############################################################################
# This target ARCH will be redefined below when doing cross compile
ARCH   := ${shell uname -m}

ifneq ($(filter tile%,${ARCH}),)  # if tile machine
  CROSS := no
else
  ifeq (${TILERA_ROOT},)
    $(error This library requires that TILERA_ROOT be set to a TILEGx MDE)
  endif
  CROSS   := yes
  CC       = ${TILERA_ROOT}/bin/tile-gcc
  CXX      = ${TILERA_ROOT}/bin/tile-g++
  MPIPE_CC = ${TILERA_ROOT}/bin/tile-mpipe-cc
  AR       = ${TILERA_ROOT}/bin/tile-ar
  RANLIB   = ${TILERA_ROOT}/bin/tile-ranlib
  STRIP    = ${TILERA_ROOT}/bin/tile-strip
  SIM      = ${TILERA_ROOT}/bin/tile-sim
  MONITOR  = ${TILERA_ROOT}/bin/tile-monitor
  VERSION  = "MDE:" ${shell ${SIM} --version }
  include  ${TILERA_ROOT}/etc/chip.mk
  ifeq (${TILE_ARCH},tilegx)
    ARCH  = tilegx
  else
    ARCH  = tilepro
  endif
endif

ifeq (${CROSS},no)
  CC       = gcc
  CXX      = g++
  MPIPE_CC = mpipe-cc
  AR       = ar
  RANLIB   = ranlib
  STRIP    = strip
  VERSION  = ${shell uname -r }
endif

ifeq ($(filter tilegx,${ARCH}),)
    $(error This library requires TILEGx or TILERA_ROOT set to a TILEGx MDE)
endif
