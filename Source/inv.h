/**
 * @file inv.h
 *
 * Interface of player inventory.
 */
#pragma once

#include "items.h"
#include "player.h"

namespace devilution {

#ifdef __cplusplus
extern "C" {
#endif

typedef enum item_color {
	// clang-format off
	ICOL_WHITE = PAL16_YELLOW + 5,
	ICOL_BLUE  = PAL16_BLUE + 5,
	ICOL_RED   = PAL16_RED + 5,
	// clang-format on
} item_color;

typedef struct InvXY {
	Sint32 X;
	Sint32 Y;
} InvXY;

extern bool invflag;
extern bool drawsbarflag;
extern const InvXY InvRect[73];

void FreeInvGFX();
void InitInv();

/**
 * @brief Render the inventory panel to the given buffer.
 */
void DrawInv(CelOutputBuffer out);

void DrawInvBelt(CelOutputBuffer out);
bool AutoEquipEnabled(const PlayerStruct &player, const ItemStruct &item);
bool AutoEquip(int playerNumber, const ItemStruct &item, bool persistItem = true);
bool AutoPlaceItemInInventory(int playerNumber, const ItemStruct &item, bool persistItem = false);
bool AutoPlaceItemInInventorySlot(int playerNumber, int slotIndex, const ItemStruct &item, bool persistItem);
bool AutoPlaceItemInBelt(int playerNumber, const ItemStruct &item, bool persistItem = false);
bool GoldAutoPlace(int pnum);
void CheckInvSwap(int pnum, BYTE bLoc, int idx, WORD wCI, int seed, bool bId, uint32_t dwBuff);
void inv_update_rem_item(int pnum, BYTE iv);
void RemoveInvItem(int pnum, int iv);
void RemoveSpdBarItem(int pnum, int iv);
void CheckInvItem(bool isShiftHeld = false);
void CheckInvScrn(bool isShiftHeld);
void CheckItemStats(int pnum);
void InvGetItem(int pnum, ItemStruct *item, int ii);
void AutoGetItem(int pnum, ItemStruct *item, int ii);
int FindGetItem(int idx, WORD ci, int iseed);
void SyncGetItem(int x, int y, int idx, WORD ci, int iseed);
bool CanPut(int x, int y);
bool TryInvPut();
void DrawInvMsg(const char *msg);
int InvPutItem(int pnum, int x, int y);
int SyncPutItem(int pnum, int x, int y, int idx, WORD icreateinfo, int iseed, int Id, int dur, int mdur, int ch, int mch, int ivalue, DWORD ibuff, int to_hit, int max_dam, int min_str, int min_mag, int min_dex, int ac);
char CheckInvHLight();
void RemoveScroll(int pnum);
bool UseScroll();
void UseStaffCharge(int pnum);
bool UseStaff();
bool UseInvItem(int pnum, int cii);
void DoTelekinesis();
int CalculateGold(int pnum);
bool DropItemBeforeTrig();

/* data */

extern int AP2x2Tbl[10];

#ifdef __cplusplus
}
#endif

}
