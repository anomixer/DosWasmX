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

#ifndef MT32EMU_MT32EMU_H
#define MT32EMU_MT32EMU_H

#include "globals.h"

#if MT32EMU_API_TYPE == 3
#  include "c_interface/c_interface.h"
#elif MT32EMU_API_TYPE == 2
#  include "c_interface/cpp_interface.h"
#elif MT32EMU_API_TYPE == 1
// C++ interface, use only the C++ API headers
#  include "Types.h"
#  include "Enumerations.h"
#  include "Synth.h"
#  include "MidiStreamParser.h"
#  include "Display.h"
#else
#  error Unrecognised MT32EMU_API_TYPE
#endif

#endif // #ifndef MT32EMU_MT32EMU_H
