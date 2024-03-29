//
// Bareflank Unwind Library
//
// Copyright (C) 2015 Assured Information Security, Inc.
// Author: Rian Quinn        <quinnr@ainfosec.com>
// Author: Brendan Kerrigan  <kerriganb@ainfosec.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#ifdef LOG
#define log(...)	printf(__VA_ARGS__);
#else
#define	log(...)
#endif

#define print(...)	printf(__VA_ARGS__);

#ifdef OBJDUMP
#define objdump(...)	printf(__VA_ARGS__);
#define dwarfprint(...)
#else	//dwarfdump
#define dwarfprint(...)	printf(__VA_ARGS__);
#define objdump(...)
#endif

#ifdef DEBUG
#define debug(fmt, args...)	printf("%s: " fmt, __func__, ##args)
#else
#define debug(fmt, args...)	
#endif

#ifdef ALERT
#define alert(fmt, args...)	 printf("\x1b[31m");	\
		printf("%s: " fmt, __func__, ##args);	\
		printf("\x1b[0m")
#else
#define alert(fmt, args...)
#endif

#endif
