/******************************************************************************/
/** @file		nl_version.h
    @date		2020-04-24
    @version	0
    @author		KSTR
    @brief		define firmware version and place string in image
*******************************************************************************/
#pragma once

#define SW_VERSION 100

#define STR_IMPL_(x) #x            //stringify argument
#define STR(x)       STR_IMPL_(x)  //indirection to expand argument macros
#if defined CORE_M4
#define CORE "M4"
#elif defined CORE_M0
#define CORE "M0"
#else
#error "either CORE_M4 or CORE_M0 must be defined!"
#endif

static const char VERSION_STRING[] = "\n\nNLL MIDI H-2-H Bridge, LPC4337 Core " CORE ", FIRMWARE VERSION: " STR(SW_VERSION) " \n\n\0\0\0";
