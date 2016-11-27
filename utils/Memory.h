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

#include "CoronaLua.h"
#include "external/aligned_allocator.h"
#include <type_traits>
#include <vector>

//
namespace MemoryXS {
	struct LuaMemory {
		lua_State * mL{nullptr};// Main Lua state for this 
		int mIndex{0};	// Index of memory
		int mRegistrySlot{LUA_NOREF};	// Slot in registry, if used
		int mStoreSlot{LUA_NOREF};	// Slot in registry for table storage, if used

		struct BookmarkDualTables {
			LuaMemory * mOwner;	// Memory TLS to repair

			~BookmarkDualTables (void) { mOwner->UnloadTable(); }
		};

		struct BookmarkIndex {
			LuaMemory * mOwner;	// Memory TLS to repair
			int mIndex;	// Saved index

			~BookmarkIndex (void) { mOwner->mIndex = mIndex; }
		};

		BookmarkDualTables BindTable (void);
		BookmarkIndex SavePosition (bool bRelocate = true);

		static LuaMemory * New (lua_State * L);

		size_t GetOldSize (int slot, void * ptr);

		void * Add (int slot, size_t size);

		int Begin (void);

		void End (void);
		void LoadTable (void);
		void PrepDualTables (void);
		void PrepMemory (int slot = 0);
		void PrepRegistry (void);
		void PushObject (int slot, void * ptr);
		void Remove (int slot, void * ptr);
		void UnloadTable (void);

		// Interface
		void FailAssert (const char * what);
		void * Malloc (size_t size);
		void * Calloc (size_t num, size_t size);
		void * Realloc (void * ptr, size_t size);
		void Free (void * ptr);
		size_t GetSize (void * ptr);
		bool Emit (void * ptr, bool bRemove = true);
		void Push (void * ptr, const char * type, bool bAsUserdata, bool bRemove = true);
	};

	#ifdef _MSC_VER
		#define ALIGNED_N(n) __declspec(align(n))
	#else
		#define ALIGNED_N(n) __attribute__((aligned(n)))
	#endif

	//
	template<typename T> struct AlignedVector16 : std::vector<T, simdpp::SIMDPP_ARCH_NAMESPACE::aligned_allocator<T, 16U>>
	{
		template<typename ... Args> AlignedVector16 (Args && ... args) : std::vector<T, allocator_type>(std::forward<Args>(args)...)
		{
		}
	};

	//
	template<typename T> struct AlignedVector64 : std::vector<T, simdpp::SIMDPP_ARCH_NAMESPACE::aligned_allocator<T, 64U>>
	{
		template<typename ... Args> AlignedVector64 (Args && ... args) : std::vector<T, allocator_type>(std::forward<Args>(args)...)
		{
		}
	};

	void * Align (size_t bound, size_t size, void *& ptr, size_t * space = nullptr);

	// Adapted from https://raw.githubusercontent.com/evgeny-panasyuk/cps_alloca/master/core_idea.cpp

	template<typename T, unsigned N, typename F>
	auto cps_alloca_static (F && f) -> decltype(f(nullptr, nullptr))
	{
		ALIGNED_N(64) struct {
			const size_t kSizeRoundedUp = (sizeof(mUnits) + 63U) & -64U;

			T mUnits[N];
			char mDad[kSizeRoundedUp - sizeof(mUnits)];
		} mData;

		return f(reinterpret_cast<T *>(&mData.mUnits[0]), reinterpret_cast<T *>(&mData.mUnits[0]) + N);
	}

	template<typename T, typename F>
	auto cps_alloca_dynamic (unsigned n, F && f) -> decltype(f(nullptr, nullptr))
	{
		const size_t kExtraN = ((sizeof(T) + 63U) & -64U) / sizeof(T);

		ALIGNED_N(64) AlignedVector64<T> data(n + kExtraN);

		return f(&data[0], &data[0] + n);
	}

	template<typename T,typename F>
	auto cps_alloca(unsigned n, F && f) -> decltype(f(nullptr, nullptr))
	{
		switch (n)
		{
			case 1: return cps_alloca_static<T, 1>(f);
			case 2: return cps_alloca_static<T, 2>(f);
			case 3: return cps_alloca_static<T, 3>(f);
			case 4: return cps_alloca_static<T, 4>(f);
			case 0: return f(nullptr, nullptr);
			default: return cps_alloca_dynamic<T>(n, f);
		}; // mpl::for_each / array / index pack / recursive bsearch / etc variacion
	}
}