############################################
#
#Created on: 2014年5月27日
#Author: Figoo
#
############################################

#主目录
MAIN_DIR = $(shell cd ../ && echo $$PWD)

#源代码目录
SRC_DIR = $(MAIN_DIR)/xdr_sender

#库目录
SRC_LIB_DIR = $(MAIN_DIR)/lib

#头文件目录
SRC_INC_DIR = $(MAIN_DIR)/include

#编译日期
BUILD_DATE = $(shell date +%Y-%m-%d)

#编译时间
BUILD_TIME = $(shell date +%H:%M:%S)

#安装目录
INSTALL_DIR = $(SRC_LIB_DIR)

xdr_sender_a = libxdr_sender.a

include $(SRC_DIR)/Make.Files
include $(SRC_DIR)/Make.properties

#编译头文件目录
C_INCL = -I. -I$(SRC_INC_DIR)  

all: 
	@echo "#!/bin/sh" > ./make_date.sh
	@echo "BUILD_DATE=\`/bin/date +%Y%m%d\`" >> ./make_date.sh
	@echo "BUILD_TIME=\`/bin/date +%k%M%S\`" >> ./make_date.sh
	@echo -e "make xdr_sender_module \"BUILD_DATE=\x24BUILD_DATE\" \"BUILD_TIME=\x24BUILD_TIME\"" >> ./make_date.sh
	@chmod a+x ./make_date.sh
	@./make_date.sh

xdr_sender_module: $(xdr_sender_a)

$(xdr_sender_a): $(OBJS) 
	$(AR) cr $@ $^  

$(OBJS): %.o:%.c
	$(CC) $(C_FLAGS) $(ALL_THR_FLAGS) $(ALL_THR_INCL) $(C_INCL) -DBUILD_DATE="$(BUILD_DATE)" -DBUILD_TIME="$(BUILD_TIME)" -c $<  -o $@
	
clean:
	rm -f $(xdr_sender_a)  $(OBJS)
	
install:
	cp $(xdr_sender_a) $(INSTALL_DIR)
	

		









