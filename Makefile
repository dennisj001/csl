
SOURCES = src/basis/compiler/machineCode.c src/basis/compiler/_compile.c src/basis/compiler/memory.c\
	src/basis/compiler/combinators.c src/basis/compiler/math.c src/basis/compiler/cpu.c src/basis/compiler/mcPrimitives.c\
	src/basis/compiler/stack.c src/basis/compiler/logic.c src/basis/compiler/contol.c src/basis/core/dataObjectRun.c\
	src/basis/core/block.c src/basis/compiler/blocks.c src/basis/core/conditionals.c src/basis/compiler/compile.c src/basis/core/_system.c\
	src/basis/compiler/optimize.c src/basis/compiler/bit.c src/basis/compiler/udis.c src/basis/compiler/arrays.c \
	src/basis/core/io.c src/basis/core/symbol.c src/basis/repl.c src/basis/core/syntax.c src/basis/core/dataObjectNew.c\
	src/basis/csl.c src/basis/core/parse.c src/basis/core/memSpace.c src/basis/init.c src/basis/system.c src/basis/core/charSet.c\
	src/basis/core/dllist.c src/basis/core/interpret.c src/basis/core/lexer.c src/basis/core/cstack.c src/basis/core/classes.c src/basis/debug/debugOutput.c\
	src/basis/core/namespace.c src/basis/history.c src/basis/core/readline.c src/basis/core/dataStack.c src/basis/context.c\
	src/basis/core/word.c src/basis/core/readTable.c src/basis/bigNum.c src/basis/core/_readline.c src/basis/core/array.c\
	src/basis/core/compiler.c src/basis/core/finder.c src/basis/core/typedef.c\
	src/basis/interpreters.c src/basis/tabCompletion.c src/basis/colors.c src/basis/sourceCode.c src/basis/debug/debugStepping.c\
	src/basis/core/string.c src/basis/core/dobject.c src/basis/core/preprocessor.c\
	src/basis/lists.c src/basis/mem.c src/basis/debug/debugDisassembly.c src/basis/typeCheck.c\
	src/basis/linux.c src/basis/exception.c src/basis/recycle.c src/basis/compiler/locals.c\
	src/basis/debug/debugger.c src/basis/interpreter.c src/basis/debug/_debug.c src/basis/lc/lambdaCalculus.c src/basis/lc/lcDebug.c\
	src/basis/lc/apply.c src/basis/lc/eval.c src/basis/lc/read.c src/basis/lc/print.c src/basis/lc/special.c \
	src/primitives/strings.c src/primitives/bits.c src/primitives/maths.c src/primitives/openvmtils.c\
	src/primitives/ios.c src/primitives/parsers.c src/primitives/interpreters.c src/primitives/namespaces.c src/primitives/systems.c\
	src/primitives/compilers.c src/primitives/words.c  src/primitives/file.c src/primitives/stacks.c \
	src/primitives/debuggers.c src/primitives/memorys.c src/primitives/primitives.c src/primitives/contexts.c\
	src/primitives/disassembler.c src/primitives/syntaxes.c src/primitives/cmaths.c src/primitives/dataObjectNews.c src/basis/openVmTil.c\
	src/primitives/ls9.c src/basis/debug/debuggerSteppingSetup.c 
	#src/primitives/fltlisp.c src/primitives/fltread.c src/primitives/s9.c src/primitives/s9core.c
	
S9_SOURCES = src/primitives/s9.c src/primitives/s9core.c

INCLUDES = src/include/machineCode.h src/include/defines.h src/include/types.h \
	src/include/csl.h src/include/macros.h src/include/lc.h\
	src/include/machineCodeMacros.h src/include/lisp.h
	
S9_INCLUDES = src/include/s9core.h src/include/s9ext.h	

OBJECTS = $(SOURCES:%.c=%.o) 
#S9_OBJECTS = $(S9_SOURCES:%.c=%.o) 
S9_OBJECTS = src/primitives/s9.o src/primitives/s9core.o
ALL_OBJECTS = $(OBJECTS) #$(S9_OBJECTS) 	
CC = gcc #-Wno-stringop-overflow
#CC = g++ -fpermissive -Wwrite-strings
OUT = csl-gdb

default : debug

debug : bin/csl-gdb 
run : csl
all: debug #csl-gdb #csls
	
