#
# Makefile for the Linux/SuperH CPU-specifc IRQ handlers.
#
obj-y	+= imask.o intc.o

obj-$(CONFIG_CPU_HAS_IPR_IRQ)		+= ipr.o
obj-$(CONFIG_CPU_HAS_MASKREG_IRQ)	+= maskreg.o
obj-$(CONFIG_CPU_SUBTYPE_STX5197)	+= st40_ilc_stx7200.o ilc3_common.o
ifeq ($(CONFIG_HW_WYBOX),y)
obj-$(CONFIG_CPU_SUBTYPE_STB7100)	+= st40_ilc_stx710x.o ilc3_common.o
else
obj-$(CONFIG_CPU_SUBTYPE_STB7100)	+= st40_ilc_sti5528.o ilc3_common.o
endif
obj-$(CONFIG_CPU_SUBTYPE_STX7105)	+= st40_ilc_stx7200.o ilc3_common.o
obj-$(CONFIG_CPU_SUBTYPE_STX7111)	+= st40_ilc_stx7200.o ilc3_common.o
obj-$(CONFIG_CPU_SUBTYPE_STX7141)	+= st40_ilc_stx7200.o ilc3_common.o
obj-$(CONFIG_CPU_SUBTYPE_STX7200)	+= st40_ilc_stx7200.o ilc3_common.o
