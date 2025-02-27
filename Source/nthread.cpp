/**
 * @file nthread.cpp
 *
 * Implementation of functions for managing game ticks.
 */
#include "all.h"
#include "../3rdParty/Storm/Source/storm.h"

namespace devilution {

BYTE sgbNetUpdateRate;
DWORD gdwMsgLenTbl[MAX_PLRS];
static CCritSect sgMemCrit;
DWORD gdwDeltaBytesSec;
bool nthread_should_run;
DWORD gdwTurnsInTransit;
uintptr_t glpMsgTbl[MAX_PLRS];
SDL_threadID glpNThreadId;
char sgbSyncCountdown;
int turn_upper_bit;
bool sgbTicsOutOfSync;
char sgbPacketCountdown;
bool sgbThreadIsRunning;
DWORD gdwLargestMsgSize;
DWORD gdwNormalMsgSize;
int last_tick;

/* data */
static SDL_Thread *sghThread = NULL;

void nthread_terminate_game(const char *pszFcn)
{
	DWORD sErr;

	sErr = SErrGetLastError();
	if (sErr == STORM_ERROR_INVALID_PLAYER) {
		return;
	} else if (sErr == STORM_ERROR_GAME_TERMINATED) {
		gbGameDestroyed = true;
	} else if (sErr == STORM_ERROR_NOT_IN_GAME) {
		gbGameDestroyed = true;
	} else {
		app_fatal("%s:\n%s", pszFcn, SDL_GetError());
	}
}

DWORD nthread_send_and_recv_turn(DWORD cur_turn, int turn_delta)
{
	DWORD new_cur_turn;
	int turn, turn_tmp;
	DWORD curTurnsInTransit;

	new_cur_turn = cur_turn;
	if (!SNetGetTurnsInTransit(&curTurnsInTransit)) {
		nthread_terminate_game("SNetGetTurnsInTransit");
		return 0;
	}
	while (curTurnsInTransit++ < gdwTurnsInTransit) {

		turn_tmp = turn_upper_bit | (new_cur_turn & 0x7FFFFFFF);
		turn_upper_bit = 0;
		turn = turn_tmp;

		if (!SNetSendTurn((char *)&turn, sizeof(turn))) {
			nthread_terminate_game("SNetSendTurn");
			return 0;
		}

		new_cur_turn += turn_delta;
		if (new_cur_turn >= 0x7FFFFFFF)
			new_cur_turn &= 0xFFFF;
	}
	return new_cur_turn;
}

bool nthread_recv_turns(bool *pfSendAsync)
{
	*pfSendAsync = false;
	sgbPacketCountdown--;
	if (sgbPacketCountdown) {
		last_tick += gnTickDelay;
		return true;
	}
	sgbSyncCountdown--;
	sgbPacketCountdown = sgbNetUpdateRate;
	if (sgbSyncCountdown != 0) {

		*pfSendAsync = true;
		last_tick += gnTickDelay;
		return true;
	}
#ifdef __3DS__
	return false;
#else
	if (!SNetReceiveTurns(0, MAX_PLRS, (char **)glpMsgTbl, gdwMsgLenTbl, (LPDWORD)player_state)) {
		if (SErrGetLastError() != STORM_ERROR_NO_MESSAGES_WAITING)
			nthread_terminate_game("SNetReceiveTurns");
		sgbTicsOutOfSync = false;
		sgbSyncCountdown = 1;
		sgbPacketCountdown = 1;
		return false;
	} else {
		if (!sgbTicsOutOfSync) {
			sgbTicsOutOfSync = true;
			last_tick = SDL_GetTicks();
		}
		sgbSyncCountdown = 4;
		multi_msg_countdown();
		*pfSendAsync = true;
		last_tick += gnTickDelay;
		return true;
	}
#endif
}

static unsigned int nthread_handler(void *data)
{
	int delta;
	bool received;

	if (nthread_should_run) {
		while (1) {
			sgMemCrit.Enter();
			if (!nthread_should_run)
				break;
			nthread_send_and_recv_turn(0, 0);
			if (nthread_recv_turns(&received))
				delta = last_tick - SDL_GetTicks();
			else
				delta = gnTickDelay;
			sgMemCrit.Leave();
			if (delta > 0)
				SDL_Delay(delta);
			if (!nthread_should_run)
				return 0;
		}
		sgMemCrit.Leave();
	}
	return 0;
}

void nthread_set_turn_upper_bit()
{
	turn_upper_bit = 0x80000000;
}

void nthread_start(bool set_turn_upper_bit)
{
	const char *err, *err2;
	DWORD largestMsgSize;
	_SNETCAPS caps;

	last_tick = SDL_GetTicks();
	sgbPacketCountdown = 1;
	sgbSyncCountdown = 1;
	sgbTicsOutOfSync = true;
	if (set_turn_upper_bit)
		nthread_set_turn_upper_bit();
	else
		turn_upper_bit = 0;
	caps.size = 36;
	if (!SNetGetProviderCaps(&caps)) {
		err = SDL_GetError();
		app_fatal("SNetGetProviderCaps:\n%s", err);
	}
	gdwTurnsInTransit = caps.defaultturnsintransit;
	if (!caps.defaultturnsintransit)
		gdwTurnsInTransit = 1;
	if (caps.defaultturnssec <= 20 && caps.defaultturnssec)
		sgbNetUpdateRate = 20 / caps.defaultturnssec;
	else
		sgbNetUpdateRate = 1;
	largestMsgSize = 512;
	if (caps.maxmessagesize < 0x200)
		largestMsgSize = caps.maxmessagesize;
	gdwDeltaBytesSec = caps.bytessec >> 2;
	gdwLargestMsgSize = largestMsgSize;
	gdwNormalMsgSize = caps.bytessec * sgbNetUpdateRate / 20;
	gdwNormalMsgSize *= 3;
	gdwNormalMsgSize >>= 2;
	if (caps.maxplayers > MAX_PLRS)
		caps.maxplayers = MAX_PLRS;
	gdwNormalMsgSize /= caps.maxplayers;
	while (gdwNormalMsgSize < 0x80) {
		gdwNormalMsgSize *= 2;
		sgbNetUpdateRate *= 2;
	}
	if (gdwNormalMsgSize > largestMsgSize)
		gdwNormalMsgSize = largestMsgSize;
	if (gbIsMultiplayer) {
		sgbThreadIsRunning = false;
		sgMemCrit.Enter();
		nthread_should_run = true;
		sghThread = CreateThread(nthread_handler, &glpNThreadId);
		if (sghThread == NULL) {
			err2 = SDL_GetError();
			app_fatal("nthread2:\n%s", err2);
		}
	}
}

void nthread_cleanup()
{
	nthread_should_run = false;
	gdwTurnsInTransit = 0;
	gdwNormalMsgSize = 0;
	gdwLargestMsgSize = 0;
	if (sghThread != NULL && glpNThreadId != SDL_GetThreadID(NULL)) {
		if (!sgbThreadIsRunning)
			sgMemCrit.Leave();
		SDL_WaitThread(sghThread, NULL);
		sghThread = NULL;
	}
}

void nthread_ignore_mutex(bool bStart)
{
	if (sghThread != NULL) {
		if (bStart)
			sgMemCrit.Leave();
		else
			sgMemCrit.Enter();
		sgbThreadIsRunning = bStart;
	}
}

/**
 * @brief Checks if it's time for the logic to advance
 * @return True if the engine should tick
 */
bool nthread_has_500ms_passed()
{
	DWORD currentTickCount;
	int ticksElapsed;

	currentTickCount = SDL_GetTicks();
	ticksElapsed = currentTickCount - last_tick;
	if (!gbIsMultiplayer && ticksElapsed > gnTickDelay * 10) {
		last_tick = currentTickCount;
		ticksElapsed = 0;
	}
	return ticksElapsed >= 0;
}

} // namespace devilution
