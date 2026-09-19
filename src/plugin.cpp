#include "plugin.h"
#include "KeyValues.h"
#include <cstring>
#include <charconv>
#include <cmath>

ShowPosPlugin plugin;
PLUGIN_EXPOSE(ShowPosPlugin, plugin);
static bool dllUnloading = false;

const char *ShowPosPlugin::GetAuthor()
{
	return "hm_error and bell_meow";
}

static void ShowPosCommand(const CCommandContext &context, const CCommand &args)
{
	plugin.Command(context.GetPlayerSlot().Get(), args);
}

static void StatusCommand(const CCommandContext &, const CCommand &)
{
	plugin.Status();
}

// Owned by Load/Unload. Engine shutdown can destroy ICvar before DLL globals;
// do not put ConCommand destructors in the DLL's process-exit destructor list.
static ConCommand *showPosCommand, *statusCommand;

static void UnregisterCommands()
{
	// The SDK's ConVar_Unregister only changes its registration flag. Destroying
	// still-valid static ConCommand refs after ICvar shutdown accesses freed state.
	for (auto command : {showPosCommand, statusCommand})
	{
		if (command)
		{
			g_SMAPI->UnregisterConCommand(g_PLAPI, command);
			command->InvalidateRef();
			delete command;
		}
	}
	showPosCommand = statusCommand = nullptr;
	ConVar_Unregister();
}

bool ShowPosPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	if (dllUnloading)
	{
		V_snprintf(error, maxlen, "ShowPos unload is still completing; retry after one second");
		return false;
	}
	PLUGIN_SAVEVARS();
	if (!KHook::__exported__khook)
	{
		V_snprintf(error, maxlen, "Metamod API 18 native detours unavailable");
		return false;
	}
	Dependencies();
	if (!kz || !mam)
	{
		V_snprintf(error, maxlen, "Load CS2KZ and MultiAddonManager before ShowPos");
		return false;
	}
	if (!engine::Open(ismm, error, maxlen))
	{
		return false;
	}
	schemaOK = true;
	auto config = new KeyValues("ShowPos");
	if (config->LoadFromFile(engine::files, "cfg/showpos.cfg", "GAME"))
	{
		chinese = !V_stricmp(config->GetString("language", "zh"), "zh");
	}
	delete config;
	showPosCommand = new ConCommand("kz_showpos", ShowPosCommand, "Open standalone ShowPos settings", FCVAR_CLIENT_CAN_EXECUTE);
	statusCommand = new ConCommand("showpos_status", StatusCommand, "Show standalone ShowPos readiness");
	META_CONVAR_REGISTER(FCVAR_NONE);
	if (KHook::FindOriginal(engine::movementAddress) == engine::movementAddress || !hooks::Init())
	{
		hooks::Cleanup();
		UnregisterCommands();
		V_snprintf(error, maxlen, "CS2KZ movement detour is not ready");
		return false;
	}
	initialized = true;
	ismm->AddListener(this, this);
	Dependencies();
	Msg("[ShowPos] Standalone API 18 loaded; CS2KZ core unchanged. Use showpos_status.\n");
	return true;
}

bool ShowPosPlugin::Unload(char *, size_t)
{
	dllUnloading = true;
	paused = true;
	for (int i = 0; i < 64; i++)
	{
		Reset(i);
	}
	initialized = false;
	hooks::Cleanup();
	UnregisterCommands();
	kz = nullptr;
	mam = nullptr;
	return true;
}

void ShowPosPlugin::Dependencies()
{
	kz = static_cast<ICS2KZ *>(g_SMAPI->MetaFactory(CS2KZ_INTERFACE, nullptr, &kzId));
	mam = static_cast<IMultiAddonManager *>(g_SMAPI->MetaFactory(MULTIADDONMANAGER_INTERFACE, nullptr, &mamId));
}

void ShowPosPlugin::AllPluginsLoaded()
{
	Dependencies();
}

void ShowPosPlugin::OnPluginLoad(PluginId)
{
	if (initialized)
	{
		Dependencies();
	}
}

void ShowPosPlugin::OnPluginUnload(PluginId id)
{
	if (id != kzId && id != mamId)
	{
		return;
	}
	if (id == kzId)
	{
		kz = nullptr;
		dependencyReloadRequired = true;
		Warning("[ShowPos] CS2KZ unloaded. Reload ShowPos after CS2KZ to restore pre-movement ordering.\n");
	}
	if (id == mamId)
	{
		mam = nullptr;
	}
	for (int i = 0; i < 64; i++)
	{
		Reset(i);
	}
}

void ShowPosPlugin::OnPluginPause(PluginId id)
{
	if (id != kzId && id != mamId)
	{
		return;
	}
	if (id == kzId)
	{
		kz = nullptr;
	}
	if (id == mamId)
	{
		mam = nullptr;
	}
	for (int i = 0; i < 64; i++)
	{
		Reset(i);
	}
}

