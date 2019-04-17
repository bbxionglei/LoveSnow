include platform.mk


# lua
LUA_INC ?= Lua_lib/lua-5-3-4
LUA_STATICLIB := $(LUA_INC)/liblua.a
LUA_LIB ?= $(LUA_STATICLIB)

CFLAGS = -g -O0 -gstabs+ -Wall -std=c++11 -fPIC -I$(LUA_INC) 

$(LUA_STATICLIB) :
	@echo "LUA_STATICLIB $(LUA_STATICLIB)"
	cd $(LUA_INC) && $(MAKE)

MY_BUILD_PATH ?= .
CSERVICE = ls_gateway ls_base ls_load_script mLualib
CSERVICE_PATH ?= .

LS_BASE_SRC = \
	ls_base.cpp ls_module.cpp
	
LS_XUE_SRC = \
	ls_xue.cpp 

all : 
	@echo -e "\033[36m before all \033[0m"
	#$(MAKE) clean
	#$(MAKE) $(foreach v, $(LUA_CLIB), $(LUA_CLIB_PATH)/$(v).so) 
	$(MAKE) $(foreach v, $(CSERVICE), $(CSERVICE_PATH)/$(v).so) 
	$(MAKE) $(MY_BUILD_PATH)/xue 
	\mv $(MY_BUILD_PATH)/xue ./Bin/ -f
	\mv $(MY_BUILD_PATH)/*.so ./Bin/ -f
	@echo -e "\033[36m after  all \033[0m"

$(MY_BUILD_PATH)/xue : $(foreach v, $(LS_XUE_SRC), ls_xue/$(v)) $(LUA_LIB) $(MALLOC_STATICLIB) \
	$(foreach v, $(CSERVICE), $(CSERVICE_PATH)/$(v).so)
	@echo -e "\033[32m before xue \033[0m"
	$(CC) $(CFLAGS) -o $@ $^ -Ils_xue -I. $(LDFLAGS) $(LOVEXUE_LIBS) 
	@echo -e "\033[32m after  xue \033[0m"

$(CSERVICE_PATH)/ls_base.so : $(foreach v, $(LS_BASE_SRC), ls_base/$(v))
	@echo -e "\033[36m ls_base.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.
	
$(CSERVICE_PATH)/ls_gateway.so : ls_gateway/ls_gateway.cpp
	@echo -e "\033[36m ls_gateway.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

$(CSERVICE_PATH)/ls_load_script.so : ls_load_script/ls_load_script.cpp $(LUA_LIB)
	@echo -e "\033[36m ls_load_script.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

$(CSERVICE_PATH)/mLualib.so : ls_script_lib/ls_script_lib.cpp $(LUA_LIB)
	@echo -e "\033[36m mLualib.so \033[0m"
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I.

clean :
	@echo -e "\033[31m before clean \033[0m"
	rm -f $(MY_BUILD_PATH)/xue $(CSERVICE_PATH)/*.so 
	@echo -e "\033[31m after  clean \033[0m"

cleanall: clean
	cd $(LUA_INC) && $(MAKE) clean
	rm -f $(LUA_STATICLIB)
