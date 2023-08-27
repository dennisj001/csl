CSL really has no grammar, just one word after another - like the language forth with namespaces ('vocabulary') changing the 'context' with some words being defined as operating infix or prefix. It may be considered syntactically more powerful and flexible than a context-sensitive language in the Chomsky hierarchy of language types, certainly it is Turing complete. For me it's an exercise in language design, theoretical and applied computer science - a renamed outgrowth of openVm and openvmtil64. I originally designed this with the thought that it could be useful for natural language processing (NLP), language translation, ai, gaming, audio, etc. and also for language design.

openVm : Tookit for Implementing (and exploring) Languages - a bottom-up, optimizing native code vm, that is an extensible, contatenative, RPN, scripting language 

*An Exploration of Language Theory - and its Machine Implementation*

*Theory* : Any computation can be represented by a PDA/Turing Machine ~ Lambda Calculus. The simplest language is an RPN (reverse polish notation - without the need of parentheses) language. The simpler the language the smaller it is, the closer it can be to machine code and the easier it is to extend. A prefix or infix language form can be converted to a postfix language form. Therefore if we can optimize a postfix rpn language we can convert any other language to it. OpenVmTil64 is a register based, machine coded RPN language that can be optimized to the fastest gcc/g++ -O3 speeds (at least in currently tested cases and it is most probably not yet completely optimized, certainly it can be improved in many ways). Admittedly rpn languages can be tedious to program in for those used to infix (c, java) or prefix languages (lisp, scheme, etc.), that is why there is a syntactic layer to relatively simply convert any language to concatenative/rpn. Currently we have a minimal C and lisp/scheme type langage for this as examples.

*Imagine a low level, optimizing, virtual machine (like llvm, the clr, or the java vm) that is an extensible scripting language and that is small enough to be easily verified, where even the runtime is reconfigurable - and minimize that.*

*Vision* <: < forth, lisp, smalltalk, self, prolog, yacc > (c, machine code) :=> maru, joy, teyjus, ometa, ocaml, haskell (w/debugger) :> : a distilled computational semantics essence as a vm common ground; inclusive, bottom-up (from machine code not assembly) vm that is also a forth-like (rpn) scripting language upon which to build an elegant parser (cf. ometa) for any language or vm (nlp, llvm, ocaml, haskell, jvm, clr, arm, etc), and libraries -> ai/game engine, os, browser, etc., ... ; sufficiently fine grain semantic primitives that can be combined to form the keywords and semantics of any high level language; the keywords and grammar specifics for each language in separate namespaces, for example. *A cross-platform, extensible vm that can be as fast as hand-coded assembly (as fast as or faster than -O3 C/C++) and is also a scripting language - a universal, extensible language* with optional compilation to .o files for c linkage or stand alone executables. Recognizing the incredible utility of the list data structure and set theory makes *lisp - lambda calculus* the interface here to human language and thinking; *"forth"* is the interface to the hardware, with C and machine code in the middle. A sort of high level, rpn, macro assembler, a "Turing tarpit" explorer/miner - *(really, exploring new runtime implementation ideas in computer science)* ; bringing ideas from forth together with smalltalk, self, lisp and prolog in c and machine code is *a long term goal*. 

The *Turing Machine*, *Formal Language Theory*, the *Lambda Calculus* and *Type/Category Theory* are the theoretical foundations.  Levels : 0/1 : Hardware -> MachineCode - Intel - ARM - Turing - state machines -> Forth - Concatenative - Moore - (joy - von Thun) ->  Lisp - Church - McCarthy -> yacc - Generative Grammar - Panini - Chomsky - -> Prolog - Curry - Logic -> Smalltalk - Category Theory - Eilenberg - MacClane -> Type Theory - Milner - Harper - Sml - Ocaml - Haskell -> Nonduality -> Integral - Wilber -> Infinity - ...

With an ideal that the best (cross platform) virtual machine or common language runtime is a minimal but maximally extensible, optimally and simply compiled (rpn) language, certainly "human readable", but also maintainable, learnable and extensible. But not as fundamentally one huge tool rather several small, relatively easily understood (groups of) tools working "organically" together - maybe most like forth but not limited by its assumptions with an action, constructive, granular, transparent, operational semantics of a higher level functional, denotational semantics. This, we feel, would also have the potential to be a step toward more cooperation in the (linux) programming world. Ometa, Yacc, uml, categorical logic, etc. with whatever syntax on top of this.

