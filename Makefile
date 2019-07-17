include platform.mk

# 1.0.2
# 1.0.1 test
# test test2
# lua
LUA_INC ?= Lua_lib/lua-5-3-4
LUA_STATICLIB := $(LUA_INC)/liblua.a
LUA_LIB ?= $(LUA_STATICLIB)

CFLAGS = -g -O0 -Wall -std=c++11 -fPIC -lpthread -I$(LUA_INC) 

$(LUA_STATICLIB) :
	@echo "LUA_STATICLIB $(LUA_STATICLIB)"
	cd $(LUA_INC) && $(MAKE)

MY_BUILD_PATH ?= .
CSERVICE = ls_base ls_gateway ls_harborm ls_load_script ls_logger
LUA_CLIB_PATH ?= .
CSERVICE_PATH ?= .
LUA_CLIB = lovesnow bson client lpeg sproto #md5

LS_BASE_SRC = \
	atomic.h framework.h ls.h ls_base.h ls_daemon.h ls_env.h ls_handle.h ls_harbor.h \
	ls_log.h ls_malloc.h ls_module.h ls_monitor.h ls_mq.h ls_server.h ls_socket.h ls_timer.h \
	luashrtbl.h malloc_hook.h rwlock.h socket_epoll.h socket_info.h \
	socket_poll.h socket_server.h spinlock.h \
	ls_base.cpp ls_daemon.cpp ls_env.cpp ls_error.cpp ls_handle.cpp ls_harbor.cpp \
	ls_log.cpp ls_module.cpp ls_monitor.cpp ls_mq.cpp ls_server.cpp ls_socket.cpp \
	ls_start.cpp ls_timer.cpp malloc_hook.cpp socket_server.cpp

LS_LUA_SRC = \
	lovesnow_core.cpp lovesnow_profile.cpp ls_cluster.cpp ls_crypt.cpp ls_datasheet.cpp \
	ls_debugchannel.cpp ls_memory.cpp ls_mongo.cpp ls_multicast.cpp ls_netpack.cpp \
	ls_script_lib.cpp ls_sharedata.cpp ls_socketdriver.cpp \
	ls_stm.cpp lsha1.cpp lua-seri.cpp

LS_SRC = \
	ls_xue.cpp 

all : 
	@echo -e "\033[36m before all \033[0m"
	#$(MAKE) clean
	$(MAKE) $(foreach v, $(LUA_CLIB), $(LUA_CLIB_PATH)/$(v).so) 
	$(MAKE) $(foreach v, $(CSERVICE), $(CSERVICE_PATH)/$(v).so) 
	$(MAKE) $(MY_BUILD_PATH)/xue 
	\mv $(MY_BUILD_PATH)/xue ./Bin/ -f
	\mv $(MY_BUILD_PATH)/*.so ./Bin/ -f
	@echo -e "\033[36m after  all \033[0m"

$(MY_BUILD_PATH)/xue : $(foreach v, $(LS_SRC), ls_xue/$(v)) $(LUA_LIB) $(MALLOC_STATICLIB) \
	$(foreach v, $(CSERVICE), $(CSERVICE_PATH)/$(v).so)
	@echo -e "\033[32m before xue \033[0m"
	$(CC) $(CFLAGS) -o $@ $^ -Ils_xue -I. $(LDFLAGS) $(LOVESNOW_LIBS) 
	@echo -e "\033[32m after  xue \033[0m"

$(CSERVICE_PATH)/ls_base.so : $(foreach v, $(LS_BASE_SRC), ls_base/$(v)) $(LUA_LIB)
	@echo -e "\033[36m ls_base.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.
	
$(CSERVICE_PATH)/ls_gateway.so : ls_gateway/ls_gateway.cpp
	@echo -e "\033[36m ls_gateway.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

$(CSERVICE_PATH)/ls_harborm.so : ls_harborm/ls_harborm.cpp
	@echo -e "\033[36m ls_harborm.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

$(CSERVICE_PATH)/ls_load_script.so : ls_load_script/ls_load_script.cpp $(LUA_LIB)
	@echo -e "\033[36m ls_load_script.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

$(CSERVICE_PATH)/ls_logger.so : ls_logger/ls_logger.cpp $(LUA_LIB)
	@echo -e "\033[36m ls_logger.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

$(LUA_CLIB_PATH)/lovesnow.so : $(foreach v, $(LS_LUA_SRC), ls_script_lib/$(v)) $(LUA_LIB)
	@echo -e "\033[36m lovesnow.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

	
$(CSERVICE_PATH)/bson.so : ls_script_bson/ls_bson.cpp $(LUA_LIB)
	@echo -e "\033[36m bson.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

$(CSERVICE_PATH)/client.so : ls_script_client/ls_clientsocket.cpp \
	ls_script_client/ls_crypt.cpp ls_script_lib/lsha1.cpp $(LUA_LIB) ls_base.so
	@echo -e "\033[36m client.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

$(CSERVICE_PATH)/lpeg.so : ls_script_lpeg/lpcap.c \
	ls_script_lpeg/lpcode.c ls_script_lpeg/lpprint.c ls_script_lpeg/lptree.c ls_script_lpeg/lpvm.c $(LUA_LIB)
	@echo -e "\033[36m lpeg.so \033[0m"
	$(GCC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

$(CSERVICE_PATH)/sproto.so : ls_script_sproto/lsproto.cc ls_script_sproto/sproto.cc $(LUA_LIB)
	@echo -e "\033[36m sproto.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

$(CSERVICE_PATH)/md5.so : ls_script_md5/md5.cpp $(LUA_LIB)
	@echo -e "\033[36m md5.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

clean :
	@echo -e "\033[31m before clean \033[0m"
	rm -f $(MY_BUILD_PATH)/xue $(CSERVICE_PATH)/*.so 
	@echo -e "\033[31m after  clean \033[0m"

cleanall: clean
	cd $(LUA_INC) && $(MAKE) clean
	rm -f $(LUA_STATICLIB)
