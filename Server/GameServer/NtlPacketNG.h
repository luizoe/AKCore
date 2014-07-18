//***********************************************************************************
//
//	File		:	NtlPacketNG.h
//
//	Begin		:	2006-05-12
//
//	Copyright	:	�� NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#pragma once

#include "NtlPacketCommon.h"
#include "Character.h"
#include "ActionPattern.h"
#include "NtlMovement.h"

#include "NtlPacketUG.h"


//------------------------------------------------------------------
//	Protocl enumeration
//------------------------------------------------------------------
enum eOPCODE_NG
{
	NG_OPCODE_BEGIN = 37000,
	NG_HEARTBEAT = NG_OPCODE_BEGIN,

	NG_GAME_DATA_REQ, // ���� ���� ����Ÿ ���� ��û
	NG_ENTER_GAME_REQ, // ���� ���� ��û
	NG_LEAVE_GAME_REQ, // ���� ���� ��û
	NG_ENTER_WORLD_REQ, // ������Ʈ ���� ���� ��û
	NG_LEAVE_WORLD_REQ, // ������Ʈ ���� ���� ��û
	NG_SPS_SERVER_EVENT,

	//NG_CHAR_SPAWN_RES, // ĳ���� ���� ����
	NG_CHAR_READY, // ĳ���� �غ� �Ϸ�
	NG_CHAR_MOVE, // ĳ���� �̵�
	NG_CHAR_DESTMOVE, // ĳ���� ������ �̵�
	NG_CHAR_FOLLOWMOVE, // ĳ���� ��ǥ �̵�
	NG_CHAR_CHANGE_HEADING, // ĳ���� ���� ȸ��

	NG_CHAR_TARGET_SELECT, // Ÿ�� ����
	NG_CHAR_TOGG_FIGHT, // ���� ���
	NG_CHAR_TOGG_RUN, // ���� ���
	NG_CHAR_TOGG_ATTACK, // ���� ����
	NG_CHAR_TOGG_SITDOWN,	// �ʱ�

	NG_CHAR_DIALOG, // ĳ���� ��ȭ
	NG_CHAR_DIRECT_PLAY, // ĳ���� ����
	NG_CHAR_DIRECT_TURNING,	// Ż�� turing
	NG_CHAR_SET_CONDITION, // ĳ���� ����� ����
	NG_CHAR_BATTLE_MODE_NFY,	// character�� battle�� ���۰� ���Ḧ �˸���.

	NG_CHAR_START_ESCORT_RES,	
	NG_CHAR_INITIALIZE, // ĳ���͸� �ʱ�ȭ�Ѵ� [12/13/2007 SGpro]
	NG_CHAR_SETSPEEDPERCENT, //ĳ���Ϳ� �ӵ��� �ۼ�Ʈ ��ŭ �����Ѵ� [12/13/2007 SGpro]

	NG_BOT_SKILL_REQ, // ��ų ��� [2/4/2008 SGpro]
	NG_SKILL_TARGET_LIST,		// ��ų Ÿ�� ��� �˸�

	NG_BOT_BOTCAUTION_NFY,// Bot ��� [2/20/2008 SGpro]

	NG_BOT_BOTCAUTION_HELPME_NFY, // Bot Help Me ��� [5/27/2008 SGpro]

	NG_CHAR_DESPAWN,// Bot ���� ��û [7/28/2008 SGpro]
	NG_CHAR_TELEPORT_REQ,// Bot �ڷ���Ʈ ��û [7/28/2008 SGpro]
	NG_CHAR_READY_TO_SPAWN,// [8/4/2008 SGpro]

	NG_CHAR_CHANGE_NICKNAME,// [11/6/2008 SGpro]

	NG_CHAR_MOB_ROLE_CHANGED_NFY,// [11/10/2008 SGpro]

	NG_CHAR_NONBATTLEACTIONPATTERNSET_UNITIDX_NFY,

	NG_SEND_SPS_EVENT,
	NG_SEND_AIS_EVENT,

	NG_CHANGE_SPS_SCENE_RES,

	NG_CHAR_LOC_AFTER_KNOCKDOWN_NFY,
	NG_CHAR_LOC_AFTER_SLIDING_NFY,
	NG_CHAR_LOC_AFTER_PUSH_NFY,

	NG_OPCODE_DUMMY,
	NG_OPCODE_END = NG_OPCODE_DUMMY - 1
};


//------------------------------------------------------------------
//
//------------------------------------------------------------------
const char * NtlGetPacketName_NG(WORD wOpCode);
//------------------------------------------------------------------


//------------------------------------------------------------------
//	Define protocol structure
//------------------------------------------------------------------
#pragma pack(1)

