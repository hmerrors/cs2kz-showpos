#pragma once
#include "common.h"
#include <iterator>
#ifdef _WIN32
#include <Windows.h>
#else
#include "module_linux.h"
#endif

// Metamod destroys its per-plugin IKHook object before asynchronous hook removal
// finishes. Keep callback helpers independent of that object's lifetime. These
// functions are declared in the public KHook API; no Metamod private state is read.
// Registration still goes through Metamod so its unloader owns every hook ID.
class ShowPosKHookAPI : public KHook::IKHook
{
public:
	bool Bind(KHook::IKHook *owner, char *error, size_t maxlen)
	{
#ifdef _WIN32
		static constexpr const char *names[] = {
			"?GetContextPtr@KHook@@YAPEAXXZ",
			"?GetOriginalFunction@KHook@@YAPEAXXZ",
			"?GetOriginalValuePtr@KHook@@YAPEAXXZ",
			"?GetOverrideValuePtr@KHook@@YAPEAXXZ",
			"?GetCurrentValuePtr@KHook@@YAPEAX_N@Z",
			"?DestroyReturnValue@KHook@@YAXXZ",
			"?FindOriginal@KHook@@YAPEAXPEAX@Z",
			"?FindOriginalVirtual@KHook@@YAPEAXPEAPEAXH@Z",
			"?DoRecall@KHook@@YAPEAXW4Action@1@PEAX_K11@Z",
			"?SaveReturnValue@KHook@@YAXW4Action@1@PEAX_K11_N@Z",
			"?LookupSignature@KHook@@YAPEAXPEAX_KPEBD@Z",
			"?WasOriginalFunctionSkipped@KHook@@YA_NXZ",
		};
#else
		// Official Linux build 1474 hides static-library exports but retains these
		// public function symbols. A stripped/custom image fails before hook setup.
		static constexpr const char *names[] = {
			"_ZN5KHook13GetContextPtrEv",
			"_ZN5KHook19GetOriginalFunctionEv",
			"_ZN5KHook19GetOriginalValuePtrEv",
			"_ZN5KHook19GetOverrideValuePtrEv",
			"_ZN5KHook18GetCurrentValuePtrEb",
			"_ZN5KHook18DestroyReturnValueEv",
			"_ZN5KHook12FindOriginalEPv",
			"_ZN5KHook19FindOriginalVirtualEPPvi",
			"_ZN5KHook8DoRecallENS_6ActionEPvmS1_S1_",
			"_ZN5KHook15SaveReturnValueENS_6ActionEPvmS1_S1_b",
			"_ZN5KHook15LookupSignatureEPvmPKc",
			"_ZN5KHook26WasOriginalFunctionSkippedEv",
		};
#endif
		if (!owner)
		{
			V_snprintf(error, maxlen, "Metamod KHook interface unavailable");
			return false;
		}
		void *addresses[std::size(names)] {};
		void *moduleAddress = (*reinterpret_cast<void ***>(owner))[0];
#ifdef _WIN32
		HMODULE module = nullptr;
		if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
								reinterpret_cast<LPCWSTR>(moduleAddress), &module))
		{
			V_snprintf(error, maxlen, "Cannot locate the loaded Metamod KHook module");
			return false;
		}
		for (size_t i = 0; i < std::size(names); ++i)
		{
			addresses[i] = reinterpret_cast<void *>(GetProcAddress(module, names[i]));
			if (!addresses[i])
			{
				V_snprintf(error, maxlen, "Metamod is missing a public KHook export: %s", names[i]);
				return false;
			}
		}
#else
		std::string diagnostic;
		if (!linux_module::ResolveSymbols(moduleAddress, names, std::size(names), addresses, diagnostic))
		{
			V_snprintf(error, maxlen, "%s", diagnostic.c_str());
			return false;
		}
