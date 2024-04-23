EXEC=basic-dyn
# EXEC=basic-static

${EXEC}:
	${CC} basic.c -lseventh -I../src -L../lib -Wl,-rpath,../lib -o ${EXEC}

clean:
	${RM} ${EXEC}