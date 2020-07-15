#
# Component Makefile

COMPONENT_ADD_INCLUDEDIRS := include

LIBS := deepbrain_ai

COMPONENT_ADD_LDFLAGS := $(COMPONENT_PATH)/lib/libdeepbrain_ai.a \


COMPONENT_ADD_LINKER_DEPS := $(patsubst %,$(COMPONENT_PATH)/lib/lib%.a,$(LIBS))
