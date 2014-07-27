#include "stdafx.h"
#include "NtlTokenizer.h"

#include "PacketGameServer.h"

#include "NtlPacketTU.h"
#include "NtlPacketUT.h"
#include "GameServer.h"

typedef std::list<SBattleData*> ListAttackBegin;
typedef ListAttackBegin::iterator BATTLEIT;
ListAttackBegin				m_listAttackBegin;
SSkillData *pSkillData;

//--------------------------------------------------------------------------------------//
//		Log into Game Server
//--------------------------------------------------------------------------------------//
void CClientSession::SendGameEnterReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_GAME_ENTER_REQ * req = (sUG_GAME_ENTER_REQ *)pPacket->GetPacketData();

	this->plr = new PlayerInfos();
	this->plr->pcProfile->charId = req->charId;
	this->plr->SetAccountID(req->accountId);
	this->plr->setMyAPP(app);
	avatarHandle = AcquireSerialId();

	this->plr->StoreHandle(avatarHandle);
	this->plr->StoreSession(this->GetHandle());
	CNtlPacket packet(sizeof(sGU_GAME_ENTER_RES));

	app->db->prepare("UPDATE characters SET IsOnline = 1 WHERE CharID = ?");
	app->db->setInt(1,  this->plr->pcProfile->charId);
	app->db->execute();

	sGU_GAME_ENTER_RES * res = (sGU_GAME_ENTER_RES *)packet.GetPacketData();

	res->wOpCode = GU_GAME_ENTER_RES;
	res->wResultCode = GAME_SUCCESS;
	strcpy_s(res->achCommunityServerIP, sizeof(res->achCommunityServerIP), IP_SERVER_ALL);
	res->wCommunityServerPort = 20400;

	packet.SetPacketLen( sizeof(sGU_GAME_ENTER_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		Send avatar char info
//--------------------------------------------------------------------------------------//
void CClientSession::CheckPlayerStat(CGameServer * app, sPC_TBLDAT *pTblData, int level)
{
	app->db->prepare("UPDATE characters SET BaseStr = ?, BaseCon = ?, BaseFoc = ?, BaseDex = ?,BaseSol = ?, BaseEng = ? WHERE CharID = ?");
	app->db->setInt(1, pTblData->byStr + (pTblData->fLevel_Up_Str * level));
	app->db->setInt(2, pTblData->byCon + (pTblData->fLevel_Up_Con * level));
	app->db->setInt(3, pTblData->byFoc + (pTblData->fLevel_Up_Foc * level));
	app->db->setInt(4, pTblData->byDex + (pTblData->fLevel_Up_Dex * level));
	app->db->setInt(5, pTblData->bySol + (pTblData->fLevel_Up_Sol * level));
	app->db->setInt(6, pTblData->byEng + (pTblData->fLevel_Up_Eng * level));
	app->db->setInt(7,  this->plr->pcProfile->charId);
	app->db->execute();

	app->db->prepare("UPDATE characters SET BaseAttackRate = ?, BaseAttackSpeedRate = ?, BaseEnergyDefence = ?, BaseEnergyOffence = ?,BasePhysicalDefence = ?, BasePhysicalOffence = ? WHERE CharID = ?");
	app->db->setInt(1, pTblData->wAttack_Rate);
	app->db->setInt(2, pTblData->wAttack_Speed_Rate);
	app->db->setInt(3, pTblData->wBasic_Energy_Defence + (pTblData->byLevel_Up_Energy_Defence * level));
	app->db->setInt(4, pTblData->wBasic_Energy_Offence + (pTblData->byLevel_Up_Energy_Offence * level));
	app->db->setInt(5, pTblData->wBasic_Physical_Defence + (pTblData->byLevel_Up_Physical_Defence * level));
	app->db->setInt(6, pTblData->wBasic_Physical_Offence + (pTblData->byLevel_Up_Physical_Offence * level));
	app->db->setInt(7,  this->plr->pcProfile->charId);
	app->db->execute();

	app->db->prepare("UPDATE characters SET BaseMaxLP = ?, BaseMaxEP = ?, BaseMaxRP = ?, BaseDodgeRate = ?, BaseAttackRate = ?, BaseBlockRate = ?, BasePhysicalCriticalRate = ?, BaseEnergyCriticalRate = ? WHERE CharID = ?");
	app->db->setInt(1, (pTblData->wBasic_LP) + (pTblData->byLevel_Up_LP * level));
	app->db->setInt(2, (pTblData->wBasic_EP) + (pTblData->byLevel_Up_EP * level));
	app->db->setInt(3, pTblData->wBasic_RP + (pTblData->byLevel_Up_RP * level));
	app->db->setInt(4, pTblData->wDodge_Rate);
	app->db->setInt(5, pTblData->wAttack_Rate);
	app->db->setInt(6, pTblData->wBlock_Rate);
	app->db->setInt(7, 10);
	app->db->setInt(8, 10);
	app->db->setInt(9,  this->plr->pcProfile->charId);
	app->db->execute();

	this->plr->SetLevelup(pTblData);
}
void CClientSession::SendAvatarCharInfo(CNtlPacket * pPacket, CGameServer * app)
{
	CNtlPacket packet(sizeof(sGU_AVATAR_CHAR_INFO));
	sGU_AVATAR_CHAR_INFO * res = (sGU_AVATAR_CHAR_INFO *)packet.GetPacketData();

	app->db->prepare("UPDATE characters SET OnlineID = ? WHERE CharID = ?");
	app->db->setInt(1, this->plr->GetAvatarandle());
	app->db->setInt(2,  this->plr->pcProfile->charId);
	app->db->execute();

	app->db->prepare("SELECT * FROM characters WHERE CharID = ?");
	app->db->setInt(1,  this->plr->pcProfile->charId);
	app->db->execute();
	app->db->fetch();

	CPCTable *pPcTable = app->g_pTableContainer->GetPcTable();
	sPC_TBLDAT *pTblData = (sPC_TBLDAT*)pPcTable->GetPcTbldat(app->db->getInt("Race"),app->db->getInt("Class"),app->db->getInt("Gender"));
	CClientSession::CheckPlayerStat(app, pTblData, app->db->getInt("Level") - 1);
	
	app->db->prepare("SELECT * FROM characters WHERE CharID = ?");
	app->db->setInt(1,  this->plr->pcProfile->charId);
	app->db->execute();
	app->db->fetch();

	res->wOpCode = GU_AVATAR_CHAR_INFO;
	res->handle = this->plr->GetAvatarandle();
	res->sPcProfile.tblidx = pTblData->tblidx;
	res->sPcProfile.bIsAdult = app->db->getBoolean("Adult");
	res->sPcProfile.charId = app->db->getInt("CharID");
	wcscpy_s(res->sPcProfile.awchName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, s2ws(app->db->getString("CharName")).c_str() );
	//PC Shape
	res->sPcProfile.sPcShape.byFace = app->db->getInt("Face");
	res->sPcProfile.sPcShape.byHair = app->db->getInt("Hair");
	res->sPcProfile.sPcShape.byHairColor = app->db->getInt("HairColor");
	res->sPcProfile.sPcShape.bySkinColor = app->db->getInt("SkinColor");
	//Character Attribute
	res->sPcProfile.avatarAttribute.wBaseMaxEP = app->db->getInt("BaseMaxEP");
	res->sPcProfile.avatarAttribute.wBaseMaxLP = app->db->getInt("BaseMaxLP");
	res->sPcProfile.avatarAttribute.wBaseMaxRP = app->db->getInt("BaseMaxRP");
	res->sPcProfile.avatarAttribute.byBaseStr = app->db->getInt("BaseStr");
	res->sPcProfile.avatarAttribute.byBaseCon = app->db->getInt("BaseCon");
	res->sPcProfile.avatarAttribute.byBaseFoc = app->db->getInt("BaseFoc");
	res->sPcProfile.avatarAttribute.byBaseDex = app->db->getInt("BaseDex");
	res->sPcProfile.avatarAttribute.byBaseSol = app->db->getInt("BaseSol");
	res->sPcProfile.avatarAttribute.byBaseEng = app->db->getInt("BaseEng");
	res->sPcProfile.avatarAttribute.wBaseMaxLP = 1000;//app->db->getInt("BaseMaxLP");
	res->sPcProfile.avatarAttribute.wBaseMaxEP = 1000;//app->db->getInt("BaseMaxEP");
	res->sPcProfile.avatarAttribute.wBaseMaxRP = 1000;//app->db->getInt("BaseMaxRP");
	res->sPcProfile.avatarAttribute.wBasePhysicalOffence = app->db->getInt("BasePhysicalOffence");
	res->sPcProfile.avatarAttribute.wBasePhysicalDefence = app->db->getInt("BasePhysicalDefence");
	res->sPcProfile.avatarAttribute.wBaseEnergyOffence = app->db->getInt("BaseEnergyOffence");
	res->sPcProfile.avatarAttribute.wBaseEnergyDefence = app->db->getInt("BaseEnergyDefence");
	res->sPcProfile.avatarAttribute.wBaseAttackRate = app->db->getInt("BaseAttackRate");
	res->sPcProfile.avatarAttribute.wBaseDodgeRate = app->db->getInt("BaseDodgeRate");
	res->sPcProfile.avatarAttribute.wBaseBlockRate = app->db->getInt("BaseBlockRate");
	res->sPcProfile.avatarAttribute.wBasePhysicalCriticalRate = app->db->getInt("BasePhysicalCriticalRate");
	res->sPcProfile.avatarAttribute.wBaseEnergyCriticalRate = app->db->getInt("BaseEnergyCriticalRate");
	res->sPcProfile.byLevel = app->db->getInt("Level");
	res->sPcProfile.dwCurExp = app->db->getInt("Exp");
	res->sPcProfile.dwMaxExpInThisLevel = app->db->getInt("MaxExpInThisLevel");
	res->sPcProfile.dwZenny = app->db->getInt("Money");
	res->sPcProfile.dwTutorialHint = -1;
	//res->sPcProfile.byBindType = DBO_BIND_TYPE_INITIAL_LOCATION;	

	res->sPcProfile.dwReputation = app->db->getInt("Reputation");
	res->sPcProfile.dwMudosaPoint = app->db->getInt("MudosaPoint");
	res->sPcProfile.dwSpPoint = app->db->getInt("SpPoint");
	// Character State
	res->sCharState.sCharStateBase.vCurLoc.x = (float)app->db->getDouble("CurLocX");
	res->sCharState.sCharStateBase.vCurLoc.y = (float)app->db->getDouble("CurLocY");
	res->sCharState.sCharStateBase.vCurLoc.z = (float)app->db->getDouble("CurLocZ");
	res->sCharState.sCharStateBase.vCurDir.x = (float)app->db->getDouble("CurDirX");
	res->sCharState.sCharStateBase.vCurDir.y = (float)app->db->getDouble("CurDirY");
	res->sCharState.sCharStateBase.vCurDir.z = (float)app->db->getDouble("CurDirZ");
	res->sCharState.sCharStateBase.dwConditionFlag = 0;
	res->sCharState.sCharStateBase.byStateID = 0;
	res->sCharState.sCharStateBase.aspectState.sAspectStateBase.byAspectStateId = 255;
	res->sCharState.sCharStateBase.aspectState.sAspectStateDetail.sGreatNamek.bySourceGrade = 0;
	res->sCharState.sCharStateBase.aspectState.sAspectStateDetail.sKaioken.bySourceGrade = 0;
	res->sCharState.sCharStateBase.aspectState.sAspectStateDetail.sPureMajin.bySourceGrade = 0;
	res->sCharState.sCharStateBase.aspectState.sAspectStateDetail.sSuperSaiyan.bySourceGrade = 0;
	res->sCharState.sCharStateBase.aspectState.sAspectStateDetail.sVehicle.idVehicleTblidx = 0;
	res->wCharStateSize = sizeof(sCHARSTATE_BASE);
	res->sPcProfile.bIsGameMaster = app->db->getBoolean("GameMaster");
	
	this->plr->SetPlayerName(app->db->getString("CharName"));
	this->plr->SetPosition(res->sCharState.sCharStateBase.vCurLoc, res->sCharState.sCharStateBase.vCurDir);
	this->plr->setPlayerStat(&res->sPcProfile, &res->sCharState);
	this->plr->calculeMyStat(app);

	res->sPcProfile.avatarAttribute.byLastCon = app->db->getInt("LastCon");
	res->sPcProfile.avatarAttribute.byLastStr = app->db->getInt("LastStr");
	res->sPcProfile.avatarAttribute.byLastFoc = app->db->getInt("LastFoc");
	res->sPcProfile.avatarAttribute.byLastDex = app->db->getInt("LastDex");
	res->sPcProfile.avatarAttribute.byLastSol = app->db->getInt("LastSol");
	res->sPcProfile.avatarAttribute.byLastEng = app->db->getInt("LastEng");
	res->sPcProfile.avatarAttribute.wLastPhysicalOffence = app->db->getInt("LastPhysicalOffence");
	res->sPcProfile.avatarAttribute.wLastPhysicalDefence = app->db->getInt("LastPhysicalDefence");
	res->sPcProfile.avatarAttribute.wLastEnergyOffence = app->db->getInt("LastEnergyOffence");
	res->sPcProfile.avatarAttribute.wLastEnergyDefence = app->db->getInt("LastEnergyDefence");
	res->sPcProfile.avatarAttribute.wLastAttackRate = app->db->getInt("LastAttackRate");
	res->sPcProfile.avatarAttribute.wLastDodgeRate = app->db->getInt("LastDodgeRate");
	res->sPcProfile.avatarAttribute.wLastBlockRate = app->db->getInt("LastBlockRate");
	res->sPcProfile.avatarAttribute.wLastPhysicalCriticalRate = app->db->getInt("LastPhysicalCriticalRate");
	res->sPcProfile.avatarAttribute.wLastEnergyCriticalRate = app->db->getInt("LastEnergyCriticalRate");
	res->sPcProfile.avatarAttribute.fLastRunSpeed = (float)app->db->getDouble("LastRunSpeed");
	res->sPcProfile.avatarAttribute.wLastMaxLP = app->db->getInt("LastMaxLP");
	res->sPcProfile.avatarAttribute.wLastMaxRP = app->db->getInt("LastMaxRP");
	res->sPcProfile.avatarAttribute.wLastMaxEP = app->db->getInt("LastMaxEP");
	res->sPcProfile.wCurLP = app->db->getInt("CurLP");
	res->sPcProfile.wCurEP = app->db->getInt("CurEP");
	res->sPcProfile.wCurRP = app->db->getInt("CurRP");
	this->plr->setPlayerStat(&res->sPcProfile, &res->sCharState);
	packet.SetPacketLen( sizeof(sGU_AVATAR_CHAR_INFO) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );
	this->plr->SetStartRPBall();
	std::string log = "Player " + this->plr->GetPlayerName() + " have join the game\n";
	this->gsf->printDebug(log.c_str());
}

//--------------------------------------------------------------------------------------//
//		Send Avatar Iteminfo
//--------------------------------------------------------------------------------------//
void CClientSession::SendAvatarItemInfo(CNtlPacket * pPacket, CGameServer * app)
{
	size_t i = 0;
	app->db->prepare("SELECT * FROM items WHERE owner_id = ? ORDER BY place ASC");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->execute();

	CNtlPacket packet(sizeof(sGU_AVATAR_ITEM_INFO));
	sGU_AVATAR_ITEM_INFO * res = (sGU_AVATAR_ITEM_INFO *)packet.GetPacketData();

	res->wOpCode = GU_AVATAR_ITEM_INFO;
	res->byBeginCount = 0;
	res->byItemCount = app->db->rowsCount();

	while( app->db->fetch() )
	{
		res->aItemProfile[i].handle = app->db->getInt("id");
		res->aItemProfile[i].tblidx = app->db->getInt("tblidx");
		res->aItemProfile[i].byPlace = app->db->getInt("place");
		res->aItemProfile[i].byPos = app->db->getInt("pos");
		res->aItemProfile[i].byStackcount = app->db->getInt("count");
		res->aItemProfile[i].byRank = app->db->getInt("rank");
		res->aItemProfile[i].byCurDur = app->db->getInt("durability");

		packet.AdjustPacketLen(sizeof(sNTLPACKETHEADER)+(2 * sizeof(BYTE)) + (res->byItemCount * sizeof(sITEM_PROFILE)));
		g_pApp->Send( this->GetHandle() , &packet );

		i++;
	}
}
void CClientSession::SendSlotInfo(CNtlPacket * pPacket, CGameServer * app)
{
	app->db->prepare("SELECT * FROM quickslot WHERE charId = ?");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->execute();
 
	CNtlPacket packet(sizeof(sGU_QUICK_SLOT_INFO));
	sGU_QUICK_SLOT_INFO * res = (sGU_QUICK_SLOT_INFO *)packet.GetPacketData();

	res->wOpCode = GU_QUICK_SLOT_INFO;
	
	app->db->fetch();
	CSkillTable * pSkillTable = app->g_pTableContainer->GetSkillTable();
	CItemTable * pItemTable = app->g_pTableContainer->GetItemTable();

	int i = 0;
	int SkillOrItem = 0;
	int slotID = 0;
	while (i < 48)
	{
		std::string query = "slotId_" + std::to_string(i);
		printf("LOading slot %s\n", query.c_str());
		SkillOrItem = app->db->getInt(query.c_str());
		res->asQuickSlotData[slotID].bySlot = 255;
		res->asQuickSlotData[slotID].tblidx = 0;		
		sSKILL_TBLDAT* pSkillData = reinterpret_cast<sSKILL_TBLDAT*>(pSkillTable->FindData(SkillOrItem));
		sITEM_TBLDAT * pItemData = reinterpret_cast<sITEM_TBLDAT*>(pItemTable->FindData(SkillOrItem));
		if (pSkillData)
		{
			res->asQuickSlotData[slotID].bySlot = i;
			res->asQuickSlotData[slotID].tblidx = pSkillData->tblidx;
			res->asQuickSlotData[slotID].byType = (pSkillData->bySkill_Class==NTL_SKILL_CLASS_HTB?QUICK_SLOT_TYPE_HTB_SKILL:QUICK_SLOT_TYPE_SKILL);
			slotID++;
		}
		else if (pItemData)
		{
			res->asQuickSlotData[slotID].bySlot = i;
			res->asQuickSlotData[slotID].tblidx = pItemData->tblidx;
			res->asQuickSlotData[slotID].hItem	= pItemData->tblidx;//Need get correct hItem
			res->asQuickSlotData[slotID].byType = QUICK_SLOT_TYPE_ITEM;
			slotID++;
		}		
		i++;
	}	
	res->byQuickSlotCount = slotID;

	packet.AdjustPacketLen(sizeof(sNTLPACKETHEADER)+(2 * sizeof(BYTE)) + (slotID * (sizeof(sQUICK_SLOT_DATA))));
	g_pApp->Send(this->GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		Send Avatar Skillinfo
//--------------------------------------------------------------------------------------//
void CClientSession::SendAvatarSkillInfo(CNtlPacket * pPacket, CGameServer * app)
{
	size_t i = 0;
	app->db->prepare("SELECT * FROM skills WHERE owner_id = ? ORDER BY SlotID ASC");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->execute();

	CNtlPacket packet(sizeof(sGU_AVATAR_SKILL_INFO));
	sGU_AVATAR_SKILL_INFO * res = (sGU_AVATAR_SKILL_INFO *)packet.GetPacketData();

	res->wOpCode = GU_AVATAR_SKILL_INFO;
	res->bySkillCount = app->db->rowsCount();
	CSkillTable * pSkillTable = app->g_pTableContainer->GetSkillTable();
	while( app->db->fetch() )
	{
		//Added because Shenron Buffs and for help to detect type of skills
		//Note Shenron Buffs does not take ANY SLOTID
		sSKILL_TBLDAT* pSkillData = reinterpret_cast<sSKILL_TBLDAT*>(pSkillTable->FindData(app->db->getInt("skill_id")));
		switch (pSkillData->bySkill_Active_Type)
		{
		case SKILL_ACTIVE_TYPE_DD:
			printf("a\n");
			printf("TBLIDX %i \n", pSkillData->tblidx);
			break;
		case SKILL_ACTIVE_TYPE_BB:
			printf("b\n");
			printf("TBLIDX %i \n", pSkillData->tblidx);
			break;
		case SKILL_ACTIVE_TYPE_CB:
			printf("c\n");
			printf("TBLIDX %i \n", pSkillData->tblidx);
			break;
		case SKILL_ACTIVE_TYPE_DB:
			printf("d\n");
			printf("TBLIDX %i \n", pSkillData->tblidx);
			break;
		case SKILL_ACTIVE_TYPE_DC:
			printf("e\n");
			printf("TBLIDX %i \n", pSkillData->tblidx);
			break;
		case SKILL_ACTIVE_TYPE_DH:
			printf("f\n");
			printf("TBLIDX %i \n", pSkillData->tblidx);
			break;
		case SKILL_ACTIVE_TYPE_DOT:
			printf("g\n");
			printf("TBLIDX %i \n", pSkillData->tblidx);
			break;
		}

		res->aSkillInfo[i].bIsRpBonusAuto = app->db->getBoolean("RpBonusAuto");
		res->aSkillInfo[i].byRpBonusType = app->db->getInt("RpBonusType");		
		this->gsf->DebugSkillType(pSkillData->bySkill_Active_Type);
 		res->aSkillInfo[i].bySlotId = app->db->getInt("SlotID");
		res->aSkillInfo[i].dwTimeRemaining = app->db->getInt("TimeRemaining");
		res->aSkillInfo[i].nExp = app->db->getInt("Exp");
		res->aSkillInfo[i].tblidx = app->db->getInt("skill_id");

		packet.AdjustPacketLen(sizeof(sNTLPACKETHEADER)+(2 * sizeof(BYTE)) + (res->bySkillCount * sizeof(sITEM_PROFILE)));
		g_pApp->Send( this->GetHandle() , &packet );

		i++;
	}
}
//--------------------------------------------------------------------------------------//
//		SendAvatarHTBInfo Luiz45
//--------------------------------------------------------------------------------------//
void CClientSession::SendAvatarHTBInfo(CNtlPacket * pPacket, CGameServer* app)
{
	size_t i = 0;
	printf("Send skill info\n");
	app->db->prepare("SELECT * FROM skills WHERE owner_id = ? ORDER BY SlotID ASC");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->execute();
	
	CNtlPacket packet(sizeof(sGU_AVATAR_HTB_INFO));
	sGU_AVATAR_HTB_INFO * res = (sGU_AVATAR_HTB_INFO *)packet.GetPacketData();
	res->wOpCode = GU_AVATAR_HTB_INFO;
	res->byHTBSkillCount = 2;//Always TWO per Race/Class
	CSkillTable * pSkillTable = app->g_pTableContainer->GetSkillTable();
	while (app->db->fetch())
	{
		//Added because Shenron Buffs and for help to detect type of skills
		//Note Shenron Buffs does not take ANY SLOTID
		sSKILL_TBLDAT* pSkillData = reinterpret_cast<sSKILL_TBLDAT*>(pSkillTable->FindData(app->db->getInt("skill_id")));
		this->gsf->DebugSkillType(pSkillData->bySkill_Active_Type);
		
		if (pSkillData->bySkill_Class == NTL_SKILL_CLASS_HTB)
		{
			res->aHTBSkillnfo[i].bySlotId = app->db->getInt("SlotID");
			res->aHTBSkillnfo[i].dwTimeRemaining = app->db->getInt("TimeRemaining");
			res->aHTBSkillnfo[i].skillId = pSkillData->tblidx;
			packet.AdjustPacketLen(sizeof(sNTLPACKETHEADER)+(2 * sizeof(BYTE)) + (res->byHTBSkillCount * sizeof(sITEM_PROFILE)));
			g_pApp->Send(this->GetHandle(), &packet);

			i++;
		}		
	}
}
//--------------------------------------------------------------------------------------//
//		SendAvatarInfoEnd
//--------------------------------------------------------------------------------------//
void CClientSession::SendAvatarInfoEnd(CNtlPacket * pPacket)
{
	CNtlPacket packet(sizeof(sGU_AVATAR_INFO_END));
	sGU_AVATAR_INFO_END * res = (sGU_AVATAR_INFO_END *)packet.GetPacketData();

	res->wOpCode = GU_AVATAR_INFO_END;

	packet.SetPacketLen( sizeof(sGU_AVATAR_INFO_END) );
	g_pApp->Send( this->GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		Login into World
//--------------------------------------------------------------------------------------//
void CClientSession::SendWorldEnterReq(CNtlPacket * pPacket, CGameServer * app)
{
	CNtlPacket packet(sizeof(sGU_AVATAR_WORLD_INFO));
	sGU_AVATAR_WORLD_INFO * res = (sGU_AVATAR_WORLD_INFO *)packet.GetPacketData();

	app->db->prepare("SELECT * FROM characters WHERE CharID = ?");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->execute();
	app->db->fetch();

	res->wOpCode = GU_AVATAR_WORLD_INFO;
	res->worldInfo.tblidx = app->db->getInt("WorldTable");
	res->worldInfo.worldID = app->db->getInt("WorldID");
	res->worldInfo.hTriggerObjectOffset = 100000;
	res->worldInfo.sRuleInfo.byRuleType = GAMERULE_NORMAL;
	res->vCurLoc.x = this->plr->GetPosition().x;
	res->vCurLoc.y = this->plr->GetPosition().y;
	res->vCurLoc.z = this->plr->GetPosition().z;
	res->vCurDir.x = this->plr->GetDirection().x;
	res->vCurDir.y = this->plr->GetDirection().y;
	res->vCurDir.z = this->plr->GetDirection().z;

	this->plr->SetWorldID(app->db->getInt("WorldID"));
	this->plr->SetWorldTableID(app->db->getInt("WorldTable"));

	packet.SetPacketLen( sizeof(sGU_AVATAR_WORLD_INFO) );
	g_pApp->Send( this->GetHandle(), &packet );

}
//--------------------------------------------------------------------------------------//
//		Character ready request
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharReadyReq(CNtlPacket * pPacket, CGameServer * app)
{
//SPAN PLAYERS
	CNtlPacket packet(sizeof(sGU_OBJECT_CREATE));
	sGU_OBJECT_CREATE * res = (sGU_OBJECT_CREATE *)packet.GetPacketData();

	app->db->prepare("SELECT * FROM characters WHERE CharID = ?");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->execute();
	app->db->fetch();

	wcscpy_s(this->plr->pcProfile->awchName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, s2ws(app->db->getString("CharName")).c_str() );

	CPCTable *pPcTable = app->g_pTableContainer->GetPcTable();
	sPC_TBLDAT *pTblData = (sPC_TBLDAT*)pPcTable->GetPcTbldat(app->db->getInt("Race"),app->db->getInt("Class"),app->db->getInt("Gender"));

	res->wOpCode = GU_OBJECT_CREATE;
	res->handle = this->GetavatarHandle();
	res->sObjectInfo.objType = OBJTYPE_PC;
	res->sObjectInfo.pcBrief.tblidx = pTblData->tblidx;
	res->sObjectInfo.pcBrief.bIsAdult = app->db->getBoolean("Adult");
	wcscpy_s(res->sObjectInfo.pcBrief.awchName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, s2ws(app->db->getString("CharName")).c_str() );
	wcscpy_s(res->sObjectInfo.pcBrief.wszGuildName, NTL_MAX_SIZE_GUILD_NAME_IN_UNICODE, s2ws(app->db->getString("GuildName")).c_str() );
	res->sObjectInfo.pcBrief.sPcShape.byFace = app->db->getInt("Face");
	res->sObjectInfo.pcBrief.sPcShape.byHair = app->db->getInt("Hair");
	res->sObjectInfo.pcBrief.sPcShape.byHairColor = app->db->getInt("HairColor");
	res->sObjectInfo.pcBrief.sPcShape.bySkinColor = app->db->getInt("SkinColor");
	res->sObjectInfo.pcBrief.wCurLP = app->db->getInt("CurLP");
	res->sObjectInfo.pcBrief.wMaxLP = app->db->getInt("BaseMaxLP");
	res->sObjectInfo.pcBrief.wCurEP = app->db->getInt("CurEP");
	res->sObjectInfo.pcBrief.wMaxEP = app->db->getInt("BaseMaxEP");
	res->sObjectInfo.pcBrief.byLevel = app->db->getInt("Level");
	res->sObjectInfo.pcBrief.fSpeed = (float)app->db->getDouble("LastRunSpeed");
	res->sObjectInfo.pcBrief.wAttackSpeedRate = app->db->getInt("BaseAttackSpeedRate");
	res->sObjectInfo.pcState.sCharStateBase.vCurLoc.x = this->plr->GetPosition().x;
	res->sObjectInfo.pcState.sCharStateBase.vCurLoc.y = this->plr->GetPosition().y;
	res->sObjectInfo.pcState.sCharStateBase.vCurLoc.z = this->plr->GetPosition().z;
	res->sObjectInfo.pcState.sCharStateBase.vCurDir.x = this->plr->GetDirection().x;
	res->sObjectInfo.pcState.sCharStateBase.vCurDir.y = this->plr->GetDirection().y;
	res->sObjectInfo.pcState.sCharStateBase.vCurDir.z = this->plr->GetDirection().z;
	res->sObjectInfo.pcState.sCharStateBase.dwConditionFlag = 0;
	res->sObjectInfo.pcState.sCharStateBase.byStateID = 0;
	res->sObjectInfo.pcState.sCharStateBase.aspectState.sAspectStateBase.byAspectStateId = 255;
	res->sObjectInfo.pcState.sCharStateBase.aspectState.sAspectStateDetail.sGreatNamek.bySourceGrade = 0;
	res->sObjectInfo.pcState.sCharStateBase.aspectState.sAspectStateDetail.sKaioken.bySourceGrade = 0;
	res->sObjectInfo.pcState.sCharStateBase.aspectState.sAspectStateDetail.sPureMajin.bySourceGrade = 0;
	res->sObjectInfo.pcState.sCharStateBase.aspectState.sAspectStateDetail.sSuperSaiyan.bySourceGrade = 0;
	res->sObjectInfo.pcState.sCharStateBase.aspectState.sAspectStateDetail.sVehicle.idVehicleTblidx = 0;
	this->plr->SetGuildName(app->db->getString("GuildName"));

	for(int i = 0; i < NTL_MAX_EQUIP_ITEM_SLOT; i++)
	{
	app->db->prepare("select * from items WHERE place=7 AND pos=? AND owner_id=?");
	app->db->setInt(1, i);
	app->db->setInt(2, this->plr->pcProfile->charId);
	app->db->execute();
	app->db->fetch();
	if(app->db->rowsCount() == 0)
		{
			res->sObjectInfo.pcBrief.sItemBrief[i].tblidx =  INVALID_TBLIDX;
		}
		else
		{
			
		res->sObjectInfo.pcBrief.sItemBrief[i].tblidx = app->db->getInt("tblidx");
		}

	}

	memcpy(&this->characterspawnInfo, res, sizeof(sGU_OBJECT_CREATE) );
	packet.SetPacketLen( sizeof(sGU_OBJECT_CREATE) );

	app->UserBroadcastothers(&packet, this);
	app->UserBroadcasFromOthers(GU_OBJECT_CREATE, this);
	app->AddUser(this->plr->GetPlayerName().c_str(), this);
}

//--------------------------------------------------------------------------------------//
//		Auth community Server
//--------------------------------------------------------------------------------------//
void CClientSession::SendAuthCommunityServer(CNtlPacket * pPacket, CGameServer * app)
{

	CNtlPacket packet(sizeof(sGU_AUTH_KEY_FOR_COMMUNITY_SERVER_RES));
	sGU_AUTH_KEY_FOR_COMMUNITY_SERVER_RES * res = (sGU_AUTH_KEY_FOR_COMMUNITY_SERVER_RES *)packet.GetPacketData();

	res->wOpCode = GU_AUTH_KEY_FOR_COMMUNITY_SERVER_RES;
	res->wResultCode = GAME_SUCCESS;
	strcpy_s((char*)res->abyAuthKey, NTL_MAX_SIZE_AUTH_KEY, "ChatCon");
	packet.SetPacketLen( sizeof(sGU_AUTH_KEY_FOR_COMMUNITY_SERVER_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		SPAWN NPC
//--------------------------------------------------------------------------------------//
void CClientSession::SendNpcCreate(CNtlPacket * pPacket, CGameServer * app)
{
	app->mob->SpawnNpcAtLogin(pPacket, this);
}
//--------------------------------------------------------------------------------------//
//		SPAWN MOBS
//--------------------------------------------------------------------------------------//
void CClientSession::SendMonsterCreate(CNtlPacket * pPacket, CGameServer * app)
{
	app->mob->SpawnMonsterAtLogin(pPacket, this);
}
//--------------------------------------------------------------------------------------//
//		SendEnterWorldComplete
//--------------------------------------------------------------------------------------//
void CClientSession::SendEnterWorldComplete(CNtlPacket * pPacket)
{
	CNtlPacket packet(sizeof(sGU_ENTER_WORLD_COMPLETE));
	sGU_ENTER_WORLD_COMPLETE * res = (sGU_ENTER_WORLD_COMPLETE *)packet.GetPacketData();

	res->wOpCode = GU_ENTER_WORLD_COMPLETE;

	packet.SetPacketLen( sizeof(sGU_ENTER_WORLD_COMPLETE) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		Tutorial Hint request
//--------------------------------------------------------------------------------------//
void CClientSession::SendTutorialHintReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_TUTORIAL_HINT_UPDATE_REQ * req = (sUG_TUTORIAL_HINT_UPDATE_REQ *)pPacket->GetPacketData();
	//req->dwTutorialHint;

	CNtlPacket packet(sizeof(sGU_TUTORIAL_HINT_UPDATE_RES));
	sGU_TUTORIAL_HINT_UPDATE_RES * res = (sGU_TUTORIAL_HINT_UPDATE_RES *)packet.GetPacketData();

	//app->db->prepare("SELECT * FROM characters WHERE CharID = ?");
	//app->db->setInt(1, this->characterID);
	//app->db->execute();
	//app->db->fetch();
	res->wOpCode = GU_TUTORIAL_HINT_UPDATE_RES;
	res->wResultCode = GAME_SUCCESS;
	res->dwTutorialHint = req->dwTutorialHint;

	packet.SetPacketLen( sizeof(sGU_TUTORIAL_HINT_UPDATE_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		Char Ready
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharReady(CNtlPacket * pPacket)
{
	CNtlPacket packet(sizeof(sUG_CHAR_READY));
	sUG_CHAR_READY * res = (sUG_CHAR_READY *)packet.GetPacketData();

	res->wOpCode = UG_CHAR_READY;
	res->byAvatarType = 0;

	packet.SetPacketLen( sizeof(sUG_CHAR_READY) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		Char Move
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharMove(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_CHAR_MOVE * req = (sUG_CHAR_MOVE*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_CHAR_MOVE));
	sGU_CHAR_MOVE * res = (sGU_CHAR_MOVE *)packet.GetPacketData();

	res->wOpCode = GU_CHAR_MOVE;
	res->handle = this->GetavatarHandle();
	res->dwTimeStamp = req->dwTimeStamp;
	res->vCurLoc.x = req->vCurLoc.x;
	res->vCurLoc.y = req->vCurLoc.y;
	res->vCurLoc.z = req->vCurLoc.z;
	res->vCurDir.x = req->vCurDir.x;
	res->vCurDir.y = 0;
	res->vCurDir.z = req->vCurDir.z;
	res->byMoveDirection = req->byMoveDirection;
	res->byMoveFlag = NTL_MOVE_FIRST;

	this->plr->SetPosition(res->vCurLoc, res->vCurDir);

	packet.SetPacketLen( sizeof(sGU_CHAR_MOVE) );
	app->UserBroadcastothers(&packet, this);

}
//--------------------------------------------------------------------------------------//
//		Char Destination Move
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharDestMove(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_CHAR_DEST_MOVE * req = (sUG_CHAR_DEST_MOVE*)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_CHAR_DEST_MOVE));
	sGU_CHAR_DEST_MOVE * res = (sGU_CHAR_DEST_MOVE *)packet.GetPacketData();
	
	res->wOpCode = GU_CHAR_DEST_MOVE;
	res->handle = this->GetavatarHandle();
	res->dwTimeStamp = req->dwTimeStamp;
	res->vCurLoc.x = req->vCurLoc.x;
	res->vCurLoc.y = req->vCurLoc.y;
	res->vCurLoc.z = req->vCurLoc.z;
	res->byMoveFlag = NTL_MOVE_MOUSE_MOVEMENT;
	res->bHaveSecondDestLoc = false;
	res->byDestLocCount = 1;
	res->avDestLoc[0].x = req->vDestLoc.x;
	res->avDestLoc[0].y = req->vDestLoc.y;
	res->avDestLoc[0].z = req->vDestLoc.z;

	packet.SetPacketLen( sizeof(sGU_CHAR_DEST_MOVE) );
	app->UserBroadcastothers(&packet, this);


	if(timeGetTime() - this->plr->Getmob_SpawnTime() >= MONSTER_SPAWN_UPDATE_TICK)
	{
		app->mob->RunSpawnCheck(&packet, this->plr->GetPosition(), this);
		this->plr->Setmob_SpawnTime(timeGetTime());
	}

}
//--------------------------------------------------------------------------------------//
//		Char Move Sync
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharMoveSync(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_CHAR_MOVE_SYNC * req = (sUG_CHAR_MOVE_SYNC*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_CHAR_MOVE_SYNC));
	sGU_CHAR_MOVE_SYNC * res = (sGU_CHAR_MOVE_SYNC *)packet.GetPacketData();

	res->wOpCode = GU_CHAR_MOVE_SYNC;
	res->handle = this->GetavatarHandle();
	res->vCurLoc.x = req->vCurLoc.x;
	res->vCurLoc.y = req->vCurLoc.y;
	res->vCurLoc.z = req->vCurLoc.z;
	res->vCurDir.x = req->vCurDir.x;
	res->vCurDir.y = req->vCurDir.y;
	res->vCurDir.z = req->vCurDir.z;

	packet.SetPacketLen( sizeof(sGU_CHAR_MOVE_SYNC) );
	app->UserBroadcastothers(&packet, this);

	if(timeGetTime() - this->plr->Getmob_SpawnTime() >= MONSTER_SPAWN_UPDATE_TICK)
	{
		app->mob->RunSpawnCheck(&packet, this->plr->GetPosition(), this);
		this->plr->Setmob_SpawnTime(timeGetTime());
	}
}
//--------------------------------------------------------------------------------------//
//		Char Change Heading
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharChangeHeading(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_CHAR_CHANGE_HEADING * req = (sUG_CHAR_CHANGE_HEADING*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_CHAR_CHANGE_HEADING));
	sGU_CHAR_CHANGE_HEADING * res = (sGU_CHAR_CHANGE_HEADING *)packet.GetPacketData();

	res->wOpCode = GU_CHAR_CHANGE_HEADING;
	res->handle = this->GetavatarHandle();
	res->vNewHeading.x = req->vCurrentHeading.x;
	res->vNewHeading.y = req->vCurrentHeading.y;
	res->vNewHeading.z = req->vCurrentHeading.z;
	res->vNewLoc.x = req->vCurrentPosition.x;
	res->vNewLoc.y = req->vCurrentPosition.y;
	res->vNewLoc.z = req->vCurrentPosition.z;

	packet.SetPacketLen( sizeof(sGU_CHAR_CHANGE_HEADING) );
	app->UserBroadcastothers(&packet, this);
}
//--------------------------------------------------------------------------------------//
//		Char Jump
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharJump(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_CHAR_CHANGE_HEADING * req = (sUG_CHAR_CHANGE_HEADING*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_CHAR_JUMP));
	sGU_CHAR_JUMP * res = (sGU_CHAR_JUMP *)packet.GetPacketData();

	res->wOpCode = GU_CHAR_JUMP;
	res->handle = this->GetavatarHandle();
	res->vCurrentHeading.x = req->vCurrentHeading.x;
	res->vCurrentHeading.y = req->vCurrentHeading.y;
	res->vCurrentHeading.z = req->vCurrentHeading.z;

	res->vJumpDir.x = 0;
	res->vJumpDir.y = 0;
	res->vJumpDir.z = 0;

	res->byMoveDirection = 1;

	packet.SetPacketLen( sizeof(sGU_CHAR_JUMP) );
	app->UserBroadcastothers(&packet, this);

}
//--------------------------------------------------------------------------------------//
//		Change Char Direction on floating
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharChangeDirOnFloating(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_CHAR_CHANGE_DIRECTION_ON_FLOATING * req = (sUG_CHAR_CHANGE_DIRECTION_ON_FLOATING*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_CHAR_CHANGE_DIRECTION_ON_FLOATING));
	sGU_CHAR_CHANGE_DIRECTION_ON_FLOATING * res = (sGU_CHAR_CHANGE_DIRECTION_ON_FLOATING *)packet.GetPacketData();

	res->wOpCode = GU_CHAR_CHANGE_DIRECTION_ON_FLOATING;
	res->hSubject = this->GetavatarHandle();
	res->vCurDir.x = req->vCurDir.x;
	res->vCurDir.y = req->vCurDir.y;
	res->vCurDir.z = req->vCurDir.z;
	res->byMoveDirection = req->byMoveDirection;

	packet.SetPacketLen( sizeof(sGU_CHAR_CHANGE_DIRECTION_ON_FLOATING) );
	app->UserBroadcastothers(&packet, this);
}
//--------------------------------------------------------------------------------------//
//		Char falling
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharFalling(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_CHAR_FALLING * req = (sUG_CHAR_FALLING*)pPacket->GetPacketData();

	req->wOpCode = UG_CHAR_FALLING;
	req->bIsFalling = true;

	req->vCurLoc.x;
	req->vCurLoc.y;
	req->vCurLoc.z;
	req->vCurDir.x;
	req->vCurDir.z;
	req->byMoveDirection;

}

//--------------------------------------------------------------------------------------//
//		GM Command
//--------------------------------------------------------------------------------------//
void CClientSession::RecvServerCommand(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_SERVER_COMMAND * pServerCmd = (sUG_SERVER_COMMAND*)pPacket;

	char chBuffer[1024];
	::WideCharToMultiByte(GetACP(), 0, pServerCmd->awchCommand, -1, chBuffer, 1024, NULL, NULL);

	CNtlTokenizer lexer(chBuffer);

	if(!lexer.IsSuccess())
		return;

	enum ECmdParseState
	{
		SERVER_CMD_NONE,
		SERVER_CMD_KEY,
		SERVER_CMD_END,
	};

	ECmdParseState eState = SERVER_CMD_KEY;
	int iOldLine = 0;
	int iLine;
	
	while(1)
	{
		std::string strToken = lexer.PeekNextToken(NULL, &iLine);

		if(strToken == "") 
			break;
	
		switch (eState)
		{
		case SERVER_CMD_KEY:
			if(strToken == "@setspeed")
			{
				printf("received char speed command");
				lexer.PopToPeek();
				strToken = lexer.PeekNextToken(NULL, &iLine);
				float fSpeed = (float)atof(strToken.c_str());
				CClientSession::SendUpdateCharSpeed(fSpeed, app);
				return;
			}
			else if(strToken == "@addmob")
			{
				lexer.PopToPeek();
				strToken = lexer.PeekNextToken(NULL, &iLine);
				unsigned int uiMobId = (unsigned int)atoi(strToken.c_str());
				lexer.PopToPeek();
				strToken = lexer.PeekNextToken(NULL, &iLine);
				float fDist = (float)atof(strToken.c_str());
				lexer.PopToPeek();

				//SendMonsterCreate(uiMobId, fDist);
				
				return;
			}
			else if(strToken == "@addmobg")
			{
				lexer.PopToPeek();
				strToken = lexer.PeekNextToken(NULL, &iLine);
				unsigned int iNum = (unsigned int)atoi(strToken.c_str());
				//SendMonsterGroupCreate(iNum);
				return;
			}
			else if(strToken == "@createitem")
			{
				lexer.PopToPeek();
				strToken = lexer.PeekNextToken(NULL, &iLine);
				unsigned int uiTblId = (unsigned int)atof(strToken.c_str());
			//	SendAddItem(uiTblId);
				return;
			}
			else if(strToken == "@learnskill")
			{
				lexer.PopToPeek();
				strToken = lexer.PeekNextToken(NULL, &iLine);
				unsigned int uiTblId = (unsigned int)atof(strToken.c_str());
			//	SendCharLearnSkillRes(uiTblId);
				return;
			}
			else if(strToken == "@learnhtb")
			{
				lexer.PopToPeek();
				strToken = lexer.PeekNextToken(NULL, &iLine);
				unsigned int uiTblId = (unsigned int)atof(strToken.c_str());
			//	SendCharLearnHTBRes(uiTblId);
				return;
			}
			else if(strToken == "@refreshlp")
			{
			//	app->db->prepare("SELECT LastMaxLp FROM characters WHERE CharID = ?");
			//	app->db->setInt(1, this->characterID);
			//	app->db->execute();
			//	app->db->fetch();
			//	int max_lp = app->db->getInt("LastMaxLp");

				return; 
			}
			else if(strToken == "@setscale")
			{
				lexer.PopToPeek();
				strToken = lexer.PeekNextToken(NULL, &iLine);
				float fScale = (float)atof(strToken.c_str());
			//	CNtlSob *pSobObj = GetNtlSobManager()->GetSobObject(m_uiTargetSerialId);
			//	if(pSobObj)
			//	{
			//		CNtlSobProxy *pSobProxy = pSobObj->GetSobProxy();
			//		pSobProxy->SetScale(fScale);
			//	}
				
				return;
			}
			else if(strToken == "@is")
			{
				lexer.PopToPeek();
				strToken = lexer.PeekNextToken(NULL, &iLine);
			//	CNtlBehaviorProjSteal::m_ffIncSpeed = (RwReal)atof(strToken.c_str());
			}
			else if(strToken == "@iw")
			{
				lexer.PopToPeek();
				strToken = lexer.PeekNextToken(NULL, &iLine);
			//	CNtlBehaviorProjSteal::m_fWaitCheckTime = (RwReal)atof(strToken.c_str());
			}
			else if(strToken == "@compilelua")
			{
		//		SLLua_Setup();
				return;
			}
			
			break;
		}

		lexer.PopToPeek();
	}
}
//--------------------------------------------------------------------------------------//
//		Update Char speed *dont work*
//--------------------------------------------------------------------------------------//
void CClientSession::SendUpdateCharSpeed(float fSpeed, CGameServer * app)
{
	CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_SPEED));
	sGU_UPDATE_CHAR_SPEED * res = (sGU_UPDATE_CHAR_SPEED *)packet.GetPacketData();

	res->wOpCode = GU_UPDATE_CHAR_SPEED;
	res->handle = this->GetavatarHandle();
	res->fLastWalkingSpeed = fSpeed;
	res->fLastRunningSpeed = fSpeed;
	this->plr->pcProfile->avatarAttribute.fLastRunSpeed = fSpeed;
	packet.SetPacketLen( sizeof(sGU_UPDATE_CHAR_SPEED) );
	app->UserBroadcastothers(&packet, this);
}
//--------------------------------------------------------------------------------------//
//		Select target
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharTargetSelect(CNtlPacket * pPacket)
{
	sUG_CHAR_TARGET_SELECT * req = (sUG_CHAR_TARGET_SELECT*)pPacket->GetPacketData();
	m_uiTargetSerialId = req->hTarget;
}
//--------------------------------------------------------------------------------------//
//		Select target
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharTargetFacing(CNtlPacket * pPacket)
{
	sUG_CHAR_TARGET_SELECT * req = (sUG_CHAR_TARGET_SELECT*)pPacket->GetPacketData();
	m_uiTargetSerialId = req->hTarget;
}
//--------------------------------------------------------------------------------------//
//		target info
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharTargetInfo(CNtlPacket * pPacket)
{
	sUG_CHAR_TARGET_SELECT * req = (sUG_CHAR_TARGET_SELECT*)pPacket->GetPacketData();
}
//--------------------------------------------------------------------------------------//
//		Send game leave request
//--------------------------------------------------------------------------------------//
void CClientSession::SendGameLeaveReq(CNtlPacket * pPacket, CGameServer * app)
{
	app->db->prepare("UPDATE characters SET IsOnline = 0, OnlineID = 0 WHERE CharID = ?");
	app->db->setInt(1,  this->plr->pcProfile->charId);
	app->db->execute();
	std::string log = "Player " + this->plr->GetPlayerName() + " have leaved the game\n";
	this->gsf->printDebug(log.c_str());
	this->plr->SaveMe();
	app->RemoveUser( this->plr->GetPlayerName().c_str() );
	CNtlPacket packet(sizeof(sGU_OBJECT_DESTROY));
	sGU_OBJECT_DESTROY * sPacket = (sGU_OBJECT_DESTROY *)packet.GetPacketData();

	sPacket->wOpCode = GU_OBJECT_DESTROY;
	sPacket->handle = this->GetavatarHandle();
	packet.SetPacketLen( sizeof(sGU_OBJECT_DESTROY) );
	app->UserBroadcastothers(&packet, this);

}
//--------------------------------------------------------------------------------------//
//		Char exit request
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharExitReq(CNtlPacket * pPacket, CGameServer * app)
{
	app->db->prepare("UPDATE characters SET IsOnline = 0, OnlineID = 0 WHERE CharID = ?");
	app->db->setInt(1,  this->plr->pcProfile->charId);
	app->db->execute();
	std::string log = "Player " + this->plr->GetPlayerName() + " have leaved the game\n";
	this->gsf->printDebug(log.c_str());
	this->plr->SaveMe();
// log out of game
	CNtlPacket packet1(sizeof(sGU_OBJECT_DESTROY));
	sGU_OBJECT_DESTROY * sPacket = (sGU_OBJECT_DESTROY *)packet1.GetPacketData();

	sPacket->wOpCode = GU_OBJECT_DESTROY;
	sPacket->handle = this->GetavatarHandle();
	packet1.SetPacketLen( sizeof(sGU_OBJECT_DESTROY) );
	app->UserBroadcastothers(&packet1, this);

	app->RemoveUser( this->plr->GetPlayerName().c_str() );

// log in to char server
	CNtlPacket packet(sizeof(sGU_CHAR_EXIT_RES));
	sGU_CHAR_EXIT_RES * res = (sGU_CHAR_EXIT_RES *)packet.GetPacketData();

	res->wOpCode = GU_CHAR_EXIT_RES;
	res->wResultCode = GAME_SUCCESS;
	strcpy_s((char*)res->achAuthKey, NTL_MAX_SIZE_AUTH_KEY, "Dbo");
	res->byServerInfoCount = 1;
	strcpy_s(res->aServerInfo[0].szCharacterServerIP, NTL_MAX_LENGTH_OF_IP, IP_SERVER_ALL);
	res->aServerInfo[0].wCharacterServerPortForClient = 20300;
	res->aServerInfo[0].dwLoad = 0;

	packet.SetPacketLen( sizeof(sGU_CHAR_EXIT_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		Char sit down
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharSitDown(CNtlPacket * pPacket, CGameServer * app)
{
	CNtlPacket packet(sizeof(sGU_CHAR_SITDOWN));
	sGU_CHAR_SITDOWN * sPacket = (sGU_CHAR_SITDOWN *)packet.GetPacketData();

	sPacket->wOpCode = GU_CHAR_SITDOWN;
	sPacket->handle = this->GetavatarHandle();
	packet.SetPacketLen( sizeof(sGU_CHAR_SITDOWN) );
	app->UserBroadcastothers(&packet, this);
}
//--------------------------------------------------------------------------------------//
//		Char stand up
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharStandUp(CNtlPacket * pPacket, CGameServer * app)
{
	CNtlPacket packet(sizeof(sGU_CHAR_STANDUP));
	sGU_CHAR_STANDUP * sPacket = (sGU_CHAR_STANDUP *)packet.GetPacketData();

	sPacket->wOpCode = GU_CHAR_STANDUP;
	sPacket->handle = this->GetavatarHandle();
	packet.SetPacketLen( sizeof(sGU_CHAR_STANDUP) );
	app->UserBroadcastothers(&packet, this);
}
//--------------------------------------------------------------------------------------//
//		char start mail
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharMailStart(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_MAIL_START_REQ * req = (sUG_MAIL_START_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_MAIL_START_RES));
	sGU_MAIL_START_RES * res = (sGU_MAIL_START_RES *)packet.GetPacketData();

	app->db->prepare("SELECT MailIsAway FROM characters WHERE CharID=?");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->execute();
	app->db->fetch();

	//res->hObject = req->hObject;
	res->wOpCode = GU_MAIL_START_RES;
	res->wResultCode = GAME_SUCCESS;
	res->bIsAway = app->db->getBoolean("MailIsAway");
	
	app->db->prepare("SELECT * FROM mail WHERE CharID = ?");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->execute();
	if (app->db->rowsCount() > 0)
	{
		while (app->db->fetch())
		{
			CNtlPacket packet2(sizeof(sGU_MAIL_LOAD_DATA));
			sGU_MAIL_LOAD_DATA * res2 = (sGU_MAIL_LOAD_DATA *)packet2.GetPacketData();
			res2->wOpCode = GU_MAIL_LOAD_DATA;
			res2->sData.bIsAccept = app->db->getBoolean("bIsAccept");
			res2->sData.bIsLock = app->db->getBoolean("bIsLock");
			res2->sData.bIsRead = app->db->getBoolean("bIsRead");
		//	res2->sData.bySenderType = app->db->getInt("SenderType");
			res2->sData.byMailType = app->db->getInt("byMailType");
			wcscpy_s(res2->wszText, NTL_MAX_LENGTH_OF_MAIL_MESSAGE_IN_UNICODE, s2ws(app->db->getString("wszText")).c_str() );
			wcscpy_s(res2->sData.wszFromName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, s2ws(app->db->getString("wszFromName")).c_str() );
			res2->byTextSize = app->db->getInt("byTextSize");
			res2->sData.dwZenny = app->db->getInt("dwZenny");
			res2->byTextSize = app->db->getInt("byTextSize");
			res2->sData.endTime = app->db->getInt("byDay");
			res2->sData.mailID = app->db->getInt("id");
			packet2.SetPacketLen( sizeof(sGU_MAIL_LOAD_DATA) );
			g_pApp->Send( this->GetHandle(), &packet2 );
		}
	}
	
	packet.SetPacketLen( sizeof(sGU_MAIL_START_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		load mails
//--------------------------------------------------------------------------------------//
void	CClientSession::SendCharMailLoadReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_MAIL_LOAD_REQ * req = (sUG_MAIL_LOAD_REQ*)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_MAIL_LOAD_RES));
	sGU_MAIL_LOAD_RES * res = (sGU_MAIL_LOAD_RES *)packet.GetPacketData();

	res->wOpCode = GU_MAIL_LOAD_RES;
	res->hObject = req->hObject;
	res->wResultCode = GAME_SUCCESS;

	packet.SetPacketLen( sizeof(sGU_MAIL_LOAD_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		reload mails
//--------------------------------------------------------------------------------------//
void	CClientSession::SendCharMailReloadReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_MAIL_RELOAD_REQ * req = (sUG_MAIL_RELOAD_REQ*)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_MAIL_RELOAD_RES));
	sGU_MAIL_RELOAD_RES * res = (sGU_MAIL_RELOAD_RES *)packet.GetPacketData();

// COUNT UNREAD MESSAGES START
	app->db->prepare("SELECT COUNT(*) AS countmsg FROM mail WHERE CharID = ? AND bIsRead=0");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->execute();
	app->db->fetch();
	RwUInt32 count_unread_msg = app->db->getInt("countmsg");
// COUNT UNREAD MESSAGES END

	app->db->prepare("SELECT * FROM mail WHERE CharID = ?");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->execute();
	RwUInt32 i = 0;
	while (app->db->fetch())
	{
		res->aMailID[i] = app->db->getInt("id");
		i++;
	}

		res->byMailCount = app->db->rowsCount(); //count all mails
		res->byManagerCount = count_unread_msg; //amount of unread messages
		res->byNormalCount = 0; 
		res->hObject = req->hObject;
		res->wOpCode = GU_MAIL_RELOAD_RES;
		res->wResultCode = GAME_SUCCESS;

		packet.SetPacketLen( sizeof(sGU_MAIL_RELOAD_RES) );
		g_pApp->Send( this->GetHandle(), &packet );

	this->SendCharMailStart(pPacket,app);
}
//--------------------------------------------------------------------------------------//
//		read mails
//--------------------------------------------------------------------------------------//
void	CClientSession::SendCharMailReadReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_MAIL_READ_REQ * req = (sUG_MAIL_READ_REQ*)pPacket->GetPacketData();

	app->db->prepare("SELECT * FROM mail WHERE id = ?");
	app->db->setInt(1, req->mailID);
	app->db->execute();
	app->db->fetch();
	
	RwUInt32 itemid = app->db->getInt("item_id");

	CNtlPacket packet(sizeof(sGU_MAIL_READ_RES));
	sGU_MAIL_READ_RES * res = (sGU_MAIL_READ_RES *)packet.GetPacketData();

	CNtlPacket packet2(sizeof(sGU_MAIL_LOAD_INFO));
	sGU_MAIL_LOAD_INFO * res2 = (sGU_MAIL_LOAD_INFO *)packet2.GetPacketData();


	res->byRemainDay = app->db->getInt("byDay");
	res->endTime = 100;
	res->hObject = req->hObject;
	res->mailID = req->mailID;
	res->wOpCode = GU_MAIL_READ_RES;
	res->wResultCode = GAME_SUCCESS;

		res2->wOpCode = GU_MAIL_LOAD_INFO;
		res2->sData.bIsAccept = app->db->getBoolean("bIsAccept");
		res2->sData.bIsLock = app->db->getBoolean("bIsLock");
		res2->sData.bIsRead = app->db->getBoolean("bIsRead");
		res2->byTextSize = app->db->getInt("byTextSize");
		res2->sData.byExpired = 100;
		res2->sData.bySenderType = eMAIL_SENDER_TYPE_BASIC;
		res2->sData.byMailType = app->db->getInt("byMailType");
		res2->sData.dwZenny = app->db->getInt("dwZenny");
		res2->sData.endTime = app->db->getInt("byDay");
		res2->sData.mailID = app->db->getInt("id");
		wcscpy_s(res2->sData.wszFromName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, s2ws(app->db->getString("wszFromName")).c_str() );
		wcscpy_s(res2->wszText, NTL_MAX_LENGTH_OF_MAIL_MESSAGE_IN_UNICODE, s2ws(app->db->getString("wszText")).c_str() );

	//	res2->sData.tCreateTime.second = app->db->getInt("byDay");
	//	res2->sData.tCreateTime.minute = app->db->getInt("byDay");
	//	res2->sData.tCreateTime.hour = app->db->getInt("byDay");
	//	res2->sData.tCreateTime.day = app->db->getInt("tCreateTime");
	//	res2->sData.tCreateTime.month = app->db->getInt("byDay");
	//	res2->sData.tCreateTime.year = app->db->getInt("byDay");

		if(app->db->getInt("byMailType") > 1){
			app->db->prepare("SELECT id,tblidx,count,rank,durability,grade FROM items WHERE id = ?");
			app->db->setInt(1, itemid);
			app->db->execute();
			app->db->fetch();
			res2->sData.sItemProfile.handle = app->db->getInt("id");
			res2->sData.sItemProfile.byCurDur = app->db->getInt("durability");
			res2->sData.sItemProfile.byStackcount = app->db->getInt("count");
			res2->sData.sItemProfile.byGrade = app->db->getInt("grade");
			res2->sData.sItemProfile.tblidx = app->db->getInt("tblidx");
			res2->sData.sItemProfile.byRank = app->db->getInt("rank");
		}

		packet2.SetPacketLen( sizeof(sGU_MAIL_LOAD_INFO) );
		g_pApp->Send( this->GetHandle(), &packet2 );

		//SET MAIL READ
		app->qry->SetMailRead(req->mailID);

	packet.SetPacketLen( sizeof(sGU_MAIL_READ_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		send mails
//--------------------------------------------------------------------------------------//
void	CClientSession::SendCharMailSendReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_MAIL_SEND_REQ * req = (sUG_MAIL_SEND_REQ*)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_MAIL_SEND_RES));
	sGU_MAIL_SEND_RES * res = (sGU_MAIL_SEND_RES *)packet.GetPacketData();

	app->db->prepare("SELECT CharID,MailIsAway FROM characters WHERE CharName = ?");

	char targetname[NTL_MAX_SIZE_CHAR_NAME_UNICODE];
	wcstombs(targetname, req->wszTargetName, NTL_MAX_SIZE_CHAR_NAME_UNICODE);

	char *text = new char[req->byTextSize];
	wcstombs(text, req->wszText, req->byTextSize);
	text[req->byTextSize] = '\0';

	app->db->setString(1, targetname);
	app->db->execute();
	app->db->fetch();
	int id = app->db->getInt("CharID");

	wcscpy_s(res->wszTargetName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, req->wszTargetName );
	res->wOpCode = GU_MAIL_SEND_RES;
	res->hObject = req->hObject;

	if(app->db->getBoolean("MailIsAway") == 1){
		res->wResultCode = GAME_MAIL_TARGET_AWAY_STATE;
	} else {
		res->wResultCode = GAME_SUCCESS;
		app->db->prepare("INSERT INTO mail (CharID, byDay, byMailType, byTextSize, dwZenny, wszText, item_id, item_place, item_pos, wszTargetName, wszFromName, bIsAccept, bIsLock, bIsRead) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
		app->db->setInt(1, id);
		app->db->setInt(2, req->byDay);
		app->db->setInt(3, req->byMailType);
		app->db->setInt(4, req->byTextSize);
		app->db->setInt(5, req->dwZenny);
		app->db->setString(6, text);
		app->db->setInt(7, req->sItemData.hItem);
		app->db->setInt(8, req->sItemData.byPlace);
		app->db->setInt(9, req->sItemData.byPos);
		app->db->setString(10, targetname);
		app->db->setString(11, this->plr->GetPlayerName().c_str());
		app->db->setInt(12, app->db->getBoolean("MailIsAway"));
		app->db->setInt(13, false);
		app->db->setInt(14, false);
		app->db->execute();

		if(req->byMailType == 2 || req->byMailType == 5 ){
		  //SET OWNER ID TO 0
			app->qry->UpdateItemOwnerIdWithUniqueID(0, req->sItemData.hItem);
		  //DEL ITEM PACKET
			this->gsf->DeleteItemByUIdPlacePos(pPacket, this, req->sItemData.hItem, req->sItemData.byPlace, req->sItemData.byPos);
		}
		else if(req->byMailType == 3){
		  //UPDATE CHAR MONEY
			app->qry->SetMinusMoney(this->plr->pcProfile->charId, req->dwZenny);
			this->plr->pcProfile->dwZenny -= req->dwZenny;
		  //UPDATE MONEY PACKET
			this->gsf->UpdateCharMoney(pPacket,this,16,this->plr->pcProfile->dwZenny,this->GetavatarHandle());
		}
		else if(req->byMailType == 4){
		  //UPDATE MONEY
			app->qry->SetMinusMoney(this->plr->pcProfile->charId, req->dwZenny);
			this->plr->pcProfile->dwZenny -= req->dwZenny;
		  //UPDATE MONEY PACKET
			this->gsf->UpdateCharMoney(pPacket,this,16,this->plr->pcProfile->dwZenny,this->GetavatarHandle());
		  //SET OWNER ID TO 0
			app->qry->UpdateItemOwnerIdWithUniqueID(0, req->sItemData.hItem);
		  //DEL ITEM PACKET
			this->gsf->DeleteItemByUIdPlacePos(pPacket, this, req->sItemData.hItem, req->sItemData.byPlace, req->sItemData.byPos);
		}

	}

	packet.SetPacketLen( sizeof(sGU_MAIL_SEND_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		delete mails
//--------------------------------------------------------------------------------------//
void	CClientSession::SendCharMailDelReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_MAIL_DEL_REQ * req = (sUG_MAIL_DEL_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_MAIL_DEL_RES));
	sGU_MAIL_DEL_RES * res = (sGU_MAIL_DEL_RES *)packet.GetPacketData();

	app->qry->DeleteFromMailByID(req->mailID);

	res->hObject = req->hObject;
	res->mailID = req->mailID;
	res->wOpCode = GU_MAIL_DEL_RES;
	res->wResultCode = GAME_SUCCESS;
	
	packet.SetPacketLen( sizeof(sGU_MAIL_DEL_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		receive item with mail
//--------------------------------------------------------------------------------------//
void	CClientSession::SendCharMailItemReceiveReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_MAIL_ITEM_RECEIVE_REQ * req = (sUG_MAIL_ITEM_RECEIVE_REQ*)pPacket->GetPacketData();

	app->db->prepare("SELECT * FROM mail WHERE id = ?");
	app->db->setInt(1, req->mailID);
	app->db->execute();
	app->db->fetch();
	
	CNtlPacket packet(sizeof(sGU_MAIL_ITEM_RECEIVE_RES));
	sGU_MAIL_ITEM_RECEIVE_RES * res = (sGU_MAIL_ITEM_RECEIVE_RES *)packet.GetPacketData();

	res->hObject = req->hObject;
	res->mailID = req->mailID;
	res->wOpCode = GU_MAIL_ITEM_RECEIVE_RES;
	res->wResultCode = GAME_SUCCESS;


	if(app->db->getInt("byMailType") == 2 ){
	  //CHANGE ITEM OWNER
		app->qry->ChangeItemOwnerByUIdPlacePos(this->plr->pcProfile->charId, app->db->getInt("item_id"), 0, 0); // 0 = place and pos
	  //CREATE ITEM PACKET
		CNtlPacket packet1(sizeof(sGU_ITEM_CREATE));
		sGU_ITEM_CREATE * res1 = (sGU_ITEM_CREATE *)packet1.GetPacketData();

		res1->wOpCode = GU_ITEM_CREATE;
		packet1.SetPacketLen(sizeof(sGU_ITEM_CREATE));
		g_pApp->Send( this->GetHandle() , &packet1 );
	}
	else if(app->db->getInt("byMailType") == 3){
	  //UPDATE MONEY
		this->plr->pcProfile->dwZenny += app->db->getInt("dwZenny");
		app->qry->SetPlusMoney(this->plr->pcProfile->charId, app->db->getInt("dwZenny"));
	  //UPDATE MONEY PACKET
		this->gsf->UpdateCharMoney(pPacket,this,17,this->plr->pcProfile->dwZenny,this->GetavatarHandle());
	}
	else if(app->db->getInt("byMailType") == 4){
	  //UPDATE MONEY
		app->qry->SetPlusMoney(this->plr->pcProfile->charId, app->db->getInt("dwZenny"));
		this->plr->pcProfile->dwZenny += app->db->getInt("dwZenny");
	  //UPDATE MONEY PACKET
		this->gsf->UpdateCharMoney(pPacket,this,17,this->plr->pcProfile->dwZenny,this->GetavatarHandle());
	  //CHANGE ITEM OWNER
		app->qry->ChangeItemOwnerByUIdPlacePos(this->plr->pcProfile->charId, app->db->getInt("item_id"), 0, 0); // 0 = place and pos
	  //CREATE ITEM PACKET
		CNtlPacket packet3(sizeof(sGU_ITEM_CREATE));
		sGU_ITEM_CREATE * res3 = (sGU_ITEM_CREATE *)packet3.GetPacketData();

		res3->wOpCode = GU_ITEM_CREATE;
		packet3.SetPacketLen(sizeof(sGU_ITEM_CREATE));
		g_pApp->Send( this->GetHandle() , &packet3 );
	}
	
	app->qry->SetMailAccept(this->plr->pcProfile->charId, req->mailID);

	packet.SetPacketLen( sizeof(sGU_MAIL_ITEM_RECEIVE_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		delete multiple mails
//--------------------------------------------------------------------------------------//
void	CClientSession::SendCharMailMultiDelReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_MAIL_MULTI_DEL_REQ * req = (sUG_MAIL_MULTI_DEL_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_MAIL_MULTI_DEL_RES));
	sGU_MAIL_MULTI_DEL_RES * res = (sGU_MAIL_MULTI_DEL_RES *)packet.GetPacketData();

	for( RwInt32 j = 0 ; j < req->byCount ; ++j )
	{
		app->qry->DeleteFromMailByID(req->aMailID[j]);

		res->wOpCode = GU_MAIL_MULTI_DEL_RES;
		res->wResultCode = GAME_SUCCESS;
		res->hObject = req->hObject;
		res->byCount = req->byCount;
		res->aMailID[j] = req->aMailID[j];
	
		packet.SetPacketLen( sizeof(sGU_MAIL_MULTI_DEL_RES) );
		g_pApp->Send( this->GetHandle(), &packet );
	}

}
//--------------------------------------------------------------------------------------//
//		lock mail
//--------------------------------------------------------------------------------------//
void	CClientSession::SendCharMailLockReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_MAIL_LOCK_REQ * req = (sUG_MAIL_LOCK_REQ*)pPacket->GetPacketData();

	app->db->prepare("SELECT * FROM mail WHERE id = ?");
	app->db->setInt(1, req->mailID);
	app->db->execute();
	app->db->fetch();
	
	CNtlPacket packet(sizeof(sGU_MAIL_LOCK_RES));
	sGU_MAIL_LOCK_RES * res = (sGU_MAIL_LOCK_RES *)packet.GetPacketData();

	res->wOpCode = GU_MAIL_LOCK_RES;
	res->wResultCode = GAME_SUCCESS;
	res->hObject = req->hObject;
	res->mailID = req->mailID;
	res->bIsLock = req->bIsLock;

	app->qry->UpdateMailLock(req->mailID, res->bIsLock);

	packet.SetPacketLen( sizeof(sGU_MAIL_LOCK_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		RETURN MAIL
//--------------------------------------------------------------------------------------//
void	CClientSession::SendCharMailReturnReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_MAIL_RETURN_REQ * req = (sUG_MAIL_RETURN_REQ*)pPacket->GetPacketData();

	app->db->prepare("SELECT item_id FROM mail WHERE id = ?");
	app->db->setInt(1, req->mailID);
	app->db->execute();
	app->db->fetch();
	
	CNtlPacket packet(sizeof(sGU_MAIL_RETURN_RES));
	sGU_MAIL_RETURN_RES * res = (sGU_MAIL_RETURN_RES *)packet.GetPacketData();

	res->wOpCode = GU_MAIL_RETURN_RES;
	res->wResultCode = GAME_SUCCESS;
	res->hObject = req->hObject;
	res->mailID = req->mailID;

	app->db->prepare("CALL ReturnMail (?,?)");
	app->db->setInt(1, req->mailID);
	app->db->setString(2, this->plr->GetPlayerName().c_str());
	app->db->execute();
	
	packet.SetPacketLen( sizeof(sGU_MAIL_RETURN_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}


//--------------------------------------------------------------------------------------//
//		char away req
//--------------------------------------------------------------------------------------//
void	CClientSession::SendCharAwayReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_CHAR_AWAY_REQ * req = (sUG_CHAR_AWAY_REQ*)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_CHAR_AWAY_RES));
	sGU_CHAR_AWAY_RES * res = (sGU_CHAR_AWAY_RES *)packet.GetPacketData();

	res->wOpCode = GU_CHAR_AWAY_RES;
	res->wResultCode = GAME_SUCCESS;
	res->bIsAway = req->bIsAway;

	app->qry->UpdateCharAwayStatus(this->plr->pcProfile->charId, req->bIsAway);

	packet.SetPacketLen( sizeof(sGU_CHAR_AWAY_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		char follow move
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharFollowMove(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_CHAR_FOLLOW_MOVE * req = (sUG_CHAR_FOLLOW_MOVE*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_CHAR_FOLLOW_MOVE));
	sGU_CHAR_FOLLOW_MOVE * res = (sGU_CHAR_FOLLOW_MOVE *)packet.GetPacketData();

	res->wOpCode = GU_CHAR_FOLLOW_MOVE;
	res->handle = this->GetavatarHandle();
	res->hTarget = this->GetTargetSerialId();
	res->fDistance = req->fDistance;
	res->byMovementReason = req->byMovementReason;
	res->byMoveFlag = NTL_MOVE_FLAG_RUN;

	packet.SetPacketLen( sizeof(sGU_CHAR_FOLLOW_MOVE) );
	app->UserBroadcastothers(&packet, this);
	int rc = g_pApp->Send( this->GetHandle(), &packet );
}



//--------------------------------------------------------------------------------------//
//		Create Guild
//--------------------------------------------------------------------------------------//
void CClientSession::SendGuildCreateReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_GUILD_CREATE_REQ * req = (sUG_GUILD_CREATE_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_GUILD_CREATE_RES));
	sGU_GUILD_CREATE_RES * res = (sGU_GUILD_CREATE_RES *)packet.GetPacketData();

	res->wOpCode = GU_GUILD_CREATE_RES;

	app->db->prepare("CALL GuildCreate (?,?, @wResultCode, @cguildid, @charactername)");
	app->db->setString(1, Ntl_WC2MB(req->wszGuildName));
	app->db->setInt(2, this->plr->pcProfile->charId);
	app->db->execute();
	app->db->execute("SELECT @wResultCode, @cguildid, @charactername");
	app->db->fetch(); 

	int result = app->db->getInt("@wResultCode");
	res->wResultCode = result;

	packet.SetPacketLen( sizeof(sGU_GUILD_CREATE_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );

	if (result == 200) { 
		
// CREATE GUILD
		CNtlPacket packet2(sizeof(sTU_GUILD_CREATED_NFY));
		sTU_GUILD_CREATED_NFY * res2 = (sTU_GUILD_CREATED_NFY *)packet2.GetPacketData();
		res2->wOpCode = TU_GUILD_CREATED_NFY;
		memcpy(res2->wszGuildName, req->wszGuildName, sizeof(wchar_t)* NTL_MAX_SIZE_GUILD_NAME_IN_UNICODE);
		packet2.SetPacketLen( sizeof(sTU_GUILD_CREATED_NFY));
		rc = g_pApp->Send( this->GetHandle(), &packet2);

// GUILD INFORMATIONS
		CNtlPacket packet3(sizeof(sTU_GUILD_INFO));
		sTU_GUILD_INFO * res3 = (sTU_GUILD_INFO *)packet3.GetPacketData();

		res3->wOpCode = TU_GUILD_INFO;
		res3->guildInfo.dwGuildReputation = 0;
		res3->guildInfo.guildId = app->db->getInt("@cguildid");
		res3->guildInfo.guildMaster = this->plr->pcProfile->charId;
		memcpy(res3->guildInfo.wszName, req->wszGuildName, sizeof(wchar_t)* NTL_MAX_SIZE_GUILD_NAME_IN_UNICODE);
		wcscpy_s(res3->guildInfo.awchName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, s2ws(app->db->getString("@charactername")).c_str());
		packet3.SetPacketLen( sizeof(sTU_GUILD_INFO));
		rc = g_pApp->Send( this->GetHandle(), &packet3);

// GUILD MEMBER INFORMATIONS
		CNtlPacket packet4(sizeof(sTU_GUILD_MEMBER_INFO));
		sTU_GUILD_MEMBER_INFO * res4 = (sTU_GUILD_MEMBER_INFO *)packet4.GetPacketData();

		res4->wOpCode = TU_GUILD_MEMBER_INFO;
		res4->guildMemberInfo.bIsOnline = true;
		res4->guildMemberInfo.charId = this->plr->pcProfile->charId;
		wcscpy_s(res4->guildMemberInfo.wszMemberName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, s2ws(app->db->getString("@charactername")).c_str());
		packet4.SetPacketLen( sizeof(sTU_GUILD_MEMBER_INFO));
		rc = g_pApp->Send( this->GetHandle(), &packet4);
		app->UserBroadcastothers(&packet4, this);
		this->plr->SetGuildName(app->db->getString("@charactername"));
	}
}

//--------------------------------------------------------------------------------------//
//		Create Party
//--------------------------------------------------------------------------------------//
void CClientSession::SendCreatePartyReq(CNtlPacket * pPacket, CGameServer * app)
{
	PlayerPartyClass *t = new PlayerPartyClass;

	t->m_partyList.begin();
	t->CreateParty(this->GetHandle());

	sUG_PARTY_CREATE_REQ * req = (sUG_PARTY_CREATE_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_PARTY_CREATE_RES));
	sGU_PARTY_CREATE_RES * res = (sGU_PARTY_CREATE_RES *)packet.GetPacketData();

	res->wOpCode = GU_PARTY_CREATE_RES;
	res->wResultCode = GAME_SUCCESS;
	memcpy(res->wszPartyName, req->wszPartyName, sizeof(wchar_t)* NTL_MAX_SIZE_PARTY_NAME_IN_UNICODE);

	packet.SetPacketLen( sizeof(sGU_PARTY_CREATE_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );

}
//--------------------------------------------------------------------------------------//
//		Disband Party
//--------------------------------------------------------------------------------------//
void CClientSession::SendDisbandPartyReq(CNtlPacket * pPacket, CGameServer * app)
{
	//printf("--- disband party request --- \n");

	CNtlPacket packet(sizeof(sGU_PARTY_DISBAND_RES));
	sGU_PARTY_DISBAND_RES * res = (sGU_PARTY_DISBAND_RES *)packet.GetPacketData();

	res->wOpCode = GU_PARTY_DISBAND_RES;
	res->wResultCode = GAME_SUCCESS;

	packet.SetPacketLen( sizeof(sGU_PARTY_DISBAND_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );


	CNtlPacket packet2(sizeof(sGU_PARTY_DISBANDED_NFY));
	sGU_PARTY_DISBANDED_NFY * sPacket2 = (sGU_PARTY_DISBANDED_NFY *)packet2.GetPacketData();
	sPacket2->wOpCode = GU_PARTY_DISBANDED_NFY;

	packet2.SetPacketLen( sizeof(sGU_PARTY_DISBANDED_NFY));
	rc = g_pApp->Send( this->GetHandle(), &packet2);

}
//--------------------------------------------------------------------------------------//
//		Send party invite request
//--------------------------------------------------------------------------------------//
void CClientSession::SendPartyInviteReq(CNtlPacket * pPacket, CGameServer * app)
{
	//printf("--- Send party invite request --- \n");
	sUG_PARTY_INVITE_REQ * req = (sUG_PARTY_INVITE_REQ*)pPacket->GetPacketData();

	app->db->prepare("SELECT * FROM characters WHERE OnlineID = ? AND isOnline = 1");
	app->db->setInt(1, req->hTarget);
	app->db->execute(); 
	app->db->fetch(); 
	this->plr->LastPartyHandle = req->hTarget;
	//Invite player
	CNtlPacket packet(sizeof(sGU_PARTY_INVITE_RES));
	sGU_PARTY_INVITE_RES * res = (sGU_PARTY_INVITE_RES *)packet.GetPacketData();

	res->wOpCode = GU_PARTY_INVITE_RES;
	res->wResultCode = GAME_SUCCESS;
	wcscpy_s(res->wszTargetName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, s2ws(app->db->getString("CharName")).c_str());

	packet.SetPacketLen( sizeof(sGU_PARTY_INVITE_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );

	//Send invitation request to player
	CNtlPacket packet2(sizeof(sGU_PARTY_INVITE_NFY));
	sGU_PARTY_INVITE_NFY * res2 = (sGU_PARTY_INVITE_NFY *)packet2.GetPacketData();

	res2->wOpCode = GU_PARTY_INVITE_NFY;
	res2->bFromPc = true;
	wcscpy_s(res2->wszInvitorPcName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, this->plr->pcProfile->awchName);

	packet2.SetPacketLen( sizeof(sGU_PARTY_INVITE_NFY));
	app->UserBroadcastothers(&packet2, this);


	/*sUG_PARTY_RESPONSE_INVITATION * req2 = (sUG_PARTY_RESPONSE_INVITATION*)pPacket->GetPacketData();
	CNtlPacket packet3(sizeof(sGU_PARTY_RESPONSE_INVITATION_RES));
	sGU_PARTY_RESPONSE_INVITATION_RES * res3 = (sGU_PARTY_RESPONSE_INVITATION_RES *)packet3.GetPacketData();

	res3->wOpCode = GU_PARTY_RESPONSE_INVITATION_RES;
	res3->wResultCode = GAME_SUCCESS;

	packet.SetPacketLen( sizeof(sGU_PARTY_RESPONSE_INVITATION_RES) );
	rc = g_pApp->Send( this->GetHandle(), &packet3 );

	printf("response: %i ",req2->byResponse);
	if(req2->byResponse == 2) // User accepted party invite
	{
	CNtlPacket packet4(sizeof(sGU_PARTY_MEMBER_JOINED_NFY));
	sGU_PARTY_MEMBER_JOINED_NFY * res4 = (sGU_PARTY_MEMBER_JOINED_NFY *)packet4.GetPacketData();

	res2->wOpCode = GU_PARTY_MEMBER_JOINED_NFY;
	wcscpy_s(res4->memberInfo.awchMemberName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, s2ws(app->db->getString("CharName")).c_str());
	
	res4->memberInfo.byClass = app->db->getInt("Class");
	res4->memberInfo.byLevel = app->db->getInt("Level");
	res4->memberInfo.byRace = app->db->getInt("Race");
	res4->memberInfo.hHandle = app->db->getInt("OnlineID");
	res4->memberInfo.vCurLoc.x = (float)app->db->getDouble("CurLocX");
	res4->memberInfo.vCurLoc.y = (float)app->db->getDouble("CurLocY");
	res4->memberInfo.vCurLoc.z = (float)app->db->getDouble("CurLocZ");
	res4->memberInfo.wCurEP = app->db->getInt("CurEP");
	res4->memberInfo.wCurLP = app->db->getInt("CurLP");
	res4->memberInfo.wMaxEP = app->db->getInt("BaseMaxEp");
	res4->memberInfo.wMaxLP = app->db->getInt("BaseMaxLp");
	res4->memberInfo.worldId = app->db->getInt("WorldID");
	res4->memberInfo.dwZenny = app->db->getInt("Money");
	res4->memberInfo.worldTblidx = app->db->getInt("WorldTable");

	packet4.SetPacketLen( sizeof(sGU_PARTY_MEMBER_JOINED_NFY));
	rc = g_pApp->Send( this->GetHandle(), &packet );
	app->UserBroadcastothers(&packet4, this);
	//printf("user invited ");
	}*/
}
//--------------------------------------------------------------------------------------//
//		Party invitation response
//--------------------------------------------------------------------------------------//
void CClientSession::SendPartyResponse(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_PARTY_RESPONSE_INVITATION * req2 = (sUG_PARTY_RESPONSE_INVITATION*)pPacket->GetPacketData();

	CNtlPacket packet3(sizeof(sGU_PARTY_RESPONSE_INVITATION_RES));
	sGU_PARTY_RESPONSE_INVITATION_RES * res3 = (sGU_PARTY_RESPONSE_INVITATION_RES *)packet3.GetPacketData();
	res3->wOpCode = GU_PARTY_RESPONSE_INVITATION_RES;
	res3->wResultCode = GAME_FAIL;

	if(req2->byResponse == 0 && this->plr->LastPartyHandle != -1) // User accepted party invite
	{
		CNtlPacket packet4(sizeof(sGU_PARTY_MEMBER_JOINED_NFY));
		sGU_PARTY_MEMBER_JOINED_NFY * res4 = (sGU_PARTY_MEMBER_JOINED_NFY *)packet4.GetPacketData();
		res4->wOpCode = GU_PARTY_MEMBER_JOINED_NFY;
		
		app->db->prepare("SELECT * FROM characters WHERE OnlineID = ? AND isOnline = 1");
		app->db->setInt(1, this->plr->LastPartyHandle);
		app->db->execute(); 
		app->db->fetch(); 
	
		wcscpy_s(res4->memberInfo.awchMemberName, NTL_MAX_SIZE_CHAR_NAME_UNICODE, s2ws(app->db->getString("CharName")).c_str());
		printf("1");
		res4->memberInfo.byClass = app->db->getInt("Class");
		res4->memberInfo.byLevel = app->db->getInt("Level");
		res4->memberInfo.byRace = app->db->getInt("Race");
		res4->memberInfo.hHandle = app->db->getInt("OnlineID");
		res4->memberInfo.vCurLoc.x = (float)app->db->getDouble("CurLocX");
		res4->memberInfo.vCurLoc.y = (float)app->db->getDouble("CurLocY");
		res4->memberInfo.vCurLoc.z = (float)app->db->getDouble("CurLocZ");
		res4->memberInfo.wCurEP = app->db->getInt("CurEP");
		res4->memberInfo.wCurLP = app->db->getInt("CurLP");
		res4->memberInfo.wMaxEP = app->db->getInt("BaseMaxEp");
		res4->memberInfo.wMaxLP = app->db->getInt("BaseMaxLp");
		res4->memberInfo.worldId = app->db->getInt("WorldID");
		res4->memberInfo.dwZenny = app->db->getInt("Money");
		res4->memberInfo.worldTblidx = app->db->getInt("WorldTable");

		packet4.SetPacketLen( sizeof(sGU_PARTY_MEMBER_JOINED_NFY));
		g_pApp->Send( this->GetHandle(), &packet4 );
		app->UserBroadcastothers(&packet4, this);
		res3->wResultCode = GAME_SUCCESS;
	}
	packet3.SetPacketLen( sizeof(sGU_PARTY_RESPONSE_INVITATION_RES) );
	g_pApp->Send( this->GetHandle(), &packet3 );
	this->plr->LastPartyHandle = -1;
}
//--------------------------------------------------------------------------------------//
//		Leave Party
//--------------------------------------------------------------------------------------//
void CClientSession::SendPartyLeaveReq(CNtlPacket * pPacket, CGameServer * app)
{
	CNtlPacket packet(sizeof(sGU_PARTY_LEAVE_RES));
	sGU_PARTY_LEAVE_RES * res = (sGU_PARTY_LEAVE_RES *)packet.GetPacketData();

	res->wOpCode = GU_PARTY_LEAVE_RES;
	res->wResultCode = GAME_SUCCESS;

	packet.SetPacketLen( sizeof(sGU_PARTY_DISBAND_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );


	CNtlPacket packet2(sizeof(sGU_PARTY_MEMBER_LEFT_NFY));
	sGU_PARTY_MEMBER_LEFT_NFY * sPacket2 = (sGU_PARTY_MEMBER_LEFT_NFY *)packet2.GetPacketData();
	sPacket2->wOpCode = GU_PARTY_MEMBER_LEFT_NFY;
	//sPacket2->hMember = GET PARTY-MEMBER ID
	

	packet2.SetPacketLen( sizeof(sGU_PARTY_MEMBER_LEFT_NFY));
	rc = g_pApp->Send( this->GetHandle(), &packet2);

	app->UserBroadcastothers(&packet2, this);
}

//--------------------------------------------------------------------------------------//
//		Execute trigger object
//--------------------------------------------------------------------------------------//
void CClientSession::SendExcuteTriggerObject(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_TS_EXCUTE_TRIGGER_OBJECT * req = (sUG_TS_EXCUTE_TRIGGER_OBJECT*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_TS_EXCUTE_TRIGGER_OBJECT_RES));
	sGU_TS_EXCUTE_TRIGGER_OBJECT_RES * res = (sGU_TS_EXCUTE_TRIGGER_OBJECT_RES *)packet.GetPacketData();

	res->wOpCode = GU_TS_EXCUTE_TRIGGER_OBJECT_RES;
	res->wResultCode = GAME_SUCCESS;
	res->hTriggerObject = req->hTarget;
	packet.SetPacketLen( sizeof(sGU_TS_EXCUTE_TRIGGER_OBJECT_RES) );
	app->UserBroadcastothers(&packet, this);
	int rc = g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		Character bind to world
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharBindReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_CHAR_BIND_REQ * req = (sUG_CHAR_BIND_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_CHAR_BIND_RES));
	sGU_CHAR_BIND_RES * res = (sGU_CHAR_BIND_RES *)packet.GetPacketData();

	app->db->prepare("CALL CharBind (?,?, @currentWorldID)");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->setInt(2, req->bindObjectTblidx);
	app->db->execute();
	app->db->execute("SELECT @currentWorldID");
	app->db->fetch(); 

	res->wOpCode = GU_CHAR_BIND_RES;
	res->wResultCode = GAME_SUCCESS;
	res->byBindType = DBO_BIND_TYPE_FIRST;
	res->bindObjectTblidx = req->bindObjectTblidx;

	res->bindWorldId = app->db->getInt("@currentWorldID");

	packet.SetPacketLen( sizeof(sGU_CHAR_BIND_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		PORTAL START REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::SendPortalStartReq(CNtlPacket * pPacket, CGameServer * app)
{
	//printf("--- PORTAL START REQUEST --- \n");
	sUG_PORTAL_START_REQ * req = (sUG_PORTAL_START_REQ *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_PORTAL_START_RES));
	sGU_PORTAL_START_RES * res = (sGU_PORTAL_START_RES *)packet.GetPacketData();

	res->wOpCode = GU_PORTAL_START_RES;
	res->wResultCode = GAME_SUCCESS;
	res->hNpcHandle = req->handle;

	CNtlPacket packet2(sizeof(sGU_PORTAL_INFO));
	sGU_PORTAL_INFO * res2 = (sGU_PORTAL_INFO *)packet2.GetPacketData();
	
	CPortalTable* pPortalTbl = app->g_pTableContainer->GetPortalTable();
	int i = 0;
	for ( CTable::TABLEIT itPortal = pPortalTbl->Begin(); itPortal != pPortalTbl->End(); ++itPortal )
	{
		sPORTAL_TBLDAT* pPortalTblData = (sPORTAL_TBLDAT*) itPortal->second;
		if ((rand() %100) >= 60)
		{
			res2->aPortalID[i] = pPortalTblData->tblidx;
			i++;
		}
	}
	res2->wOpCode = GU_PORTAL_INFO;
	res2->byCount = i;

	packet2.SetPacketLen( sizeof(sGU_PORTAL_INFO));
	int rc = g_pApp->Send( this->GetHandle(), &packet2);
	Sleep(1);
	packet.SetPacketLen( sizeof(sGU_PORTAL_START_RES) );
	rc = g_pApp->Send( this->GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		PORTAL ADD REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::SendPortalAddReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_PORTAL_ADD_REQ* req = (sUG_PORTAL_ADD_REQ *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_PORTAL_ADD_RES));
	sGU_PORTAL_ADD_RES * res = (sGU_PORTAL_ADD_RES *)packet.GetPacketData();

	res->wOpCode = GU_PORTAL_ADD_RES;
	res->wResultCode = GAME_SUCCESS;
	res->hNpcHandle = req->handle;
	res->PortalID = 1;

	packet.SetPacketLen( sizeof(sGU_PORTAL_ADD_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );
}
void CClientSession::SendPortalTelReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_PORTAL_REQ* req = (sUG_PORTAL_REQ *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_PORTAL_RES));
	sGU_PORTAL_RES * res = (sGU_PORTAL_RES *)packet.GetPacketData();

	CNtlPacket packet2(sizeof(sGU_CHAR_TELEPORT_RES));
	sGU_CHAR_TELEPORT_RES * res2 = (sGU_CHAR_TELEPORT_RES *)packet2.GetPacketData();

	CNtlPacket packet3(sizeof(sGU_UPDATE_CHAR_STATE));
	sGU_UPDATE_CHAR_STATE * res3 = (sGU_UPDATE_CHAR_STATE *)packet3.GetPacketData();

	packet.SetPacketLen( sizeof(sGU_PORTAL_RES) );
	packet2.SetPacketLen( sizeof(sGU_CHAR_TELEPORT_RES) );
	packet3.SetPacketLen( sizeof(sGU_UPDATE_CHAR_STATE) );
	
	sPORTAL_TBLDAT* pPortalTblData = (sPORTAL_TBLDAT*)app->g_pTableContainer->GetPortalTable()->FindData(req->byPoint);
	if (req->byPoint == pPortalTblData->tblidx)
	{
		//pPortalTblData->adwPointZenny;
		//pPortalTblData->dwPointName;
		//pPortalTblData->szPointNameText;
		//pPortalTblData->vMap;
		res->vDir = pPortalTblData->vDir;
		res->vLoc = pPortalTblData->vLoc;
		res->byPoint = pPortalTblData->tblidx;
		res->worldID = pPortalTblData->worldId;
		res->wOpCode = GU_PORTAL_RES;
		res->wResultCode = GAME_SUCCESS;
		res->hNpcHandle = this->GetTargetSerialId();

		res2->bIsToMoveAnotherServer = false;
		//res2->sWorldInfo.sRuleInfo.byRuleType = GAMERULE_NORMAL;
		//res2->sWorldInfo.hTriggerObjectOffset = 100000;
		//res2->sWorldInfo.tblidx = this->plr->GetWorldTableID();
		//res2->sWorldInfo.worldID = res->worldID;
		res2->vNewDir.x = res->vDir.x;
		res2->vNewDir.y = res->vDir.y;
		res2->vNewDir.z = res->vDir.z;
		res2->vNewLoc.x = res->vLoc.x;
		res2->vNewLoc.y = res->vLoc.y;
		res2->vNewLoc.z = res->vLoc.z;
		res2->wOpCode = GU_CHAR_TELEPORT_RES;
		res2->wResultCode = GAME_SUCCESS;
		//res2->sWorldInfo.sRuleInfo.sTimeQuestRuleInfo;

		this->plr->SetPosition(res2->vNewLoc, res2->vNewDir);
		app->db->prepare("UPDATE characters SET CurLocX=? , CurLocY=? , CurLocZ=? , CurDirX=? , CurDirZ=? WHERE CharID = ?");
		app->db->setInt(1, res->vLoc.x);
		app->db->setInt(2, res->vLoc.y);
		app->db->setInt(3, res->vLoc.z);
		app->db->setInt(4, res->vDir.x);
		app->db->setInt(5, res->vDir.z);
		app->db->setInt(6, this->plr->pcProfile->charId);
		app->db->execute();

		res3->handle = this->GetavatarHandle();
		res3->wOpCode = GU_UPDATE_CHAR_STATE;
		res3->sCharState.sCharStateBase.byStateID = CHARSTATE_TELEPORTING;
		res3->sCharState.sCharStateBase.vCurLoc.x = this->plr->GetPosition().x;
		res3->sCharState.sCharStateBase.vCurLoc.y = this->plr->GetPosition().y;
		res3->sCharState.sCharStateBase.vCurLoc.z = this->plr->GetPosition().z;
		res3->sCharState.sCharStateBase.vCurDir.x = this->plr->GetDirection().x;
		res3->sCharState.sCharStateBase.vCurDir.y = this->plr->GetDirection().y;
		res3->sCharState.sCharStateBase.vCurDir.z = this->plr->GetDirection().z;
		res3->sCharState.sCharStateDetail.sCharStateTeleporting.byTeleportType = TELEPORT_TYPE_NPC_PORTAL;
		
		g_pApp->Send( this->GetHandle(), &packet );
		g_pApp->Send( this->GetHandle(), &packet2 );
		g_pApp->Send( this->GetHandle(), &packet3 );
		app->UserBroadcast(&packet);
		app->UserBroadcast(&packet2);
		//app->UserBroadcast(&packet);
	}
	else
	{
		res->wOpCode = GU_PORTAL_RES;
		res->wResultCode = GAME_PORTAL_NOT_EXIST;
		g_pApp->Send( this->GetHandle(), &packet );
		this->gsf->printError("An error is occured in SendPortalTelReq: GAME_PORTAL_NOT_EXIST");
	}
}

//--------------------------------------------------------------------------------------//
//		ATTACK BEGIN
//--------------------------------------------------------------------------------------//
void CClientSession::SendAttackBegin(CNtlPacket * pPacket, CGameServer * app)
{
	
	sUG_CHAR_ATTACK_BEGIN* req = (sUG_CHAR_ATTACK_BEGIN *)pPacket->GetPacketData();

	if(req->byType == 0)
	{
		AddAttackBegin(this->GetavatarHandle(), this->GetTargetSerialId());
		SendCharActionAttack(this->GetavatarHandle(), this->GetTargetSerialId(), pPacket);
	}
	else if(req->byType == 1)
	{
		this->gsf->printError("An error is occured in SendAttackBegin: req->byType == 1");
	}
}
//--------------------------------------------------------------------------------------//
//		ATTACK END
//---------------------------------------------------------------------------------------//
void CClientSession::SendAttackEnd(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_CHAR_ATTACK_END* req = (sUG_CHAR_ATTACK_END *)pPacket->GetPacketData();
	
	if(req->byType == 0)
	{
		RemoveAttackBegin(this->GetavatarHandle(), this->GetTargetSerialId());
	}
	else if(req->byType == 1)
	{
		this->gsf->printError("An error is occured in SendAttackEnd: req->byType == 1");
	}
}

void CClientSession::AddAttackBegin(RwUInt32 uiSerialId, RwUInt32 m_uiTargetSerialId)
{
	SBattleData *pBattleData = new SBattleData;

	pBattleData->uiSerialId			= uiSerialId;
	pBattleData->m_uiTargetSerialId	= m_uiTargetSerialId;
	pBattleData->bAttackMode		= true;
	pBattleData->dwCurrTime			= timeGetTime();

	m_listAttackBegin.push_back(pBattleData);

}

void CClientSession::RemoveAttackBegin(RwUInt32 uiSerialId, RwUInt32 m_uiTargetSerialId)
{
	SBattleData *pBattleData;
	for(BATTLEIT it = m_listAttackBegin.begin(); it != m_listAttackBegin.end(); it++)
	{
		pBattleData = (*it);
		if(pBattleData->uiSerialId == uiSerialId)
		{
			RWS_DELETE(pBattleData);
			m_listAttackBegin.erase(it);
			return;
		}
	}
}

void CClientSession::SendCharActionAttack(RwUInt32 uiSerialId, RwUInt32 m_uiTargetSerialId, CNtlPacket * pPacket)
{
	CGameServer * app = (CGameServer*)NtlSfxGetApp();
	static RwUInt8 byChainAttack = 0;
	int CurHP = 0;
	RwBool bDamageApply = true;

	CNtlPacket packet(sizeof(sGU_CHAR_ACTION_ATTACK));
	sGU_CHAR_ACTION_ATTACK * res = (sGU_CHAR_ACTION_ATTACK *)packet.GetPacketData();

	res->wOpCode = GU_CHAR_ACTION_ATTACK;
	res->hSubject = uiSerialId;
	res->hTarget = m_uiTargetSerialId;
	res->dwLpEpEventId = 255;
	res->bChainAttack = false;
	res->byBlockedAction = 255;

	if (IsMonsterInsideList(m_uiTargetSerialId) == true)
	{
		MobActivity::CreatureData *lol = app->mob->GetMobByHandle(m_uiTargetSerialId);
		if (lol != NULL)
		{
			CurHP = (RwUInt32)lol->CurLP;
		}
	}
	/*else if (IsMonsterInsideList(m_uiTargetSerialId) == false)
	{
		app->db->prepare("SELECT CurLP FROM characters WHERE onlineID = ?");
		app->db->setInt(1, m_uiTargetSerialId);
		app->db->execute();
		app->db->fetch();
		m_iCurrentHp = app->db->getInt("CurLP");
	}*/
	res->wAttackResultValue = 100;
	res->fReflectedDamage = 0;
	res->vShift.x = this->plr->GetPosition().x;
	res->vShift.y = this->plr->GetPosition().y;
	res->vShift.z = this->plr->GetPosition().z;

	res->byAttackSequence = byChainAttack;//rand()%6;

	if(res->byAttackSequence == 6)
	{
		byChainAttack = 0;
		if(rand()%2)
		{
			res->byAttackResult = BATTLE_ATTACK_RESULT_KNOCKDOWN;
		}
		else
		{
			res->byAttackResult = BATTLE_ATTACK_RESULT_SLIDING;
		}
	}
	else
	{
		RwInt32 iRandValue = rand()%5;
		if(iRandValue <= 2)
		{
			res->byAttackResult = BATTLE_ATTACK_RESULT_HIT;
			bDamageApply = true;
		}
		else if(iRandValue == 3)
		{
			bDamageApply = false;
			res->byAttackResult = BATTLE_ATTACK_RESULT_DODGE;
		}
		else
		{
			bDamageApply = false;
			res->byAttackResult = BATTLE_ATTACK_RESULT_BLOCK;
		}
	}
	packet.SetPacketLen( sizeof(sGU_CHAR_ACTION_ATTACK) );
	int rc = g_pApp->Send( this->GetHandle() , &packet );
	app->UserBroadcast(&packet);
	byChainAttack++;
	// update LP
	if(bDamageApply == true)
	{
		this->gsf->printOk("OK");
		CurHP -= res->wAttackResultValue;
	}
	else if (bDamageApply == false)
		this->gsf->printError("DAMAGES NOT SENDED");
	else
		this->gsf->printError("WHAT THE FUCK");
	if (CurHP <= 0)
	{
		CClientSession::SendMobLoot(&packet, app, m_uiTargetSerialId);
		this->gsf->printOk("DIE MOTHER FUCKER");
		CurHP = 0;
		SendCharUpdateFaintingState(pPacket, app, uiSerialId, m_uiTargetSerialId);
	}
	else
		SendCharUpdateLp(pPacket, app, CurHP, m_uiTargetSerialId);
}

void CClientSession::SendCharUpdateLp(CNtlPacket * pPacket, CGameServer * app, RwUInt16 wLp, RwUInt32 m_uiTargetSerialId)
{
	CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_LP));
	sGU_UPDATE_CHAR_LP * res = (sGU_UPDATE_CHAR_LP *)packet.GetPacketData();

	res->wOpCode = GU_UPDATE_CHAR_LP;
	res->handle = m_uiTargetSerialId;
	if (IsMonsterInsideList(m_uiTargetSerialId) == true)
	{
		MobActivity::CreatureData *lol = app->mob->GetMobByHandle(m_uiTargetSerialId);
		if (lol != NULL)
		{
			printf("wLP: %d, lol->CurLP: %d\n", wLp, lol->CurLP);
			lol->FightMode = true;
			lol->CurLP = (WORD)wLp;
			res->wCurLP = lol->CurLP;
			res->wMaxLP = lol->MaxLP;
			res->dwLpEpEventId = 255;
			packet.SetPacketLen( sizeof(sGU_UPDATE_CHAR_LP) );
			app->UserBroadcastothers(&packet, this);
			g_pApp->Send( this->GetHandle() , &packet );
			printf("wLP: %d, lol->CurLP: %d\n", wLp, lol->CurLP);
		}
	}
	/*else if (IsMonsterInsideList(m_uiTargetSerialId) == false)
	{
		this->gsf->printOk("PAS OK");
		PlayerInfos *targetPlr = NULL;
		app->GetUserSession(m_uiTargetSerialId, targetPlr);
		if (targetPlr)
		{
			res->wCurLP = targetPlr->pcProfile->wCurLP = wLp;
			res->wMaxLP = targetPlr->pcProfile->avatarAttribute.wLastMaxLP;
			packet.SetPacketLen( sizeof(sGU_UPDATE_CHAR_LP) );

			app->UserBroadcastothers(&packet, this);
			g_pApp->Send( this->GetHandle() , &packet );
			delete targetPlr;
		}
	}*/
}
void	CClientSession::SendMobLoot(CNtlPacket * pPacket, CGameServer * app, RwUInt32 m_uiTargetSerialId)
{
	CNtlPacket packet(sizeof(sGU_OBJECT_CREATE));
	sGU_OBJECT_CREATE * res = (sGU_OBJECT_CREATE *)packet.GetPacketData();

	CNtlPacket packet2(sizeof(sGU_OBJECT_CREATE));
	sGU_OBJECT_CREATE * res2 = (sGU_OBJECT_CREATE *)packet2.GetPacketData();

	int mobid = 0;

	if ((mobid = IsMonsterIDInsideList(m_uiTargetSerialId)) != 0)
	{
		sMOB_TBLDAT* mob = (sMOB_TBLDAT*)app->g_pTableContainer->GetMobTable()->FindData(mobid);
		/*printf("Each rate control %d\n", mob->byDropEachRateControl);
		printf("Drop Eeach Item %d\n", mob->byDropEItemRateControl);
		printf("Drop Legendary Item %d\n", mob->byDropLItemRateControl);
		printf("Drop Normal Item %d\n", mob->byDropNItemRateControl);
		printf("Drop Superior Item %d\n", mob->byDropSItemRateControl);
		printf("Drop Type %d\n", mob->byDropTypeRateControl);
		printf("Drop each tblidx %d\n", mob->dropEachTblidx);
		printf("Drop Quest tblidx %d\n", mob->dropQuestTblidx);
		printf("Drop Type TBLIDX %d\n", mob->dropTypeTblidx);
		printf("Drop Item TBLIDX %d\n", mob->drop_Item_Tblidx);*/
		CEachDropTable * edrop = app->g_pTableContainer->GetEachDropTable();
		CNormalDropTable * ndrop = app->g_pTableContainer->GetNormalDropTable();
		CSuperiorDropTable * sdrop = app->g_pTableContainer->GetSuperiorDropTable();
		CLegendaryDropTable * ldrop = app->g_pTableContainer->GetLegendaryDropTable();

		if( (rand() % 100)  >= mob->fDrop_Zenny_Rate)
		{
			res->handle = AcquireSerialId();
			res->sObjectInfo.objType = OBJTYPE_DROPMONEY;
			res->sObjectInfo.moneyBrief.dwZenny = mob->dwDrop_Zenny;
			res->sObjectInfo.moneyState.bIsNew = true;
			sVECTOR3 mypos = this->plr->GetPosition();
			res->sObjectInfo.moneyState.vCurLoc.x = mypos.x + rand() % 2;
			res->sObjectInfo.moneyState.vCurLoc.y = mypos.y;
			res->sObjectInfo.moneyState.vCurLoc.z = mypos.z + rand() % 2;
			res->wOpCode = GU_OBJECT_CREATE;
		
			packet.SetPacketLen( sizeof(sGU_OBJECT_CREATE) );
			g_pApp->Send( this->GetHandle() , &packet );
			app->AddNewZennyAmount(res->handle, mob->dwDrop_Zenny);
		}
			/*res2->handle = AcquireSerialId();
			res2->sObjectInfo.objType = OBJTYPE_DROPITEM;
			res2->sObjectInfo.itemBrief.tblidx = mob->drop_Item_Tblidx;
			res2->sObjectInfo.itemBrief.byGrade = 1;
			res2->sObjectInfo.itemBrief.byRank = 1;
			res2->sObjectInfo.itemState.bIsNew = true;
			res2->sObjectInfo.itemState.vCurLoc = this->plr->GetPosition();
			res2->wOpCode = GU_OBJECT_CREATE;
		
			packet2.SetPacketLen( sizeof(sGU_OBJECT_CREATE) );
			g_pApp->Send( this->GetHandle() , &packet2 );*/
		
		if (this->plr->pcProfile->byLevel < 50)
			CClientSession::SendPlayerLevelUpCheck(app, mob->wExp);
	}
}
void CClientSession::SendCharUpdateFaintingState(CNtlPacket * pPacket, CGameServer * app, RwUInt32 uiSerialId, RwUInt32 m_uiTargetSerialId)
{
	RemoveAttackBegin(uiSerialId, m_uiTargetSerialId);

	CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_STATE));
	sGU_UPDATE_CHAR_STATE * res = (sGU_UPDATE_CHAR_STATE *)packet.GetPacketData();

	res->wOpCode = GU_UPDATE_CHAR_STATE;
	res->handle = m_uiTargetSerialId;
	res->sCharState.sCharStateBase.byStateID = CHARSTATE_FAINTING;
	res->sCharState.sCharStateBase.vCurLoc.x = this->plr->GetPosition().x;
	res->sCharState.sCharStateBase.vCurLoc.y = this->plr->GetPosition().y;
	res->sCharState.sCharStateBase.vCurLoc.z = this->plr->GetPosition().z;
	res->sCharState.sCharStateBase.vCurDir.x = this->plr->GetDirection().x;
	res->sCharState.sCharStateBase.vCurDir.y = this->plr->GetDirection().y;
	res->sCharState.sCharStateBase.vCurDir.z = this->plr->GetDirection().z;

	packet.SetPacketLen( sizeof(sGU_UPDATE_CHAR_STATE) );
	app->UserBroadcastothers(&packet, this);
	g_pApp->Send( this->GetHandle() , &packet );
	app->mob->UpdateDeathStatus(m_uiTargetSerialId, true);
}

void CClientSession::SendCharSkillRes(CNtlPacket * pPacket, CGameServer * app)
{
	SSkillData *pSkillData = new SSkillData();

	sUG_CHAR_SKILL_REQ *pCharSkillReq = (sUG_CHAR_SKILL_REQ*)pPacket->GetPacketData();

	app->db->prepare("SELECT * FROM skills WHERE owner_id=? AND SlotID= ? ");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->setInt(2, pCharSkillReq->bySlotIndex);
	app->db->execute();
	app->db->fetch();
	int skillID = app->db->getInt("skill_id");
	CSkillTable * skillTable = app->g_pTableContainer->GetSkillTable();
 	sSKILL_TBLDAT * skillDataOriginal = reinterpret_cast<sSKILL_TBLDAT*>(skillTable->FindData(skillID));
	pSkillData->pCharSkillTarget = pCharSkillReq->hTarget;
	pSkillData->m_uiSkillTblId = skillID;	//GetSkillId[m_Char.iClass][pCharSkillReq->bySlotIndex];
	pSkillData->m_bySkillActiveType = SKILL_TYPE_CASTING; 
	pSkillData->m_uiSkillTime = timeGetTime();

	CNtlPacket packet(sizeof(sGU_CHAR_SKILL_RES));
	sGU_CHAR_SKILL_RES * sPacket = (sGU_CHAR_SKILL_RES *)packet.GetPacketData();

	sPacket->wOpCode = GU_CHAR_SKILL_RES;
	sPacket->wResultCode = GAME_SUCCESS;

	packet.SetPacketLen(sizeof(sGU_CHAR_SKILL_RES));
	int rc = g_pApp->Send(this->GetHandle(), &packet);
	app->UserBroadcast(&packet);
	if ((skillDataOriginal->dwKeepTimeInMilliSecs != 0) || (skillDataOriginal->dwTransform_Use_Info_Bit_Flag==1))
		SendCharSkillCasting(pPacket, app, skillID);
 	else
 		SendCharSkillAction(pPacket, app, skillID);
 		
}
//--------------------------------------------------------------------------------------//
//		Char Skill Send
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharSkillAction(CNtlPacket * pPacket, CGameServer * app, int _skillID)
{
	CNtlPacket packet(sizeof(sGU_CHAR_ACTION_SKILL));
	sGU_CHAR_ACTION_SKILL * res = (sGU_CHAR_ACTION_SKILL *)packet.GetPacketData();	
	
	CSkillTable *pSkillTbl = app->g_pTableContainer->GetSkillTable();
	
	int skillID = _skillID;

	sSKILL_TBLDAT *pSkillTblData = reinterpret_cast<sSKILL_TBLDAT*>(pSkillTbl->FindData(skillID));
	res->wOpCode = GU_CHAR_ACTION_SKILL;
	res->handle = this->GetavatarHandle();
	res->wResultCode = GAME_SUCCESS;
	res->hAppointedTarget = this->GetTargetSerialId();
	res->skillId = skillID;
	res->dwLpEpEventId = skillID;
	res->bySkillResultCount = 1;
	res->aSkillResult[0].hTarget = this->GetTargetSerialId();
	res->aSkillResult[0].byAttackResult = BATTLE_ATTACK_RESULT_HIT;
	res->aSkillResult[0].effectResult1.fResultValue = pSkillTblData->fSkill_Effect_Value[0];
	res->aSkillResult[0].effectResult2.fResultValue = pSkillTblData->fSkill_Effect_Value[1];
	res->aSkillResult[0].byBlockedAction = 255;
	res->aSkillResult[1].hTarget = this->GetTargetSerialId() +1;
	res->aSkillResult[1].byAttackResult = BATTLE_ATTACK_RESULT_HIT;
	res->aSkillResult[1].effectResult1.fResultValue = pSkillTblData->fSkill_Effect_Value[0];
	res->aSkillResult[1].effectResult2.fResultValue = pSkillTblData->fSkill_Effect_Value[1];
	res->aSkillResult[1].byBlockedAction = 255;
	packet.SetPacketLen(sizeof(sGU_CHAR_ACTION_SKILL));
	int rc = g_pApp->Send(this->GetHandle(), &packet);
	app->UserBroadcastothers(&packet, this);
	
	float newLP = 0;
	if (IsMonsterInsideList(res->hAppointedTarget) == true)
	{
		MobActivity::CreatureData *lol = app->mob->GetMobByHandle(res->hAppointedTarget);
		if (lol != NULL)
		{
			lol->FightMode = true;
			newLP = (float)lol->CurLP;
			newLP -= res->aSkillResult[0].effectResult1.fResultValue;
			printf("LP: %f, damage: %f\n", newLP, res->aSkillResult[0].effectResult1.fResultValue);
			if (newLP <= 0 || (newLP > lol->MaxLP))
			{
				lol->IsDead = true;
				CClientSession::SendMobLoot(&packet, app, res->hAppointedTarget);
				this->gsf->printOk("DIE MOTHER FUCKER");
				SendCharUpdateFaintingState(pPacket, app, this->GetavatarHandle(), res->hAppointedTarget);
			}
			else if (newLP > 0 && lol->IsDead == false)
			{
				SendCharUpdateLp(pPacket, app, newLP, res->hAppointedTarget);
			}
		}
	}
}


//-------------------------------------------------------------------//
//----------Fixed Casting Buff/Transform Skills - Luiz45-------------//
//-------------------------------------------------------------------//
void CClientSession::SendCharSkillCasting(CNtlPacket * pPacket, CGameServer * app, int _skillID)
{
	printf("USE SKILL\n");
	//Skill Events Prepare
 	CNtlPacket packet(sizeof(sGU_CHAR_ACTION_SKILL));
 	CSkillTable *pSkillTbl = app->g_pTableContainer->GetSkillTable();
 	int skillID = _skillID;
 	sSKILL_TBLDAT *pSkillTblData = reinterpret_cast<sSKILL_TBLDAT*>(pSkillTbl->FindData(skillID));
 	sGU_CHAR_ACTION_SKILL * res = (sGU_CHAR_ACTION_SKILL *)packet.GetPacketData();
 	
 	res->skillId = pSkillTblData->tblidx;
 	res->wResultCode = GAME_SUCCESS;
 	res->wOpCode = GU_CHAR_ACTION_SKILL;
 	res->handle = this->GetavatarHandle();//My Handle
 	res->hAppointedTarget = this->GetTargetSerialId();//Get myself

	//Buff Events Prepare
 	CNtlPacket packet2(sizeof(sGU_BUFF_REGISTERED));
 	sGU_BUFF_REGISTERED * pBuffData = (sGU_BUFF_REGISTERED*)packet2.GetPacketData();
 	pBuffData->wOpCode = GU_BUFF_REGISTERED;
 	pBuffData->tblidx = pSkillTblData->tblidx;
 	pBuffData->hHandle = this->GetavatarHandle();
	pBuffData->dwInitialDuration = pSkillTblData->dwKeepTimeInMilliSecs;
	pBuffData->dwTimeRemaining = pSkillTblData->dwCoolTimeInMilliSecs;
 	pBuffData->afEffectValue[0] = pSkillTblData->fSkill_Effect_Value[0];
 	pBuffData->afEffectValue[1] = pSkillTblData->fSkill_Effect_Value[1];
 	//pBuffData->bNeedToDisplayMessage = pSkillTblData->bDefaultDisplayOff;//Only to display...String ID bla bla not found...if you want see uncomment this line
 	pBuffData->bySourceType = DBO_OBJECT_SOURCE_SKILL;


	packet.SetPacketLen(sizeof(sGU_CHAR_ACTION_SKILL));
  	packet2.SetPacketLen(sizeof(sGU_BUFF_REGISTERED));  	
 	g_pApp->Send(this->GetHandle(), &packet2);
 	g_pApp->Send(this->GetHandle(), &packet);
 	app->UserBroadcastothers(&packet2, this);
  	app->UserBroadcastothers(&packet, this);
}

void CGameServer::UpdateClient(CNtlPacket * pPacket, CClientSession * pSession)
{
	CGameServer * app = (CGameServer*) NtlSfxGetApp();
	
// BASIC ATTACK
	SBattleData *pBattleData;

	for(BATTLEIT it = m_listAttackBegin.begin(); it != m_listAttackBegin.end(); it++)
	{
		pBattleData = (*it);
		if(timeGetTime() - pBattleData->dwCurrTime >= MONSTER_ATTACK_UPDATE_TICK)
		{
			//app->pSession->SendCharActionAttack(pBattleData->uiSerialId, pBattleData->m_uiTargetSerialId, pPacket);
			pBattleData->dwCurrTime = timeGetTime();
		}
	}

// MAKE MOBS MOVE
	if(timeGetTime() - app->mob->last_mobMove >= MONSTER_MOVE_UPDATE_TICK)
	{
		//app->mob->MonsterRandomWalk(NULL);
		//app->mob->last_mobMove = timeGetTime();
	}
}

//--------------------------------------------------------------------------------------//
//		Char toggle fight
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharToggleFighting(CNtlPacket * pPacket, CGameServer * app)
{
//CHAR_TOGG_FIGHTING
//	app->mob->AddToWorld(pPacket, this);
	/*CNtlPacket packet(sizeof(sUG_CHAR_TOGG_FIGHTING));
	sUG_CHAR_TOGG_FIGHTING * req = (sUG_CHAR_TOGG_FIGHTING *)packet.GetPacketData();

	CNtlPacket packet2(sizeof(sGU_UPDATE_CHAR_STATE));
	sGU_UPDATE_CHAR_STATE * res = (sGU_UPDATE_CHAR_STATE *)packet2.GetPacketData();

	/*req->byAvatarType = DBO_AVATAR_TYPE_AVATAR;
	req->bFightMode = true;
	req->wOpCode = UG_CHAR_TOGG_FIGHTING;
	packet.SetPacketLen(sizeof(sUG_CHAR_TOGG_FIGHTING));
	g_pApp->Send(this->GetHandle(), &packet);

	res->handle = this->plr->GetAvatarandle();
	res->sCharState.sCharStateBase.bFightMode = true;
	res->wOpCode = GU_UPDATE_CHAR_STATE;
	packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_STATE));
	g_pApp->Send(this->GetHandle(), &packet2);

	*/
}

//--------------------------------------------------------------------------------------//
//		SKILL SHOP REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::SendShopSkillReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_SHOP_SKILL_BUY_REQ * req = (sUG_SHOP_SKILL_BUY_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_SHOP_SKILL_BUY_RES));
	sGU_SHOP_SKILL_BUY_RES * res = (sGU_SHOP_SKILL_BUY_RES *)packet.GetPacketData();

	res->hNpchandle = req->hNpchandle;
	res->wOpCode = GU_SHOP_SKILL_BUY_RES;
	res->wResultCode  = GAME_SUCCESS;

	packet.SetPacketLen( sizeof(sGU_SHOP_SKILL_BUY_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		Char learn skill
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharLearnSkillReq(CNtlPacket * pPacket, CGameServer * app)
{
	WORD		skill_learn_result;
// BUY SKILL FROM SHOP
	sUG_SHOP_SKILL_BUY_REQ * req0 = (sUG_SHOP_SKILL_BUY_REQ*)pPacket->GetPacketData();

	CNtlPacket packet0(sizeof(sGU_SHOP_SKILL_BUY_RES));
	sGU_SHOP_SKILL_BUY_RES * res0 = (sGU_SHOP_SKILL_BUY_RES *)packet0.GetPacketData();

	res0->wOpCode = GU_SHOP_SKILL_BUY_RES;
	res0->hNpchandle = req0->hNpchandle;
	res0->wResultCode  = GAME_SUCCESS;

	packet0.SetPacketLen( sizeof(sGU_SHOP_SKILL_BUY_RES) );
	int rc = g_pApp->Send( this->GetHandle(), &packet0 );

// LEARN SKILL
	sUG_SKILL_LEARN_REQ * req = (sUG_SKILL_LEARN_REQ*)pPacket->GetPacketData();

	CSkillTable* pSkillTable = app->g_pTableContainer->GetSkillTable();
	CMerchantTable* pSkillMasterItemTable = app->g_pTableContainer->GetMerchantTable();
	CNPCTable* pNpcTable = app->g_pTableContainer->GetNpcTable();
	size_t iSkillCount = this->gsf->GetTotalSlotSkill(this->plr->pcProfile->charId);
	for ( CTable::TABLEIT itNPCSpawn = pNpcTable->Begin(); itNPCSpawn != pNpcTable->End(); ++itNPCSpawn )
	{
		sNPC_TBLDAT* pNPCtData = (sNPC_TBLDAT*) itNPCSpawn->second;
		if(pNPCtData->tblidx == req0->hNpchandle)
		{
			sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*) pSkillMasterItemTable->FindData( pNPCtData->amerchant_Tblidx[req0->byMerchantTab] );
			if( pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_SKILL )
			{
				for( RwInt32 j = 0 ; j < NTL_MAX_MERCHANT_COUNT ; ++j )
				{
					if( pMerchantData->aitem_Tblidx[j] == INVALID_TBLIDX )
					{
						skill_learn_result = 1004;
						break;
					}

					if(req0->byPos == j)
					{
						sSKILL_TBLDAT* pSkillData = (sSKILL_TBLDAT*) pSkillTable->FindData( pMerchantData->aitem_Tblidx[j] );
						if(app->qry->CheckIfSkillAlreadyLearned(pSkillData->tblidx, this->plr->pcProfile->charId) == false)
						{
							if(this->plr->pcProfile->dwZenny >= pSkillData->dwRequire_Zenny)
							{
								if(this->plr->pcProfile->byLevel >= pSkillData->byRequire_Train_Level)
								{
									if(this->plr->pcProfile->dwSpPoint >= pSkillData->wRequireSP)
									{
										skill_learn_result = 500;
										// Skill learned notification
										CNtlPacket packet2(sizeof(sGU_SKILL_LEARNED_NFY));
										sGU_SKILL_LEARNED_NFY * res2 = (sGU_SKILL_LEARNED_NFY *)packet2.GetPacketData();
										iSkillCount++;//don't send this directly if you learned SSJ first...because we always are sending 1...nevermind :P
										res2->wOpCode = GU_SKILL_LEARNED_NFY;
										res2->skillId = pSkillData->tblidx;
										res2->bySlot = iSkillCount;
										app->qry->InsertNewSkill(pSkillData->tblidx, this->plr->pcProfile->charId, iSkillCount, pSkillData->wKeep_Time, pSkillData->wNext_Skill_Train_Exp);
										this->plr->pcProfile->dwZenny -= pSkillData->dwRequire_Zenny;
										this->plr->pcProfile->dwSpPoint -= pSkillData->wRequireSP;

										packet2.SetPacketLen( sizeof(sGU_SKILL_LEARNED_NFY) );
										g_pApp->Send( this->GetHandle() , &packet2 );
										app->qry->UpdateSPPoint(this->plr->pcProfile->charId, this->plr->pcProfile->dwSpPoint);
										CNtlPacket packet3(sizeof(sGU_UPDATE_CHAR_SP));
 										sGU_UPDATE_CHAR_SP * res3 = (sGU_UPDATE_CHAR_SP *)packet3.GetPacketData();
 										res3->wOpCode = GU_UPDATE_CHAR_SP;
 										res3->dwSpPoint = this->plr->pcProfile->dwSpPoint;
 										packet3.SetPacketLen(sizeof(sGU_UPDATE_CHAR_SP));
 										g_pApp->Send(this->GetHandle(), &packet3);
										app->qry->SetMinusMoney(this->plr->pcProfile->charId, pSkillData->dwRequire_Zenny);
										CNtlPacket packet4(sizeof(sGU_UPDATE_CHAR_ZENNY));
										sGU_UPDATE_CHAR_ZENNY * res4 = (sGU_UPDATE_CHAR_ZENNY *)packet4.GetPacketData();
										res4->bIsNew = true;
										res4->byChangeType = 0;
										res4->dwZenny = this->plr->pcProfile->dwZenny;
										res4->handle = this->GetavatarHandle();
										res4->wOpCode = GU_UPDATE_CHAR_ZENNY;
										packet4.SetPacketLen(sizeof(sGU_UPDATE_CHAR_ZENNY));
 										g_pApp->Send(this->GetHandle(), &packet4);
										break;
								}else
								skill_learn_result = 645;
								break;

							}else
								skill_learn_result = 501;
								break;
						}else
							skill_learn_result = 617;
							break;
					  }else
						skill_learn_result = 620;
						break;
					}
				}
			}
		}
	}

//LEARN SKILL RESULT
	CNtlPacket packet(sizeof(sGU_SKILL_LEARN_RES));
	sGU_SKILL_LEARN_RES * res = (sGU_SKILL_LEARN_RES *)packet.GetPacketData();

	res->wOpCode = GU_SKILL_LEARN_RES;
	res->wResultCode = skill_learn_result;

	packet.SetPacketLen( sizeof(sGU_SKILL_LEARN_RES) );
	g_pApp->Send( this->GetHandle() , &packet );

}


//--------------------------------------------------------------------------------------//
//		MOVE ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::SendItemMoveReq(CNtlPacket * pPacket, CGameServer * app)
{
    sUG_ITEM_MOVE_REQ * req = (sUG_ITEM_MOVE_REQ*)pPacket->GetPacketData();

    CNtlPacket packet(sizeof(sGU_ITEM_MOVE_RES));
    sGU_ITEM_MOVE_RES * res = (sGU_ITEM_MOVE_RES *)packet.GetPacketData();
	
	//For equipment
	CNtlPacket eqPak(sizeof(sGU_UPDATE_ITEM_EQUIP));
    sGU_UPDATE_ITEM_EQUIP * eq = (sGU_UPDATE_ITEM_EQUIP *)eqPak.GetPacketData();
	
	app->db->prepare("SELECT * FROM items WHERE owner_id=? AND place=? AND pos=?");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->setInt(2, req->bySrcPlace);
	app->db->setInt(3, req->bySrcPos);
	app->db->execute();
	app->db->fetch();
	RwUInt32 uniqueID = app->db->getInt("id");
	RwUInt32 ID = app->db->getInt("tblidx");
	
	
	if((app->qry->CheckIfCanMoveItemThere(this->plr->pcProfile->charId, req->byDestPlace, req->byDestPos) == false))
	{
		res->wResultCode = GAME_MOVE_CANT_GO_THERE;
		this->gsf->printError("An error is occured in SendItemMoveReq: GAME_MOVE_CANT_GO_THERE");
	} 
	else 
	{
		if (req->byDestPlace == 7 || req->bySrcPlace == 7)
		{
			CItemTable *itemTbl = app->g_pTableContainer->GetItemTable();
			sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*) itemTbl->FindData(ID);
			if (this->plr->pcProfile->byLevel >= pItemData->byNeed_Level)
			{
				res->wResultCode = GAME_SUCCESS;
				app->qry->UpdateItemPlaceAndPos(uniqueID, req->byDestPlace, req->byDestPos);
				if(req->byDestPlace == 7)
				{
				eq->byPos = req->byDestPos;
				eq->handle = this->plr->GetAvatarandle();
				eq->sItemBrief.tblidx = ID;
				eq->wOpCode = GU_UPDATE_ITEM_EQUIP;
				this->plr->calculeMyStat(app);
				}
				if(req->bySrcPlace == 7)
				{
					eq->handle = this->plr->GetAvatarandle();
					eq->sItemBrief.tblidx = INVALID_TBLIDX;
					eq->byPos = req->bySrcPos;
					eq->wOpCode = GU_UPDATE_ITEM_EQUIP;
					this->plr->calculeMyStat(app);
				}

			}
			else
			{
				res->wResultCode = GAME_MOVE_CANT_GO_THERE;
				this->gsf->printError("An error is occured in SendItemMoveReq: Level < LevelRequiert");
			}
		}
		else
		{
			app->qry->UpdateItemPlaceAndPos(uniqueID, req->byDestPlace, req->byDestPos);
			res->wResultCode = GAME_SUCCESS;
		}
	}
	res->wOpCode = GU_ITEM_MOVE_RES;
	res->hSrcItem = uniqueID;
	res->bySrcPlace = req->bySrcPlace;
	res->bySrcPos = req->bySrcPos;
	res->hDestItem = -1;
	res->byDestPlace = req->byDestPlace;
	res->byDestPos = req->byDestPos;
	packet.SetPacketLen(sizeof(sGU_ITEM_MOVE_RES));
	g_pApp->Send( this->GetHandle() , &packet );
	eqPak.SetPacketLen(sizeof(sGU_UPDATE_ITEM_EQUIP));
	g_pApp->Send( this->GetHandle(), &eqPak);
	app->UserBroadcastothers(&eqPak, this);
}

//--------------------------------------------------------------------------------------//
//		DELETE ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::SendItemDeleteReq(CNtlPacket * pPacket, CGameServer * app)
{
// GET DELETE ITEM
	sUG_ITEM_DELETE_REQ * req = (sUG_ITEM_DELETE_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_ITEM_DELETE_RES));
	sGU_ITEM_DELETE_RES * res = (sGU_ITEM_DELETE_RES *)packet.GetPacketData();
	
	app->db->prepare("SELECT id,place,pos FROM items WHERE owner_id=? AND place=? AND pos=?");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->setInt(2, req->bySrcPlace);
	app->db->setInt(3, req->bySrcPos);
	app->db->execute();
	app->db->fetch();

	RwUInt32 u_itemid = app->db->getInt("id");
	RwUInt32 item_place = app->db->getInt("place");
	RwUInt32 item_pos = app->db->getInt("pos");

	res->wOpCode = GU_ITEM_DELETE_RES;
	res->wResultCode = GAME_SUCCESS;
	res->byPlace = req->bySrcPlace;
	res->byPos = req->bySrcPos;

	packet.SetPacketLen( sizeof(sGU_ITEM_DELETE_RES) );
	g_pApp->Send( this->GetHandle() , &packet );

// DELETE ITEM
	app->qry->DeleteItemById(u_itemid);

	CNtlPacket packet2(sizeof(sGU_ITEM_DELETE));
	sGU_ITEM_DELETE * res2 = (sGU_ITEM_DELETE *)packet2.GetPacketData();

	res2->bySrcPlace = item_place;
	res2->bySrcPos = item_pos;
	res2->hSrcItem = u_itemid;
	res2->wOpCode = GU_ITEM_DELETE;

	packet2.SetPacketLen( sizeof(sGU_ITEM_DELETE) );
	g_pApp->Send( this->GetHandle() , &packet2 );
}

//--------------------------------------------------------------------------------------//
//		STACK ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::SendItemStackReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_ITEM_MOVE_STACK_REQ * req = (sUG_ITEM_MOVE_STACK_REQ*)pPacket->GetPacketData();

// GET DATA FROM MYSQL
	app->db->prepare("SELECT id,tblidx FROM items WHERE owner_id=? AND place=? AND pos=?");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->setInt(2, req->bySrcPlace);
	app->db->setInt(3, req->bySrcPos);
	app->db->execute();
	app->db->fetch();
	unsigned int uniqueID = app->db->getInt("id");
	unsigned int item1ID = app->db->getInt("tblidx");

	app->db->prepare("SELECT id,count,tblidx FROM items WHERE owner_id=? AND place=? AND pos=?");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->setInt(2, req->byDestPlace);
	app->db->setInt(3, req->byDestPos);
	app->db->execute();
	app->db->fetch();
	unsigned int uniqueID2 = app->db->getInt("id");
	unsigned int item2ID = app->db->getInt("tblidx");

	if (item1ID == item2ID)
	{
		// UPDATE ITEMS
		CNtlPacket packet(sizeof(sGU_ITEM_MOVE_STACK_RES));
		sGU_ITEM_MOVE_STACK_RES * res = (sGU_ITEM_MOVE_STACK_RES *)packet.GetPacketData();

		CNtlPacket packet1(sizeof(sGU_ITEM_DELETE));
		sGU_ITEM_DELETE * res1 = (sGU_ITEM_DELETE *)packet1.GetPacketData();

		res->wOpCode = GU_ITEM_MOVE_STACK_RES;
		res->bySrcPlace = req->bySrcPlace;
		res->bySrcPos = req->bySrcPos;
		res->byDestPlace = req->byDestPlace;
		res->byDestPos = req->byDestPos;
		res->hSrcItem = uniqueID;
		res->hDestItem = uniqueID2;
		res->byStackCount1 = req->byStackCount;
		res->byStackCount2 = req->byStackCount + app->db->getInt("count");
		res->wResultCode = GAME_SUCCESS;

		res1->bySrcPlace = req->bySrcPlace;
		res1->bySrcPos = req->bySrcPos;
		res1->hSrcItem = uniqueID;
		res1->wOpCode = GU_ITEM_DELETE;

	// UPDATE AND DELETE
		app->db->prepare("UPDATE items SET count=? WHERE id=?");
		app->db->setInt(1, res->byStackCount2);
		app->db->setInt(2, uniqueID2);
		app->db->execute();

		app->db->prepare("DELETE FROM items WHERE id=?");
		app->db->setInt(1, uniqueID);
		app->db->execute();


	// Send packet to client
		packet.SetPacketLen(sizeof(sGU_ITEM_MOVE_STACK_RES));
		g_pApp->Send( this->GetHandle() , &packet );
		packet1.SetPacketLen(sizeof(sGU_ITEM_DELETE));
		g_pApp->Send( this->GetHandle() , &packet1 );
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_ITEM_MOVE_STACK_RES));
		sGU_ITEM_MOVE_STACK_RES * res = (sGU_ITEM_MOVE_STACK_RES *)packet.GetPacketData();
		res->wOpCode = GU_ITEM_MOVE_STACK_RES;
		res->wResultCode = GAME_FAIL;
		packet.SetPacketLen(sizeof(sGU_ITEM_MOVE_STACK_RES));
		g_pApp->Send( this->GetHandle() , &packet );
		this->gsf->printError("An error is occured in SendItemStackReq: GAME_FAIL item1ID != item2ID");
	}
}

void CClientSession::SendShopStartReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_SHOP_START_REQ * req = (sUG_SHOP_START_REQ *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_SHOP_START_RES));
	sGU_SHOP_START_RES * res = (sGU_SHOP_START_RES *)packet.GetPacketData();

	res->wOpCode = GU_SHOP_START_RES;
	res->wResultCode = GAME_SUCCESS;
	res->handle = req->handle;
	res->byType = 1;

	packet.SetPacketLen( sizeof(sGU_SHOP_START_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
void CClientSession::SendShopBuyReq(CNtlPacket * pPacket, CGameServer * app)
{
	WORD		buy_item_result;

	sUG_SHOP_BUY_REQ * req = (sUG_SHOP_BUY_REQ *)pPacket->GetPacketData();
	CMerchantTable* pMerchantItemTable = app->g_pTableContainer->GetMerchantTable();
	CItemTable *itemTbl = app->g_pTableContainer->GetItemTable();
	CNPCTable* pNpcTable = app->g_pTableContainer->GetNpcTable();
	for ( CTable::TABLEIT itNPCSpawn = pNpcTable->Begin(); itNPCSpawn != pNpcTable->End(); ++itNPCSpawn )
	{
		sNPC_TBLDAT* pNPCtData = (sNPC_TBLDAT*) itNPCSpawn->second;
		if(pNPCtData->tblidx == req->handle)
		{
			sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*) pMerchantItemTable->FindData( pNPCtData->amerchant_Tblidx[req->sBuyData[0].byMerchantTab] );
			if( pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_ITEM )
			{
				for( RwInt32 j = 0 ; j < NTL_MAX_MERCHANT_COUNT ; j++ )
				{
					if( pMerchantData->aitem_Tblidx[j] == INVALID_TBLIDX )
					{
						buy_item_result = 501;
						break;
					}
					if (j == 0)
						j = req->sBuyData[j].byItemPos;
					for(int l = 12; l >= 0; l--)
					{
						if(req->sBuyData[l].byItemPos == j)
						{
							sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*) itemTbl->FindData( pMerchantData->aitem_Tblidx[j] );
							int ItemPos = 0;

							app->db->prepare("SELECT * FROM items WHERE owner_ID = ? AND place=1 ORDER BY pos ASC");
							app->db->setInt(1, this->plr->pcProfile->charId);
							app->db->execute();
							int k = 0;
							while(app->db->fetch())
							{
								if(app->db->getInt("pos") < NTL_MAX_ITEM_SLOT)
								ItemPos = app->db->getInt("pos") + 1;
								else
								ItemPos = app->db->getInt("pos");
								k++;
							}
							app->db->prepare("CALL BuyItemFromShop (?,?,?,?,?, @unique_iID)");
							app->db->setInt(1, pMerchantData->aitem_Tblidx[j]);
							app->db->setInt(2, this->plr->pcProfile->charId);
							app->db->setInt(3, ItemPos);
							app->db->setInt(4, pItemData->byRank);
							app->db->setInt(5, pItemData->byDurability);
							app->db->execute();
							app->db->execute("SELECT @unique_iID");
							app->db->fetch();

							CNtlPacket packet2(sizeof(sGU_ITEM_CREATE));
							sGU_ITEM_CREATE * res2 = (sGU_ITEM_CREATE *)packet2.GetPacketData();

							res2->bIsNew = true;
							res2->wOpCode = GU_ITEM_CREATE;
							res2->handle = app->db->getInt("@unique_iID");
							res2->sItemData.charId = this->GetavatarHandle();
							res2->sItemData.itemNo = pItemData->tblidx;
							res2->sItemData.byStackcount = req->sBuyData[0].byStack;
							res2->sItemData.itemId = app->db->getInt("@unique_iID");
							res2->sItemData.byPlace = 1;
							res2->sItemData.byPosition = ItemPos;
							res2->sItemData.byCurrentDurability = pItemData->byDurability;
							res2->sItemData.byRank = pItemData->byRank;

							packet2.SetPacketLen( sizeof(sGU_ITEM_CREATE) );
							g_pApp->Send( this->GetHandle(), &packet2 );

							break;
						}

					}
				}
			}
		}
	}

	CNtlPacket packet(sizeof(sGU_SHOP_BUY_RES));
	sGU_SHOP_BUY_RES * res = (sGU_SHOP_BUY_RES *)packet.GetPacketData();

	res->wOpCode = GU_SHOP_BUY_RES;
	res->wResultCode = GAME_SUCCESS;
	res->handle = req->handle;

	packet.SetPacketLen( sizeof(sGU_SHOP_BUY_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
void CClientSession::SendShopEndReq(CNtlPacket * pPacket, CGameServer * app)
{
	CNtlPacket packet(sizeof(sGU_SHOP_END_RES));
	sGU_SHOP_END_RES * res = (sGU_SHOP_END_RES *)packet.GetPacketData();

	res->wOpCode = GU_SHOP_END_RES;
	res->wResultCode = GAME_SUCCESS;

	packet.SetPacketLen( sizeof(sGU_SHOP_END_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
void	CClientSession::SendShopSellReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_SHOP_SELL_REQ * req = (sUG_SHOP_SELL_REQ *)pPacket->GetPacketData();
	CNtlPacket packet(sizeof(sGU_SHOP_SELL_RES));
	sGU_SHOP_SELL_RES * res = (sGU_SHOP_SELL_RES *)packet.GetPacketData();
	CNtlPacket packet1(sizeof(sGU_ITEM_DELETE));
	sGU_ITEM_DELETE * res1 = (sGU_ITEM_DELETE *)packet1.GetPacketData();
	CNtlPacket packet2(sizeof(sGU_UPDATE_CHAR_ZENNY));
	sGU_UPDATE_CHAR_ZENNY * res2 = (sGU_UPDATE_CHAR_ZENNY *)packet2.GetPacketData();

	CItemTable *itemTbl = app->g_pTableContainer->GetItemTable();
	int zenit_amount = 0;
	for (int i = 0; (req->sSellData[i].byStack != 0 ); i++)
	{
		app->db->prepare("SELECT * FROM items WHERE owner_ID = ? AND place = ? AND pos = ?");
		app->db->setInt(1, this->plr->pcProfile->charId);
		app->db->setInt(2, req->sSellData[i].byPlace);
		app->db->setInt(3, req->sSellData[i].byPos);
		app->db->execute();
		app->db->fetch();
		int item_id = app->db->getInt("tblidx");
		int id = app->db->getInt("id");

		res1->bySrcPlace = req->sSellData[i].byPlace;
		res1->bySrcPos = req->sSellData[i].byPos;
		res1->hSrcItem = id;
		res1->wOpCode = GU_ITEM_DELETE;
		packet1.SetPacketLen(sizeof(sGU_ITEM_DELETE));
		sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*) itemTbl->FindData( item_id );
		zenit_amount += pItemData->bySell_Price * req->sSellData[i].byStack;
		int count_less = req->sSellData[i].byStack - app->db->getInt("count");
		if (count_less <= 0)
		{
			g_pApp->Send( this->GetHandle() , &packet1 );
			app->db->prepare("DELETE FROM items WHERE id = ?");
			app->db->setInt(1, id);
			app->db->execute();
		}
		else if (count_less >= 1)
		{
			app->db->prepare("UPDATE items SET count=? WHERE id=?");
			app->db->setInt(1, count_less);
			app->db->setInt(2, id);
			app->db->execute();
		}
	}
	this->plr->pcProfile->dwZenny += zenit_amount;
	res->handle = req->handle;
	res->wOpCode = GU_SHOP_SELL_RES;
	res->wResultCode = GAME_SUCCESS;
	res2->bIsNew = true;
	res2->byChangeType = 1;
	res2->dwZenny = this->plr->pcProfile->dwZenny;
	res2->handle = this->GetavatarHandle();
	res2->wOpCode = GU_UPDATE_CHAR_ZENNY;
	app->qry->SetPlusMoney(this->plr->pcProfile->charId, zenit_amount);
	packet.SetPacketLen(sizeof(sGU_SHOP_SELL_RES));
	packet2.SetPacketLen(sizeof(sGU_UPDATE_CHAR_ZENNY));
	g_pApp->Send( this->GetHandle() , &packet );
	g_pApp->Send( this->GetHandle() , &packet1 );
	g_pApp->Send( this->GetHandle() , &packet2 );
}
//ROLL DICE
void	CClientSession::SendRollDice(CNtlPacket * pPacket, CGameServer * app)
{
	CNtlPacket packet(sizeof(sGU_DICE_ROLL_RES));
	sGU_DICE_ROLL_RES * res = (sGU_DICE_ROLL_RES *)packet.GetPacketData();

	res->wOpCode = GU_DICE_ROLL_RES;
	res->wDiceResult = (WORD) rand() % 100;
	res->wResultCode = GAME_SUCCESS;

	packet.SetPacketLen( sizeof(sGU_DICE_ROLL_RES) );
	g_pApp->Send( this->GetHandle() , &packet );

	CNtlPacket packet2(sizeof(sGU_DICE_ROLLED_NFY));
	sGU_DICE_ROLLED_NFY * res2 = (sGU_DICE_ROLLED_NFY *)packet2.GetPacketData();

	res2->wDiceResult = res->wDiceResult;
	res2->wOpCode = GU_DICE_ROLLED_NFY;
	res2->hSubject = this->GetavatarHandle();

	packet2.SetPacketLen( sizeof(sGU_DICE_ROLLED_NFY) );
	g_pApp->Send( this->GetHandle() , &packet2 );
	app->UserBroadcastothers(&packet2, this);
}

void	CClientSession::SendScouterIndicatorReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_SCOUTER_INDICATOR_REQ * req = (sUG_SCOUTER_INDICATOR_REQ *)pPacket->GetPacketData();
	CNtlPacket packet(sizeof(sGU_SCOUTER_INDICATOR_RES));
	sGU_SCOUTER_INDICATOR_RES * res = (sGU_SCOUTER_INDICATOR_RES *)packet.GetPacketData();

	res->hTarget = req->hTarget;
	res->dwRetValue = 0;
	res->wOpCode = GU_SCOUTER_INDICATOR_RES;

	int mobid = 0;
	if ((mobid = IsMonsterIDInsideList(req->hTarget)) != 0)
	{
		CMobTable *Mob = app->g_pTableContainer->GetMobTable();
		for ( CTable::TABLEIT itmob = Mob->Begin(); itmob != Mob->End(); ++itmob )
		{
			sMOB_TBLDAT* pMOBtData = (sMOB_TBLDAT*) itmob->second;
			if (pMOBtData->tblidx == mobid)
			{
				res->dwRetValue = this->gsf->CalculePowerLevel(pMOBtData);
				res->wResultCode = GAME_SUCCESS;
				break;
			}
			else
				res->wResultCode = GAME_SCOUTER_TARGET_FAIL;
		}
	}
	else
	{
		PlayerInfos *targetPlr = new PlayerInfos();
		app->GetUserSession(req->hTarget, targetPlr);
		if (targetPlr != NULL)
		{
			res->dwRetValue = this->gsf->CalculePowerLevelPlayer(targetPlr);
			res->wResultCode = GAME_SUCCESS;
			delete targetPlr;
		}
		else
			res->wResultCode = GAME_SCOUTER_TARGET_FAIL;
	}
	packet.SetPacketLen( sizeof(sGU_SCOUTER_INDICATOR_RES) );
	g_pApp->Send( this->GetHandle() , &packet );
}
void	CClientSession::SendDragonBallCheckReq(CNtlPacket * pPacket, CGameServer * app) // THIS IS THE FIRST VERSION
{
	//printf("--- UG_DRAGONBALL_CHECK_REQ --- \n");
	sUG_DRAGONBALL_CHECK_REQ * req = (sUG_DRAGONBALL_CHECK_REQ *)pPacket->GetPacketData();
	CNtlPacket packet(sizeof(sGU_DRAGONBALL_CHECK_RES));
	sGU_DRAGONBALL_CHECK_RES * res = (sGU_DRAGONBALL_CHECK_RES *)packet.GetPacketData();
	
	CNtlPacket packet2(sizeof(sGU_OBJECT_CREATE));
	sGU_OBJECT_CREATE * obj = (sGU_OBJECT_CREATE *)packet2.GetPacketData();

	CNtlPacket packet3(sizeof(sGU_AVATAR_ZONE_INFO));
	sGU_AVATAR_ZONE_INFO * zone = (sGU_AVATAR_ZONE_INFO *)packet3.GetPacketData();

	int dragonBall[7] = {9, 9, 9, 9, 9, 9, 9};// 7 because we need loop 0 - 6 because position start to 0 :)
	int i = 0;
	while (i <= 6)
	{
		app->db->prepare("SELECT * FROM items WHERE id = ?");
		app->db->setInt(1, req->sData[i].hItem);
		app->db->execute();
		app->db->fetch();
		dragonBall[i] = app->db->getInt("tblidx");
		i++;
	}
	i = 0;
	std::sort(dragonBall, dragonBall+6);
	while (i <= 6)
	{
		if (req->sData[i].byPos == i && dragonBall[i] == (200001 + i));
		else
		{
			res->hObject = req->hObject;
			res->wResultCode = GAME_DRAGONBALL_NOT_FOUND;
			res->wOpCode = GU_DRAGONBALL_CHECK_RES;
			packet.SetPacketLen( sizeof(sGU_DRAGONBALL_CHECK_RES) );
			g_pApp->Send( this->GetHandle() , &packet );
			this->gsf->printError("An error is occured in SendDragonBallReq: GAME_DRAGONBALL_NOT_FOUND");
			i = 0;
			break;
		}
		i++;
	}
	if (i == 7)
	{
		
	
		zone->wOpCode = GU_AVATAR_ZONE_INFO;
		zone->zoneInfo.bIsDark = true;
		zone->zoneInfo.zoneId = 0; // 0 namek start zone
		
		packet3.SetPacketLen( sizeof(sGU_AVATAR_ZONE_INFO) );
		g_pApp->Send( this->GetHandle() , &packet3 );
		app->UserBroadcastothers(&packet3, this);

		//sSPAWN_TBLDAT* pMOBTblData = (sSPAWN_TBLDAT*)app->g_pTableContainer->GetMobSpawnTable(1)->FindData(6361105);

		obj->handle = 90000;//this->plr->GetAvatarandle(); // this is wrong
		obj->wOpCode = GU_OBJECT_CREATE;
		obj->sObjectInfo.objType = OBJTYPE_NPC; // this is wrong
		obj->sObjectInfo.npcBrief.tblidx = 6361105; // this is wrong
		obj->sObjectInfo.npcState.sCharStateBase.vCurLoc.x = 4708;
		obj->sObjectInfo.npcState.sCharStateBase.vCurLoc.y = -52;
		obj->sObjectInfo.npcState.sCharStateBase.vCurLoc.z = 4001;
		obj->sObjectInfo.npcState.sCharStateBase.byStateID = CHARSTATE_SPAWNING;
		obj->sObjectInfo.npcBrief.wCurEP = 100;
		obj->sObjectInfo.npcBrief.wCurLP = 100;
		obj->sObjectInfo.npcBrief.wMaxEP = 100;
		obj->sObjectInfo.npcBrief.wMaxLP = 100;

		packet2.SetPacketLen( sizeof(sGU_OBJECT_CREATE) );
		g_pApp->Send( this->GetHandle() , &packet2 );
		app->UserBroadcastothers(&packet2, this);

		res->hObject = req->hObject;
		res->wResultCode = GAME_SUCCESS;
		res->wOpCode = GU_DRAGONBALL_CHECK_RES;
		packet.SetPacketLen( sizeof(sGU_DRAGONBALL_CHECK_RES) );
		g_pApp->Send( this->GetHandle() , &packet );
	}
}

void	CClientSession::SendDragonBallRewardReq(CNtlPacket * pPacket, CGameServer * app) // THIS IS THE FIRST VERSION
{
	sUG_DRAGONBALL_REWARD_REQ * req = (sUG_DRAGONBALL_REWARD_REQ *)pPacket->GetPacketData();
	CNtlPacket packet(sizeof(sGU_DRAGONBALL_REWARD_RES));
	sGU_DRAGONBALL_REWARD_RES * res = (sGU_DRAGONBALL_REWARD_RES *)packet.GetPacketData();

	sDRAGONBALL_REWARD_TBLDAT* pDBtData = (sDRAGONBALL_REWARD_TBLDAT*)app->g_pTableContainer->GetDragonBallRewardTable()->FindData(req->rewardTblidx);
	
	//printf("Datacontainer = byBallType = %d, byRewardCategoryDepth = %d, rewardType = %d\ndwRewardZenny = %d, catergoryDialogue = %d\nCategoryName = %d, RewardDialog1 = %d, RewardDialog2 = %d\nrewardlinktblidx = %d, rewardname = %d, tdblidx = %d\n",
		//pDBtData->byBallType, pDBtData->byRewardCategoryDepth, pDBtData->byRewardType, pDBtData->dwRewardZenny, pDBtData->rewardCategoryDialog,pDBtData->rewardCategoryName,
		//pDBtData->rewardDialog1, pDBtData->rewardDialog2,pDBtData->rewardLinkTblidx, pDBtData->rewardName, pDBtData->tblidx);
	//printf("Reward have been found.\nReward id = %d.\n", req->rewardTblidx);
	//------------ Wishlist --------//

	CSkillTable* pSkillTable = app->g_pTableContainer->GetSkillTable();
	sSKILL_TBLDAT* pSkillData = (sSKILL_TBLDAT*)pSkillTable->FindData(pDBtData->rewardLinkTblidx);	
 
	CItemTable* pItemTable = app->g_pTableContainer->GetItemTable();
 	sITEM_TBLDAT * pItemData = (sITEM_TBLDAT*)pItemTable->FindData(pDBtData->rewardLinkTblidx);
 
	CNtlPacket packet3(sizeof(sGU_AVATAR_ZONE_INFO));
	sGU_AVATAR_ZONE_INFO * zone = (sGU_AVATAR_ZONE_INFO *)packet3.GetPacketData();

 	switch (pDBtData->byRewardType)
  	{
			case DRAGONBALL_REWARD_TYPE_SKILL:{
 				CNtlPacket packet3(sizeof(sGU_SKILL_LEARNED_NFY));
 				sGU_SKILL_LEARNED_NFY * res3 = (sGU_SKILL_LEARNED_NFY *)packet3.GetPacketData();
 				//Fixed Slot Index for Shenron's Buff Skill
 				res3->wOpCode = GU_SKILL_LEARNED_NFY;
 				res3->skillId = pSkillData->tblidx;
 				res3->bySlot = (this->gsf->GetTotalSlotSkill(this->plr->pcProfile->charId)+1);
 				app->qry->InsertNewSkill(pSkillData->tblidx, this->plr->pcProfile->charId, res3->bySlot, pSkillData->wKeep_Time, 0);
 				packet3.SetPacketLen(sizeof(sGU_SKILL_LEARNED_NFY));
 				g_pApp->Send(this->GetHandle(), &packet3);
 		}
 		break;
 		case DRAGONBALL_REWARD_TYPE_ITEM:{
 				CNtlPacket packet4(sizeof(sGU_ITEM_PICK_RES));
 				sGU_ITEM_PICK_RES * res4 = (sGU_ITEM_PICK_RES*)packet4.GetPacketData();
 				res4->itemTblidx = pItemData->tblidx;
 				res4->wOpCode = GU_ITEM_PICK_RES;
 				res4->wResultCode = GAME_SUCCESS;
 				int ItemPos = 0;
 
 				app->db->prepare("SELECT * FROM items WHERE owner_ID = ? AND place=1 ORDER BY pos ASC");
 				app->db->setInt(1, this->plr->pcProfile->charId);
 				app->db->execute();
 				int k = 0;
 				//Need a right loop 
 				while (app->db->fetch())
 				{
 					if (app->db->getInt("pos") < NTL_MAX_ITEM_SLOT)
 						ItemPos = app->db->getInt("pos") + 1;
 					else
 						ItemPos = app->db->getInt("pos");
 					k++;
 				}
 				app->db->prepare("CALL BuyItemFromShop (?,?,?,?,?, @unique_iID)");//this basicaly a insert into...
 				app->db->setInt(1, pItemData->tblidx);
 				app->db->setInt(2, this->plr->pcProfile->charId);
 				app->db->setInt(3, ItemPos);
 				app->db->setInt(4, pItemData->byRank);
 				app->db->setInt(5, pItemData->byDurability);
 				app->db->execute();
 				app->db->execute("SELECT @unique_iID");
 				app->db->fetch();
 
 				CNtlPacket packet2(sizeof(sGU_ITEM_CREATE));
 				sGU_ITEM_CREATE * res2 = (sGU_ITEM_CREATE *)packet2.GetPacketData();
 
 				res2->bIsNew = true;
 				res2->wOpCode = GU_ITEM_CREATE;
 				res2->handle = app->db->getInt("@unique_iID");
 				res2->sItemData.charId = this->GetavatarHandle();
 				res2->sItemData.itemNo = pItemData->tblidx;
 				res2->sItemData.byStackcount = pItemData->byMax_Stack;//1 is need to be default,you can use byMaxStack(but if you choose senzubeans the correct is receive 3(like dragon ball Saga) but give you 20
 				res2->sItemData.itemId = app->db->getInt("@unique_iID");
 				res2->sItemData.byPlace = 1;
 				res2->sItemData.byPosition = ItemPos;
 				res2->sItemData.byCurrentDurability = pItemData->byDurability;
 				res2->sItemData.byRank = pItemData->byRank;
 
 				packet2.SetPacketLen(sizeof(sGU_ITEM_CREATE));
 				packet4.SetPacketLen(sizeof(sGU_ITEM_PICK_RES));
 				g_pApp->Send(this->GetHandle(), &packet2);
 				g_pApp->Send(this->GetHandle(), &packet4);
 		}
 		break;
 		case DRAGONBALL_REWARD_TYPE_ZENNY:{
 				CNtlPacket packet5(sizeof(sGU_UPDATE_CHAR_ZENNY));
 				sGU_UPDATE_CHAR_ZENNY * res5 = (sGU_UPDATE_CHAR_ZENNY *)packet5.GetPacketData();
				res5->dwZenny = this->plr->pcProfile->dwZenny + req->rewardTblidx;//by analazying this is the ammount...				
 				res5->bIsNew = true;
 				res5->handle = this->GetavatarHandle();
 				res5->byChangeType = 0;//never mind
 				res5->wOpCode = GU_UPDATE_CHAR_ZENNY;
 				packet5.SetPacketLen(sizeof(sGU_UPDATE_CHAR_ZENNY));
 				g_pApp->Send(this->GetHandle(), &packet5);
				app->qry->SetPlusMoney( this->plr->pcProfile->charId, (res5->dwZenny -= this->plr->pcProfile->dwZenny));
				this->plr->pcProfile->dwZenny += res5->dwZenny;
				
 		}
 		break;
  	}
	
	//---------------End Wish Table---------------------------------------------------------------------------//
	res->hObject = req->hObject;
	res->wOpCode = GU_DRAGONBALL_REWARD_RES;
	res->wResultCode = GAME_SUCCESS;
	packet.SetPacketLen( sizeof(sGU_DRAGONBALL_REWARD_RES) );
	g_pApp->Send( this->GetHandle() , &packet );

	zone->wOpCode = GU_AVATAR_ZONE_INFO;
	zone->zoneInfo.bIsDark = false;
	zone->zoneInfo.zoneId = 0;
		
	packet3.SetPacketLen( sizeof(sGU_AVATAR_ZONE_INFO) );
	g_pApp->Send( this->GetHandle() , &packet3 );
	app->UserBroadcastothers(&packet3, this);

	CNtlPacket packet2(sizeof(sGU_DRAGONBALL_SCHEDULE_INFO));
	sGU_DRAGONBALL_SCHEDULE_INFO * res2 = (sGU_DRAGONBALL_SCHEDULE_INFO *)packet2.GetPacketData();
	res2->bIsAlive = true;
	res2->byEventType = SCHEDULE_EVENT_TYPE_NORMAL_DRAGONBALL;
	res2->byTermType = 0;
	res2->nStartTime = timeGetTime();
	res2->nEndTime = timeGetTime() * 2;
	res2->wOpCode = GU_DRAGONBALL_SCHEDULE_INFO;

	packet2.SetPacketLen( sizeof(sGU_DRAGONBALL_SCHEDULE_INFO) );
	g_pApp->Send( this->GetHandle() , &packet2 );
	app->UserBroadcastothers(&packet2, this);

	CNtlPacket packet5(sizeof(sGU_UPDATE_CHAR_STATE));
	sGU_UPDATE_CHAR_STATE * res3 = (sGU_UPDATE_CHAR_STATE *)packet5.GetPacketData();

	res3->handle = 90000;
	res3->sCharState.sCharStateBase.byStateID = CHARSTATE_DESPAWNING;
	res3->wOpCode = GU_UPDATE_CHAR_STATE;
	packet5.SetPacketLen( sizeof(sGU_UPDATE_CHAR_STATE) );
	g_pApp->Send( this->GetHandle() , &packet5 );
	app->UserBroadcastothers(&packet5, this);



	Sleep(10000);
	CNtlPacket packet4(sizeof(sGU_OBJECT_DESTROY));
	sGU_OBJECT_DESTROY * res4 = (sGU_OBJECT_DESTROY *)packet4.GetPacketData();
	res4->handle = 90000;
	res4->wOpCode = GU_OBJECT_DESTROY;
	packet4.SetPacketLen( sizeof(sGU_OBJECT_DESTROY) );
	g_pApp->Send( this->GetHandle() , &packet4 );
	app->UserBroadcastothers(&packet4, this);

}
void CClientSession::SendGambleBuyReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_SHOP_GAMBLE_BUY_REQ *req = (sUG_SHOP_GAMBLE_BUY_REQ*)pPacket->GetPacketData();
	CNtlPacket packet(sizeof(sGU_SHOP_GAMBLE_BUY_RES));
	sGU_SHOP_GAMBLE_BUY_RES * res = (sGU_SHOP_GAMBLE_BUY_RES *)packet.GetPacketData();

	res->handle = req->handle;
	res->wOpCode = GU_SHOP_GAMBLE_BUY_RES;
	res->wResultCode = GAME_FAIL;
	CItemMixMachineTable *mudo = app->g_pTableContainer->GetItemMixMachineTable();
	for ( CTable::TABLEIT itmob = mudo->Begin(); itmob != mudo->End(); ++itmob )
	{
		sITEM_MIX_MACHINE_TBLDAT* pMusoData = (sITEM_MIX_MACHINE_TBLDAT*) itmob->second;
		if (pMusoData)
		{
			//printf("%d, %d, %d, %d, %d, %d, %d, %d, %S", pMusoData->aBuiltInRecipeTblidx[0], pMusoData->aBuiltInRecipeTblidx[1], pMusoData->aBuiltInRecipeTblidx[2], pMusoData->bValidityAble, pMusoData->byMachineType, pMusoData->byMixZennyDiscountRate, pMusoData->dynamicObjectTblidx, pMusoData->tblidx, pMusoData->wFunctionBitFlag);
			res->hItem = pMusoData->aBuiltInRecipeTblidx[0];
		}
	}
	packet.SetPacketLen(sizeof(sGU_SHOP_GAMBLE_BUY_RES));
	int rc = g_pApp->Send(this->GetHandle(), &packet);
	this->gsf->printError("An error is occured in SendGambleBuyReq: GAME_FAIL");
}
//------------------------------------------------
// Character Skill Upgrade By luiz45
//------------------------------------------------
void CClientSession::SendCharSkillUpgrade(CNtlPacket * pPacket, CGameServer * app)
{
	//Upgrading Process
	sUG_SKILL_UPGRADE_REQ * req = (sUG_SKILL_UPGRADE_REQ*)pPacket->GetPacketData();
	CSkillTable* pSkillTable = app->g_pTableContainer->GetSkillTable();
	
	app->db->prepare("SELECT * FROM skills WHERE owner_id=? AND SlotID = ? ");
	app->db->setInt(1, this->plr->pcProfile->charId);
	app->db->setInt(2, req->bySlotIndex);
	app->db->execute();
	app->db->fetch();

	int skillID = app->db->getInt("skill_id");
	sSKILL_TBLDAT* pSkillData = reinterpret_cast<sSKILL_TBLDAT*>(pSkillTable->FindData(skillID));
 	
 	if (pSkillData->dwNextSkillTblidx)
 	{
 		CNtlPacket packet(sizeof(sGU_SKILL_UPGRADE_RES));
 		sGU_SKILL_UPGRADE_RES * res = (sGU_SKILL_UPGRADE_RES *)packet.GetPacketData();
 		
 		CNtlPacket packet2(sizeof(sGU_UPDATE_CHAR_SP));
 		sGU_UPDATE_CHAR_SP * res2 = (sGU_UPDATE_CHAR_SP *)packet2.GetPacketData();
 
 		res->wOpCode = GU_SKILL_UPGRADE_RES;
 		res->wResultCode = GAME_SUCCESS;
 		res->skillId = pSkillData->dwNextSkillTblidx;
 		res->bySlot = req->bySlotIndex;
 		packet.SetPacketLen(sizeof(sGU_SKILL_UPGRADE_RES));
 		g_pApp->Send(this->GetHandle(), &packet);
 		//Skill Level(ID)
 		app->db->prepare("UPDATE skills SET skill_id=? WHERE owner_id=? AND skill_id=?");
 		app->db->setInt(1, pSkillData->dwNextSkillTblidx);
 		app->db->setInt(2, this->plr->pcProfile->charId);
 		app->db->setInt(3, skillID);
 		app->db->execute();
 
 		//Update player's SP
		this->plr->pcProfile->dwSpPoint -= 1;
		app->qry->UpdateSPPoint(this->plr->pcProfile->charId, this->plr->pcProfile->dwSpPoint);
 
 		//Send a response to client to get Update SP OK
 		res2->wOpCode = GU_UPDATE_CHAR_SP;
 		res2->dwSpPoint = this->plr->pcProfile->dwSpPoint;
 		
 		packet2.SetPacketLen(sizeof(sGU_UPDATE_CHAR_SP));
 		g_pApp->Send(this->GetHandle(), &packet2);
	}
}
void		CClientSession::SendBankStartReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_BANK_START_REQ * req = (sUG_BANK_START_REQ*)pPacket->GetPacketData();
	CNtlPacket packet(sizeof(sGU_BANK_START_RES));
	sGU_BANK_START_RES * res = (sGU_BANK_START_RES *)packet.GetPacketData();

	res->handle = req->handle;
	res->wOpCode = GU_BANK_START_RES;
	res->wResultCode = GAME_SUCCESS;

	packet.SetPacketLen(sizeof(sGU_BANK_START_RES));
	g_pApp->Send(this->GetHandle(), &packet);
}
void		CClientSession::SendBankEndReq(CNtlPacket * pPacket, CGameServer * app)
{
	CNtlPacket packet(sizeof(sGU_BANK_END_RES));
	sGU_BANK_END_RES * res = (sGU_BANK_END_RES *)packet.GetPacketData();

	res->wOpCode = GU_BANK_END_RES;
	res->wResultCode = GAME_SUCCESS;

	packet.SetPacketLen(sizeof(sGU_BANK_END_RES));
 	g_pApp->Send(this->GetHandle(), &packet);
}
void		CClientSession::SendBankBuyReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_BANK_BUY_REQ * req = (sUG_BANK_BUY_REQ*)pPacket->GetPacketData();
	CNtlPacket packet(sizeof(sGU_BANK_BUY_RES));
	sGU_BANK_BUY_RES * res = (sGU_BANK_BUY_RES *)packet.GetPacketData();

	res->hItemhandle;
	res->hNpchandle;
	res->sData;
	res->wOpCode = GU_BANK_BUY_RES;
	res->wResultCode = GAME_SUCCESS;
}
void	CClientSession::SendBankLoadReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_BANK_LOAD_REQ * req = (sUG_BANK_LOAD_REQ*)pPacket->GetPacketData();
	CNtlPacket packet(sizeof(sGU_BANK_LOAD_RES));
	sGU_BANK_LOAD_RES * res = (sGU_BANK_LOAD_RES *)packet.GetPacketData();

	res->handle = req->handle;
	res->wOpCode = GU_BANK_LOAD_RES;
	res->wResultCode = GAME_SUCCESS;
	
	packet.SetPacketLen(sizeof(sGU_BANK_LOAD_RES));
 	g_pApp->Send(this->GetHandle(), &packet);
}

//-------------------------------------------------
//      Quick Slot Update insert luiz45
//-------------------------------------------------
void CClientSession::SendCharUpdQuickSlot(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_QUICK_SLOT_UPDATE_REQ * req = (sUG_QUICK_SLOT_UPDATE_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_QUICK_SLOT_UPDATE_RES));
	sGU_QUICK_SLOT_UPDATE_RES * res = (sGU_QUICK_SLOT_UPDATE_RES*)packet.GetPacketData();	

	app->qry->InsertRemoveQuickSlot(req->tblidx, req->bySlotID, this->plr->pcProfile->charId);

	res->wResultCode = GAME_SUCCESS;
	res->wOpCode = GU_QUICK_SLOT_UPDATE_RES;

	packet.SetPacketLen(sizeof(sGU_QUICK_SLOT_UPDATE_RES));
	g_pApp->Send(this->GetHandle(), &packet);
}
//-------------------------------------------------
//      Quick Slot Update Delete luiz45
//-------------------------------------------------
void CClientSession::SendCharDelQuickSlot(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_QUICK_SLOT_DEL_REQ * req = (sUG_QUICK_SLOT_DEL_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_QUICK_SLOT_DEL_NFY));
	sGU_QUICK_SLOT_DEL_NFY * response = (sGU_QUICK_SLOT_DEL_NFY*)packet.GetPacketData();
	app->qry->InsertRemoveQuickSlot(0, req->bySlotID, this->plr->pcProfile->charId);

	response->bySlotID = req->bySlotID;
	response->wOpCode = GU_QUICK_SLOT_DEL_NFY;

	packet.SetPacketLen(sizeof(sGU_QUICK_SLOT_DEL_NFY));
	g_pApp->Send(this->GetHandle(), &packet);
}
void CClientSession::SendPlayerLevelUpCheck(CGameServer * app, int exp)
{
	/*CExpTable *expT = app->g_pTableContainer->GetExpTable();
	for ( CTable::TABLEIT itNPCSpawn = expT->Begin(); itNPCSpawn != expT->End(); ++itNPCSpawn )
	{
		sEXP_TBLDAT *expTbl = (sEXP_TBLDAT *)itNPCSpawn->second;
		printf("%d, %d\n",expTbl->dwExp, expTbl->dwNeed_Exp);
	}*/
	CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_EXP));	
	sGU_UPDATE_CHAR_EXP * response = (sGU_UPDATE_CHAR_EXP*)packet.GetPacketData();
	response->dwAcquisitionExp = exp;
	response->dwCurExp = this->plr->pcProfile->dwCurExp;
	response->dwIncreasedExp = exp + rand() % this->plr->pcProfile->byLevel * 2;
	response->dwBonusExp = (exp - response->dwIncreasedExp);
	response->handle = this->plr->GetAvatarandle();
	response->wOpCode = GU_UPDATE_CHAR_EXP;
	this->plr->pcProfile->dwCurExp += response->dwIncreasedExp;
	if (this->plr->pcProfile->dwCurExp >= this->plr->pcProfile->dwMaxExpInThisLevel)
	{
		CNtlPacket packet2(sizeof(sGU_UPDATE_CHAR_SP));
 		sGU_UPDATE_CHAR_SP * res2 = (sGU_UPDATE_CHAR_SP *)packet2.GetPacketData();
		this->plr->pcProfile->dwCurExp -= this->plr->pcProfile->dwMaxExpInThisLevel;
		this->plr->pcProfile->dwMaxExpInThisLevel += this->plr->pcProfile->dwMaxExpInThisLevel;
		CNtlPacket packet1(sizeof(sGU_UPDATE_CHAR_LEVEL));	
		sGU_UPDATE_CHAR_LEVEL * response1 = (sGU_UPDATE_CHAR_LEVEL*)packet1.GetPacketData();
		this->plr->pcProfile->byLevel++;
		response1->byCurLevel = this->plr->pcProfile->byLevel;
		response1->byPrevLevel = this->plr->pcProfile->byLevel - 1;
		response1->dwMaxExpInThisLevel = this->plr->pcProfile->dwMaxExpInThisLevel;
		response1->handle = this->plr->GetAvatarandle();
		response1->wOpCode = GU_UPDATE_CHAR_LEVEL;
		packet1.SetPacketLen(sizeof(sGU_UPDATE_CHAR_LEVEL));
		g_pApp->Send(this->GetHandle(), &packet1);
		this->plr->LevelUpPlayer();
		this->plr->calculeMyStat(app);
		this->plr->pcProfile->dwSpPoint += 1;
		app->qry->UpdateSPPoint(this->plr->pcProfile->charId, this->plr->pcProfile->dwSpPoint);
		app->qry->UpdatePlayerLevel(this->plr->pcProfile->byLevel, this->plr->pcProfile->charId, this->plr->pcProfile->dwCurExp, this->plr->pcProfile->dwMaxExpInThisLevel);
		response->dwCurExp = this->plr->pcProfile->dwCurExp;
		this->plr->UpdateRPBall();
		res2->wOpCode = GU_UPDATE_CHAR_SP;
 		res2->dwSpPoint = this->plr->pcProfile->dwSpPoint;
 		packet2.SetPacketLen(sizeof(sGU_UPDATE_CHAR_SP));
 		g_pApp->Send(this->GetHandle(), &packet2);
	}
	packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_EXP));
	g_pApp->Send(this->GetHandle(), &packet);
}
void CClientSession::SendPlayerQuestReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_TS_CONFIRM_STEP_REQ* req = (sUG_TS_CONFIRM_STEP_REQ *)pPacket->GetPacketData();
	CNtlPacket packet(sizeof(sGU_TS_CONFIRM_STEP_RES));
	sGU_TS_CONFIRM_STEP_RES * res = (sGU_TS_CONFIRM_STEP_RES *)packet.GetPacketData();

	req->byEventType;

	res->byTsType = req->byTsType;
	res->dwParam = req->dwParam;
	res->tcCurId = req->tcCurId;
	res->tcNextId = req->tcNextId;
	res->tId = req->tId;
	res->wOpCode = GU_TS_CONFIRM_STEP_RES;
	res->wResultCode = RESULT_SUCCESS;

	if (res->tcNextId == 255)
	{
		this->gsf->printError("We need to add the reward");
		// should be the reward because when we rewarsd a quest res->tcNextId is all the time 255
		// WE NEED THE CORRECT TBLIDX FOR THE REWARD

		/*sQUEST_REWARD_TBLDAT *rew = (sQUEST_REWARD_TBLDAT*)app->g_pTableContainer->GetQuestRewardTable()->FindData(res->tId);
		printf("%d %d %d %d\n%d %d %d %d\n%d\n",rew->arsDefRwd[0], rew->arsDefRwd[1], rew->arsDefRwd[2], rew->arsDefRwd[3], rew->arsSelRwd[0], rew->arsSelRwd[1], rew->arsSelRwd[2], rew->arsSelRwd[3], rew->tblidx);
		for(int i = 0; i <= QUEST_REWARD_DEF_MAX_CNT; i++ )
		{
			switch (rew->arsDefRwd[i].byRewardType)
			{
				case eREWARD_TYPE_NORMAL_ITEM:
				{
					rew->arsDefRwd[i].dwRewardIdx;
					rew->arsDefRwd[i].dwRewardVal;
					gsf->printOk("Reward Normal Item");
				}
				case eREWARD_TYPE_QUEST_ITEM:
				{
					rew->arsDefRwd[i].dwRewardIdx;
					rew->arsDefRwd[i].dwRewardVal;
					gsf->printOk("Reward Quest Item");
				}
				case eREWARD_TYPE_EXP:
				{
					rew->arsDefRwd[i].dwRewardIdx;
					rew->arsDefRwd[i].dwRewardVal;
					gsf->printOk("Reward Experience");
				}
				case eREWARD_TYPE_SKILL:
				{
					rew->arsDefRwd[i].dwRewardIdx;
					rew->arsDefRwd[i].dwRewardVal;
					gsf->printOk("Reward Skill");
				}
				case eREWARD_TYPE_ZENY:
				{
					rew->arsDefRwd[i].dwRewardIdx;
					rew->arsDefRwd[i].dwRewardVal;
					gsf->printOk("Reward Zenny");
				}
				case eREWARD_TYPE_CHANGE_CLASS:
				{
					rew->arsDefRwd[i].dwRewardIdx;
					rew->arsDefRwd[i].dwRewardVal;
					gsf->printOk("Reward Type Change Class");
				}
				case eREWARD_TYPE_PROBABILITY:
				{
					rew->arsDefRwd[i].dwRewardIdx;
					rew->arsDefRwd[i].dwRewardVal;
					gsf->printOk("Reward Probability");
				}
				case eREWARD_TYPE_REPUTATION:
				{
					rew->arsDefRwd[i].dwRewardIdx;
					rew->arsDefRwd[i].dwRewardVal;
					gsf->printOk("Reward Reputation");
				}
				case eREWARD_TYPE_CHANGE_ADULT:
				{
					rew->arsDefRwd[i].dwRewardIdx;
					rew->arsDefRwd[i].dwRewardVal;
					gsf->printOk("Reward Change Adult");
				}
				case eREWARD_TYPE_GET_CONVERT_CLASS_RIGHT:
				{
					rew->arsDefRwd[i].dwRewardIdx;
					rew->arsDefRwd[i].dwRewardVal;
					gsf->printOk("Reward Convert Class");
				}
				default:
				{
					gsf->printError("Unknown Reward Type");
				}
			}
		}*/
	}
	//printf("res->byTsType = %d, res->dwParam = %d, res->tcCurId = %d, res->tcNextId = %d, res->tId = %d\n",res->byTsType, res->dwParam, res->tcCurId, res->tcNextId, res->tId); 
	packet.SetPacketLen( sizeof(sGU_TS_CONFIRM_STEP_RES) );
	g_pApp->Send( this->GetHandle() , &packet );
}
void	CClientSession::SendZennyPickUpReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_ZENNY_PICK_REQ* req = (sUG_ZENNY_PICK_REQ *)pPacket->GetPacketData();
	CNtlPacket packet(sizeof(sGU_ZENNY_PICK_RES));
	sGU_ZENNY_PICK_RES * res = (sGU_ZENNY_PICK_RES *)packet.GetPacketData();
	
	int amnt = app->FindZenny(req->handle);

	if (amnt > 0)
	{
		app->RemoveZenny(req->handle);
		res->bSharedInParty = false; //this->plr->isInParty();
		res->dwBonusZenny = rand() % 100;
		//res->dwOriginalZenny = amnt;
		this->plr->pcProfile->dwZenny += (amnt + res->dwBonusZenny);
		res->dwZenny = this->plr->pcProfile->dwZenny;
		res->dwAcquisitionZenny = res->dwZenny;
		res->wResultCode = ZENNY_CHANGE_TYPE_PICK;

		app->qry->SetPlusMoney(this->plr->pcProfile->charId, res->dwAcquisitionZenny);

		CNtlPacket packet3(sizeof(sGU_OBJECT_DESTROY));
		sGU_OBJECT_DESTROY * res3 = (sGU_OBJECT_DESTROY *)packet3.GetPacketData();
		res3->handle = req->handle;
		res3->wOpCode = GU_OBJECT_DESTROY;
		packet3.SetPacketLen( sizeof(sGU_OBJECT_DESTROY) );
		g_pApp->Send( this->GetHandle() , &packet3 );

		CNtlPacket packet5(sizeof(sGU_UPDATE_CHAR_ZENNY));
 		sGU_UPDATE_CHAR_ZENNY * res5 = (sGU_UPDATE_CHAR_ZENNY *)packet5.GetPacketData();
		res5->dwZenny = this->plr->pcProfile->dwZenny;//by analazying this is the ammount...				
 		res5->bIsNew = true;
 		res5->handle = this->GetavatarHandle();
 		res5->byChangeType = 0;//never mind
 		res5->wOpCode = GU_UPDATE_CHAR_ZENNY;
 		packet5.SetPacketLen(sizeof(sGU_UPDATE_CHAR_ZENNY));
 		g_pApp->Send(this->GetHandle(), &packet5);
	}
	else
	{
		res->wResultCode = GAME_FAIL;
	}
	res->wOpCode = GU_ZENNY_PICK_RES;
	packet.SetPacketLen( sizeof(sGU_ZENNY_PICK_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
}
void	CClientSession::SendFreeBattleReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_FREEBATTLE_CHALLENGE_REQ* req = (sUG_FREEBATTLE_CHALLENGE_REQ *)pPacket->GetPacketData();
	CNtlPacket packet(sizeof(sGU_FREEBATTLE_CHALLENGE_RES));
	sGU_FREEBATTLE_CHALLENGE_RES * res = (sGU_FREEBATTLE_CHALLENGE_RES *)packet.GetPacketData();

	CNtlPacket packet2(sizeof(sGU_FREEBATTLE_ACCEPT_REQ));
	sGU_FREEBATTLE_ACCEPT_REQ * res2 = (sGU_FREEBATTLE_ACCEPT_REQ *)packet2.GetPacketData();

	res->hTarget = req->hTarget;
	res->wOpCode = GU_FREEBATTLE_CHALLENGE_RES;
	res->wResultCode = GAME_SUCCESS;

	res2->hChallenger = this->plr->GetAvatarandle();
	res2->wOpCode = GU_FREEBATTLE_ACCEPT_REQ;

	packet.SetPacketLen( sizeof(sGU_FREEBATTLE_CHALLENGE_RES) );
	g_pApp->Send( this->GetHandle(), &packet );
	packet2.SetPacketLen( sizeof(sGU_FREEBATTLE_ACCEPT_REQ) );
	g_pApp->Send( this->GetHandle(), &packet2 );
	app->UserBroadcastothers(&packet, this);
}
void	CClientSession::SendFreeBattleAccpetReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_FREEBATTLE_ACCEPT_RES* req = (sUG_FREEBATTLE_ACCEPT_RES *)pPacket->GetPacketData();
	
	if (req->byAccept == 1)
	{
		CNtlPacket packet2(sizeof(sGU_FREEBATTLE_START_NFY));
		sGU_FREEBATTLE_START_NFY * res2 = (sGU_FREEBATTLE_START_NFY *)packet2.GetPacketData();
		res2->hTarget = this->plr->GetAvatarandle();
		res2->vRefreeLoc = this->plr->GetPosition();
		res2->vRefreeLoc.x += rand() % 10;
		res2->wOpCode = GU_FREEBATTLE_START_NFY;
		packet2.SetPacketLen( sizeof(sGU_FREEBATTLE_START_NFY) );
		g_pApp->Send( this->GetHandle() , &packet2 );
		app->UserBroadcastothers(&packet2, this);
	}
	else
	{
		CNtlPacket packet2(sizeof(sGU_FREEBATTLE_CANCEL_NFY));
		sGU_FREEBATTLE_CANCEL_NFY * res2 = (sGU_FREEBATTLE_CANCEL_NFY *)packet2.GetPacketData();
		res2->wOpCode = GU_FREEBATTLE_CANCEL_NFY;
		res2->wResultCode = GAME_FREEBATTLE_CHALLENGE_ACCEPT_DENIED;
		packet2.SetPacketLen( sizeof(sGU_FREEBATTLE_CANCEL_NFY) );
		g_pApp->Send( this->GetHandle() , &packet2 );
		app->UserBroadcastothers(&packet2, this);
	}
}
void	CClientSession::SendItemUseReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_ITEM_USE_REQ * req = (sUG_ITEM_USE_REQ*)pPacket->GetPacketData();
 	
 	CNtlPacket packet(sizeof(sGU_ITEM_USE_RES));
 	sGU_ITEM_USE_RES * res = (sGU_ITEM_USE_RES*)packet.GetPacketData();
 	app->db->prepare("SELECT * FROM items WHERE owner_id = ? AND place = ? AND pos = ?");
 	app->db->setInt(1, this->plr->pcProfile->charId);
 	app->db->setInt(2, req->byPlace);
 	app->db->setInt(3, req->byPos);
 	app->db->execute();
 	app->db->fetch();
 
 	sCHARSTATE_CASTING_ITEM * charState;
 	sITEM_TBLDAT * itemTBL = reinterpret_cast<sITEM_TBLDAT*>(app->g_pTableContainer->GetItemTable()->FindData(app->db->getInt("tblidx")));
 	
 	res->byPlace = app->db->getInt("place");
 	res->byPos = app->db->getInt("pos");
 	res->tblidxItem = app->db->getInt("tblidx");
 	res->wOpCode = GU_ITEM_USE_RES;
 	res->wResultCode = GAME_SUCCESS;
 	packet.SetPacketLen(sizeof(sGU_ITEM_USE_RES));
 	g_pApp->Send(this->GetHandle(), &packet);
 	printf("Pass");
 	//app->UserBroadcastothers(&packet, this);
 	
}
 //---------------------------------------------------------------------//
 //------------------Skill Transform Cancel - Luiz45--------------------//
 //---------------------------------------------------------------------//
