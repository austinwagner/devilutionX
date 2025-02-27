#include "selgame.h"

#include "all.h"
#include "../3rdParty/Storm/Source/storm.h"
#include "config.h"
#include "DiabloUI/diabloui.h"
#include "DiabloUI/text.h"
#include "DiabloUI/dialogs.h"
#include "DiabloUI/selok.h"
#include "DiabloUI/selhero.h"
#include "options.h"

namespace devilution {

char selgame_Label[32];
char selgame_Ip[129] = "";
char selgame_Password[16] = "";
char selgame_Description[256];
bool selgame_enteringGame;
int selgame_selectedGame;
bool selgame_endMenu;
int *gdwPlayerId;
int nDifficulty;
int nTickRate;
int heroLevel;

static GameData *m_game_data;
extern int provider;

#define DESCRIPTION_WIDTH 205

namespace {

char title[32];

std::vector<UiListItem *> vecSelGameDlgItems;
std::vector<UiItemBase *> vecSelGameDialog;

} // namespace

void selgame_FreeVectors()
{
	for (std::size_t i = 0; i < vecSelGameDlgItems.size(); i++) {
		UiListItem *pUIItem = vecSelGameDlgItems[i];
		delete pUIItem;
	}
	vecSelGameDlgItems.clear();

	for (std::size_t i = 0; i < vecSelGameDialog.size(); i++) {
		UiItemBase *pUIItem = vecSelGameDialog[i];
		delete pUIItem;
	}
	vecSelGameDialog.clear();
}

void selgame_Free()
{
	ArtBackground.Unload();

	selgame_FreeVectors();
}

void selgame_GameSelection_Init()
{
	selgame_enteringGame = false;
	selgame_selectedGame = 0;

	if (provider == SELCONN_LOOPBACK) {
		selgame_enteringGame = true;
		selgame_GameSelection_Select(0);
		return;
	}

	strcpy(selgame_Ip, sgOptions.Network.szPreviousHost);

	selgame_FreeVectors();

	UiAddBackground(&vecSelGameDialog);
	UiAddLogo(&vecSelGameDialog);

	SDL_Rect rect1 = { (Sint16)(PANEL_LEFT + 24), (Sint16)(UI_OFFSET_Y + 161), 590, 35 };
	vecSelGameDialog.push_back(new UiArtText("Client-Server (TCP)", rect1, UIS_CENTER | UIS_BIG));

	SDL_Rect rect2 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 211), 205, 192 };
	vecSelGameDialog.push_back(new UiArtText("Description:", rect2, UIS_MED));

	SDL_Rect rect3 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 256), DESCRIPTION_WIDTH, 192 };
	vecSelGameDialog.push_back(new UiArtText(selgame_Description, rect3));

	SDL_Rect rect4 = { (Sint16)(PANEL_LEFT + 300), (Sint16)(UI_OFFSET_Y + 211), 295, 33 };
	vecSelGameDialog.push_back(new UiArtText("Select Action", rect4, UIS_CENTER | UIS_BIG));

	vecSelGameDlgItems.push_back(new UiListItem("Create Game", 0));
	vecSelGameDlgItems.push_back(new UiListItem("Join Game", 1));

	vecSelGameDialog.push_back(new UiList(vecSelGameDlgItems, PANEL_LEFT + 305, (UI_OFFSET_Y + 255), 285, 26, UIS_CENTER | UIS_MED | UIS_GOLD));

	SDL_Rect rect5 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(new UiArtTextButton("OK", &UiFocusNavigationSelect, rect5, UIS_CENTER | UIS_VCENTER | UIS_BIG | UIS_GOLD));

	SDL_Rect rect6 = { (Sint16)(PANEL_LEFT + 449), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(new UiArtTextButton("CANCEL", &UiFocusNavigationEsc, rect6, UIS_CENTER | UIS_VCENTER | UIS_BIG | UIS_GOLD));

	UiInitList(vecSelGameDlgItems.size(), selgame_GameSelection_Focus, selgame_GameSelection_Select, selgame_GameSelection_Esc, vecSelGameDialog);
}

