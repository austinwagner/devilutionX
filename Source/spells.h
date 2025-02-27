/**
 * @file spells.h
 *
 * Interface of functionality for casting player spells.
 */
#pragma once

namespace devilution {

#ifdef __cplusplus
extern "C" {
#endif

int GetManaAmount(int id, int sn);
void UseMana(int id, int sn);
Uint64 GetSpellBitmask(int spellId);
bool CheckSpell(int id, int sn, char st, bool manaonly);
void EnsureValidReadiedSpell(PlayerStruct &player);
void CastSpell(int id, int spl, int sx, int sy, int dx, int dy, int spllvl);
void DoResurrect(int pnum, int rid);
void DoHealOther(int pnum, int rid);
int GetSpellBookLevel(spell_id s);
int GetSpellStaffLevel(spell_id s);

#ifdef __cplusplus
}
#endif

}
