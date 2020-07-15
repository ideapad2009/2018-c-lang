#
# Component Makefile
COMPONENT_ADD_INCLUDEDIRS := . include include/amr include/stagefright

COMPONENT_SRCDIRS := .

LIBS := esp-amrwbenc resampling asr_engine esp-opus esp-stagefright esp-flac esp-tremor esp-fdk esp-ogg-container esp-m4a esp-amr esp-mad 


COMPONENT_ADD_LDFLAGS :=  -L$(COMPONENT_PATH)/lib \
                           $(addprefix -l,$(LIBS)) \

ALL_LIB_FILES := $(patsubst %,$(COMPONENT_PATH)/lib/lib%.a,$(LIBS))