A minimal, extensible, cooperative, sustainable,  all level capable language - machine code to scripting, compiler-interpreter, concatenative, anonymous subroutines, nested locals, dynamic variables, optimized machine code compiler, debugger, history, tab completion, ..., "organically" growing, experimental. All the "keywords" and syntactic characters are user changeable. Different problem domains can not only have different classes/objects/morphisms but can have different syntax.

*Currently working : * a concatenative, (forth like) interpreter with an optimizing native code x64, register machine, compiler, single instruction stepping x64 debugger (using udis for instruction disassembly) with breakpoints, classes - namespaces, interpreter, history and tab completion, bignums, nested locals, readline, raw readTable, lexical readTable, minimal C and lisp/scheme compiler

*Current Visionary Focus and Direction* :
  - self-hosting : list objects and a lisp like eval, ometa/yacc/bison support : support for a midlevel language modeled after scheme/lisp/racket to support a higher level modeled after sml/ocaml/haskell and prolog/teyjus/hol + ometa/peg/yacc/bison for a sort of applied unified field theory of programming languages, similar in vision to *maru* by *VPRI-Ian Piumarta* ; we have a forth like system and we are adding c/lisp/scheme/smalltalk/prolog semantics and syntax.
  - OS interface - linux/plan9/etc : file, terminal, gui, process/threads, audio, opengl, browsers, html5, flash, game engine, etc.

*Current focus (to do) : * minimal bootstrap, self-hosting, patterns/sets, logic, tail call, type checking, gui

*Goals* : Explore the power of simplicity as a software design principle - how simple can it be and still be totally effective. A correct, reliable, fast, small, and extensible machine coding compiler supporting any and all high level language semantics with an integrated machine language debugger. An optimal language core for fundamental (Turing Machine + Push Down Automata) semantics - the one language that holds us all together is, of course, the Turing Machine.  Based on this small layer, simple as mathematically possible, and transparently correct, add a (minimal) set of primitives, clearly defined, easily understandable, extensible, easily learnable, computationally complete and efficient, compiling to native machine code or a reflective vm. So it's languages all the way down and up but integrated. Forth + C + joy + smalltalk, for the core semantics, syntactic compiler-compilers above for anything else, ie. javaScript, io, lisp, prolog, perl, python, ruby, (anything using a virtual machine), etc. Simple syntax - concatenation (left-associative (LA) grammar -- believed by some researchers to be closest to natural language), understandable, not trivial but easily learnable, only a minimimal number of fairly simple, useful ideas have to be mastered. Fast, small, simple interpreter, compiler, lowest levels easily verified by inspection, and a maximally minimal set of primitives, or operators capabable of interacting with the cpu, operating system, gui, other compiled or scripting languages, shells, etc.; ie. at any level and across levels. Using some useful ideas in formal language theory and (combinatory) logic (blocks/quotations) and LA grammar, I want to develop complete, simple and sufficienty fine grain access to the advantages of machine code for a language designed to transcend and include machine code, a sort of higher level assembly language. Using ideas from joy, forth, factor, smalltalk, lisp mainly, with some keywords from c/c++/c#. '{' and '}' used for blocks/quotations because blocks are anonymous subroutines, like blocks in C. Combinators operate on blocks/subroutines and are postfix. This software is especially for those who want to be close to the implementation so it can be creatively adapted.

  Optimizing native code compiler ("assembler"), single stepping x64 debugger (using udis for instruction disassembly) with breakpoints, classes - namespaces, interpreter, history and tab completion, bignums, nested locals, readline, raw readTable, lexical readTable, minimal C and lisp/scheme compilers

*Status* : the core seems stable now, with some beta and alpha parts - still adding new features. It currently provides fairly easy, low level machine code access with high level language constructs (blocks, with nested locals, conditionals, 'for', 'case/switch', etc.), interpreter with single step native code debugger, powerful readline with history, tab completion, etc. It is currently only using an advanced "instruction pipeline" optimization techniques but achieving (at least sometimes) 0.88 x -03 gcc speeds with much smaller code (type 'test' and 'demo' and scroll back to see speeds for fibonacci and factorial examples). Consider also that the actual code base is only a few hundred K compressed so learning and changing things is relatively easy. The focus has been on basic bottom level infrastructure, performance and transparency not yet on expressiveness and data. 

