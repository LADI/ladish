AM_LDFLAGS = -module -avoidversion -Wc,-nostartfiles

plugindir = @ladspadir@

plugin_LTLIBRARIES = \
	hz_voct_4200.la \
	range_trans_4210.la \
	formant_filter_4300.la \
	adenv.la \
	adenv_lvl.la \
	dahdsr_fexp.la \
	dahdsr_hexp.la \
	prob_switch_2667.la \
	signal_abs_2669.la \
	slide_2741.la \
	slew_limiter_2743.la \
	waveguide_mesh_2670.la \
	fast_crossfade_4410.la \
	multiplexer_4420.la \
	power_4400.la \
	masher_4310.la \
	sample_and_hold_4430.la \
	comparison_4440.la

# Stolen from swh-plugins, makes stupid libtool versions go away
install-pluginLTLIBRARIES: $(plugin_LTLIBRARIES)
	mkdir -p @ladspadir@
	list='$(plugin_LTLIBRARIES)'; \
	for file in $$list; do \
	  sofile=`basename $$file .la`.so; \
	  $(INSTALL_PROGRAM) .libs/$$sofile @ladspadir@; \
	done

uninstall-pluginLTLIBRARIES: $(plugin_LTLIBRARIES)
	list='$(plugin_LTLIBRARIES)'; \
	for file in $$list; do \
	  sofile=`basename $$file .la`.so; \
	  rm -f @ladspadir@/$$sofile; \
	done