void selgame_GameSelection_Focus(int value)
{
	switch (vecSelGameDlgItems[value]->m_value) {
	case 0:
		strncpy(selgame_Description, "Create a new game with a difficulty setting of your choice.", sizeof(selgame_Description) - 1);
		break;
	case 1:
		strncpy(selgame_Description, "Enter an IP or a hostname and join a game already in progress at that address.", sizeof(selgame_Description) - 1);
		break;
	}
	WordWrapArtStr(selgame_Description, DESCRIPTION_WIDTH);
}

/**
 * @brief Load the current hero level from save file
 * @param pInfo Hero info
 * @return always true
 */
bool UpdateHeroLevel(_uiheroinfo *pInfo)
{
	if (strcasecmp(pInfo->name, gszHero) == 0)
		heroLevel = pInfo->level;

	return true;
}

void selgame_GameSelection_Select(int value)
{
	selgame_enteringGame = true;
	selgame_selectedGame = value;

	gfnHeroInfo(UpdateHeroLevel);

	selgame_FreeVectors();

	UiAddBackground(&vecSelGameDialog);
	UiAddLogo(&vecSelGameDialog);

	SDL_Rect rect1 = { (Sint16)(PANEL_LEFT + 24), (Sint16)(UI_OFFSET_Y + 161), 590, 35 };
	vecSelGameDialog.push_back(new UiArtText(title, rect1, UIS_CENTER | UIS_BIG));

	SDL_Rect rect2 = { (Sint16)(PANEL_LEFT + 34), (Sint16)(UI_OFFSET_Y + 211), 205, 33 };
	vecSelGameDialog.push_back(new UiArtText(selgame_Label, rect2, UIS_CENTER | UIS_BIG));

	SDL_Rect rect3 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 256), DESCRIPTION_WIDTH, 192 };
	vecSelGameDialog.push_back(new UiArtText(selgame_Description, rect3));

	switch (value) {
	case 0: {
		strncpy(title, "Create Game", sizeof(title) - 1);

		SDL_Rect rect4 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 211), 295, 35 };
		vecSelGameDialog.push_back(new UiArtText("Select Difficulty", rect4, UIS_CENTER | UIS_BIG));

		vecSelGameDlgItems.push_back(new UiListItem("Normal", DIFF_NORMAL));
		vecSelGameDlgItems.push_back(new UiListItem("Nightmare", DIFF_NIGHTMARE));
		vecSelGameDlgItems.push_back(new UiListItem("Hell", DIFF_HELL));

		vecSelGameDialog.push_back(new UiList(vecSelGameDlgItems, PANEL_LEFT + 300, (UI_OFFSET_Y + 282), 295, 26, UIS_CENTER | UIS_MED | UIS_GOLD));

		SDL_Rect rect5 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
		vecSelGameDialog.push_back(new UiArtTextButton("OK", &UiFocusNavigationSelect, rect5, UIS_CENTER | UIS_VCENTER | UIS_BIG | UIS_GOLD));

		SDL_Rect rect6 = { (Sint16)(PANEL_LEFT + 449), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
		vecSelGameDialog.push_back(new UiArtTextButton("CANCEL", &UiFocusNavigationEsc, rect6, UIS_CENTER | UIS_VCENTER | UIS_BIG | UIS_GOLD));

		UiInitList(vecSelGameDlgItems.size(), selgame_Diff_Focus, selgame_Diff_Select, selgame_Diff_Esc, vecSelGameDialog, true);
		break;
	}
	case 1: {
		strncpy(title, "Join TCP Games", sizeof(title) - 1);

		SDL_Rect rect4 = { (Sint16)(PANEL_LEFT + 305), (Sint16)(UI_OFFSET_Y + 211), 285, 33 };
		vecSelGameDialog.push_back(new UiArtText("Enter address", rect4, UIS_CENTER | UIS_BIG));

		SDL_Rect rect5 = { (Sint16)(PANEL_LEFT + 305), (Sint16)(UI_OFFSET_Y + 314), 285, 33 };
		vecSelGameDialog.push_back(new UiEdit(selgame_Ip, 128, rect5, UIS_MED | UIS_GOLD));

		SDL_Rect rect6 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
		vecSelGameDialog.push_back(new UiArtTextButton("OK", &UiFocusNavigationSelect, rect6, UIS_CENTER | UIS_VCENTER | UIS_BIG | UIS_GOLD));

		SDL_Rect rect7 = { (Sint16)(PANEL_LEFT + 449), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
		vecSelGameDialog.push_back(new UiArtTextButton("CANCEL", &UiFocusNavigationEsc, rect7, UIS_CENTER | UIS_VCENTER | UIS_BIG | UIS_GOLD));

		UiInitList(0, NULL, selgame_Password_Init, selgame_GameSelection_Init, vecSelGameDialog);
		break;
	}
	}
}