The high level syntax work has not been done yet so the syntactic form is still minimal postfix with a few prefix words (mini-languages in themselves) and may not look as elegant as C#/Java/C++ or the newer languages like JavaScript, Rust, OCaml, Haskell, Go, Vala, etc. But postfix languages need no parenthesis and have elegance and power in minimalism. CSL has smart operators and other experimental syntactic forms. Even so i am interested in supporting a compiler compiler for any desired language syntax/grammar. An idea i am using is that with optimization off it has a simple, basic forth look (syntax and semantics) but with optimize on it is not only much speeded up but also has "smart operators" that can allow *both* a forth "look" and more of a C "look" to the code in some ways. Variables will be interpreted as lvalues or rvalues by the operators and the forth '@' is implicit (eg. ++/--). These are evolving ideas ; it could also be more controlled by switching namespaces.

This project is for me research into logic, language, cognitive and computer science; experimental, a sort of virtual research and exercise (fun) space. Recent work has mainly been on the lowest level, the compiler. Probably not for beginning programmers -- it can be very difficult to debug and extend (or maybe just a very challenging, fun logic/language puzzle). It certainly can be improved! Has a readline, and rudimentary signal processing built-in. You can crash it but it usually automatically restarts. 

*NB* : The above stated *Goals* are not necessarily to be taken as the current *Status* of the project. For now only the core functionality is (almost) always there but it is feeling stable now. Verify and evaluate its usefulness for yourself moment by moment, it is *not guaranteed for anything. Use at your own risk*.

*CSL* (formerly cfrtil : pronounced maybe as "C-fertile") - *Compiler Foundations Research - Tookit for Implementing Languages* : is a *name* chosen for a *C* based first implementation of an *openVmTil*, strongly influenced by *Forth*, and RPN (reverse polish notation) or simple *Reasonableness* (logic). The *Til* originally refered to *Threaded Interpretive Languages* but i outgrew that name and all forth related threading techniques when i went to optimized native register code.

*Acknowledgements* : Special thanks for : Haskell B. Curry (*Foundations of Mathematical Logic*), Alan Turing (the Turing Machine), Alonzo Church (Lambda Calculus), John McCarthy, John Backus, Noam Chomsky, Donald Knuth, Martin Gardner, Panini and Jiva Goswami (Sanskrit grammarians), *Starting Forth*, *Threaded Interpretive Languages*, *Compiler Design in C*, Category Theory, Combinatory Logic ; "Strange attractors" : sml (ocaml, f#, scala, haskell), teyjus (prolog, hol, hol light, coq, agda), lisp, (scheme, clojure), factor (retro, reva, jonesForth), self (smalltalk), llvm (clr, mono, jvm, plan9), mathematica, joy, javaSript, rust, python, j, perl, bash, ... ; also the developers of linux, xorg, forth, gcc/g++, ubuntu, kde, netbeans, udis, gmp, cproto, and so many others on whose (code) shoulders we stand.

I think of language design and coding as *an Art form as well as a Science*.
I feel all computer languages are derived from (applied) logic ~ currently best represented by the Turing Machine, Category Theory, the Lambda Calculus, and Type Theory - (our one universal language?). This is currently only an independent amateur research prototype but i think this is may be a direction we need to consider in compiler design and implementation.

Important points/words (defined in primitives.c or init.csl) : 
'd:' is a prefix word that turns on debugger in the interpreter
'<dbg>' is also a prefix word that turns on runtime debugger just after its runtime location

'csls' is the static version which needs only libc to run independent of other linked libraries
'cslo3' would be an C -03 optimized version

Type 'test' or 'tc' and then 'demo' (without the single quotes, of course) for some examples.
  - 'fr' - full restart
  - up arrow/down arrow : command history
  - tab key : tab-completion
"init.csl" is the startup initialization file. Follow what it does and you will be led to all the internals of the program.
I use netbeans (cpp) for debugging.
