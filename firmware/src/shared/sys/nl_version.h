/******************************************************************************/
/** @file		nl_version.h
    @date		2020-04-24
    @version	0
    @author		KSTR
    @brief		define firmware version and place string in image
*******************************************************************************/
#pragma once

#define SW_VERSION_MAJOR 1
#define SW_VERSION_MINOR 2

#define STR_IMPL_(x) #x            //stringify argument
#define STR(x)       STR_IMPL_(x)  //indirection to expand argument macros

#define SW_VERSION      \
  STR(SW_VERSION_MAJOR) \
  "." STR(SW_VERSION_MINOR)

static const char VERSION_STRING[]          = "\n\nNLL MIDI Host-to-Host Bridge, LPC4337, FIRMWARE VERSION: " SW_VERSION " \n\n\0\0\0";
static const char VERSION_STRING_STRIPPED[] = "NLL MIDI Host-to-Host Bridge, Version: " SW_VERSION;
