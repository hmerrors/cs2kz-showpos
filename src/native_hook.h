#pragma once
#include "common.h"
#include "movedata.h"

// A true pre-only callback (null native post thunk), registered AFTER CS2KZ's
// mixed movement hook. Native API 18 integration tests verify this ordering.
// All runtime detour targets belong to the engine, never an unloadable plugin.
class MovementHook
{
public:
	using Callback = void (*)(void *, CMoveData *);
	KHook::HookID_t id {KHook::INVALID_HOOK};
	Callback callback {};

	bool Install(void *address, Callback cb, bool mixed = false)
	{
		callback = cb;
		id = KHook::SetupHook(address, this, reinterpret_cast<void *>(&Removed), KHook::ExtractMFP(&MovementHook::Pre),
							  mixed ? KHook::ExtractMFP(&MovementHook::Post) : nullptr, KHook::ExtractMFP(&MovementHook::Return),
							  KHook::ExtractMFP(&MovementHook::Original), 32, false);
		return id != KHook::INVALID_HOOK;
	}

	void Clear()
	{
		if (id != KHook::INVALID_HOOK)
		{
			KHook::RemoveHook(id, false);
		}
	}

	static void Removed(KHook::HookID_t)
	{
		KHook::GetContext<MovementHook>()->id = KHook::INVALID_HOOK;
	}

	void Pre(CMoveData *move)
	{
		auto ctx = KHook::GetContext<MovementHook>();
		ctx->callback(this, move);
		KHook::__internal__savereturnvalue(KHook::Return<void> {KHook::Action::Ignore}, false);
	}

	void Post(CMoveData *)
	{
		KHook::__internal__savereturnvalue(KHook::Return<void> {KHook::Action::Ignore}, false);
	}

	void Return(CMoveData *)
	{
		KHook::DestroyReturnValue();
	}

	void Original(CMoveData *move)
	{
		auto fn = KHook::BuildMFP<void (MovementHook::*)(CMoveData *)>(KHook::GetOriginalFunction());
		(this->*fn)(move);
		KHook::__internal__savereturnvalue(KHook::Return<void> {KHook::Action::Ignore}, true);
	}
};
