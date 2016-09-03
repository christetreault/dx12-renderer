#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#else
#include <stdlib.h>
#endif

// Microsoft headers
#include "d3dx12.h"
#include <wrl.h>
#include <windows.h>
#include <DXGI1_4.h>

// STL headers
#include <string>
#include <unordered_map>
#include <cassert>
#include <memory>
#include <limits>
#include <functional>
#include <array>

// DMP headers
#include "Util.h"