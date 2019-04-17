PLAT ?= none
PLATS = linux freebsd macosx

CC := g++

#ifneq ($(PLAT), none)

.PHONY : default

default :
	$(MAKE) $(PLAT)

#endif

none :
	@echo "Please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "   $(PLATS)"

MY_LIBS := -lpthread -lm 

SHARED := -fPIC --shared -fpermissive -Wno-sign-compare

EXPORT := -Wl,-E

linux : PLAT = linux
macosx : PLAT = macosx
freebsd : PLAT = freebsd

macosx : SHARED := -fPIC -dynamiclib -Wl,-undefined,dynamic_lookup
macosx : EXPORT :=
macosx linux : LOVEXUE_LIBS += -ldl
linux freebsd : LOVEXUE_LIBS += -lrt

# Turn off jemalloc and malloc hook on macosx
macosx : MALLOC_STATICLIB :=
macosx : LOVEXUE_DEFINES :=-DNOUSE_JEMALLOC

linux macosx freebsd :
	@echo -e "\033[33m before make all \033[0m"
	$(MAKE) all PLAT=$@ MY_LIBS="$(MY_LIBS)" SHARED="$(SHARED)" EXPORT="$(EXPORT)" MALLOC_STATICLIB="$(MALLOC_STATICLIB)" LOVEXUE_DEFINES="$(LOVEXUE_DEFINES)" LOVEXUE_LIBS="$(LOVEXUE_LIBS)"
	@echo -e "\033[33m after  make all \033[0m"


