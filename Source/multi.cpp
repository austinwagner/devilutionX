/**
 * @file multi.cpp
 *
 * Implementation of functions for keeping multiplaye games in sync.
 */
#include "all.h"
#include "options.h"
#include "../3rdParty/Storm/Source/storm.h"
#include "../DiabloUI/diabloui.h"
#include <config.h>

namespace devilution {

bool gbSomebodyWonGameKludge;
TBuffer sgHiPriBuf;
char szPlayerDescript[128];
WORD sgwPackPlrOffsetTbl[MAX_PLRS];
PkPlayerStruct netplr[MAX_PLRS];
bool sgbPlayerTurnBitTbl[MAX_PLRS];
bool sgbPlayerLeftGameTbl[MAX_PLRS];
DWORD sgbSentThisCycle;
bool gbShouldValidatePackage;
BYTE gbActivePlayers;
bool gbGameDestroyed;
bool sgbSendDeltaTbl[MAX_PLRS];
GameData sgGameInitInfo;
bool gbSelectProvider;
int sglTimeoutStart;
int sgdwPlayerLeftReasonTbl[MAX_PLRS];
TBuffer sgLoPriBuf;
DWORD sgdwGameLoops;
/**
 * Specifies the maximum number of players in a game, where 1
 * represents a single player game and 4 represents a multi player game.
 */
bool gbIsMultiplayer;
bool sgbTimeout;
char szPlayerName[128];
BYTE gbDeltaSender;
bool sgbNetInited;
int player_state[MAX_PLRS];

/**
 * Contains the set of supported event types supported by the multiplayer
 * event handler.
 */
const event_type event_types[3] = {
	EVENT_TYPE_PLAYER_LEAVE_GAME,
	EVENT_TYPE_PLAYER_CREATE_GAME,
	EVENT_TYPE_PLAYER_MESSAGE
};

static void buffer_init(TBuffer *pBuf)
{
	pBuf->dwNextWriteOffset = 0;
	pBuf->bData[0] = 0;
}

// Microsoft VisualC 2-11/net runtime
static int multi_check_pkt_valid(TBuffer *pBuf)
{
	return pBuf->dwNextWriteOffset == 0;
}

static void multi_copy_packet(TBuffer *buf, void *packet, BYTE size)
{
	BYTE *p;

	if (buf->dwNextWriteOffset + size + 2 > 0x1000) {
		return;
	}

	p = &buf->bData[buf->dwNextWriteOffset];
	buf->dwNextWriteOffset += size + 1;
	*p = size;
	p++;
	memcpy(p, packet, size);
	p[size] = 0;
}

static BYTE *multi_recv_packet(TBuffer *pBuf, BYTE *body, DWORD *size)
{
	BYTE *src_ptr;
	size_t chunk_size;

	if (pBuf->dwNextWriteOffset != 0) {
		src_ptr = pBuf->bData;
		while (true) {
			if (*src_ptr == 0)
				break;
			chunk_size = *src_ptr;
			if (chunk_size > *size)
				break;
			src_ptr++;
			memcpy(body, src_ptr, chunk_size);
			body += chunk_size;
			src_ptr += chunk_size;
			*size -= chunk_size;
		}
		memcpy(pBuf->bData, src_ptr, (pBuf->bData - src_ptr) + pBuf->dwNextWriteOffset + 1);
		pBuf->dwNextWriteOffset += (pBuf->bData - src_ptr);
		return body;
	}
	return body;
}

static void NetRecvPlrData(TPkt *pkt)
{
	pkt->hdr.wCheck = LOAD_BE32("\0\0ip");
	pkt->hdr.px = plr[myplr]._px;
	pkt->hdr.py = plr[myplr]._py;
	pkt->hdr.targx = plr[myplr]._ptargx;
	pkt->hdr.targy = plr[myplr]._ptargy;
	pkt->hdr.php = plr[myplr]._pHitPoints;
	pkt->hdr.pmhp = plr[myplr]._pMaxHP;
	pkt->hdr.bstr = plr[myplr]._pBaseStr;
	pkt->hdr.bmag = plr[myplr]._pBaseMag;
	pkt->hdr.bdex = plr[myplr]._pBaseDex;
}

void multi_msg_add(BYTE *pbMsg, BYTE bLen)
{
	if (pbMsg && bLen) {
		tmsg_add(pbMsg, bLen);
	}
}

static void multi_send_packet(void *packet, BYTE dwSize)
{
	TPkt pkt;

	NetRecvPlrData(&pkt);
	pkt.hdr.wLen = dwSize + sizeof(pkt.hdr);
	memcpy(pkt.body, packet, dwSize);
	if (!SNetSendMessage(myplr, &pkt.hdr, pkt.hdr.wLen))
		nthread_terminate_game("SNetSendMessage0");
}

void NetSendLoPri(BYTE *pbMsg, BYTE bLen)
{
	if (pbMsg && bLen) {
		multi_copy_packet(&sgLoPriBuf, pbMsg, bLen);
		multi_send_packet(pbMsg, bLen);
	}
}

void NetSendHiPri(BYTE *pbMsg, BYTE bLen)
{
	BYTE *hipri_body;
	BYTE *lowpri_body;
	DWORD size, len;
	TPkt pkt;

	if (pbMsg && bLen) {
		multi_copy_packet(&sgHiPriBuf, pbMsg, bLen);
		multi_send_packet(pbMsg, bLen);
	}
	if (!gbShouldValidatePackage) {
		gbShouldValidatePackage = true;
		NetRecvPlrData(&pkt);
		size = gdwNormalMsgSize - sizeof(TPktHdr);
		hipri_body = multi_recv_packet(&sgHiPriBuf, pkt.body, &size);
		lowpri_body = multi_recv_packet(&sgLoPriBuf, hipri_body, &size);
		size = sync_all_monsters(lowpri_body, size);
		len = gdwNormalMsgSize - size;
		pkt.hdr.wLen = len;
		if (!SNetSendMessage(-2, &pkt.hdr, len))
			nthread_terminate_game("SNetSendMessage");
	}
}

void multi_send_msg_packet(int pmask, BYTE *src, BYTE len)
{
	DWORD v, p, t;
	TPkt pkt;

	NetRecvPlrData(&pkt);
	t = len + sizeof(pkt.hdr);
	pkt.hdr.wLen = t;
	memcpy(pkt.body, src, len);
	for (v = 1, p = 0; p < MAX_PLRS; p++, v <<= 1) {
		if (v & pmask) {
			if (!SNetSendMessage(p, &pkt.hdr, t) && SErrGetLastError() != STORM_ERROR_INVALID_PLAYER) {
				nthread_terminate_game("SNetSendMessage");
				return;
			}
		}
	}
}

static void multi_mon_seeds()
{
	int i;
	DWORD l;

	sgdwGameLoops++;
	l = (sgdwGameLoops >> 8) | (sgdwGameLoops << 24); // _rotr(sgdwGameLoops, 8)
	for (i = 0; i < MAXMONSTERS; i++)
		monster[i]._mAISeed = l + i;
}

static void multi_handle_turn_upper_bit(int pnum)
{
	int i;

	for (i = 0; i < MAX_PLRS; i++) {
		if (player_state[i] & PS_CONNECTED && i != pnum)
			break;
	}

	if (myplr == i) {
		sgbSendDeltaTbl[pnum] = true;
	} else if (myplr == pnum) {
		gbDeltaSender = i;
	}
}

static void multi_parse_turn(int pnum, int turn)
{
	DWORD absTurns;

	if (turn >> 31)
		multi_handle_turn_upper_bit(pnum);
	absTurns = turn & 0x7FFFFFFF;
	if (sgbSentThisCycle < gdwTurnsInTransit + absTurns) {
		if (absTurns >= 0x7FFFFFFF)
			absTurns &= 0xFFFF;
		sgbSentThisCycle = absTurns + gdwTurnsInTransit;
		sgdwGameLoops = 4 * absTurns * sgbNetUpdateRate;
	}
}

void multi_msg_countdown()
{
	int i;

	for (i = 0; i < MAX_PLRS; i++) {
		if (player_state[i] & PS_TURN_ARRIVED) {
			if (gdwMsgLenTbl[i] == 4)
				multi_parse_turn(i, *(DWORD *)glpMsgTbl[i]);
		}
	}
}

static void multi_player_left_msg(int pnum, int left)
{
	const char *pszFmt;

	if (plr[pnum].plractive) {
		RemovePlrFromMap(pnum);
		RemovePortalMissile(pnum);
		DeactivatePortal(pnum);
		delta_close_portal(pnum);
		RemovePlrMissiles(pnum);
		if (left) {
			pszFmt = "Player '%s' just left the game";
			switch (sgdwPlayerLeftReasonTbl[pnum]) {
			case LEAVE_ENDING:
				pszFmt = "Player '%s' killed Diablo and left the game!";
				gbSomebodyWonGameKludge = true;
				break;
			case LEAVE_DROP:
				pszFmt = "Player '%s' dropped due to timeout";
				break;
			}
			EventPlrMsg(pszFmt, plr[pnum]._pName);
		}
		plr[pnum].plractive = false;
		plr[pnum]._pName[0] = '\0';
		FreePlayerGFX(pnum);
		gbActivePlayers--;
	}
}

static void multi_clear_left_tbl()
{
	int i;

	for (i = 0; i < MAX_PLRS; i++) {
		if (sgbPlayerLeftGameTbl[i]) {
			if (gbBufferMsgs == 1)
				msg_send_drop_pkt(i, sgdwPlayerLeftReasonTbl[i]);
			else
				multi_player_left_msg(i, 1);

			sgbPlayerLeftGameTbl[i] = false;
			sgdwPlayerLeftReasonTbl[i] = 0;
		}
	}
}

void multi_player_left(int pnum, int reason)
{
	sgbPlayerLeftGameTbl[pnum] = true;
	sgdwPlayerLeftReasonTbl[pnum] = reason;
	multi_clear_left_tbl();
}

void multi_net_ping()
{
	sgbTimeout = true;
	sglTimeoutStart = SDL_GetTicks();
}

static void multi_check_drop_player()
{
	int i;

	for (i = 0; i < MAX_PLRS; i++) {
		if (!(player_state[i] & PS_ACTIVE) && player_state[i] & PS_CONNECTED) {
			SNetDropPlayer(i, LEAVE_DROP);
		}
	}
}

static void multi_begin_timeout()
{
	int i, nTicks, nState, nLowestActive, nLowestPlayer;
	BYTE bGroupPlayers, bGroupCount;

	if (!sgbTimeout) {
		return;
	}
#ifdef _DEBUG
	if (debug_mode_key_i) {
		return;
	}
#endif

	nTicks = SDL_GetTicks() - sglTimeoutStart;
	if (nTicks > 20000) {
		gbRunGame = false;
		return;
	}
	if (nTicks < 10000) {
		return;
	}

	nLowestActive = -1;
	nLowestPlayer = -1;
	bGroupPlayers = 0;
	bGroupCount = 0;
	for (i = 0; i < MAX_PLRS; i++) {
		nState = player_state[i];
		if (nState & PS_CONNECTED) {
			if (nLowestPlayer == -1) {
				nLowestPlayer = i;
			}
			if (nState & PS_ACTIVE) {
				bGroupPlayers++;
				if (nLowestActive == -1) {
					nLowestActive = i;
				}
			} else {
				bGroupCount++;
			}
		}
	}

	/// ASSERT: assert(bGroupPlayers);
	/// ASSERT: assert(nLowestActive != -1);
	/// ASSERT: assert(nLowestPlayer != -1);

	if (bGroupPlayers < bGroupCount) {
		gbGameDestroyed = true;
	} else if (bGroupPlayers == bGroupCount) {
		if (nLowestPlayer != nLowestActive) {
			gbGameDestroyed = true;
		} else if (nLowestActive == myplr) {
			multi_check_drop_player();
		}
	} else if (nLowestActive == myplr) {
		multi_check_drop_player();
	}
}

/**
 * @return Always true for singleplayer
 */
int multi_handle_delta()
{
	int i;
	bool received;

	if (gbGameDestroyed) {
		gbRunGame = false;
		return false;
	}

	for (i = 0; i < MAX_PLRS; i++) {
		if (sgbSendDeltaTbl[i]) {
			sgbSendDeltaTbl[i] = false;
			DeltaExportData(i);
		}
	}

	sgbSentThisCycle = nthread_send_and_recv_turn(sgbSentThisCycle, 1);
	if (!nthread_recv_turns(&received)) {
		multi_begin_timeout();
		return false;
	}

	sgbTimeout = false;
	if (received) {
		if (!gbShouldValidatePackage) {
			NetSendHiPri(0, 0);
			gbShouldValidatePackage = false;
		} else {
			gbShouldValidatePackage = false;
			if (!multi_check_pkt_valid(&sgHiPriBuf))
				NetSendHiPri(0, 0);
		}
	}
	multi_mon_seeds();

	return true;
}

static void multi_handle_all_packets(int pnum, BYTE *pData, int nSize)
{
	int nLen;

	while (nSize != 0) {
		nLen = ParseCmd(pnum, (TCmd *)pData);
		if (nLen == 0) {
			break;
		}
		pData += nLen;
		nSize -= nLen;
	}
}

static void multi_process_tmsgs()
{
	int cnt;
	TPkt pkt;

	while ((cnt = tmsg_get((BYTE *)&pkt, 512)) != 0) {
		multi_handle_all_packets(myplr, (BYTE *)&pkt, cnt);
	}
}

void multi_process_network_packets()
{
	int dx, dy;
	TPktHdr *pkt;
	DWORD dwMsgSize;
	DWORD dwID;
	bool cond;
	char *data;

	multi_clear_left_tbl();
	multi_process_tmsgs();
	while (SNetReceiveMessage((int *)&dwID, &data, (int *)&dwMsgSize)) {
		dwRecCount++;
		multi_clear_left_tbl();
		pkt = (TPktHdr *)data;
		if (dwMsgSize < sizeof(TPktHdr))
			continue;
		if (dwID >= MAX_PLRS)
			continue;
		if (pkt->wCheck != LOAD_BE32("\0\0ip"))
			continue;
		if (pkt->wLen != dwMsgSize)
			continue;
		plr[dwID]._pownerx = pkt->px;
		plr[dwID]._pownery = pkt->py;
		if (dwID != myplr) {
			// ASSERT: gbBufferMsgs != BUFFER_PROCESS (2)
			plr[dwID]._pHitPoints = pkt->php;
			plr[dwID]._pMaxHP = pkt->pmhp;
			cond = gbBufferMsgs == 1;
			plr[dwID]._pBaseStr = pkt->bstr;
			plr[dwID]._pBaseMag = pkt->bmag;
			plr[dwID]._pBaseDex = pkt->bdex;
			if (!cond && plr[dwID].plractive && plr[dwID]._pHitPoints != 0) {
				if (currlevel == plr[dwID].plrlevel && !plr[dwID]._pLvlChanging) {
					dx = abs(plr[dwID]._px - pkt->px);
					dy = abs(plr[dwID]._py - pkt->py);
					if ((dx > 3 || dy > 3) && dPlayer[pkt->px][pkt->py] == 0) {
						FixPlrWalkTags(dwID);
						plr[dwID]._poldx = plr[dwID]._px;
						plr[dwID]._poldy = plr[dwID]._py;
						FixPlrWalkTags(dwID);
						plr[dwID]._px = pkt->px;
						plr[dwID]._py = pkt->py;
						plr[dwID]._pfutx = pkt->px;
						plr[dwID]._pfuty = pkt->py;
						dPlayer[plr[dwID]._px][plr[dwID]._py] = dwID + 1;
					}
					dx = abs(plr[dwID]._pfutx - plr[dwID]._px);
					dy = abs(plr[dwID]._pfuty - plr[dwID]._py);
					if (dx > 1 || dy > 1) {
						plr[dwID]._pfutx = plr[dwID]._px;
						plr[dwID]._pfuty = plr[dwID]._py;
					}
					MakePlrPath(dwID, pkt->targx, pkt->targy, true);
				} else {
					plr[dwID]._px = pkt->px;
					plr[dwID]._py = pkt->py;
					plr[dwID]._pfutx = pkt->px;
					plr[dwID]._pfuty = pkt->py;
					plr[dwID]._ptargx = pkt->targx;
					plr[dwID]._ptargy = pkt->targy;
				}
			}
		}
		multi_handle_all_packets(dwID, (BYTE *)(pkt + 1), dwMsgSize - sizeof(TPktHdr));
	}
	if (SErrGetLastError() != STORM_ERROR_NO_MESSAGES_WAITING)
		nthread_terminate_game("SNetReceiveMsg");
}

void multi_send_zero_packet(int pnum, _cmd_id bCmd, BYTE *pbSrc, DWORD dwLen)
{
	DWORD dwOffset, dwBody, dwMsg;
	TPkt pkt;
	TCmdPlrInfoHdr *p;

	/// ASSERT: assert(pnum != myplr);
	/// ASSERT: assert(pbSrc);
	/// ASSERT: assert(dwLen <= 0x0ffff);

	dwOffset = 0;

	while (dwLen != 0) {
		pkt.hdr.wCheck = LOAD_BE32("\0\0ip");
		pkt.hdr.px = 0;
		pkt.hdr.py = 0;
		pkt.hdr.targx = 0;
		pkt.hdr.targy = 0;
		pkt.hdr.php = 0;
		pkt.hdr.pmhp = 0;
		pkt.hdr.bstr = 0;
		pkt.hdr.bmag = 0;
		pkt.hdr.bdex = 0;
		p = (TCmdPlrInfoHdr *)pkt.body;
		p->bCmd = bCmd;
		p->wOffset = dwOffset;
		dwBody = gdwLargestMsgSize - sizeof(pkt.hdr) - sizeof(*p);
		if (dwLen < dwBody) {
			dwBody = dwLen;
		}
		/// ASSERT: assert(dwBody <= 0x0ffff);
		p->wBytes = dwBody;
		memcpy(&pkt.body[sizeof(*p)], pbSrc, p->wBytes);
		dwMsg = sizeof(pkt.hdr);
		dwMsg += sizeof(*p);
		dwMsg += p->wBytes;
		pkt.hdr.wLen = dwMsg;
		if (!SNetSendMessage(pnum, &pkt, dwMsg)) {
			nthread_terminate_game("SNetSendMessage2");
			return;
		}
#if 0
		if((DWORD)pnum >= MAX_PLRS) {
			if(myplr != 0) {
				debug_plr_tbl[0]++;
			}
			if(myplr != 1) {
				debug_plr_tbl[1]++;
			}
			if(myplr != 2) {
				debug_plr_tbl[2]++;
			}
			if(myplr != 3) {
				debug_plr_tbl[3]++;
			}
		} else {
			debug_plr_tbl[pnum]++;
		}
#endif
		pbSrc += p->wBytes;
		dwLen -= p->wBytes;
		dwOffset += p->wBytes;
	}
}

static void multi_send_pinfo(int pnum, char cmd)
{
	PkPlayerStruct pkplr;

	PackPlayer(&pkplr, myplr, true);
	dthread_send_delta(pnum, cmd, &pkplr, sizeof(pkplr));
}

static dungeon_type InitLevelType(int l)
{
	if (l == 0)
		return DTYPE_TOWN;
	if (l >= 1 && l <= 4)
		return DTYPE_CATHEDRAL;
	if (l >= 5 && l <= 8)
		return DTYPE_CATACOMBS;
	if (l >= 9 && l <= 12)
		return DTYPE_CAVES;
	if (l >= 13 && l <= 16)
		return DTYPE_HELL;
	if (l >= 21 && l <= 24)
		return DTYPE_CATHEDRAL; // Crypt
	if (l >= 17 && l <= 20)
		return DTYPE_CAVES; // Hive

	return DTYPE_CATHEDRAL;
}

static void SetupLocalCoords()
{
	int x, y;

	if (!leveldebug || gbIsMultiplayer) {
		currlevel = 0;
		leveltype = DTYPE_TOWN;
		setlevel = false;
	}
	x = 75;
	y = 68;
#ifdef _DEBUG
	if (debug_mode_key_inverted_v || debug_mode_key_d) {
		x = 49;
		y = 23;
	}
#endif
	x += plrxoff[myplr];
	y += plryoff[myplr];
	plr[myplr]._px = x;
	plr[myplr]._py = y;
	plr[myplr]._pfutx = x;
	plr[myplr]._pfuty = y;
	plr[myplr]._ptargx = x;
	plr[myplr]._ptargy = y;
	plr[myplr].plrlevel = currlevel;
	plr[myplr]._pLvlChanging = true;
	plr[myplr].pLvlLoad = 0;
	plr[myplr]._pmode = PM_NEWLVL;
	plr[myplr].destAction = ACTION_NONE;
}

static bool multi_upgrade(bool *pfExitProgram)
{
	bool result;
	int status;

	SNetPerformUpgrade((LPDWORD)&status);
	result = true;
	if (status && status != 1) {
		if (status != 2) {
			if (status == -1) {
				DrawDlg("Network upgrade failed");
			}
		} else {
			*pfExitProgram = 1;
		}

		result = false;
	}

	return result;
}

static void multi_handle_events(_SNETEVENT *pEvt)
{
	DWORD LeftReason;
	GameData *gameData;

	switch (pEvt->eventid) {
	case EVENT_TYPE_PLAYER_CREATE_GAME: {
		GameData *gameData = (GameData *)pEvt->data;
		if (gameData->size != sizeof(GameData))
			app_fatal("Invalid size of game data: %d", gameData->size);
		sgGameInitInfo = *gameData;
		sgbPlayerTurnBitTbl[pEvt->playerid] = true;
		break;
	}
	case EVENT_TYPE_PLAYER_LEAVE_GAME:
		sgbPlayerLeftGameTbl[pEvt->playerid] = true;
		sgbPlayerTurnBitTbl[pEvt->playerid] = false;

		LeftReason = 0;
		if (pEvt->data && pEvt->databytes >= sizeof(DWORD))
			LeftReason = *(DWORD *)pEvt->data;
		sgdwPlayerLeftReasonTbl[pEvt->playerid] = LeftReason;
		if (LeftReason == LEAVE_ENDING)
			gbSomebodyWonGameKludge = true;

		sgbSendDeltaTbl[pEvt->playerid] = false;
		dthread_remove_player(pEvt->playerid);

		if (gbDeltaSender == pEvt->playerid)
			gbDeltaSender = MAX_PLRS;
		break;
	case EVENT_TYPE_PLAYER_MESSAGE:
		ErrorPlrMsg((char *)pEvt->data);
		break;
	}
}

static void multi_event_handler(bool add)
{
	DWORD i;
	bool (*fn)(event_type, SEVTHANDLER);

	if (add)
		fn = SNetRegisterEventHandler;
	else
		fn = SNetUnregisterEventHandler;

	for (i = 0; i < 3; i++) {
		if (!fn(event_types[i], multi_handle_events) && add) {
			app_fatal("SNetRegisterEventHandler:\n%s", SDL_GetError());
		}
	}
}

void NetClose()
{
	if (!sgbNetInited) {
		return;
	}

	sgbNetInited = false;
	nthread_cleanup();
	dthread_cleanup();
	tmsg_cleanup();
	multi_event_handler(false);
	SNetLeaveGame(3);
	if (gbIsMultiplayer)
		SDL_Delay(2000);
}

bool NetInit(bool bSinglePlayer, bool *pfExitProgram)
{
	while (1) {
		*pfExitProgram = false;
		SetRndSeed(0);
		sgGameInitInfo.size = sizeof(sgGameInitInfo);
		sgGameInitInfo.dwSeed = time(NULL);
		sgGameInitInfo.programid = GAME_ID;
		sgGameInitInfo.versionMajor = PROJECT_VERSION_MAJOR;
		sgGameInitInfo.versionMinor = PROJECT_VERSION_MINOR;
		sgGameInitInfo.versionPatch = PROJECT_VERSION_PATCH;
		sgGameInitInfo.nDifficulty = gnDifficulty;
		sgGameInitInfo.nTickRate = sgOptions.Gameplay.nTickRate;
		sgGameInitInfo.bRunInTown = sgOptions.Gameplay.bRunInTown;
		sgGameInitInfo.bTheoQuest = sgOptions.Gameplay.bTheoQuest;
		sgGameInitInfo.bCowQuest = sgOptions.Gameplay.bCowQuest;
		sgGameInitInfo.bFriendlyFire = sgOptions.Gameplay.bFriendlyFire;
		memset(sgbPlayerTurnBitTbl, 0, sizeof(sgbPlayerTurnBitTbl));
		gbGameDestroyed = false;
		memset(sgbPlayerLeftGameTbl, 0, sizeof(sgbPlayerLeftGameTbl));
		memset(sgdwPlayerLeftReasonTbl, 0, sizeof(sgdwPlayerLeftReasonTbl));
		memset(sgbSendDeltaTbl, 0, sizeof(sgbSendDeltaTbl));
		memset(plr, 0, sizeof(plr));
		memset(sgwPackPlrOffsetTbl, 0, sizeof(sgwPackPlrOffsetTbl));
		SNetSetBasePlayer(0);
		if (bSinglePlayer) {
			if (!multi_init_single(&sgGameInitInfo))
				return false;
		} else {
			if (!multi_init_multi(&sgGameInitInfo, pfExitProgram))
				return false;
		}
		sgbNetInited = true;
		sgbTimeout = false;
		delta_init();
		InitPlrMsg();
		buffer_init(&sgHiPriBuf);
		buffer_init(&sgLoPriBuf);
		gbShouldValidatePackage = false;
		sync_init();
		nthread_start(sgbPlayerTurnBitTbl[myplr]);
		dthread_start();
		tmsg_start();
		sgdwGameLoops = 0;
		sgbSentThisCycle = 0;
		gbDeltaSender = myplr;
		gbSomebodyWonGameKludge = false;
		nthread_send_and_recv_turn(0, 0);
		SetupLocalCoords();
		multi_send_pinfo(-2, CMD_SEND_PLRINFO);

		InitPlrGFXMem(myplr);
		plr[myplr].plractive = true;
		gbActivePlayers = 1;

		if (!sgbPlayerTurnBitTbl[myplr] || msg_wait_resync())
			break;
		NetClose();
		gbSelectProvider = false;
	}
	SetRndSeed(sgGameInitInfo.dwSeed);
	gnDifficulty = sgGameInitInfo.nDifficulty;
	gnTickRate = sgGameInitInfo.nTickRate;
	gnTickDelay = 1000 / gnTickRate;
	gbRunInTown = sgGameInitInfo.bRunInTown;
	gbTheoQuest = sgGameInitInfo.bTheoQuest;
	gbCowQuest = sgGameInitInfo.bCowQuest;
	gbFriendlyFire = sgGameInitInfo.bFriendlyFire;

	for (int i = 0; i < NUMLEVELS; i++) {
		glSeedTbl[i] = AdvanceRndSeed();
		gnLevelTypeTbl[i] = InitLevelType(i);
	}
	if (!SNetGetGameInfo(GAMEINFO_NAME, szPlayerName, 128))
		nthread_terminate_game("SNetGetGameInfo1");
	if (!SNetGetGameInfo(GAMEINFO_PASSWORD, szPlayerDescript, 128))
		nthread_terminate_game("SNetGetGameInfo2");

	return true;
}

bool multi_init_single(GameData *gameData)
{
	int unused;

	if (!SNetInitializeProvider(SELCONN_LOOPBACK, gameData)) {
		SErrGetLastError();
		return false;
	}

	unused = 0;
	if (!SNetCreateGame("local", "local", "local", 0, (char *)&sgGameInitInfo, sizeof(sgGameInitInfo), 1, "local", "local", &unused)) {
		app_fatal("SNetCreateGame1:\n%s", SDL_GetError());
	}

	myplr = 0;
	gbIsMultiplayer = false;

	return true;
}

bool multi_init_multi(GameData *gameData, bool *pfExitProgram)
{
	bool first;
	int playerId;

	for (first = true;; first = false) {
		if (gbSelectProvider) {
			if (!UiSelectProvider(gameData)
			    && (!first || SErrGetLastError() != STORM_ERROR_REQUIRES_UPGRADE || !multi_upgrade(pfExitProgram))) {
				return false;
			}
		}

		multi_event_handler(true);
		if (UiSelectGame(gameData, &playerId))
			break;

		gbSelectProvider = true;
	}

	if ((DWORD)playerId >= MAX_PLRS) {
		return false;
	} else {
		myplr = playerId;
		gbIsMultiplayer = true;

		pfile_read_player_from_save();

		return true;
	}
}

void recv_plrinfo(int pnum, TCmdPlrInfoHdr *p, bool recv)
{
	const char *szEvent;

	if (myplr == pnum) {
		return;
	}
	/// ASSERT: assert((DWORD)pnum < MAX_PLRS);

	if (sgwPackPlrOffsetTbl[pnum] != p->wOffset) {
		sgwPackPlrOffsetTbl[pnum] = 0;
		if (p->wOffset != 0) {
			return;
		}
	}
	if (!recv && sgwPackPlrOffsetTbl[pnum] == 0) {
		multi_send_pinfo(pnum, CMD_ACK_PLRINFO);
	}

	memcpy((char *)&netplr[pnum] + p->wOffset, &p[1], p->wBytes); /* todo: cast? */
	sgwPackPlrOffsetTbl[pnum] += p->wBytes;
	if (sgwPackPlrOffsetTbl[pnum] != sizeof(*netplr)) {
		return;
	}

	sgwPackPlrOffsetTbl[pnum] = 0;
	multi_player_left_msg(pnum, 0);
	UnPackPlayer(&netplr[pnum], pnum, true);

	if (!recv) {
		return;
	}

	InitPlrGFXMem(pnum);
	plr[pnum].plractive = true;
	gbActivePlayers++;

	if (sgbPlayerTurnBitTbl[pnum]) {
		szEvent = "Player '%s' (level %d) just joined the game";
	} else {
		szEvent = "Player '%s' (level %d) is already in the game";
	}
	EventPlrMsg(szEvent, plr[pnum]._pName, plr[pnum]._pLevel);

	LoadPlrGFX(pnum, PFILE_STAND);
	SyncInitPlr(pnum);

	if (plr[pnum].plrlevel == currlevel) {
		if (plr[pnum]._pHitPoints >> 6 > 0) {
			StartStand(pnum, DIR_S);
		} else {
			plr[pnum]._pgfxnum = 0;
			LoadPlrGFX(pnum, PFILE_DEATH);
			plr[pnum]._pmode = PM_DEATH;
			NewPlrAnim(pnum, plr[pnum]._pDAnim[DIR_S], plr[pnum]._pDFrames, 1, plr[pnum]._pDWidth);
			plr[pnum]._pAnimFrame = plr[pnum]._pAnimLen - 1;
			plr[pnum]._pVar8 = 2 * plr[pnum]._pAnimLen;
			dFlags[plr[pnum]._px][plr[pnum]._py] |= BFLAG_DEAD_PLAYER;
		}
	}
}

} // namespace devilution
