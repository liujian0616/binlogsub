CC = gcc
CCC = g++
AR = ar
CFLAGS = -g -Wall 
SO_CFLAGS = -fPIC
SO_LDFLAGS = -shared
AR_FLAGS = -cr
LIB_DIR = -L ../lib
LIBS = -lmysqlclient -lhiredis -lssl 
INC_DIR = -I tinyxml
LOG_DIR = log

APP_DIR = .
APP_NAME = inform_mysqlrep
APP_OBJ_DIR = obj


all: ${APP_DIR} ${APP_OBJ_DIR} ${APP_DIR}/${APP_NAME} 


#for sbin
APP_OBJ = ${APP_OBJ_DIR}/inform_mysqlrep.o \
    ${APP_OBJ_DIR}/bus_charset.o \
	${APP_OBJ_DIR}/bus_config.o \
	${APP_OBJ_DIR}/bus_event.o \
 	${APP_OBJ_DIR}/bus_log.o \
 	${APP_OBJ_DIR}/bus_row.o \
 	${APP_OBJ_DIR}/bus_util.o \
 	${APP_OBJ_DIR}/my_time.o \
 	${APP_OBJ_DIR}/bus_interface.o \
 	${APP_OBJ_DIR}/myredis.o \
 	${APP_OBJ_DIR}/inform_process.o \



${APP_DIR}:
	if [ ! -d ${APP_DIR} ]; then mkdir ${APP_DIR};  fi

${APP_OBJ_DIR}:
	if [ ! -d ${APP_OBJ_DIR} ]; then mkdir ${APP_OBJ_DIR};  fi


${LOG_DIR}:
	if [ ! -d ${LOG_DIR} ]; then mkdir ${LOG_DIR};  fi

${APP_DIR}/${APP_NAME}:${APP_OBJ}
	${CCC} -g -o $@  $(filter %.o, $^) ${LIB_DIR} ${LIBS}

${APP_OBJ_DIR}/%.o:%.cpp
	${CCC} -c ${CFLAGS}  ${INC_DIR} -o $@ $<

${APP_OBJ_DIR}/%.o:%.cc
	${CCC} -c ${CFLAGS}  ${INC_DIR} -o $@ $<	

${APP_OBJ_DIR}/%.o:bus/%.cc
	${CCC} -c ${CFLAGS}  ${INC_DIR} -o $@ $<	





clean:
	rm -rf ${APP_OBJ_DIR}
	rm -rf ${APP_DIR}/${APP_NAME}
