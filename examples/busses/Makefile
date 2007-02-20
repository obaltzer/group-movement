include depend.mk

all: cc_all nc_all wc_all

%.emp:
	@bash map.sh $@

%.fi: emp
	@bash fim.sh $@

%.cl: fi
	@bash match.sh $@

%.grp: cl
	@bash group.sh $@

emp: $(EMP)

fi: $(FI)

cl: $(CL)

cc_all: $(CC)

nc_all: $(NC)

wc_all: $(WC)
