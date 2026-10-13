MODULE_MAIN_OBJ := module.o
COMMON_MODULES := utils gpio atecc
UDEV_RULES := 99-stratopimax.rules
UDEV_GROUP := stratopimax

SOURCE_DIR := $(if $(src),$(src),$(CURDIR))
include $(SOURCE_DIR)/commons/scripts/kmod-common.mk