void selgame_GameSelection_Esc()
{
	UiInitList_clear();
	selgame_enteringGame = false;
	selgame_endMenu = true;
}

void selgame_Diff_Focus(int value)
{
	switch (vecSelGameDlgItems[value]->m_value) {
	case DIFF_NORMAL:
		strncpy(selgame_Label, "Normal", sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, "Normal Difficulty\nThis is where a starting character should begin the quest to defeat Diablo.", sizeof(selgame_Description) - 1);
		break;
	case DIFF_NIGHTMARE:
		strncpy(selgame_Label, "Nightmare", sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, "Nightmare Difficulty\nThe denizens of the Labyrinth have been bolstered and will prove to be a greater challenge. This is recommended for experienced characters only.", sizeof(selgame_Description) - 1);
		break;
	case DIFF_HELL:
		strncpy(selgame_Label, "Hell", sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, "Hell Difficulty\nThe most powerful of the underworld's creatures lurk at the gateway into Hell. Only the most experienced characters should venture in this realm.", sizeof(selgame_Description) - 1);
		break;
	}
	WordWrapArtStr(selgame_Description, DESCRIPTION_WIDTH);
}

bool IsDifficultyAllowed(int value)
{
	if (value == 0 || (value == 1 && heroLevel >= 20) || (value == 2 && heroLevel >= 30)) {
		return true;
	}

	selgame_Free();

	if (value == 1)
		UiSelOkDialog(title, "Your character must reach level 20 before you can enter a multiplayer game of Nightmare difficulty.", false);
	if (value == 2)
		UiSelOkDialog(title, "Your character must reach level 30 before you can enter a multiplayer game of Hell difficulty.", false);

	LoadBackgroundArt("ui_art\\selgame.pcx");

	return false;
}

void selgame_Diff_Select(int value)
{
	if (selhero_isMultiPlayer && !IsDifficultyAllowed(vecSelGameDlgItems[value]->m_value)) {
		selgame_GameSelection_Select(0);
		return;
	}

	nDifficulty = value;

	if (!selhero_isMultiPlayer) {
		// This is part of a dangerous hack to enable difficulty selection in single-player.
		// FIXME: Dialogs should not refer to each other's variables.

		// We're in the selhero loop instead of the selgame one.
		// Free the selgame data and flag the end of the selhero loop.
		selhero_endMenu = true;

		// We only call FreeVectors because ArtBackground.Unload()
		// will be called by selheroFree().
		selgame_FreeVectors();

		// We must clear the InitList because selhero's loop will perform
		// one more iteration after this function exits.
		UiInitList_clear();

		return;
	}

	selgame_GameSpeedSelection();
}

void selgame_Diff_Esc()
{
	if (!selhero_isMultiPlayer) {
		selgame_Free();

		selhero_Init();
		selhero_List_Init();
		return;
	}

	if (provider == SELCONN_LOOPBACK) {
		selgame_GameSelection_Esc();
		return;
	}

	selgame_GameSelection_Init();
}

