// Adapted from CS2KZ v0.0.173, AGPL-3.0. Declarations only; no KZPlayer internals.
#pragma once
#include "common.h"
#include "ehandle.h"
#include "gametrace.h"
class CCSPlayerPawn;
struct touchlist_t
{
	Vector deltavelocity;
	trace_t trace;
};

struct SubtickMove
{
	float when;
	uint64 button;

	union
	{
		bool pressed;

		struct
		{
			float analog_forward_delta;
			float analog_left_delta;
		} analogMove;
	};

	float pitch;
	float yaw;

	bool IsAnalogInput() const
	{
		return button == 0;
	}
};

class CMoveDataBase
{
public:
	CMoveDataBase() = default;
	CMoveDataBase(const CMoveDataBase &source)
		// clang-format off
		: m_bHasZeroFrametime {source.m_bHasZeroFrametime},
		m_bIsLateCommand {source.m_bIsLateCommand}, 
		m_nPlayerHandle {source.m_nPlayerHandle},
		m_vecAbsViewAngles {source.m_vecAbsViewAngles},
		m_vecViewAngles {source.m_vecViewAngles},
		m_vecLastMovementImpulses {source.m_vecLastMovementImpulses},
		m_flForwardMove {source.m_flForwardMove}, 
		m_flSideMove {source.m_flSideMove}, 
		m_flUpMove {source.m_flUpMove},
		m_vecVelocity {source.m_vecVelocity}, 
		m_vecAngles {source.m_vecAngles},
		m_vecUnknown {source.m_vecUnknown},
		m_bHasSubtickInputs {source.m_bHasSubtickInputs},
		unknown {source.unknown},
		m_collisionNormal {source.m_collisionNormal},
		m_groundNormal {source.m_groundNormal},
		m_vecAbsOrigin {source.m_vecAbsOrigin},
		m_nTickCount {source.m_nTickCount},
		m_nTargetTick {source.m_nTargetTick},
		m_flSubtickStartFraction {source.m_flSubtickStartFraction},
		m_flSubtickEndFraction {source.m_flSubtickEndFraction}
	// clang-format on
	{
		for (int i = 0; i < source.m_AttackSubtickMoves.Count(); i++)
		{
			this->m_AttackSubtickMoves.AddToTail(source.m_AttackSubtickMoves[i]);
		}
		for (int i = 0; i < source.m_SubtickMoves.Count(); i++)
		{
			this->m_SubtickMoves.AddToTail(source.m_SubtickMoves[i]);
		}
		for (int i = 0; i < source.m_TouchList.Count(); i++)
		{
			auto touch = this->m_TouchList.AddToTailGetPtr();
			touch->deltavelocity = m_TouchList[i].deltavelocity;
			touch->trace.m_pSurfaceProperties = m_TouchList[i].trace.m_pSurfaceProperties;
			touch->trace.m_pEnt = m_TouchList[i].trace.m_pEnt;
			touch->trace.m_pHitbox = m_TouchList[i].trace.m_pHitbox;
			touch->trace.m_hBody = m_TouchList[i].trace.m_hBody;
			touch->trace.m_hShape = m_TouchList[i].trace.m_hShape;
			touch->trace.m_nContents = m_TouchList[i].trace.m_nContents;
			touch->trace.m_BodyTransform = m_TouchList[i].trace.m_BodyTransform;
			touch->trace.m_vHitNormal = m_TouchList[i].trace.m_vHitNormal;
			touch->trace.m_vHitPoint = m_TouchList[i].trace.m_vHitPoint;
			touch->trace.m_flHitOffset = m_TouchList[i].trace.m_flHitOffset;
			touch->trace.m_flFraction = m_TouchList[i].trace.m_flFraction;
			touch->trace.m_nTriangle = m_TouchList[i].trace.m_nTriangle;
			touch->trace.m_nHitboxBoneIndex = m_TouchList[i].trace.m_nHitboxBoneIndex;
			touch->trace.m_eRayType = m_TouchList[i].trace.m_eRayType;
			touch->trace.m_bStartInSolid = m_TouchList[i].trace.m_bStartInSolid;
			touch->trace.m_bExactHitPoint = m_TouchList[i].trace.m_bExactHitPoint;
		}
	}

public:
	bool m_bHasZeroFrametime: 1;
	bool m_bIsLateCommand: 1;
	CHandle<CCSPlayerPawn> m_nPlayerHandle;
	QAngle m_vecAbsViewAngles;
	QAngle m_vecViewAngles;
	Vector m_vecLastMovementImpulses;
	float m_flForwardMove;
	float m_flSideMove; // Warning! Flipped compared to CS:GO, moving right gives negative value
	float m_flUpMove;
	Vector m_vecVelocity;
	QAngle m_vecAngles;
	Vector m_vecUnknown; // Unused. Probably pulled from engine upstream.
	CUtlVector<SubtickMove> m_SubtickMoves;
	CUtlVector<SubtickMove> m_AttackSubtickMoves;
	bool m_bHasSubtickInputs;
	float unknown; // Set to 1.0 during SetupMove, never change during gameplay. Is apparently used for weapon services stuff.
	CUtlVector<touchlist_t> m_TouchList;
	Vector m_collisionNormal;
	Vector m_groundNormal;
	Vector m_vecAbsOrigin;
	int32_t m_nTickCount;
	int32_t m_nTargetTick;
	float m_flSubtickStartFraction;
	float m_flSubtickEndFraction;
};

class CMoveData : public CMoveDataBase
{
public:
	CMoveData() = default;

	CMoveData(const CMoveData &source)
		: CMoveDataBase(source), m_outWishVel {source.m_outWishVel}, m_vecOldAngles {source.m_vecOldAngles},
		  m_vecWalkWishVel {source.m_vecWalkWishVel}, m_vecContinousAcceleration {source.m_vecContinousAcceleration},
		  m_vecFrameVelocityDelta {source.m_vecFrameVelocityDelta}, m_flMaxSpeed {source.m_flMaxSpeed}
	{
	}

	Vector m_outWishVel;
	QAngle m_vecOldAngles;
	Vector2D m_vecWalkWishVel;
	// u/s^2.
	Vector m_vecContinousAcceleration;
	// Immediate delta in u/s. Air acceleration bypasses per second acceleration, applies up to half of its impulse to the velocity and the rest goes
	// straight into this.
	Vector m_vecFrameVelocityDelta;
	float m_flMaxSpeed;
	float m_flClientMaxSpeed;
	float m_flFrictionDecel;
	// 2026-01-21 update adds these fields to calculate exactly when during the tick the player hit the ground using physics equations
	// rather than just assuming they landed at the end of the tick, somewhat similar to how CS2KZ landingTimeActual formula works.
	float m_flPreAirMovePosZ;
	float m_flPreAirMoveVelZ;
	float m_flPreAirMoveAccelZ;
	bool m_bInAir;
	bool m_bGameCodeMovedPlayer; // true if usercmd cmd number == (m_nGameCodeHasMovedPlayerAfterCommand + 1)
};

#ifdef _WIN32
static_assert(sizeof(CMoveData) == 320, "Class didn't match expected size");
#endif

