# the directory where the plugins should be. Make sure this is in your
# shell's LADSPA_PATH
DESTINATION_DIR=~/.ladspa

LO_CFLAGS=`pkg-config --cflags liblo`
LO_LIBS=`pkg-config --libs liblo` -lm
LADSPA=-I/users/justinsmith/ardour/libs/ardour/ardour/

LINUX_LD=-shared
OSX_LD=-dylib
LDFLAGS=${OSX_LD}

install: install_op install_osc
install_op: op.so
	mkdir -p ${DESTINATION_DIR}
	cp op.so ${DESTINATION_DIR}
op.so: operator.o
	ld operator.o ${LO_LIBS} -o op.so ${LDFLAGS}
operator.o: operator.c
	gcc -c -fPIC ${LADSPA} operator.c
install_osc: osc.so
	mkdir -p ${DESTINATION_DIR}
	cp osc.so ${DESTINATION_DIR}
osc.so: osc.o
	ld osc.o ${LO_LIBS} -o osc.so ${LDFLAGS}
osc.o: lad-osc.c
	gcc -c ${LO_CFLAGS} ${LADSPA} -fPIC -g lad-osc.c -o osc.o
clean:
	rm -f *.o *~ *.so