void ShowPosPlugin::OnPluginUnpause(PluginId)
{
	Dependencies();
}

void ShowPosPlugin::OnLevelShutdown()
{
	for (int i = 0; i < 64; i++)
	{
		Reset(i);
	}
	nextFrame = 0;
}

void ShowPosPlugin::OnLevelInit(const char *, const char *, const char *, const char *, bool, bool)
{
	for (int i = 0; i < 64; i++)
	{
		Reset(i, false);
	}
	schemaOK = engine::SchemaReady();
	nextFrame = 0;
	Dependencies();
}

bool ShowPosPlugin::QueryRunning(char *error, size_t maxlen)
{
	if (initialized && kz && mam && schemaOK && !dependencyReloadRequired)
	{
		return true;
	}
	V_snprintf(error, maxlen, "Requires CS2KZ ICS2KZ001, MultiAddonManager003, and supported game schema");
	return false;
}

bool ShowPosPlugin::Pause(char *, size_t)
{
	paused = true;
	for (int i = 0; i < 64; i++)
	{
		hud::Destroy(i);
		players[i].valid = false;
		prefs::Save(players[i]);
	}
	return true;
}

bool ShowPosPlugin::Unpause(char *, size_t)
{
	paused = false;
	Dependencies();
	return true;
}

void ShowPosPlugin::Reset(int slot, bool remove)
{
	auto &p = players[slot];
	if (!prefs::Save(p))
	{
		Warning("[ShowPos] Could not save preferences for %llu\n", p.steam);
	}
	if (remove)
	{
		hud::Destroy(slot);
	}
	p = Player();
}

void ShowPosPlugin::Invalidate(CEntityInstance *p)
{
	if (!initialized || !p || strcmp(p->GetClassname(), "player"))
	{
		return;
	}
	int slot = engine::Slot(p);
	if (slot >= 0 && slot < 64)
	{
		players[slot].valid = false;
	}
}

void ShowPosPlugin::Capture(void *ms, CMoveData *move)
{
	if (!Ready() || !schemaOK || !move || !ms || !GameEntitySystem())
	{
		return;
	}
	CEntityHandle pawnHandle(move->m_nPlayerHandle);
	auto p = pawnHandle.Get();
	if (!engine::Alive(p) || engine::moveServices.Read<void *>(p) != ms)
	{
		return;
	}
	int slot = engine::Slot(p);
	if (slot < 0 || slot >= 64 || engine::Pawn(slot) != p)
	{
		return;
	}
	auto &state = players[slot];
	state.sample.origin = move->m_vecAbsOrigin;
	state.sample.angles = move->m_vecViewAngles;
	state.sample.velocity = move->m_vecVelocity;
	auto read = [&](const Field &field, float &value, bool &available)
	{
		auto v = field.At<float>(ms);
		available = v && std::isfinite(*v);
		value = available ? *v : 0;
	};
	read(engine::stamina, state.sample.stamina, state.sample.hasStamina);
	read(engine::duckAmount, state.sample.duckAmount, state.sample.hasDuckAmount);
	read(engine::duckSpeed, state.sample.duckSpeed, state.sample.hasDuckSpeed);
	state.samplePawn = pawnHandle;
	state.sampleTime = engine::Now();
	state.sampleTeam = engine::team.Read<int>(p);
	state.valid = true;
	samples++;
}

void ShowPosPlugin::Frame()
{
	if (!Ready() || !schemaOK || !GameEntitySystem())
	{
		return;
	}
	double now = engine::Now();
	if (now < nextFrame && nextFrame - now < 1)
	{
		return;
	}
	nextFrame = now + 0.05;
	for (int slot = 0; slot < 64; slot++)
	{
		auto &p = players[slot];
		auto controller = engine::Controller(slot);
		if (!controller || !kz->IsValidPlayer(slot))
		{
			if (p.prefsLoaded || p.menu.IsValid() || p.hud.IsValid())
			{
				Reset(slot);
			}
			continue;
		}
		auto pawn = engine::Pawn(slot);
		if (engine::Alive(pawn))
		{
			hooks::Pawn(pawn);
		}
		else
		{
			p.valid = false;
		}
		if (engine::hltv.Read<bool>(controller))
		{
			continue;
		}
		uint64 steam = engine::server->IsClientFullyAuthenticated(CPlayerSlot(slot)) ? kz->GetSteamID64(slot) : 0;
		if (p.steam && steam && p.steam != steam)
		{
			Reset(slot);
		}
		if (!p.prefsLoaded && steam)
		{
			prefs::Load(p, steam);
		}
		if (p.dirty && p.prefsLoaded && now >= p.saveAt)
		{
			if (!prefs::Save(p))
			{
				Warning("[ShowPos] Preference save failed for %llu\n", p.steam);
			}
			p.saveAt = now + 5;
		}
		if (p.menuOpen && now >= p.menuExpires)
		{
			hud::CloseMenu(slot);
		}
		int source = slot;
		if (!engine::Alive(pawn))
		{
			auto observer = engine::Pawn(slot, true);
			void *services = engine::observers.Read<void *>(observer);
			uint32 mode = engine::observerMode.Read<uint32>(services);
			auto target = engine::observerTarget.At<CEntityHandle>(services);
			// Guard the actual engine target before calling the CS2KZ spectator helper.
			if ((mode != 2 && mode != 3) || !target || !engine::Alive(target->Get()))
			{
				source = -1;
			}
			else
			{
				source = kz->GetSpectatedSlot(slot);
			}
		}
		const ShowPosSnapshot *sample = nullptr;
		if (source >= 0 && source < 64)
		{
			auto &s = players[source];
			auto target = engine::Pawn(source);
			if (s.valid && target && s.samplePawn == target->GetRefEHandle() && engine::Alive(target)
				&& engine::team.Read<int>(target) == s.sampleTeam && now >= s.sampleTime && now - s.sampleTime <= 0.25)
			{
				sample = &s.sample;
			}
		}
		hud::Draw(slot, sample);
	}
}

