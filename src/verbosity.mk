VERBOSE ?= 0

ifdef CC
	_ACTUAL_CC := $(CC)
	_CC_0 = @echo "CC     $<"; $(_ACTUAL_CC)
	_CC_1 = $(_ACTUAL_CC)
	CC = $(_CC_$(VERBOSE))
endif

ifdef CDEPS
	_ACTUAL_CDEPS := $(CDEPS)
	_CDEPS_0 = @echo "DEPS   $<"; $(_ACTUAL_CDEPS)
	_CDEPS_1 = $(_ACTUAL_CDEPS)
	CDEPS = $(_CDEPS_$(VERBOSE))
endif

ifdef LD
	_ACTUAL_LD := $(LD)
	_LD_0 = @echo "LD     $(notdir $@)"; $(_ACTUAL_LD)
	_LD_1 = $(_ACTUAL_LD)
	LD = $(_LD_$(VERBOSE))
endif

ifdef MAKE
	_ACTUAL_MAKE := $(MAKE)
	_MAKE_0 = @echo "MAKE    $(dir $@)"; $(_ACTUAL_MAKE)
	_MAKE_1 = $(_ACTUAL_MAKE)
	MAKE = $(_MAKE_$(VERBOSE)) VERBOSE=$(VERBOSE)
endif

ifdef AR
	_ACTUAL_AR := $(AR)
	_AR_0 = @echo "AR      $@"; $(_ACTUAL_AR)
	_AR_1 = $(_ACTUAL_AR)
	AR = $(_AR_$(VERBOSE))
endif

ifdef CP
	_ACTUAL_CP := $(CP)
	_CP_0 = @echo "CP     $(notdir $@)"; $(_ACTUAL_CP)
	_CP_1 = $(_ACTUAL_CP)
	CP = $(_CP_$(VERBOSE))
endif

