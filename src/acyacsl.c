#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "acyacsl.h"

Allocator stdAllocator = {malloc, free};

#define ACYACSL_IMPLEMENTATION
#include "acyacsl.h"