//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_GAME_DATA_REQ )
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_ENTER_GAME_REQ )
	BYTE				objType; // eOBJTYPE
	union
	{
		sNPC_DATA			sNpcData; // npc
		sMOB_DATA			sMobData; // mob
		sPET_DATA			sPetData;
	};
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_LEAVE_GAME_REQ )
	HOBJECT				handle;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_ENTER_WORLD_REQ )
	HOBJECT				handle;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_LEAVE_WORLD_REQ )
	HOBJECT				handle;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_SPS_SERVER_EVENT )
	HOBJECT			hSubject;				// ���� ĳ���� �ڵ�
	BYTE			byEventType;			// eSPS_EVENT_TYPE
	BYTE			byTriggerType;			// Trigger Type
	DWORD			eventID;				// event id	
END_PROTOCOL()
//------------------------------------------------------------------
//BEGIN_PROTOCOL( NG_CHAR_SPAWN_RES )
//END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_READY )
	HOBJECT				handle;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_MOVE )
	HOBJECT				handle;
	sVECTOR3			vCurLoc;
	sVECTOR3			vCurDir;
	BYTE				byMoveDirection;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_DESTMOVE )
	HOBJECT				handle;
	bool				bHaveSecondDestLoc;
	sVECTOR3			vSecondDestLoc;
	BYTE				byDestLocCount;
	sVECTOR3			avDestLoc[DBO_MAX_NEXT_DEST_LOC_COUNT];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_FOLLOWMOVE )
	HOBJECT				handle;
	HOBJECT				hTarget;
	float				fDistance;
	BYTE				byMovementReason;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_CHANGE_HEADING )
	HOBJECT				handle;
	sVECTOR3			vCurLoc;
	sVECTOR3			vCurDir;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_TARGET_SELECT )
	HOBJECT				handle;
	HOBJECT				hTarget;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_TOGG_FIGHT )
	HOBJECT				handle;
	bool				bFightMode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_TOGG_RUN )
	HOBJECT				handle;
	bool				bRunFlag;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_TOGG_ATTACK )
	HOBJECT				handle;
	bool				bAttackProgress;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_TOGG_SITDOWN )
	HOBJECT				handle;
	bool				bSitDown;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_DIALOG )
	HOBJECT			hSubject;				// ��ȭ �ϴ� ĳ���� �ڵ�
	BYTE			byDialogType;			// ��ȭ ���� ( eCHAR_DIALOG_TYPE )
	TBLIDX			textTblidx;				// ��ȭ ���� ���̺� ��ȣ ( INVALID �� �ƴϸ� �ش� �ؽ�Ʈ�� ã�� ����ϰ�, INVALID�� �Ʒ��� �ؽ�Ʈ�� ����� �� ��)
	bool			bIsRefSpeechTable;		// ����ġ ���̺��� ������ ������
	WORD			wTextLen;				// ��ȭ������ ����
	WCHAR			awchText[NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1]; // ��ȭ����
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_DIRECT_PLAY )
	HOBJECT			hSubject;				// ��ȭ �ϴ� ĳ���� �ڵ�
	TBLIDX			directionTblidx;		// ���� ���̺� �ε���
	BYTE			byDirectPlayType;		// ���� Ÿ��
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_DIRECT_TURNING )
	HOBJECT			hSubject;				// Ż�� handle
	TBLIDX			directionTblidx;		// ���� ���̺� �ε���
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_SET_CONDITION )
	HOBJECT			hSubject;					// ��ȭ �ϴ� ĳ���� �ڵ�
	DWORD			dwAddCharConditionFlag;		// �߰��� ĳ���� �����
	DWORD			dwRemoveCharConditionFlag;	// ������ ĳ���� �����	
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_BATTLE_MODE_NFY )
	HOBJECT			hSubject;
	bool			bIsBattle;					// �������ΰ�?
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_START_ESCORT_RES )
	HOBJECT			hSubject;				// ������Ʈ ��ü ĳ���� ( PC )
	HOBJECT			hSource;				// ������Ʈ ��� ĳ���� ( NPC )
	WORD			wResultCode;			// ���� �ڵ�
	BYTE			byTriggerType;			// Ʈ���� Ÿ��
	NTL_TS_T_ID		tid;					// �����ų trigger id
	sTSM_SERIAL		sTSMSerial;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_INITIALIZE )		// [12/13/2007 SGpro]
	HOBJECT			hSubject;				// �ʱ�ȭ�ϴ� ĳ���� �ڵ�
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_SETSPEEDPERCENT )	// [12/13/2007 SGpro]
	HOBJECT			hSubject;				// �ʱ�ȭ�ϴ� ĳ���� �ڵ�
	float			fPercentRunSpeed;		// �޸��� �ӵ� (��з�)
	float			fPercentWalkSpeed;		// �ȴ� �ӵ� (��з�)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_BOT_SKILL_REQ )			// [2/4/2008 SGpro]
	HOBJECT				handle;
	BYTE				byAvatarType;		// eDBO_AVATAR_TYPE
	TBLIDX				tblidxSkill;
	HOBJECT				hTarget;
	// If skill effect should be applied on a target character, too, the target character must be included in both byApplyTargetCount and ahApplyTarget.
	// ��ų�� Ÿ�� ĳ���Ͱ� ��ų ���� ����� ��쿡�� Ÿ�� ĳ���͵� byApplyTargetCount�� ahApplyTarget�� ���ԵǾ�� �Ѵ�.
	// by YOSHIKI(2007-01-12)
	BYTE				byApplyTargetCount;
	HOBJECT				ahApplyTarget[NTL_MAX_NUMBER_OF_SKILL_TARGET];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_SKILL_TARGET_LIST )
	HOBJECT				hSubject;
	// If skill effect should be applied on a target character, too, the target character must be included in both byApplyTargetCount and ahApplyTarget.
	// ��ų�� Ÿ�� ĳ���Ͱ� ��ų ���� ����� ��쿡�� Ÿ�� ĳ���͵� byApplyTargetCount�� ahApplyTarget�� ���ԵǾ�� �Ѵ�.
	// by YOSHIKI(2007-02-22)
	BYTE				byApplyTargetCount;
	HOBJECT				ahApplyTarget[NTL_MAX_NUMBER_OF_SKILL_TARGET];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_BOT_BOTCAUTION_NFY )		//  [2/20/2008 SGpro]
	HOBJECT				handle;
	HOBJECT				hTarget;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_BOT_BOTCAUTION_HELPME_NFY ) // [5/27/2008 SGpro]
	HOBJECT				handle;
	HOBJECT				hTarget;
	bool				bIsRequester; // true : SOS�� ��û�� ��
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_DESPAWN )// Bot ���� ��û [7/28/2008 SGpro]
	HOBJECT			handle;
	BYTE			byTeleportType; // �ڷ���ƮŸ��
	WORLDID			worldId;
	TBLIDX			worldTblidx;
	CNtlVector		vTeleportLoc;
	CNtlVector		vTeleportDir;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_TELEPORT_REQ )// Bot �ڷ���Ʈ ��û [7/28/2008 SGpro]
	HOBJECT			handle;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_READY_TO_SPAWN )// [8/4/2008 SGpro]
	HOBJECT			handle;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_CHANGE_NICKNAME )// [11/6/2008 SGpro]
	HOBJECT			handle;
	TBLIDX			tblidxNickName;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_MOB_ROLE_CHANGED_NFY )// [11/10/2008 SGpro]
	HOBJECT			handle;
	BYTE			byBotRoleType;		// eBOTROLETYPE
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHAR_NONBATTLEACTIONPATTERNSET_UNITIDX_NFY )
	HOBJECT				handle;
	ACTIONPATTERN_FIELD unitIdx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_SEND_SPS_EVENT )
	BYTE				byEventType;	// eSSD_SCRIPT_TYPE : SSD_SCRIPT_TYPE_TS | SSD_SCRIPT_TYPE_TQS | SSD_SCRIPT_TYPE_WPS �� ���
	HOBJECT				hSender;
	BYTE				bySenderType;	// eOBJTYPE : OBJTYPE_NPC | OBJTYPE_MOB
	TBLIDX				senderTblidx;
	// escort event
	HOBJECT				hPc;
	NTL_TS_EVENT_ID		teId;
	// tqs, wps event
	WORLDID				worldId;
	TBLIDX				wpsTblidx;
	DWORD				eventId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_SEND_AIS_EVENT )
	BYTE				byEventType;	// eSSD_SCRIPT_TYPE : SSD_SCRIPT_TYPE_TQS | SSD_SCRIPT_TYPE_WPS �� ���
	HOBJECT				hSender;
	BYTE				bySenderType;	// eOBJTYPE : OBJTYPE_NPC | OBJTYPE_MOB
	TBLIDX				senderTblidx;
	WORLDID				worldId;
	TBLIDX				wpsTblidx;		// only wps
	DWORD				eventId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( NG_CHANGE_SPS_SCENE_RES )
	WORD				wResultCode;

	HOBJECT				hTarget;
	TBLIDX				targetTblidx;
	DWORD				dwPlayScript;
	DWORD				dwPlayScene;
	WORLDID				worldId;
	TBLIDX				wpsTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(NG_CHAR_LOC_AFTER_KNOCKDOWN_NFY)
	HOBJECT				hSubject;
	sVECTOR3			vCurLoc;
	sVECTOR2			vCurDir;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(NG_CHAR_LOC_AFTER_SLIDING_NFY)
	HOBJECT				hSubject;
	sVECTOR3			vCurLoc;
	sVECTOR2			vCurDir;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(NG_CHAR_LOC_AFTER_PUSH_NFY)
	HOBJECT				hSubject;
	sVECTOR3			vCurLoc;
	sVECTOR2			vCurDir;
END_PROTOCOL()
//------------------------------------------------------------------

#pragma pack()