void CClientSession::SendCharSkillTransformCancel(CNtlPacket * pPacket, CGameServer * app)
{
 	//Response Skill
 	sUG_TRANSFORM_CANCEL_REQ * req = (sUG_TRANSFORM_CANCEL_REQ*)pPacket->GetPacketData();
 	CNtlPacket packet(sizeof(sGU_TRANSFORM_CANCEL_RES));
 	sGU_TRANSFORM_CANCEL_RES * res =(sGU_TRANSFORM_CANCEL_RES*)packet.GetPacketData();
 	res->wOpCode = GU_TRANSFORM_CANCEL_RES;
 	res->wResultCode = GAME_SUCCESS;
 
 	//Update Char State
 	CNtlPacket packet2(sizeof(sGU_UPDATE_CHAR_ASPECT_STATE));
 	sGU_UPDATE_CHAR_ASPECT_STATE * myPlayerState = (sGU_UPDATE_CHAR_ASPECT_STATE*)packet2.GetPacketData();
 	myPlayerState->handle = this->plr->GetAvatarandle();
 	myPlayerState->wOpCode = GU_UPDATE_CHAR_ASPECT_STATE;
 	myPlayerState->aspectState.sAspectStateBase.byAspectStateId = 255;//Don't see any Const then i send 0 because i'm not going transform ^^
 	
 	//Packets Sending
 	packet.SetPacketLen(sizeof(sGU_TRANSFORM_CANCEL_RES));
 	packet2.SetPacketLen(sizeof(sGU_UPDATE_CHAR_ASPECT_STATE));
 	g_pApp->Send(this->GetHandle(), &packet);
 	g_pApp->Send(this->GetHandle(), &packet2);
 
	//Sending to others
	app->UserBroadcastothers(&packet, this);
	app->UserBroadcastothers(&packet2, this);
}


 void CClientSession::SendSocialSkillRes(CNtlPacket * pPacket, CGameServer * app)
 {
	/*sUG_SOCIAL_ACTION * req = (sUG_SOCIAL_ACTION*)pPacket->GetPacketData();
 	CNtlPacket packet(sizeof(sGU_SOCIAL_ACTION));
 	sGU_SOCIAL_ACTION* res =(sGU_SOCIAL_ACTION*)packet.GetPacketData();

	res->hSubject = this->GetavatarHandle();
	res->socialActionId = req->socialActionId;
	res->wOpCode = GU_SOCIAL_ACTION;

	packet.SetPacketLen(sizeof(GU_SOCIAL_ACTION));
	g_pApp->Send(this->GetHandle(), &packet);
	//app->UserBroadcastothers(&packet, this);
	this->gsf->printOk("Sended");*/
 }

 void CClientSession::SendRpCharge(CNtlPacket *pPacket, CGameServer * app)
 {
	 sUG_CHAR_CHARGE * req = (sUG_CHAR_CHARGE*)pPacket->GetPacketData();
 	 CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_STATE));
	sGU_UPDATE_CHAR_STATE * res = (sGU_UPDATE_CHAR_STATE *)packet.GetPacketData();
	
	res->handle = this->GetHandle();
	res->sCharState.sCharStateBase.byStateID = CHARSTATE_CHARGING;
	res->wOpCode = GU_UPDATE_CHAR_STATE;

	g_pApp->Send(this->GetHandle(), &packet);
	app->UserBroadcastothers(&packet, this);
 }

