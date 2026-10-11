#include "plugin.h"
#include "engine2/isource2server.h"
#include "cstrike15_usermessages.pb.h"
#include <cstring>

#include "native_hook.h"
#include "khook_api.h"

// Keep every registered ID until Metamod removes it after active calls finish.
// Removing an ID here would leave it in Metamod's unload ownership set.
template<class Class, class Ret, class... Args>
class OwnedHook : public KHook::Member<Class, void, Args...>
{
	using Base = KHook::Member<Class, void, Args...>;

public:
	using Base::Base;

	void Stop()
	{
		this->_pre_callback = this->_post_callback = nullptr;
		this->_in_deletion = true;
		std::lock_guard guard(this->_hooks_stored);
		this->_hook_ids.clear();
		this->_associated_hook_id = KHook::INVALID_HOOK;
		targets.clear();
	}

	void Attach(Class *object, int index)
	{
		void *address = KHook::FindOriginalVirtual(*reinterpret_cast<void ***>(object), index);
		if (!address || this->_in_deletion || targets.count(address))
		{
			return;
		}
		// Different pawn classes can implement Teleport at different addresses.
		// Retain each unique implementation instead of replacing its predecessor.
		this->_associated_hook_id = KHook::INVALID_HOOK;
		this->Configure(address);
		if (this->_associated_hook_id != KHook::INVALID_HOOK)
		{
			targets.insert(address);
		}
	}

	template<class M>
	void Attach(Class *object, M method)
	{
		Attach(object, KHook::GetVtableIndex(method));
	}

private:
	std::unordered_set<void *> targets;
};

static ShowPosKHookAPI hookAPI;
static MovementHook movement;

static void Capture(void *p, CMoveData *mv)
{
	plugin.Capture(p, mv);
}

static KHook::Return<void> Frame(ISource2Server *, bool simulating, bool, bool)
{
	if (simulating)
	{
		plugin.Frame();
	}
	return {KHook::Action::Ignore};
}

static OwnedHook<ISource2Server, void, bool, bool, bool> frame(nullptr, Frame);

static KHook::Return<void> Disconnect(ISource2GameClients *, CPlayerSlot slot, ENetworkDisconnectionReason, const char *, uint64, const char *)
{
	if (plugin.initialized && slot.Get() >= 0 && slot.Get() < 64)
	{
		plugin.Reset(slot.Get());
	}
	return {KHook::Action::Ignore};
}

static OwnedHook<ISource2GameClients, void, CPlayerSlot, ENetworkDisconnectionReason, const char *, uint64, const char *> disconnect(Disconnect,
																																	 nullptr);

static KHook::Return<void> Click(ISource2GameClients *, CPlayerSlot slot, int type, uint32 size, const void *data)
{
	if (type == CS_UM_CustomHudClicked && data && size <= 1024)
	{
		CCSUsrMsg_CustomHudClicked msg;
		if (msg.ParseFromArray(data, int(size)))
		{
			plugin.Click(slot.Get(), msg.custom_hud_layout(), msg.button_id().c_str());
		}
	}
	return {KHook::Action::Ignore};
}

static OwnedHook<ISource2GameClients, void, CPlayerSlot, int, uint32, const void *> click(Click, nullptr);

static KHook::Return<void> Dispatch(ICvar *, ConCommandRef, const CCommandContext &ctx, const CCommand &args)
{
	if (!plugin.initialized)
	{
		return {KHook::Action::Ignore};
	}
	if (args.ArgC() >= 2 && (!V_stricmp(args[0], "say") || !V_stricmp(args[0], "say_team")))
	{
		CCommand text;
		text.Tokenize(args.Arg(1));
		if (text.ArgC() && (!V_stricmp(text[0], "!showpos") || !V_stricmp(text[0], "/showpos")))
		{
			plugin.Command(ctx.GetPlayerSlot().Get(), text);
			return {KHook::Action::Supersede};
		}
	}
	return {KHook::Action::Ignore};
}

