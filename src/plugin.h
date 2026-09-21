#pragma once
#include "engine.h"
#include "ics2kz.h"
#include "imultiaddonmanager.h"
#include <array>

struct Player
{
	ShowPosPrefs prefs;
	ShowPosSnapshot sample;
	CEntityHandle samplePawn, hud, menu;
	uint64 steam {};
	double sampleTime {-1}, nextMenu {}, menuExpires {}, saveAt {};
	int sampleTeam {};
	bool valid {}, menuOpen {}, dirty {}, prefsLoaded {};
	char lastText[1024] {};
	int lastX {-999}, lastY {-999}, lastSize {-999};
	bool hudVisible {};
};

class ShowPosPlugin final : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) override;
	bool Unload(char *error, size_t maxlen) override;
	void AllPluginsLoaded() override;
	void OnPluginLoad(PluginId id) override;
	void OnPluginUnload(PluginId id) override;
	void OnPluginPause(PluginId id) override;
	void OnPluginUnpause(PluginId id) override;
	void OnLevelShutdown() override;
	void OnLevelInit(const char *, const char *, const char *, const char *, bool, bool) override;
	bool QueryRunning(char *error, size_t maxlen) override;
	bool Pause(char *error, size_t maxlen) override;
	bool Unpause(char *error, size_t maxlen) override;

	const char *GetAuthor() override;

	const char *GetName() override
	{
		return "CS2KZ ShowPos (standalone)";
	}

	const char *GetDescription() override
	{
		return "Read-only pre-movement ShowPos, Metamod API 18";
	}

	const char *GetURL() override
	{
		return "";
	}

	const char *GetLicense() override
	{
		return "AGPL-3.0";
	}

	const char *GetVersion() override
	{
		return "1.0.0-rc2";
	}

	const char *GetDate() override
	{
		return __DATE__;
	}

	const char *GetLogTag() override
	{
		return "ShowPos";
	}

	void Dependencies();
	void Frame();
	void Capture(void *ms, CMoveData *move);
	void Reset(int slot, bool remove = true);
	void Command(int slot, const CCommand &args);
	void Click(int slot, uint32 handle, const char *button);
	void Status();
	void Invalidate(CEntityInstance *p);

	bool Ready() const
	{
		return initialized && !paused && kz && mam && !dependencyReloadRequired;
	}

	bool Assets() const;
	std::array<Player, 64> players;
	ICS2KZ *kz {};
	IMultiAddonManager *mam {};
	PluginId kzId {}, mamId {};
	bool initialized {}, paused {}, schemaOK {}, dependencyReloadRequired {};
	bool chinese {true};
	double nextFrame {};
	uint64 samples {}, renders {};
};

extern ShowPosPlugin plugin;

namespace hud
{
	bool OpenMenu(int slot);
	void CloseMenu(int slot);
	void Draw(int slot, const ShowPosSnapshot *sample);
	void RenderMenu(int slot);
	void Destroy(int slot);
} // namespace hud

namespace prefs
{
	void Load(Player &p, uint64 steam);
	bool Save(Player &p);
	void Cycle(ShowPosPrefs &p, int row);
	void Normalize(ShowPosPrefs &p);
} // namespace prefs

namespace hooks
{
	bool Init();
	void Cleanup();
	void Pawn(CEntityInstance *p);
} // namespace hooks
