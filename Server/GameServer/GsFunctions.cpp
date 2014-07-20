#include "stdafx.h"
#include "GameServer.h"


bool	GsFunctionsClass::DeleteItemByUIdPlacePos(CNtlPacket * pPacket, CClientSession * pSession, RwUInt32 UniqueID, RwUInt32 Place, RwUInt32 Pos)
{
	CGameServer * app = (CGameServer*) NtlSfxGetApp();

	CNtlPacket packet(sizeof(sGU_ITEM_DELETE));
	sGU_ITEM_DELETE * res = (sGU_ITEM_DELETE *)packet.GetPacketData();

	res->wOpCode = GU_ITEM_DELETE;
	res->hSrcItem = UniqueID;
	res->bySrcPlace = Place;
	res->bySrcPos = Pos;

	packet.SetPacketLen(sizeof(sGU_ITEM_DELETE));
	g_pApp->Send( pSession->GetHandle() , &packet );

	return true;
}
void	GsFunctionsClass::DebugSkillType(BYTE skillActType)
{
	switch (skillActType)
	{
	case SKILL_ACTIVE_TYPE_DD:
		printf("SKILL_ACTIVE_TYPE_DD\n");
		break;
	case SKILL_ACTIVE_TYPE_BB:
		printf("SKILL_ACTIVE_TYPE_BB\n");
		break;
	case SKILL_ACTIVE_TYPE_CB:
		printf("SKILL_ACTIVE_TYPE_CB\n");
		break;
	case SKILL_ACTIVE_TYPE_DB:
		printf("SKILL_ACTIVE_TYPE_DB\n");
		break;
	case SKILL_ACTIVE_TYPE_DC:
		printf("SKILL_ACTIVE_TYPE_DC\n");
		break;
	case SKILL_ACTIVE_TYPE_DH:
		printf("SKILL_ACTIVE_TYPE_DH\n");
		break;
	case SKILL_ACTIVE_TYPE_DOT:
		printf("SKILL_ACTIVE_TYPE_DOT\n");
		break;
	}
}
bool	GsFunctionsClass::UpdateCharMoney(CNtlPacket * pPacket, CClientSession * pSession, RwUInt32 ChangeType, RwUInt32 MoneyAmount, RwUInt32 AvatarHandle)
{
	CGameServer * app = (CGameServer*) NtlSfxGetApp();

	CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_ZENNY));
	sGU_UPDATE_CHAR_ZENNY * res = (sGU_UPDATE_CHAR_ZENNY *)packet.GetPacketData();

	res->byChangeType = ChangeType;
	res->dwZenny = MoneyAmount;
	res->handle = AvatarHandle;
	res->wOpCode = GU_UPDATE_CHAR_ZENNY;

	packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_ZENNY));
	g_pApp->Send( pSession->GetHandle() , &packet );

	return true;
}

DWORD	GsFunctionsClass::CalculePowerLevel(sMOB_TBLDAT* pMOBtData)
{
	return Dbo_CalculatePowerLevel(pMOBtData->byStr, pMOBtData->byCon, pMOBtData->byFoc,pMOBtData->byDex, pMOBtData->bySol,
		pMOBtData->byEng, pMOBtData->wBasic_Offence, pMOBtData->wBasic_Physical_Defence, pMOBtData->wBasic_Energy_Defence, pMOBtData->wBasic_Energy_Defence,
		pMOBtData->wAttack_Rate, pMOBtData->wDodge_Rate, pMOBtData->wCurse_Success_Rate, pMOBtData->wCurse_Tolerance_Rate, 5, 
		5, pMOBtData->wAttack_Speed_Rate,pMOBtData->wBasic_LP,pMOBtData->wBasic_EP, pMOBtData->wBasic_LP, pMOBtData->wBasic_EP,1,pMOBtData->byLevel, pMOBtData->byGrade);
}






