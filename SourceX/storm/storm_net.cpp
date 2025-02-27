#include <memory>
#ifndef NONET
#include <mutex>
#include <thread>
#endif

#include "all.h"
#include "options.h"
#include "stubs.h"
#include "dvlnet/abstract_net.h"
#include "storm/storm_dvlnet.h"

namespace devilution {

static std::unique_ptr<net::abstract_net> dvlnet_inst;
static char gpszGameName[128] = {};
static char gpszGamePassword[128] = {};

#ifndef NONET
static std::mutex storm_net_mutex;
#endif

bool SNetReceiveMessage(int *senderplayerid, char **data, int *databytes)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	if (!dvlnet_inst->SNetReceiveMessage(senderplayerid, data, databytes)) {
		SErrSetLastError(STORM_ERROR_NO_MESSAGES_WAITING);
		return false;
	}
	return true;
}

bool SNetSendMessage(int playerID, void *data, unsigned int databytes)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	return dvlnet_inst->SNetSendMessage(playerID, data, databytes);
}

bool SNetReceiveTurns(int a1, int arraysize, char **arraydata, unsigned int *arraydatabytes,
    DWORD *arrayplayerstatus)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	if (a1 != 0)
		UNIMPLEMENTED();
	if (arraysize != MAX_PLRS)
		UNIMPLEMENTED();
	if (!dvlnet_inst->SNetReceiveTurns(arraydata, arraydatabytes, arrayplayerstatus)) {
		SErrSetLastError(STORM_ERROR_NO_MESSAGES_WAITING);
		return false;
	}
	return true;
}

bool SNetSendTurn(char *data, unsigned int databytes)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	return dvlnet_inst->SNetSendTurn(data, databytes);
}

int SNetGetProviderCaps(struct _SNETCAPS *caps)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	return dvlnet_inst->SNetGetProviderCaps(caps);
}

bool SNetUnregisterEventHandler(event_type evtype, SEVTHANDLER func)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	return dvlnet_inst->SNetUnregisterEventHandler(evtype, func);
}

bool SNetRegisterEventHandler(event_type evtype, SEVTHANDLER func)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	return dvlnet_inst->SNetRegisterEventHandler(evtype, func);
}

bool SNetDestroy()
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	return true;
}

bool SNetDropPlayer(int playerid, DWORD flags)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	return dvlnet_inst->SNetDropPlayer(playerid, flags);
}

bool SNetGetGameInfo(int type, void *dst, unsigned int length)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	switch (type) {
	case GAMEINFO_NAME:
		strncpy((char *)dst, gpszGameName, length);
		break;
	case GAMEINFO_PASSWORD:
		strncpy((char *)dst, gpszGamePassword, length);
		break;
	}

	return true;
}

bool SNetLeaveGame(int type)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	if (dvlnet_inst == NULL)
		return true;
	return dvlnet_inst->SNetLeaveGame(type);
}

/**
 * @brief Called by engine for single, called by ui for multi
 * @param provider BNET, IPXN, MODM, SCBL or UDPN
 */
int SNetInitializeProvider(Uint32 provider, struct GameData *gameData)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	dvlnet_inst = net::abstract_net::make_net(provider);
	return mainmenu_select_hero_dialog(gameData);
}

/**
 * @brief Called by engine for single, called by ui for multi
 */
bool SNetCreateGame(const char *pszGameName, const char *pszGamePassword, const char *pszGameStatString,
    DWORD dwGameType, char *GameTemplateData, int GameTemplateSize, int playerCount,
    const char *creatorName, const char *a11, int *playerID)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	if (GameTemplateSize != sizeof(GameData))
		ABORT();
	net::buffer_t game_init_info(GameTemplateData, GameTemplateData + GameTemplateSize);
	dvlnet_inst->setup_gameinfo(std::move(game_init_info));

	std::string default_name;
	if(!pszGameName) {
		default_name = dvlnet_inst->make_default_gamename();
		pszGameName = default_name.c_str();
	}

	strncpy(gpszGameName, pszGameName, sizeof(gpszGameName) - 1);
	if (pszGamePassword)
		strncpy(gpszGamePassword, pszGamePassword, sizeof(gpszGamePassword) - 1);
	*playerID = dvlnet_inst->create(pszGameName, pszGamePassword);
	return *playerID != -1;
}

bool SNetJoinGame(int id, char *pszGameName, char *pszGamePassword, char *playerName, char *userStats, int *playerID)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	if (pszGameName)
		strncpy(gpszGameName, pszGameName, sizeof(gpszGameName) - 1);
	if (pszGamePassword)
		strncpy(gpszGamePassword, pszGamePassword, sizeof(gpszGamePassword) - 1);
	*playerID = dvlnet_inst->join(pszGameName, pszGamePassword);
	return *playerID != -1;
}

/**
 * @brief Is this the mirror image of SNetGetTurnsInTransit?
 */
bool SNetGetOwnerTurnsWaiting(DWORD *turns)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	return dvlnet_inst->SNetGetOwnerTurnsWaiting(turns);
}

bool SNetGetTurnsInTransit(DWORD *turns)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	return dvlnet_inst->SNetGetTurnsInTransit(turns);
}

/**
 * @brief engine calls this only once with argument 1
 */
bool SNetSetBasePlayer(int)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	return true;
}

/**
 * @brief since we never signal STORM_ERROR_REQUIRES_UPGRADE the engine will not call this function
 */
bool SNetPerformUpgrade(DWORD *upgradestatus)
{
#ifndef NONET
	std::lock_guard<std::mutex> lg(storm_net_mutex);
#endif
	UNIMPLEMENTED();
}

void DvlNet_SendInfoRequest()
{
	dvlnet_inst->send_info_request();
}

std::vector<std::string> DvlNet_GetGamelist()
{
	return dvlnet_inst->get_gamelist();
}

void DvlNet_SetPassword(std::string pw)
{
	dvlnet_inst->setup_password(pw);
}

} // namespace devilution
