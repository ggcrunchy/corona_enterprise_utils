/*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

#include "utils/SIMD.h"

#ifdef __APPLE__
	#include "TargetConditionals.h"
#elif __ANDROID__
	#include <pthread.h>
	#include <cpu-features.h>
#endif

namespace SimdXS {
	bool CanUseNeon (void)
	{
	#ifdef _WIN32
		return false;
	#elif __APPLE__
		#if !TARGET_OS_SIMULATOR && (TARGET_OS_IOS || TARGET_OS_TV)
			return true;
		#else
			return false;
		#endif
	#elif __ANDROID__
		static bool using_neon;
		static pthread_once_t neon_once = PTHREAD_ONCE_INIT;

		pthread_once(&neon_once, []()
		{
			using_neon = android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM && (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0;
		});

		return using_neon;
	#endif
	}

	void FloatsToUnorm8s (const float * pfloats, unsigned char * u8, size_t n)
	{
		// TODO! (DirectXMath has some stuff that looks viable)
	}

	void Unorm8sToFloats (const unsigned char * u8, float * pfloats, size_t n)
	{
		for (size_t i = 0; i < n; ++i) pfloats[i] = float(u8[i]) / 255.0f; // TODO: This is NOT the SIMD version :D (See above note)
	}
}