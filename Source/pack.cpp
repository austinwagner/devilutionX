/**
 * @file pack.cpp
 *
 * Implementation of functions for minifying player data structure.
 */
#include "all.h"

namespace devilution {

void PackItem(PkItemStruct *id, const ItemStruct *is)
{
	memset(id, 0, sizeof(*id));
	if (is->isEmpty()) {
		id->idx = 0xFFFF;
	} else {
		int idx = is->IDidx;
		if (!gbIsHellfire) {
			idx = RemapItemIdxToDiablo(idx);
		}
		id->idx = SwapLE16(idx);
		if (is->IDidx == IDI_EAR) {
			id->iCreateInfo = is->_iName[8] | (is->_iName[7] << 8);
			id->iSeed = LOAD_BE32(&is->_iName[9]);
			id->bId = is->_iName[13];
			id->bDur = is->_iName[14];
			id->bMDur = is->_iName[15];
			id->bCh = is->_iName[16];
			id->bMCh = is->_iName[17];
			id->wValue = SwapLE16(is->_ivalue | (is->_iName[18] << 8) | ((is->_iCurs - ICURS_EAR_SORCERER) << 6));
			id->dwBuff = LOAD_BE32(&is->_iName[19]);
		} else {
			id->iSeed = SwapLE32(is->_iSeed);
			id->iCreateInfo = SwapLE16(is->_iCreateInfo);
			id->bId = is->_iIdentified + 2 * is->_iMagical;
			id->bDur = is->_iDurability;
			id->bMDur = is->_iMaxDur;
			id->bCh = is->_iCharges;
			id->bMCh = is->_iMaxCharges;
			if (is->IDidx == IDI_GOLD)
				id->wValue = SwapLE16(is->_ivalue);
			id->dwBuff = is->dwBuff;
		}
	}
}

void PackPlayer(PkPlayerStruct *pPack, int pnum, bool manashield)
{
	PlayerStruct *pPlayer;
	int i;
	ItemStruct *pi;
	PkItemStruct *pki;

	memset(pPack, 0, sizeof(*pPack));
	pPlayer = &plr[pnum];
	pPack->destAction = pPlayer->destAction;
	pPack->destParam1 = pPlayer->destParam1;
	pPack->destParam2 = pPlayer->destParam2;
	pPack->plrlevel = pPlayer->plrlevel;
	pPack->px = pPlayer->_px;
	pPack->py = pPlayer->_py;
	pPack->targx = pPlayer->_ptargx;
	pPack->targy = pPlayer->_ptargy;
	strcpy(pPack->pName, pPlayer->_pName);
	pPack->pClass = pPlayer->_pClass;
	pPack->pBaseStr = pPlayer->_pBaseStr;
	pPack->pBaseMag = pPlayer->_pBaseMag;
	pPack->pBaseDex = pPlayer->_pBaseDex;
	pPack->pBaseVit = pPlayer->_pBaseVit;
	pPack->pLevel = pPlayer->_pLevel;
	pPack->pStatPts = pPlayer->_pStatPts;
	pPack->pExperience = SwapLE32(pPlayer->_pExperience);
	pPack->pGold = SwapLE32(pPlayer->_pGold);
	pPack->pHPBase = SwapLE32(pPlayer->_pHPBase);
	pPack->pMaxHPBase = SwapLE32(pPlayer->_pMaxHPBase);
	pPack->pManaBase = SwapLE32(pPlayer->_pManaBase);
	pPack->pMaxManaBase = SwapLE32(pPlayer->_pMaxManaBase);
	pPack->pMemSpells = SDL_SwapLE64(pPlayer->_pMemSpells);

	for (i = 0; i < 37; i++) // Should be MAX_SPELLS but set to 37 to make save games compatible
		pPack->pSplLvl[i] = pPlayer->_pSplLvl[i];
	for (i = 37; i < 47; i++)
		pPack->pSplLvl2[i - 37] = pPlayer->_pSplLvl[i];

	pki = &pPack->InvBody[0];
	pi = &pPlayer->InvBody[0];

	for (i = 0; i < NUM_INVLOC; i++) {
		PackItem(pki, pi);
		pki++;
		pi++;
	}

	pki = &pPack->InvList[0];
	pi = &pPlayer->InvList[0];

	for (i = 0; i < NUM_INV_GRID_ELEM; i++) {
		PackItem(pki, pi);
		pki++;
		pi++;
	}

	for (i = 0; i < NUM_INV_GRID_ELEM; i++)
		pPack->InvGrid[i] = pPlayer->InvGrid[i];

	pPack->_pNumInv = pPlayer->_pNumInv;
	pki = &pPack->SpdList[0];
	pi = &pPlayer->SpdList[0];

	for (i = 0; i < MAXBELTITEMS; i++) {
		PackItem(pki, pi);
		pki++;
		pi++;
	}

	pPack->wReflections = SwapLE16(pPlayer->wReflections);
	pPack->pDifficulty = SwapLE32(pPlayer->pDifficulty);
	pPack->pDamAcFlags = SwapLE32(pPlayer->pDamAcFlags);
	pPack->pDiabloKillLevel = SwapLE32(pPlayer->pDiabloKillLevel);
	pPack->bIsHellfire = gbIsHellfire;

	if (!gbIsMultiplayer || manashield)
		pPack->pManaShield = SwapLE32(pPlayer->pManaShield);
	else
		pPack->pManaShield = false;
}

/**
 * Expand a PkItemStruct in to a ItemStruct
 *
 * Note: last slot of item[MAXITEMS+1] used as temporary buffer
 * find real name reference below, possibly [sizeof(item[])/sizeof(ItemStruct)]
 * @param is The source packed item
 * @param id The distination item
 */
void UnPackItem(const PkItemStruct *is, ItemStruct *id, bool isHellfire)
{
	WORD idx = SwapLE16(is->idx);
	if (idx == 0xFFFF) {
		id->_itype = ITYPE_NONE;
		return;
	}

	if (!isHellfire) {
		idx = RemapItemIdxFromDiablo(idx);
	}

	if (!IsItemAvailable(idx)) {
		id->_itype = ITYPE_NONE;
		return;
	}

	if (idx == IDI_EAR) {
		RecreateEar(
		    MAXITEMS,
		    SwapLE16(is->iCreateInfo),
		    SwapLE32(is->iSeed),
		    is->bId,
		    is->bDur,
		    is->bMDur,
		    is->bCh,
		    is->bMCh,
		    SwapLE16(is->wValue),
		    SwapLE32(is->dwBuff));
	} else {
		memset(&items[MAXITEMS], 0, sizeof(*items));
		RecreateItem(MAXITEMS, idx, SwapLE16(is->iCreateInfo), SwapLE32(is->iSeed), SwapLE16(is->wValue), isHellfire);
		items[MAXITEMS]._iMagical = is->bId >> 1;
		items[MAXITEMS]._iIdentified = is->bId & 1;
		items[MAXITEMS]._iDurability = is->bDur;
		items[MAXITEMS]._iMaxDur = is->bMDur;
		items[MAXITEMS]._iCharges = is->bCh;
		items[MAXITEMS]._iMaxCharges = is->bMCh;

		RemoveInvalidItem(&items[MAXITEMS]);

		if (isHellfire)
			items[MAXITEMS].dwBuff |= CF_HELLFIRE;
		else
			items[MAXITEMS].dwBuff &= ~CF_HELLFIRE;
	}
	*id = items[MAXITEMS];
}

void VerifyGoldSeeds(PlayerStruct *pPlayer)
{
	int i, j;

	for (i = 0; i < pPlayer->_pNumInv; i++) {
		if (pPlayer->InvList[i].IDidx == IDI_GOLD) {
			for (j = 0; j < pPlayer->_pNumInv; j++) {
				if (i != j) {
					if (pPlayer->InvList[j].IDidx == IDI_GOLD && pPlayer->InvList[i]._iSeed == pPlayer->InvList[j]._iSeed) {
						pPlayer->InvList[i]._iSeed = AdvanceRndSeed();
						j = -1;
					}
				}
			}
		}
	}
}

void UnPackPlayer(PkPlayerStruct *pPack, int pnum, bool netSync)
{
	PlayerStruct *pPlayer;
	int i;
	ItemStruct *pi;
	PkItemStruct *pki;

	pPlayer = &plr[pnum];
	pPlayer->_px = pPack->px;
	pPlayer->_py = pPack->py;
	pPlayer->_pfutx = pPack->px;
	pPlayer->_pfuty = pPack->py;
	pPlayer->_ptargx = pPack->targx;
	pPlayer->_ptargy = pPack->targy;
	pPlayer->plrlevel = pPack->plrlevel;
	ClrPlrPath(pnum);
	pPlayer->destAction = ACTION_NONE;
	strcpy(pPlayer->_pName, pPack->pName);
	pPlayer->_pClass = (plr_class)pPack->pClass;
	InitPlayer(pnum, true);
	pPlayer->_pBaseStr = pPack->pBaseStr;
	pPlayer->_pStrength = pPack->pBaseStr;
	pPlayer->_pBaseMag = pPack->pBaseMag;
	pPlayer->_pMagic = pPack->pBaseMag;
	pPlayer->_pBaseDex = pPack->pBaseDex;
	pPlayer->_pDexterity = pPack->pBaseDex;
	pPlayer->_pBaseVit = pPack->pBaseVit;
	pPlayer->_pVitality = pPack->pBaseVit;
	pPlayer->_pLevel = pPack->pLevel;
	pPlayer->_pStatPts = pPack->pStatPts;
	pPlayer->_pExperience = SwapLE32(pPack->pExperience);
	pPlayer->_pGold = SwapLE32(pPack->pGold);
	pPlayer->_pMaxHPBase = SwapLE32(pPack->pMaxHPBase);
	pPlayer->_pHPBase = SwapLE32(pPack->pHPBase);
	pPlayer->_pBaseToBlk = ToBlkTbl[pPlayer->_pClass];
	if (!netSync)
		if ((int)(pPlayer->_pHPBase & 0xFFFFFFC0) < 64)
			pPlayer->_pHPBase = 64;

	pPlayer->_pMaxManaBase = SwapLE32(pPack->pMaxManaBase);
	pPlayer->_pManaBase = SwapLE32(pPack->pManaBase);
	pPlayer->_pMemSpells = SDL_SwapLE64(pPack->pMemSpells);

	for (i = 0; i < 37; i++) // Should be MAX_SPELLS but set to 36 to make save games compatible
		pPlayer->_pSplLvl[i] = pPack->pSplLvl[i];
	for (i = 37; i < 47; i++)
		pPlayer->_pSplLvl[i] = pPack->pSplLvl2[i - 37];

	pki = &pPack->InvBody[0];
	pi = &pPlayer->InvBody[0];

	for (i = 0; i < NUM_INVLOC; i++) {
		bool isHellfire = netSync ? ((pki->dwBuff & CF_HELLFIRE) != 0) : pPack->bIsHellfire;
		UnPackItem(pki, pi, isHellfire);
		pki++;
		pi++;
	}

	pki = &pPack->InvList[0];
	pi = &pPlayer->InvList[0];

	for (i = 0; i < NUM_INV_GRID_ELEM; i++) {
		bool isHellfire = netSync ? ((pki->dwBuff & CF_HELLFIRE) != 0) : pPack->bIsHellfire;
		UnPackItem(pki, pi, isHellfire);
		pki++;
		pi++;
	}

	for (i = 0; i < NUM_INV_GRID_ELEM; i++)
		pPlayer->InvGrid[i] = pPack->InvGrid[i];

	pPlayer->_pNumInv = pPack->_pNumInv;
	VerifyGoldSeeds(pPlayer);

	pki = &pPack->SpdList[0];
	pi = &pPlayer->SpdList[0];

	for (i = 0; i < MAXBELTITEMS; i++) {
		bool isHellfire = netSync ? ((pki->dwBuff & CF_HELLFIRE) != 0) : pPack->bIsHellfire;
		UnPackItem(pki, pi, isHellfire);
		pki++;
		pi++;
	}

	if (pnum == myplr) {
		for (i = 0; i < 20; i++)
			witchitem[i]._itype = ITYPE_NONE;
	}

	CalcPlrInv(pnum, false);
	pPlayer->wReflections = SwapLE16(pPack->wReflections);
	pPlayer->pTownWarps = 0;
	pPlayer->pDungMsgs = 0;
	pPlayer->pDungMsgs2 = 0;
	pPlayer->pLvlLoad = 0;
	pPlayer->pDiabloKillLevel = SwapLE32(pPack->pDiabloKillLevel);
	pPlayer->pBattleNet = pPack->pBattleNet;
	pPlayer->pManaShield = SwapLE32(pPack->pManaShield);
	pPlayer->pDifficulty = SwapLE32(pPack->pDifficulty);
	pPlayer->pDamAcFlags = SwapLE32(pPack->pDamAcFlags);
}

} // namespace devilution
