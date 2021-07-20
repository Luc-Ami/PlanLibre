AM_CFLAGS = $(PLANLIBRE_CFLAGS)

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"

bin_PROGRAMS = planlibre

planlibre_SOURCES = \
	main.c \
	misc.c misc.h \
	undoredo.c undoredo.h \
	properties.c properties.h \
	ressources.c ressources.h \
	tasks.c tasks.h \
	tasksdialogs.c tasksdialogs.h \
	tasksutils.c tasksutils.h \
	import.c import.h \
	trello.c trello.h \
	files.c files.h \
	links.c links.h \
	timeline.c timeline.h \
	assign.c assign.h \
	cursor.c cursor.h \
	home.c home.h \
	search.c search.h \
	report.c report.h \
	export.c export.h \
	settings.c settings.h \
	mail.c mail.h \
	secret.c secret.h \
	msproject.c msproject.h \	
	calendars.c calendars.h \
	dashboard.c dashboard.h \
	savestate.c savestate.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	support.c support.h 

planlibre_LDFLAGS = \
	-Wl,--export-dynamic

planlibre_LDADD = $(PLANLIBRE_LIBS)

