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
#if 0
#include "CoronaLua.h"
#include <stdlib.h>

#define TLS(prefix)	/* Thread-local storage key */									\
					static pthread_key_t prefix##_TLS_Key;							\
					/* Once-only initialisation of the key */						\
					static pthread_once_t prefix##_TLS_key_once = PTHREAD_ONCE_INIT

#define TLS_GETTER(prefix, type) type * prefix##_GetTLS (void) { return (type *)pthread_getspecific(prefix##_TLS_Key); }

#define TLS_PUBLIC_GETTER(prefix, type) TLS(prefix);	\
							TLS_GETTER(prefix, type)

#define TLS_STATIC_GETTER(prefix, type) TLS(prefix);	\
							static TLS_GETTER(prefix, type)

#define TLS_INIT_NO_REGISTER(prefix, type)	pthread_once(&prefix##_TLS_key_once, []()					\
											{															\
												pthread_key_create(&prefix##_TLS_Key, NULL);			\
												atexit([]() { pthread_key_delete(prefix##_TLS_Key); });	\
											});															\
																										\
											lua_pushlightuserdata(L, &prefix##_TLS_Key);				\
																										\
											type * tls = (type *)lua_newuserdata(L, sizeof(type));		\
																										\
											pthread_setspecific(prefix##_TLS_Key, tls)

#define TLS_INIT(prefix, type)	TLS_INIT_NO_REGISTER(prefix, type);	\
								lua_settable(L, LUA_REGISTRYINDEX)

#define TLS_MEMORY_INIT(prefix, type)	TLS_INIT(prefix, type);	\
																\
										new (tls) type();		\
																\
										tls->mL = L

struct MemoryTLS {
	lua_State * mL;	// Main Lua state for this 
	int mIndex;	// Index of memory
	int mRegistrySlot;	// Slot in registry, if used
	int mStoreSlot;	// Slot in registry for table storage, if used

	struct BookmarkDualTables {
		MemoryTLS * mOwner;	// Memory TLS to repair

		~BookmarkDualTables (void) { mOwner->UnloadTable(); }
	};

	struct BookmarkIndex {
		MemoryTLS * mOwner;	// Memory TLS to repair
		int mIndex;	// Saved index

		~BookmarkIndex (void) { mOwner->mIndex = mIndex; }
	};

	BookmarkDualTables BindTable (void);
	BookmarkIndex SavePosition (bool bRelocate = true);

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

	MemoryTLS (void) : mL(NULL), mIndex(0), mRegistrySlot(LUA_NOREF), mStoreSlot(LUA_NOREF) {}
};

void FailAssertTLS (MemoryTLS * tls, const char * what);
void * MallocTLS (MemoryTLS * tls, size_t size);
void * CallocTLS (MemoryTLS * tls, size_t num, size_t size);
void * ReallocTLS (MemoryTLS * tls, void * ptr, size_t size);
void FreeTLS (MemoryTLS * tls, void * ptr);
size_t GetSizeTLS (MemoryTLS * tls, void * ptr);
bool EmitTLS (MemoryTLS * tls, void * ptr, bool bRemove = true);
void PushTLS (MemoryTLS * tls, void * ptr, const char * type, bool bAsUserdata, bool bRemove = true);

template<typename T, T * (*GET)(void)> struct MemoryFuncsTLS {
	typedef MemoryFuncsTLS<T, GET> FuncsType;

	static void FailAssert (const char * what) { FailAssertTLS(GET(), what); }
	static void * Malloc (size_t size) { return MallocTLS(GET(), size); }
	static void * Calloc (size_t num, size_t size) { return CallocTLS(GET(), num, size); }
	static void * Realloc (void * ptr, size_t size) { return ReallocTLS(GET(), ptr, size); }
	static void Free (void * ptr) { FreeTLS(GET(), ptr); }
	static size_t GetSize (void * ptr) { return GetSizeTLS(GET(), ptr); }
	static bool Emit (void * ptr) { return EmitTLS(GET(), ptr); }
	static void Push (void * ptr, const char * type, bool as_userdata) { PushTLS(GET(), ptr, type, as_userdata); }
};
#endif
#ifdef _WIN32
	#define DECL_TLS __declspec(thread)
#else
	#define DECL_TLS __thread
#endif