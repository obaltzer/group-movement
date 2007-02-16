include depend.mk

all: emp fi cl cc_all nc_all

%.emp:
	@bash map.sh $@

%.fi:
	@bash fim.sh $@

%.cl:
	@bash match.sh $@

%.grp:
	@bash group.sh $@

emp: $(EMP)

fi: $(FI)

cl: $(CL)

cc_all: $(CC)

nc_all: $(NC)
