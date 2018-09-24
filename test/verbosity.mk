VERBOSE ?= 0

ifdef CC
	ACTUAL := $(CC)
	CC_0 = @echo "CC    $<"; $(ACTUAL)
	CC_1 = $(ACTUAL)
	CC = $(CC_$(VERBOSE))
endif

ifdef CDEPS
	ACTUAL := $(CDEPS)
	CDEPS_0 = @echo "DEPS   $<"; $(ACTUAL)
	CDEPS_1 = $(ACTUAL)
	CDEPS = $(CDEPS_$(VERBOSE))
endif

ifdef LD
	ACTUAL := $(LD)
	LD_0 = @echo "LD    $(notdir $@)"; $(ACTUAL)
	LD_1 = $(ACTUAL)
	LD = $(LD_$(VERBOSE))
endif

ifdef CP
	ACTUAL_CP := $(CP)
	CP_0 = @echo "CP    $(notdir $@)"; $(ACTUAL_CP)
	CP_1 = $(ACTUAL_CP)
	CP = $(CP_$(VERBOSE))
endif