void selgame_GameSpeedSelection()
{
	gfnHeroInfo(UpdateHeroLevel);

	selgame_FreeVectors();

	UiAddBackground(&vecSelGameDialog);
	UiAddLogo(&vecSelGameDialog);

	SDL_Rect rect1 = { (Sint16)(PANEL_LEFT + 24), (Sint16)(UI_OFFSET_Y + 161), 590, 35 };
	vecSelGameDialog.push_back(new UiArtText("Create Game", rect1, UIS_CENTER | UIS_BIG));

	SDL_Rect rect2 = { (Sint16)(PANEL_LEFT + 34), (Sint16)(UI_OFFSET_Y + 211), 205, 33 };
	vecSelGameDialog.push_back(new UiArtText(selgame_Label, rect2, UIS_CENTER | UIS_BIG));

	SDL_Rect rect3 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 256), DESCRIPTION_WIDTH, 192 };
	vecSelGameDialog.push_back(new UiArtText(selgame_Description, rect3));

	SDL_Rect rect4 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 211), 295, 35 };
	vecSelGameDialog.push_back(new UiArtText("Select Game Speed", rect4, UIS_CENTER | UIS_BIG));

	vecSelGameDlgItems.push_back(new UiListItem("Normal", 20));
	vecSelGameDlgItems.push_back(new UiListItem("Fast", 30));
	vecSelGameDlgItems.push_back(new UiListItem("Faster", 40));
	vecSelGameDlgItems.push_back(new UiListItem("Fastest", 50));

	vecSelGameDialog.push_back(new UiList(vecSelGameDlgItems, PANEL_LEFT + 300, (UI_OFFSET_Y + 279), 295, 26, UIS_CENTER | UIS_MED | UIS_GOLD));

	SDL_Rect rect5 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(new UiArtTextButton("OK", &UiFocusNavigationSelect, rect5, UIS_CENTER | UIS_VCENTER | UIS_BIG | UIS_GOLD));

	SDL_Rect rect6 = { (Sint16)(PANEL_LEFT + 449), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(new UiArtTextButton("CANCEL", &UiFocusNavigationEsc, rect6, UIS_CENTER | UIS_VCENTER | UIS_BIG | UIS_GOLD));

	UiInitList(vecSelGameDlgItems.size(), selgame_Speed_Focus, selgame_Speed_Select, selgame_Speed_Esc, vecSelGameDialog, true);
}

void selgame_Speed_Focus(int value)
{
	switch (vecSelGameDlgItems[value]->m_value) {
	case 20:
		strncpy(selgame_Label, "Normal", sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, "Normal Speed\nThis is where a starting character should begin the quest to defeat Diablo.", sizeof(selgame_Description) - 1);
		break;
	case 30:
		strncpy(selgame_Label, "Fast", sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, "Fast Speed\nThe denizens of the Labyrinth have been hastened and will prove to be a greater challenge. This is recommended for experienced characters only.", sizeof(selgame_Description) - 1);
		break;
	case 40:
		strncpy(selgame_Label, "Faster", sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, "Faster Speed\nMost monsters of the dungeon will seek you out quicker than ever before. Only an experienced champion should try their luck at this speed.", sizeof(selgame_Description) - 1);
		break;
	case 50:
		strncpy(selgame_Label, "Fastest", sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, "Fastest Speed\nThe minions of the underworld will rush to attack without hesitation. Only a true speed demon should enter at this pace.", sizeof(selgame_Description) - 1);
		break;
	}
	WordWrapArtStr(selgame_Description, DESCRIPTION_WIDTH);
}

void selgame_Speed_Esc()
{
	selgame_GameSelection_Select(0);
}

void selgame_Speed_Select(int value)
{
	nTickRate = vecSelGameDlgItems[value]->m_value;

	if (provider == SELCONN_LOOPBACK) {
		selgame_Password_Select(0);
		return;
	}

	selgame_Password_Init(0);
}

