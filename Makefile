MODULE_MAIN_OBJ := module.o
COMMON_MODULES := utils gpio atecc
MODULE_EXTRA_OBJS :=
UDEV_RULES := 99-stratopimax.rules

include commons/scripts/kmod-common.mk
