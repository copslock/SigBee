############################################
#
#Created on: 2014年5月27日
#Author: Figoo
#
############################################

#主目录
MAIN_DIR = $(shell cd ./ && echo $$PWD)

#源代码目录
SRC_DIR = $(MAIN_DIR)

#可执行目录
MAIN_EXE_DIR = $(MAIN_DIR)/bin

#库目录
SRC_LIB_DIR = $(SRC_DIR)/lib

#头文件目录
SRC_INC_DIR = $(MAIN_DIR)/include

#编译日期
BUILD_DATE = $(shell date +%Y-%m-%d)

#编译时间
BUILD_TIME = $(shell date +%H:%M:%S)

#安装目录
INSTALL_DIR = /usr/sbin/

#debug目录
DEBUG_DIR = debugs


include $(SRC_DIR)/Make.Files
include $(SRC_DIR)/Make.properties

#编译头文件目录
C_INCL = -I. -I$(SRC_INC_DIR) 

#！这里根据程序要求可能添加需要的标准库及其他库，格式-Llib_dir -lxxx
C_LIBS_DIR = -L$(SRC_LIB_DIR) 
C_LIBS =  -lbase_lib -llog -lsig_parser -ldata_parser -lui_sender -l xdr_sender -lm
#C_LIBS =  -lbase_lib -llog -lm

exe = sigbee

exe_debug = sigbee_debug

all: 
	@echo "#!/bin/sh" > ./make_date.sh
	@echo "BUILD_DATE=\`/bin/date +%Y%m%d\`" >> ./make_date.sh
	@echo "BUILD_TIME=\`/bin/date +%k%M%S\`" >> ./make_date.sh
	@echo -e "make tes \"BUILD_DATE=\x24BUILD_DATE\" \"BUILD_TIME=\x24BUILD_TIME\"" >> ./make_date.sh
	@chmod a+x ./make_date.sh
	@./make_date.sh


debug: 
	@mkdir -p $(DEBUG_DIR)
	@echo "#!/bin/sh" > ./make_date.sh
	@echo "BUILD_DATE=\`/bin/date +%Y%m%d\`" >> ./make_date.sh
	@echo "BUILD_TIME=\`/bin/date +%k%M%S\`" >> ./make_date.sh
	@echo -e "make tes_debug \"BUILD_DATE=\x24BUILD_DATE\" \"BUILD_TIME=\x24BUILD_TIME\"" >> ./make_date.sh
	@chmod a+x ./make_date.sh
	@./make_date.sh

tes: $(exe)
	
tes_debug : $(exe_debug)	

$(exe): $(OBJS) 
	$(CA) $(C_FLAGS) $(ALL_THR_FLAGS) $(ALL_THR_INCL) $(C_INCL)  -o $(MAIN_EXE_DIR)/$(exe) $(OBJS) $(C_LIBS_DIR) $(C_LIBS) $(ALL_THR_LIBS)

$(exe_debug): 
	(cd $(DEBUG_DIR);	$(CC) $(C_FLAGS) $(ALL_THR_FLAGS) $(ALL_THR_INCL) $(C_INCL) -DBUILD_DATE="$(BUILD_DATE)" -DBUILD_TIME="$(BUILD_TIME)" -DDEBUG -c $(SRCS);	$(CC) $(C_FLAGS) $(C_INCL)  $(ALL_THR_FLAGS) $(ALL_THR_INCL) -o $(exe_debug) *.o $(C_LIBS_DIR)  $(C_LIBS)  $(ALL_THR_LIBS);	cd .. )

$(OBJS): %.o:%.c
	$(CC) $(C_FLAGS) $(ALL_THR_FLAGS) $(ALL_THR_INCL) $(C_INCL) -DBUILD_DATE="$(BUILD_DATE)" -DBUILD_TIME="$(BUILD_TIME)" -c $<  -o $@
	
clean:
	rm -f $(MAIN_EXE_DIR)/$(exe) $(DEBUG_DIR)/$(exe_debug) $(OBJS) $(DEBUG_DIR)/*.o
	
install:
	cp $(MAIN_EXE_DIR)/$(exe) $(INSTALL_DIR)
	@echo "installed!"

