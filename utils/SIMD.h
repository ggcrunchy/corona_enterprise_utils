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

#pragma once

#include "utils/Compat.h"
#include "utils/Namespace.h"
#include "external/aligned_allocator.h"
#include <utility>
#include <vector>

CEU_BEGIN_NAMESPACE(SimdXS) {
	bool CanUseNeon (void);

	void FloatsToUnorm8s (const float * _RESTRICT pfloats, unsigned char * _RESTRICT u8, size_t n, bool bNoTile = false);
	void Unorm8sToFloats (const unsigned char * _RESTRICT u8, float * _RESTRICT pfloats, size_t n, bool bNoTile = false);

	template<bool = false> struct HasSIMD : public std::false_type {};
CEU_END_NAMESPACE(SimdXS)
