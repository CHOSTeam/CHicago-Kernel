/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 16:12 BRT
 * Last edited on February 08 of 2021, at 14:30 BRT */

#pragma once

#include <types.hxx>

/* With C++, we can use an "enum class", instead of using a bunch of defines, this makes everything a bit prettier, and
 * also easier to relocate the codes up and down/add new ones lol. */

enum class Status : Int32 {
    /* Misc status codes. */

	Success = 0, InvalidArg,
	
	/* Memory errors. */
	
	OutOfMemory
};
