#ifndef COMPILER_H
#define COMPILER_H

#include <QtGlobal>

/**
 * \file compiler.h
 *
 * This header provides convenience macros to use compiler dependant features,
 * in particular, C++11 features.
 */

// Detect C++11 support
// For GCC's version specific support, see http://gcc.gnu.org/projects/cxx0x.html

/**
 * \def DECL_OVERRIDE
 * DECL_OVERRIDE expands to the "override" keyword, if available. It allows to
 * explicitely marks a virtual function to override a method of the base class.
 */
#ifdef Q_DECL_OVERRIDE
    /* Trust Qt's declaration, if available */
#   define DECL_OVERRIDE Q_DECL_OVERRIDE
#else
#   if defined(__GNUC__)
//#       if (__GNUC__ * 100 + __GNUC_MINOR__) >= 407
//#           error "You are using GCC >= 4.7, this version is known to break the build!"
#           if defined(__GXX_EXPERIMENTAL_CXX0X__)
                /* C++0x features supported in GCC 4.7: */
#               define DECL_OVERRIDE override
#           endif
//#       endif /* __GXX_EXPERIMENTAL_CXX0X__ */
#   endif /* __GNUC__ */
#endif /* Q_DECL_OVERRIDE */

#ifndef DECL_OVERRIDE
#   define DECL_OVERRIDE
#endif

#endif // COMPILER_H
