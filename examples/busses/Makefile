include depend.mk

all: emp fi cl cc_all nc_all

%.emp:
	@sh map.sh $@

%.fi:
	@sh fim.sh $@

%.cl:
	@sh match.sh $@

%.grp:
	@sh group.sh $@

emp: $(EMP)

fi: $(FI)

cl: $(CL)

cc_all: $(CC)

nc_all: $(NC)
