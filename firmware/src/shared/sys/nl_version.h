/******************************************************************************/
/** @file		nl_version.h
    @brief		define firmware version and place string in image
*******************************************************************************/
#pragma once

// just set up the X.YZ style of version number vertically in below
#define SW_VERSION_MAJOR 1
// delimiter              .
#define SW_VERSION_MINOR_H 0
#define SW_VERSION_MINOR_L 2

#define STR_IMPL_(x) #x            //stringify argument
#define STR(x)       STR_IMPL_(x)  //indirection to expand argument macros

#define SW_VERSION      \
  STR(SW_VERSION_MAJOR) \
  "." STR(SW_VERSION_MINOR_H) STR(SW_VERSION_MINOR_L)

static const char VERSION_STRING[] = "\n\nNLL MIDI Host-to-Host Bridge, LPC4337, FIRMWARE VERSION: " SW_VERSION " \n\n\0\0\0";