// The console command can be "meta unload showpos" while this very hook is active.
// Let Metamod retain the DLL and remove hooks after their active calls return.
// Forget the wrapper's IDs: Metamod owns the final removal, not a DLL destructor.
class CommandHook : public KHook::Virtual<ICvar, void, ConCommandRef, const CCommandContext &, const CCommand &>
{
	using Base = KHook::Virtual<ICvar, void, ConCommandRef, const CCommandContext &, const CCommand &>;

public:
	using Base::Base;

	void Stop()
	{
		_pre_callback = _post_callback = nullptr;
		_in_deletion = true;
		std::lock_guard guard(_hooks_stored);
		_hook_ids_addr.clear();
		_addr_hook_ids.clear();
	}
};

static CommandHook dispatch(Dispatch, nullptr);

static KHook::Return<void> Transmit(ISource2GameEntities *, CCheckTransmitInfo **infos, int count, CBitVec<16384> &, CBitVec<16384> &,
									const Entity2Networkable_t **, const uint16 *, int)
{
	if (!plugin.initialized || !GameEntitySystem())
	{
		return {KHook::Action::Ignore};
	}
	int indices[64][2];
	for (int owner = 0; owner < 64; owner++)
	{
		auto &p = plugin.players[owner];
		indices[owner][0] = p.hud.Get() ? p.hud.GetEntryIndex() : -1;
		indices[owner][1] = p.menu.Get() ? p.menu.GetEntryIndex() : -1;
	}
	for (int i = 0; i < count; i++)
	{
		auto info = infos[i];
		if (!info || !info->m_pTransmitEntity)
		{
			continue;
		}
		int recipient = info->m_nPlayerSlot.Get();
		for (int owner = 0; owner < 64; owner++)
		{
			if (owner != recipient)
			{
				for (int k = 0; k < 2; k++)
				{
					if (indices[owner][k] >= 0 && indices[owner][k] < 16384)
					{
						info->m_pTransmitEntity->Clear(indices[owner][k]);
					}
				}
			}
		}
	}
	return {KHook::Action::Ignore};
}

static OwnedHook<ISource2GameEntities, void, CCheckTransmitInfo **, int, CBitVec<16384> &, CBitVec<16384> &, const Entity2Networkable_t **,
				 const uint16 *, int>
	transmit(nullptr, Transmit);

static KHook::Return<void> Teleport(CEntityInstance *p, const Vector *, const QAngle *, const Vector *)
{
	plugin.Invalidate(p);
	return {KHook::Action::Ignore};
}

static OwnedHook<CEntityInstance, void, const Vector *, const QAngle *, const Vector *> teleport(Teleport, nullptr);

void hooks::Pawn(CEntityInstance *p)
{
	if (p)
	{
		teleport.Attach(p, engine::teleportIndex);
	}
}

bool hooks::Init(char *error, size_t maxlen)
{
	if (!hookAPI.Bind(KHook::__exported__khook, error, maxlen))
	{
		return false;
	}
	KHook::__exported__khook = &hookAPI;
	if (!movement.Install(engine::movementAddress, Capture))
	{
		V_snprintf(error, maxlen, "Cannot install the CS2KZ movement observer");
		return false;
	}
	frame.Attach(engine::game, &ISource2Server::GameFrame);
	disconnect.Attach(engine::clients, &ISource2GameClients::ClientDisconnect);
	click.Attach(engine::clients, &ISource2GameClients::ClientSvcUserMessage);
	dispatch.Configure(&ICvar::DispatchConCommand);
	dispatch.Add(g_pCVar);
	transmit.Attach(engine::entities, &ISource2GameEntities::CheckTransmit);

	return true;
}

void hooks::Cleanup()
{
	movement.Retire();
	frame.Stop();
	disconnect.Stop();
	click.Stop();
	dispatch.Stop();
	transmit.Stop();
	teleport.Stop();
	hookAPI.Retire();
}
