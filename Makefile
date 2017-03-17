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
APP_NAME = binlogsub 
APP_OBJ_DIR = obj


all: ${APP_DIR} ${APP_OBJ_DIR} ${APP_DIR}/${APP_NAME} 


APP_OBJ = ${APP_OBJ_DIR}/process.o \
		${APP_OBJ_DIR}/config.o \
		${APP_OBJ_DIR}/myregex.o \
		${APP_OBJ_DIR}/schema.o \
		${APP_OBJ_DIR}/log.o \
		${APP_OBJ_DIR}/mysqlProcess.o \
		${APP_OBJ_DIR}/util.o \
		${APP_OBJ_DIR}/packet.o \
		${APP_OBJ_DIR}/memblock.o \
		${APP_OBJ_DIR}/event.o \
		${APP_OBJ_DIR}/row.o \
		${APP_OBJ_DIR}/my_time.o \
		${APP_OBJ_DIR}/mydecimal.o \
		${APP_OBJ_DIR}/business.o \
		${APP_OBJ_DIR}/myredis.o \



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