void selgame_Password_Init(int value)
{
	memset(&selgame_Password, 0, sizeof(selgame_Password));

	selgame_FreeVectors();

	UiAddBackground(&vecSelGameDialog);
	UiAddLogo(&vecSelGameDialog);

	SDL_Rect rect1 = { (Sint16)(PANEL_LEFT + 24), (Sint16)(UI_OFFSET_Y + 161), 590, 35 };
	vecSelGameDialog.push_back(new UiArtText("Client-Server (TCP)", rect1, UIS_CENTER | UIS_BIG));

	SDL_Rect rect2 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 211), 205, 192 };
	vecSelGameDialog.push_back(new UiArtText("Description:", rect2, UIS_MED));

	SDL_Rect rect3 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 256), DESCRIPTION_WIDTH, 192 };
	vecSelGameDialog.push_back(new UiArtText(selgame_Description, rect3));

	SDL_Rect rect4 = { (Sint16)(PANEL_LEFT + 305), (Sint16)(UI_OFFSET_Y + 211), 285, 33 };
	vecSelGameDialog.push_back(new UiArtText("Enter Password", rect4, UIS_CENTER | UIS_BIG));

	SDL_Rect rect5 = { (Sint16)(PANEL_LEFT + 305), (Sint16)(UI_OFFSET_Y + 314), 285, 33 };
	vecSelGameDialog.push_back(new UiEdit(selgame_Password, 15, rect5, UIS_MED | UIS_GOLD));

	SDL_Rect rect6 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(new UiArtTextButton("OK", &UiFocusNavigationSelect, rect6, UIS_CENTER | UIS_VCENTER | UIS_BIG | UIS_GOLD));

	SDL_Rect rect7 = { (Sint16)(PANEL_LEFT + 449), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(new UiArtTextButton("CANCEL", &UiFocusNavigationEsc, rect7, UIS_CENTER | UIS_VCENTER | UIS_BIG | UIS_GOLD));

	UiInitList(0, NULL, selgame_Password_Select, selgame_Password_Esc, vecSelGameDialog);
}

static bool IsGameCompatible(GameData *data)
{
	if (data->versionMajor == PROJECT_VERSION_MAJOR
	    && data->versionMinor == PROJECT_VERSION_MINOR
	    && data->versionPatch == PROJECT_VERSION_PATCH
	    && data->programid == GAME_ID) {
		return IsDifficultyAllowed(data->nDifficulty);
	}

	selgame_Free();

	if (data->programid != GAME_ID) {
		UiSelOkDialog(title, "The host is running a different game than you.", false);
	} else {
		char msg[64];
		sprintf(msg, "Your version %s does not match the host %d.%d.%d.", PROJECT_VERSION, PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);

		UiSelOkDialog(title, msg, false);
	}

	LoadBackgroundArt("ui_art\\selgame.pcx");

	return false;
}

void selgame_Password_Select(int value)
{
	if (selgame_selectedGame) {
		strcpy(sgOptions.Network.szPreviousHost, selgame_Ip);
		if (SNetJoinGame(selgame_selectedGame, selgame_Ip, selgame_Password, NULL, NULL, gdwPlayerId)) {
			if (!IsGameCompatible(m_game_data)) {
				selgame_GameSelection_Select(1);
				return;
			}

			UiInitList_clear();
			selgame_endMenu = true;
		} else {
			selgame_Free();
			UiSelOkDialog("Multi Player Game", SDL_GetError(), false);
			LoadBackgroundArt("ui_art\\selgame.pcx");
			selgame_Password_Init(selgame_selectedGame);
		}
		return;
	}

	m_game_data->nDifficulty = nDifficulty;
	m_game_data->nTickRate = nTickRate;
	m_game_data->bRunInTown = sgOptions.Gameplay.bRunInTown;
	m_game_data->bTheoQuest = sgOptions.Gameplay.bTheoQuest;
	m_game_data->bCowQuest = sgOptions.Gameplay.bCowQuest;

	if (SNetCreateGame(NULL, selgame_Password, NULL, 0, (char *)m_game_data, sizeof(GameData), MAX_PLRS, NULL, NULL, gdwPlayerId)) {
		UiInitList_clear();
		selgame_endMenu = true;
	} else {
		selgame_Free();
		UiSelOkDialog("Multi Player Game", SDL_GetError(), false);
		LoadBackgroundArt("ui_art\\selgame.pcx");
		selgame_Password_Init(0);
	}
}

void selgame_Password_Esc()
{
	if (selgame_selectedGame == 1)
		selgame_GameSelection_Select(1);
	else
		selgame_GameSpeedSelection();
}

int UiSelectGame(GameData *gameData, int *playerId)
{
	gdwPlayerId = playerId;
	m_game_data = gameData;
	LoadBackgroundArt("ui_art\\selgame.pcx");
	selgame_GameSelection_Init();

	selgame_endMenu = false;
	while (!selgame_endMenu) {
		UiClearScreen();
		UiPollAndRender();
	}
	selgame_Free();

	return selgame_enteringGame;
}
} // namespace devilution
