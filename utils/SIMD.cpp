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

#include "utils/Memory.h"
#include "utils/SIMD.h"

#ifdef _WIN32
	#include "DirectXMath/Inc/DirectXMath.h"
	#include "DirectXMath/Inc/DirectXPackedVector.h"
#elif __APPLE__
	#include "TargetConditionals.h"

	#if !TARGET_OS_IOS
		#include <Accelerate/Accelerate.h>
	#endif
#else
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
		static struct UsingNeon {
			bool mUsing;

			UsingNeon (void)
			{
				mUsing = android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM && (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0;
			}
		} sNeon;

		return sNeon.mUsing;
	#endif
	}

	#if defined(__ANDROID__) || (!TARGET_OS_SIMULATOR && (TARGET_OS_IOS || TARGET_OS_TV))
		namespace Neon {	// Everything here is pared down from DirectXMath
			typedef float32x4_t XMVECTOR;
			typedef const XMVECTOR FXMVECTOR;

			struct XMFLOAT4
			{
				float x;
				float y;
				float z;
				float w;

				explicit XMFLOAT4 (const float *pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]), w(pArray[3]) {}
			};

			ALIGNED_N_BEGIN(16) struct XMVECTORF32
			{
				union
				{
					float f[4];
					XMVECTOR v;
				};

				inline operator XMVECTOR() const { return v; }
				inline operator const float*() const { return f; }
			} ALIGNED_N_END(16);

			inline XMVECTOR XMLoadFloat4 (const XMFLOAT4* pSource)
			{
				return vld1q_f32( reinterpret_cast<const float*>(pSource) );
			}

			namespace PackedVector {
				struct XMUBYTEN4
				{
					union
					{
						struct
						{
							uint8_t x;
							uint8_t y;
							uint8_t z;
							uint8_t w;
						};
						uint32_t v;
					};

					explicit XMUBYTEN4 (const float *pArray)
					{
						XMStoreUByteN4(this, XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(pArray)));
					}
				};

				inline XMVECTOR XMLoadUByteN4 (const XMUBYTEN4* pSource)
				{
					uint32x2_t vInt8 = vld1_dup_u32( reinterpret_cast<const uint32_t*>( pSource ) );
					uint16x8_t vInt16 = vmovl_u8( vreinterpret_u8_u32(vInt8) );
					uint32x4_t vInt = vmovl_u16( vget_low_u16(vInt16) );
					float32x4_t R = vcvtq_f32_u32(vInt);
					return vmulq_n_f32( R, 1.0f/255.0f );
				}

				inline void XMStoreUByteN4 (XMUBYTEN4* pDestination, FXMVECTOR V)
				{
					float32x4_t R = vmaxq_f32(V, vdupq_n_f32(0) );
					R = vminq_f32(R, vdupq_n_f32(1.0f));
					R = vmulq_n_f32( R, 255.0f );
					uint32x4_t vInt32 = vcvtq_u32_f32(R);
					uint16x4_t vInt16 = vqmovn_u32( vInt32 );
					uint8x8_t vInt8 = vqmovn_u16( vcombine_u16(vInt16,vInt16) );
					vst1_lane_u32( &pDestination->v, vreinterpret_u32_u8(vInt8), 0 );
				}
			}
		}

		namespace ns_f2u8 = Neon;
	#elif _WIN32
		namespace ns_f2u8 = DirectX;
	#endif

	void FloatsToUnorm8s (const float * _RESTRICT pfloats, unsigned char * _RESTRICT u8, size_t n)
	{
		void * ptr = const_cast<float *>(pfloats);

	#if !TARGET_OS_MAC
		#ifdef __ANDROID__
			if (CanUseNeon()) {
		#endif

		MemoryXS::Align(16U, n * sizeof(float), ptr);

		ns_f2u8::PackedVector::XMUBYTEN4 out;

		// Peel off any leading floats.
		if (pfloats != ptr)
		{
			size_t extra = static_cast<const float *>(ptr) - pfloats;
			ns_f2u8::XMVECTORF32 padding = { 0 };

			for (size_t i = 0; i < extra; ++i, --n) padding.f[4U - extra + i] = pfloats[i];

			ns_f2u8::PackedVector::XMStoreUByteN4(&out, padding.v);

			memcpy(u8, reinterpret_cast<unsigned char *>(&out) + 4U - extra, extra);

			u8 += extra;
		}

		// Blast through the aligned region.
		auto from = reinterpret_cast<const ns_f2u8::XMVECTOR *>(ptr);

		for (; n >= 4U; u8 += 4U, n-= 4U) ns_f2u8::PackedVector::XMStoreUByteN4(reinterpret_cast<ns_f2u8::PackedVector::XMUBYTEN4 *>(u8), *from++);

		// Peel off any trailing floats.
		if (n)
		{
			ns_f2u8::XMVECTORF32 padding = { 0 };

			memcpy(padding.f, from, n * sizeof(float));

			ns_f2u8::PackedVector::XMStoreUByteN4(&out, padding.v);

			memcpy(u8, &out, n);
		}

		return;	// TODO: temporary
	#else
		// Accelerate: try PlanarF -> Planar8

	#endif

	#ifdef __ANDROID__
		} else
	#endif
		// Neon unavailable, so just use scalar approach.
		for (size_t i = 0; i < n; ++i) u8[i] = (unsigned char)(pfloats[i] * 255.0f);
	}

	void Unorm8sToFloats (const unsigned char * _RESTRICT u8, float * _RESTRICT pfloats, size_t n)
	{
		void * ptr = pfloats;

	#if !TARGET_OS_MAC
		#ifdef __ANDROID__
			if (CanUseNeon()) {
		#endif

		MemoryXS::Align(16U, n * sizeof(float), ptr);

		// Peel off any leading floats.
		if (pfloats != ptr)
		{
			size_t extra = static_cast<float *>(ptr) - pfloats;
			uint8_t padding[4] = { 0 };

			for (size_t i = 0; i < extra; ++i, --n) padding[4U - extra + i] = *u8++;
			
			ns_f2u8::PackedVector::XMUBYTEN4 bytes{padding};

			auto result = ns_f2u8::PackedVector::XMLoadUByteN4(&bytes);

			memcpy(pfloats - extra, reinterpret_cast<float *>(&result) + 4U - extra, extra * sizeof(float));
		}

		// Blast through the aligned region.
		auto to = reinterpret_cast<ns_f2u8::XMVECTOR *>(ptr);

		for (; n >= 4U; u8 += 4U, n-= 4U)
		{
			ns_f2u8::PackedVector::XMUBYTEN4 bytes{u8};

			*to++ = ns_f2u8::PackedVector::XMLoadUByteN4(&bytes);
		}

		// Peel off any trailing floats.
		if (n)
		{
			uint8_t padding[4] = { 0 };

			for (size_t i = 0; i < n; ++i) padding[i] = u8[i];

			ns_f2u8::PackedVector::XMUBYTEN4 bytes{padding};

			auto result = ns_f2u8::PackedVector::XMLoadUByteN4(&bytes);

			memcpy(to, &result, n * sizeof(float));
		}

		return;	// TODO: temporary
	#else
		// Accelerate: try Planar8 -> PlanarF
	#endif

	#ifdef __ANDROID__
		} else
	#endif
		// Neon unavailable, so just use scalar approach.
		for (size_t i = 0; i < n; ++i) pfloats[i] = float(u8[i]) / 255.0f;
	}
}