/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   old.h
 * Author: dennisj
 *
 * Created on February 14, 2022, 11:21 PM
 */

#ifndef OLD_H
#define OLD_H

// old stuff
//#include "codegen_x86.h" // i want to make sure i have this - not using much now but probably later on
//#include "lisp.h"
//extern value_t FL_NIL, T, FL_LAMBDA, FL_MACRO, FL_LABEL, FL_QUOTE ;
#if 0
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
//#include "/usr/include/x86_64-linux-gnu/asm/unistd.h"
//#include <arch/ia64/asm/thread_info.h>
#endif
#define TCC 0
#if TCC
const void *const __dso_handle __attribute__ ((__visibility__ ("hidden")))
  = &__dso_handle;
#endif
#if 0 //def 0 //S9
#include "s9core.h"
#define S9_S9CORE
#include "s9import.h"
#include "s9ext.h"
#include "s9_prototypes.h"
#endif

#endif /* OLD_H */

