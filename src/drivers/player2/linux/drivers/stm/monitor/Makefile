CFLAGS += -I$(TREE_ROOT)/linux/include/linux/stm/

CONFIG_MONITOR_HAVANA=m

CFLAGS += -DDEBUG_MONITOR


obj-$(CONFIG_MONITOR_HAVANA)+= stm_monitor.o

stm_monitor-objs:=      monitor_module.o \
			monitor_device.o \
			monitor_mme.o


