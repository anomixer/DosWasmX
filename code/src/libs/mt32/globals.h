/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2022 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Refer to the comments in mt32emu.h for documentation of the C++ and C APIs.

#ifndef MT32EMU_GLOBALS_H
#define MT32EMU_GLOBALS_H

/* Useful for passing to MT32EMU_API_TYPE */
#define MT32EMU_CPP_API 1
#define MT32EMU_C_CPP_API 2
#define MT32EMU_C_API 3

/* Define this to the required API type */
#ifndef MT32EMU_API_TYPE
#  define MT32EMU_API_TYPE MT32EMU_C_CPP_API
#endif

#if MT32EMU_API_TYPE == MT32EMU_CPP_API || MT32EMU_API_TYPE == MT32EMU_C_CPP_API

/* Visibility control for C++ symbols */
#ifndef MT32EMU_EXPORT
#  if defined _WIN32 || defined __CYGWIN__
#    ifdef MT32EMU_EXPORTS_TYPE
#      ifdef __GNUC__
#        define MT32EMU_EXPORT __attribute__((dllexport))
#      else
#        define MT32EMU_EXPORT __declspec(dllexport)
#      endif
#    else
#      ifdef __GNUC__
#        define MT32EMU_EXPORT __attribute__((dllimport))
#      else
#        define MT32EMU_EXPORT __declspec(dllimport)
#      endif
#    endif
#  elif defined __GNUC__ && __GNUC__ >= 4
#    if defined MT32EMU_EXPORTS_TYPE
#      define MT32EMU_EXPORT __attribute__((visibility("default")))
#    else
#      define MT32EMU_EXPORT
#    endif
#  else
#    define MT32EMU_EXPORT
#  endif
#endif

#endif /* MT32EMU_API_TYPE == MT32EMU_CPP_API || MT32EMU_API_TYPE == MT32EMU_C_CPP_API */

/* Uncomment if you want to prevent inlining of some small functions. This may
 * improve the speed due to better cache utilisation, but it depends. */
/* #define MT32EMU_DISABLE_INLINING */

#ifndef MT32EMU_INLINE
#  ifdef MT32EMU_DISABLE_INLINING
#    define MT32EMU_INLINE
#  elif defined (_MSC_VER)
#    define MT32EMU_INLINE __forceinline
#  elif defined __GNUC__
#    define MT32EMU_INLINE __attribute__((always_inline)) inline
#  else
#    define MT32EMU_INLINE inline
#  endif
#endif

#endif /* #ifndef MT32EMU_GLOBALS_H */