#CFLAGS_CORE = -finline-functions -fno-use-cxa-atexit 
CFLAGS = $(CFLAGS_CORE) -Wall 
LIBS = -L/usr/local/lib -ludis86 -lrt -lc -ldl -lm  -lmpfr -lgmp #-ltcc #-lpython3.7#-lFOX-1.6  -lX11

oclean : 
	-rm src/basis/*.o src/primitives/*.o src/basis/compiler/*.o src/basis/core/*.o src/basis/lc/*.o
	
clean : 
	make oclean
	touch src/include/defines.h
	make src/include/prototypes.h
	-rm bin/csl*

src/include/prototypes.h : $(INCLUDES) #$(S9_INCLUDES)
	cp src/include/_proto.h src/include/prototypes.h
	#cp src/include/_proto.h src/include/s9_prototypes.h
	cproto -o proto.h $(SOURCES)
	#cproto -o s9_proto.h $(S9_SOURCES)
	#mv s9_proto.h src/include/s9_prototypes.h
	mv proto.h src/include/prototypes.h
	make oclean

csl : CFLAGS = $(CFLAGS_CORE)
csl : $(INCLUDES) $(ALL_OBJECTS) #_csl_O3
	$(CC) $(CFLAGS) $(ALL_OBJECTS) -o csl $(LIBS)
	#strip -o csl csl
	mv csl bin/
	
csls : CFLAGS = $(CFLAGS_CORE) -O3
csls : src/include/prototypes.h $(ALL_OBJECTS)
	$(CC) -static $(CFLAGS) $(ALL_OBJECTS) -O3 -o csls $(LIBS) 
	strip csls
	mv csls bin/
	
static : CFLAGS = $(CFLAGS_CORE)
static : src/include/prototypes.h $(ALL_OBJECTS)
	$(CC) -static $(CFLAGS) $(ALL_OBJECTS) -o csls $(LIBS)
	strip csls
	mv csls bin/

bin/csl-gdb : CFLAGS = $(CFLAGS_CORE) -ggdb 
bin/csl-gdb : src/include/prototypes.h $(ALL_OBJECTS) 
	$(CC) $(CFLAGS) $(ALL_OBJECTS) -o bin/csl-gdb $(LIBS)
	strip -o bin/csl bin/csl-gdb
	gcc --version
	

cslo : oclean $(ALL_OBJECTS)
	$(CC) $(CFLAGS) $(ALL_OBJECTS) -o $(OUT) $(LIBS)
	strip $(OUT)
	mv $(OUT) bin
	-cp bin/$(OUT) bin/csl
	-sudo cp bin/$(OUT) /usr/local/bin/csl

_csl_O1 : CFLAGS = $(CFLAGS_CORE) -O1
_csl_O1 : OUT = cslo1
_csl_O1 : cslo

_csl_O2 : CFLAGS = $(CFLAGS_CORE) -O2
_csl_O2 : OUT = cslo2
_csl_O2 : cslo

_csl_O3 : CFLAGS = $(CFLAGS_CORE) -O3
_csl_O3 : OUT = cslo3
_csl_O3 : cslo

_cslo :  src/include/prototypes.h $(ALL_OBJECTS)
	$(CC) $(CFLAGS) $(ALL_OBJECTS) -o $(OUT) $(LIBS)

src/primitives/cmaths.o : src/primitives/cmaths.c
	$(CC) $(CFLAGS) -O3 -c src/primitives/cmaths.c -o src/primitives/cmaths.o

src/primitives/flisp.o : src/primitives/flisp.c src/primitives/fltlread.c	
	$(CC) $(CFLAGS) -O3 -c src/primitives/fltlisp.c -o src/primitives/fltlisp.o
#	$(CC) $(CFLAGS) -ggdb -c src/primitives/flisp.c -o src/primitives/flisp.o

src/primitives/ls9.o:	src/primitives/ls9.c #s9core.h s9import.h s9ext.h
	$(CC) -o src/primitives/ls9.o  -ggdb $(CFLAGS_CORE) -c src/primitives/ls9.c

src/primitives/s9.o:	src/primitives/s9.c #s9core.h s9import.h s9ext.h
	gcc -o src/primitives/s9.o -ggdb $(CFLAGS_CORE) -c src/primitives/s9.c

src/primitives/s9core.o:    src/primitives/s9core.c #s9core.h s9.image
	$(CC) -o src/primitives/s9core.o  -ggdb $(CFLAGS_CORE) -c src/primitives/s9core.c
	
s9.image:	s9 s9.scm ext/sys-unix/unix.scm ext/curses/curses.scm \
		ext/csv/csv.scm config.scm
	$(BUILD_ENV) ./s9 -i - $(EXTRA_SCM) -l config.scm -d s9.image
	
proto:
	touch src/include/defines.h
	make src/include/prototypes.h

optimize1 : oclean _csl_O1

optimize2 : oclean _csl_O2

optimize3 : oclean _csl_O3

optimize : 
	#-rm bin/csl*
	#make optimize1
	#make optimize2
	make -j 20 optimize3
	#make optimize3
	make csls
	make oclean
	#make
	#-sudo cp bin/cslo3 /usr/local/bin/csl
	#cp bin/cslo3 bin/csl

editorClean :
	rm *.*~ src/basis/*.*~ src/basis/compiler/*.*~ src/primitives/*.*~ src/include/*.*~

realClean : oclean editorClean
	-rm csl csl-gdb

udis :
	wget http://prdownloads.sourceforge.net/udis86/udis86-1.7.2.tar.gz
	tar -xvf udis86-1.7.2.tar.gz 
	cd udis86-1.7.2 && \
	./configure --enable-shared && \
	make && \
	sudo make install && \
	sudo ldconfig
	
_gmp : 
	wget https://gmplib.org/download/gmp/gmp-6.2.1.tar.lz
	tar -xvf gmp-6.2.1.tar.lz
	cd gmp-6.2.1 && \
	./configure --enable-shared && \
	make && \
	sudo make install && \
	sudo ldconfig
	
_mpfr :	
	wget https://www.mpfr.org/mpfr-current/mpfr-4.1.0.tar.xz
	tar xvf mpfr-4.1.0.tar.xz
	cd mpfr-4.1.0 && \
	./configure --enable-shared && \
	make && \
	sudo make install && \
	sudo ldconfig
mpfr :
	sudo apt-get install libmpfr4 libmpfr-dev
	
gmp :
	sudo apt-get install libgmp10 libgmp-dev
	
_cproto :
	wget https://launchpad.net/ubuntu/+archive/primary/+files/cproto_4.7m.orig.tar.gz 
	tar -xvf cproto_4.7m.orig.tar.gz
	cd cproto-4.7m && \
	./configure --enable-shared && \
	make && \
	sudo make install

cproto : 
	sudo apt-get install cproto
	
tar.xz :	
	tar -c --xz --exclude=lib --exclude=archive --exclude=nbproject --exclude=objects --exclude=archive--exclude=mpfr* --exclude=.git --exclude=*-gdb  --exclude=*.o --exclude *.kdev* -f ../csl.tar.xz * 

all.tar.xz :	
	tar -c --xz --exclude=nbproject --exclude=objects  --exclude=mpfr* --exclude=.git --exclude=*.png --exclude=*-gdb --exclude=*.o --exclude *.kdev* -f ../csl.tar.xz * 

xz : 
	-rm ../csl/core
	-rm -rf ../backup/csl/
	-cp -r ../csl/ ../backup/csl/
	make tar.xz

all.xz : 
	-rm ../csl/core
	-rm -rf ../backup/csl/
	-cp -r ../csl/ ../backup/csl/
	make all.tar.xz

_all : realClean install
	make xz

_install : 
	-cp ./init.csl ./namespaces/
	-sudo rm /usr/local/bin/cx
	-sudo rm -rf /usr/local/lib/csl/
	-sudo cp -r ../csl /usr/local/lib/
	-sudo cp -r ../csl/etc /usr/local/
	-sudo cp ../csl/lib/* /usr/local/lib
	-killall csl
	-sudo cp bin/* /usr/local/bin
	-sudo ldconfig
	ls -l /usr/local/bin/csl*

install :
	#make
	make optimize 
	make _install
	
install_s9fes :
	make
	#make optimize // so far s9fes doesn't work optimized ??
	make _install
	
install_ls9 :
	#make
	make optimize 
	make _install
	
run :
	csl

runLocal :
	./bin/csl

