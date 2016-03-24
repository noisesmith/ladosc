# the directory where the plugins should be. Make sure this is in your
# shell's LADSPA_PATH
DESTINATION_DIR=~/.ladspa

LIBLO=/usr/lib/liblo.so

install: install_op install_osc
install_op: op.so
	mkdir -p ${DESTINATION_DIR}
	cp op.so ${DESTINATION_DIR}
op.so: operator.o
	ld operator.o -o op.so -shared
operator.o: operator.c
	gcc -c -fPIC operator.c
install_osc: osc.so
	mkdir -p ${DESTINATION_DIR}
	cp osc.so ${DESTINATION_DIR}
osc.so: osc.o ${LIBLO}
	ld osc.o ${LIBLO} -o osc.so -shared
osc.o: lad-osc.c
	gcc -c -fPIC -g lad-osc.c -o osc.o
clean:
	rm -f *.o *~ *.so
