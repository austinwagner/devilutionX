/**
 * @file spelldat.h
 *
 * Interface of all spell data.
 */
#pragma once

#include "effects.h"

namespace devilution {

#ifdef __cplusplus
extern "C" {
#endif

typedef enum spell_id : int8_t {
	SPL_NULL,
	SPL_FIREBOLT,
	SPL_HEAL,
	SPL_LIGHTNING,
	SPL_FLASH,
	SPL_IDENTIFY,
	SPL_FIREWALL,
	SPL_TOWN,
	SPL_STONE,
	SPL_INFRA,
	SPL_RNDTELEPORT,
	SPL_MANASHIELD,
	SPL_FIREBALL,
	SPL_GUARDIAN,
	SPL_CHAIN,
	SPL_WAVE,
	SPL_DOOMSERP,
	SPL_BLODRIT,
	SPL_NOVA,
	SPL_INVISIBIL,
	SPL_FLAME,
	SPL_GOLEM,
	SPL_BLODBOIL,
	SPL_TELEPORT,
	SPL_APOCA,
	SPL_ETHEREALIZE,
	SPL_REPAIR,
	SPL_RECHARGE,
	SPL_DISARM,
	SPL_ELEMENT,
	SPL_CBOLT,
	SPL_HBOLT,
	SPL_RESURRECT,
	SPL_TELEKINESIS,
	SPL_HEALOTHER,
	SPL_FLARE,
	SPL_BONESPIRIT,
	SPL_LASTDIABLO = SPL_BONESPIRIT,
	SPL_MANA,
	SPL_MAGI,
	SPL_JESTER,
	SPL_LIGHTWALL,
	SPL_IMMOLAT,
	SPL_WARP,
	SPL_REFLECT,
	SPL_BERSERK,
	SPL_FIRERING,
	SPL_SEARCH,
	SPL_RUNEFIRE,
	SPL_RUNELIGHT,
	SPL_RUNENOVA,
	SPL_RUNEIMMOLAT,
	SPL_RUNESTONE,
	SPL_INVALID = -1,
} spell_id;

typedef enum magic_type {
	STYPE_FIRE,
	STYPE_LIGHTNING,
	STYPE_MAGIC,
} magic_type;

typedef enum missile_id {
	MIS_ARROW,
	MIS_FIREBOLT,
	MIS_GUARDIAN,
	MIS_RNDTELEPORT,
	MIS_LIGHTBALL,
	MIS_FIREWALL,
	MIS_FIREBALL,
	MIS_LIGHTCTRL,
	MIS_LIGHTNING,
	MIS_MISEXP,
	MIS_TOWN,
	MIS_FLASH,
	MIS_FLASH2,
	MIS_MANASHIELD,
	MIS_FIREMOVE,
	MIS_CHAIN,
	MIS_SENTINAL,
	MIS_BLODSTAR,
	MIS_BONE,
	MIS_METLHIT,
	MIS_RHINO,
	MIS_MAGMABALL,
	MIS_LIGHTCTRL2,
	MIS_LIGHTNING2,
	MIS_FLARE,
	MIS_MISEXP2,
	MIS_TELEPORT,
	MIS_FARROW,
	MIS_DOOMSERP,
	MIS_FIREWALLA,
	MIS_STONE,
	MIS_NULL_1F,
	MIS_INVISIBL,
	MIS_GOLEM,
	MIS_ETHEREALIZE,
	MIS_BLODBUR,
	MIS_BOOM,
	MIS_HEAL,
	MIS_FIREWALLC,
	MIS_INFRA,
	MIS_IDENTIFY,
	MIS_WAVE,
	MIS_NOVA,
	MIS_BLODBOIL,
	MIS_APOCA,
	MIS_REPAIR,
	MIS_RECHARGE,
	MIS_DISARM,
	MIS_FLAME,
	MIS_FLAMEC,
	MIS_FIREMAN,
	MIS_KRULL,
	MIS_CBOLT,
	MIS_HBOLT,
	MIS_RESURRECT,
	MIS_TELEKINESIS,
	MIS_LARROW,
	MIS_ACID,
	MIS_MISEXP3,
	MIS_ACIDPUD,
	MIS_HEALOTHER,
	MIS_ELEMENT,
	MIS_RESURRECTBEAM,
	MIS_BONESPIRIT,
	MIS_WEAPEXP,
	MIS_RPORTAL,
	MIS_BOOM2,
	MIS_DIABAPOCA,
	MIS_MANA,
	MIS_MAGI,
	MIS_LIGHTWALL,
	MIS_LIGHTNINGWALL,
	MIS_IMMOLATION,
	MIS_SPECARROW,
	MIS_FIRENOVA,
	MIS_LIGHTARROW,
	MIS_CBOLTARROW,
	MIS_HBOLTARROW,
	MIS_WARP,
	MIS_REFLECT,
	MIS_BERSERK,
	MIS_FIRERING,
	MIS_STEALPOTS,
	MIS_MANATRAP,
	MIS_LIGHTRING,
	MIS_SEARCH,
	MIS_FLASHFR,
	MIS_FLASHBK,
	MIS_IMMOLATION2,
	MIS_RUNEFIRE,
	MIS_RUNELIGHT,
	MIS_RUNENOVA,
	MIS_RUNEIMMOLAT,
	MIS_RUNESTONE,
	MIS_HIVEEXP,
	MIS_HORKDMN,
	MIS_JESTER,
	MIS_HIVEEXP2,
	MIS_LICH,
	MIS_PSYCHORB,
	MIS_NECROMORB,
	MIS_ARCHLICH,
	MIS_BONEDEMON,
	MIS_EXYEL2,
	MIS_EXRED3,
	MIS_EXBL2,
	MIS_EXBL3,
	MIS_EXORA1,
	MIS_NULL = -1,
} missile_id;

typedef struct SpellData {
	spell_id sName;
	Uint8 sManaCost;
	magic_type sType;
	const char *sNameText;
	const char *sSkillText;
	Sint32 sBookLvl;
	Sint32 sStaffLvl;
	bool sTargeted;
	bool sTownSpell;
	Sint32 sMinInt;
	_sfx_id sSFX;
	missile_id sMissiles[3];
	Uint8 sManaAdj;
	Uint8 sMinMana;
	Sint32 sStaffMin;
	Sint32 sStaffMax;
	Sint32 sBookCost;
	Sint32 sStaffCost;
} SpellData;

extern SpellData spelldata[];

#ifdef __cplusplus
}
#endif

}