#endif
		getContext = reinterpret_cast<decltype(getContext)>(addresses[0]);
		getOriginalFunction = reinterpret_cast<decltype(getOriginalFunction)>(addresses[1]);
		getOriginalValue = reinterpret_cast<decltype(getOriginalValue)>(addresses[2]);
		getOverrideValue = reinterpret_cast<decltype(getOverrideValue)>(addresses[3]);
		getCurrentValue = reinterpret_cast<decltype(getCurrentValue)>(addresses[4]);
		destroyReturn = reinterpret_cast<decltype(destroyReturn)>(addresses[5]);
		findOriginal = reinterpret_cast<decltype(findOriginal)>(addresses[6]);
		findOriginalVirtual = reinterpret_cast<decltype(findOriginalVirtual)>(addresses[7]);
		doRecall = reinterpret_cast<decltype(doRecall)>(addresses[8]);
		saveReturn = reinterpret_cast<decltype(saveReturn)>(addresses[9]);
		lookupSignature = reinterpret_cast<decltype(lookupSignature)>(addresses[10]);
		wasOriginalSkipped = reinterpret_cast<decltype(wasOriginalSkipped)>(addresses[11]);
		registration = owner;
		return true;
	}

	void Retire()
	{
		registration = nullptr;
	}

	KHook::HookID_t SetupHook(void *function, void *context, void *removed, void *pre, void *post, void *makeReturn, void *callOriginal,
							  unsigned int stackSize, bool async = false) override
	{
		return registration ? registration->SetupHook(function, context, removed, pre, post, makeReturn, callOriginal, stackSize, async)
							: KHook::INVALID_HOOK;
	}

	KHook::HookID_t SetupVirtualHook(void **vtable, int index, void *context, void *removed, void *pre, void *post, void *makeReturn,
									 void *callOriginal, unsigned int stackSize, bool async = false) override
	{
		return registration ? registration->SetupVirtualHook(vtable, index, context, removed, pre, post, makeReturn, callOriginal, stackSize, async)
							: KHook::INVALID_HOOK;
	}

	void RemoveHook(KHook::HookID_t id, bool async = false, void (*removed)(KHook::HookID_t, void *) = nullptr, void *context = nullptr) override
	{
		if (registration)
		{
			registration->RemoveHook(id, async, removed, context);
		}
	}

	void *GetContextPtr() override
	{
		return getContext();
	}

	void *GetOriginalFunction() override
	{
		return getOriginalFunction();
	}

	void *GetOriginalValuePtr() override
	{
		return getOriginalValue();
	}

	void *GetOverrideValuePtr() override
	{
		return getOverrideValue();
	}

	void *GetCurrentValuePtr(bool pop = false) override
	{
		return getCurrentValue(pop);
	}

	void DestroyReturnValue() override
	{
		destroyReturn();
	}

	void *FindOriginal(void *function) override
	{
		return findOriginal(function);
	}

	void *FindOriginalVirtual(void **vtable, int index) override
	{
		return findOriginalVirtual(vtable, index);
	}

	void *DoRecall(KHook::Action action, void *value, std::size_t size, void *init, void *destroy) override
	{
		return doRecall(action, value, size, init, destroy);
	}

	void SaveReturnValue(KHook::Action action, void *value, std::size_t size, void *init, void *destroy, bool original) override
	{
		saveReturn(action, value, size, init, destroy, original);
	}

	void *LookupSignature(void *start, std::size_t size, const char *signature) override
	{
		return lookupSignature(start, size, signature);
	}

	bool WasOriginalFunctionSkipped() override
	{
		return wasOriginalSkipped();
	}

private:
	KHook::IKHook *registration {};
	decltype(&KHook::GetContextPtr) getContext {};
	decltype(&KHook::GetOriginalFunction) getOriginalFunction {};
	decltype(&KHook::GetOriginalValuePtr) getOriginalValue {};
	decltype(&KHook::GetOverrideValuePtr) getOverrideValue {};
	decltype(&KHook::GetCurrentValuePtr) getCurrentValue {};
	decltype(&KHook::DestroyReturnValue) destroyReturn {};
	decltype(&KHook::FindOriginal) findOriginal {};
	decltype(&KHook::FindOriginalVirtual) findOriginalVirtual {};
	decltype(&KHook::DoRecall) doRecall {};
	decltype(&KHook::SaveReturnValue) saveReturn {};
	decltype(&KHook::LookupSignature) lookupSignature {};
	decltype(&KHook::WasOriginalFunctionSkipped) wasOriginalSkipped {};
};