//-----------------------------------------------------------------//
//-------------------Skill/Item BUFF Drop--------------------------//
//-----------------------------------------------------------------//
void CClientSession::SendCharSkillBuffDrop(CNtlPacket * pPacket, CGameServer * app)
{
 	sUG_BUFF_DROP_REQ * req = (sUG_BUFF_DROP_REQ*)pPacket->GetPacketData();
 	//Get Skill to Remove
 	CSkillTable * pSkillTable = app->g_pTableContainer->GetSkillTable();
 	sSKILL_RESULT * pSkillData = (sSKILL_RESULT*)pPacket->GetPacketData();
 
 	//Response Prepare
 	CNtlPacket packet(sizeof(sGU_BUFF_DROP_RES));
 	sGU_BUFF_DROP_RES * res = (sGU_BUFF_DROP_RES*)packet.GetPacketData();
 	res->wOpCode = GU_BUFF_DROP_RES;
 	res->wResultCode = GAME_SUCCESS;
 
 	//Dropp Event Prepare
 	CNtlPacket packet2(sizeof(sGU_BUFF_DROPPED));
 	sGU_BUFF_DROPPED * pBuffDrop = (sGU_BUFF_DROPPED*)packet2.GetPacketData();
 	pBuffDrop->hHandle = this->GetavatarHandle();
 	pBuffDrop->bySourceType = DBO_OBJECT_SOURCE_SKILL;//Need be rechecked because this can be a type DBO_OBJECT_SOURCE_ITEM
 	pBuffDrop->wOpCode = GU_BUFF_DROPPED;
 	pBuffDrop->tblidx = req->tblidx;
 
 	//First Drop,Second Resp to client
 	packet2.SetPacketLen(sizeof(sGU_BUFF_DROPPED));
 	packet.SetPacketLen(sizeof(sGU_BUFF_DROP_RES));
 	g_pApp->Send(this->GetHandle(), &packet2);
 	g_pApp->Send(this->GetHandle(), &packet);
 	app->UserBroadcastothers(&packet2, this);
  	app->UserBroadcastothers(&packet, this);
 }
//-----------------------------------------------------------//
//-----------GMT UPDATES        Luiz45  ---------------------//
//-----------------------------------------------------------//
void CClientSession::SendGmtUpdateReq(CNtlPacket * pPacket, CGameServer * app)
{
	sUG_GMT_UPDATE_REQ * req = (sUG_GMT_UPDATE_REQ*)pPacket->GetPacketData();
	sGAME_MANIA_TIME * gmMania = (sGAME_MANIA_TIME*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_GMT_UPDATE_RES));
	sGU_GMT_UPDATE_RES * res = (sGU_GMT_UPDATE_RES*)packet.GetPacketData();	
	res->wOpCode = GU_GMT_UPDATE_RES;
	res->wResultCode = GAME_SUCCESS;

	packet.SetPacketLen(sizeof(sGU_GMT_UPDATE_RES));
	g_pApp->Send(this->GetHandle(), &packet);
	app->UserBroadcastothers(&packet, this);
}