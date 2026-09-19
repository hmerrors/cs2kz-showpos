#pragma once
#include "common.h"
#include "schemasystem/schemasystem.h"
#include "filesystem.h"
#include "movedata.h"
#include "types.h"
#include <string>

struct Field
{
	int offset {-1};
	SchemaCollectionManipulatorFn_t manip {};

	template<class T>
	T *At(void *p) const
	{
		return p && offset >= 0 ? reinterpret_cast<T *>(static_cast<char *>(p) + offset) : nullptr;
	}

	template<class T>
	T Read(void *p, T fallback = {}) const
	{
		auto v = At<T>(p);
		return v ? *v : fallback;
	}

	int Count(void *p) const;
	void *Element(void *p, int index) const;
	void *Append(void *p) const;
};

namespace engine
{
	extern IVEngineServer2 *server;
	extern ISource2Server *game;
	extern ISource2GameClients *clients;
	extern ISource2GameEntities *entities;
	extern IFileSystem *files;
	extern CSchemaSystem *schemas;
	extern void *resource;
	extern void *movementAddress;
	extern int transmitSlot, entitySystemOffset, teleportIndex, stateNetworkIndex;
	extern std::string gameDir;
	extern CEntityInstance *(*createEntity)(const char *, int);
	extern void (*spawnEntity)(CEntityInstance *, CEntityKeyValues *);
	extern void (*removeEntity)(CEntityInstance *);
	extern Field life, team, pawn, currentPawn, controller, moveServices, observers, observerMode, observerTarget, stamina, duckAmount, duckSpeed,
		hltv;
	extern Field panels, classes, variables, globalState, playerStates, hasClasses, dialogStrings, capture, stateSlot;
	extern Field classPanel, className, classStatus, dialogPanel, dialogName, dialogValue, dialogSet;
	bool Open(ISmmAPI *ismm, char *error, size_t maxlen);
	bool SchemaReady();
	CEntityInstance *Controller(int slot);
	CEntityInstance *Pawn(int slot, bool current = false);
	bool Alive(CEntityInstance *p);
	double Now();
	int Slot(CEntityInstance *p);
	void *Resolve(const char *signature, char *error, size_t maxlen);
} // namespace engine