void ShowPosPlugin::Command(int slot, const CCommand &args)
{
	if (slot < 0 || slot >= 64)
	{
		Status();
		Msg("[ShowPos] kz_showpos is a PLAYER command.\n");
		return;
	}
	if (!Ready() || !schemaOK || !kz->IsValidPlayer(slot))
	{
		return;
	}
	auto &p = players[slot];
	double now = engine::Now();
	if (now < p.nextMenu)
	{
		return;
	}
	p.nextMenu = now + 0.15;
	if (!p.prefsLoaded && engine::server->IsClientFullyAuthenticated(CPlayerSlot(slot)))
	{
		prefs::Load(p, kz->GetSteamID64(slot));
	}
	if (args.ArgC() > 1 && !V_stricmp(args[1], "close"))
	{
		hud::CloseMenu(slot);
		return;
	}
	if (args.ArgC() == 3)
	{
		int value;
		auto t = args[2];
		auto parse = std::from_chars(t, t + strlen(t), value);
		if (parse.ec == std::errc() && parse.ptr == t + strlen(t))
		{
			if (!V_stricmp(args[1], "x"))
			{
				p.prefs.x = value;
			}
			else if (!V_stricmp(args[1], "y"))
			{
				p.prefs.y = value;
			}
			else if (!V_stricmp(args[1], "size"))
			{
				p.prefs.size = value;
			}
			else
			{
				return;
			}
			prefs::Normalize(p.prefs);
			p.dirty = true;
			p.saveAt = now + 1;
		}
	}
	if (!hud::OpenMenu(slot))
	{
		engine::server->ClientPrintf(CPlayerSlot(slot), "[ShowPos] CS2KZ HUD assets are not mounted. Check MultiAddonManager; see showpos_status.\n");
	}
}

void ShowPosPlugin::Click(int slot, uint32 handle, const char *button)
{
	if (!Ready() || slot < 0 || slot >= 64 || !button)
	{
		return;
	}
	auto &p = players[slot];
	auto ent = p.menu.Get();
	if (!p.menuOpen || !ent || uint32(ent->GetRefEHandle().ToPackedInt()) != handle)
	{
		return;
	}
	if (!strcmp(button, "m_close"))
	{
		hud::CloseMenu(slot);
		return;
	}
	double now = engine::Now();
	if (now < p.nextMenu)
	{
		return;
	}
	p.nextMenu = now + 0.10;
	if (strlen(button) == 5 && !strncmp(button, "item", 4) && button[4] >= '0' && button[4] <= '8')
	{
		prefs::Cycle(p.prefs, button[4] - '0');
		p.dirty = true;
		p.saveAt = now + 1;
		p.menuExpires = now + 60;
		hud::RenderMenu(slot);
	}
}

void ShowPosPlugin::Status()
{
	int huds = 0, menus = 0, fresh = 0;
	for (auto &p : players)
	{
		huds += p.hud.IsValid();
		menus += p.menu.IsValid();
		fresh += p.valid;
	}
	Msg("[ShowPos] 1.0.0-rc1 / API 18 / Windows x64\n[ShowPos] CS2KZ=%s MAM=%s schema=%s assets=%s paused=%s\n[ShowPos] Pre-movement samples=%llu, "
		"changed HUD text updates=%llu\n",
		kz ? "ready" : "missing", mam ? "ready" : "missing", schemaOK ? "ready" : "missing", Assets() ? "ready" : "missing", paused ? "yes" : "no",
		samples, renders);
	if (dependencyReloadRequired)
	{
		Msg("[ShowPos] Reload required: CS2KZ was unloaded.\n");
	}
	Msg("[ShowPos] Snapshot slots=%d, owned HUDs=%d, owned menus=%d\n", fresh, huds, menus);
}
