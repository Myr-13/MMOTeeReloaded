#ifndef GAME_SERVER_MMO_ITEMS_H
#define GAME_SERVER_MMO_ITEMS_H

enum
{
	ITEM_WOOD,
	ITEM_CARROT,
	ITEM_POTATO,
	ITEM_TOMATO,
	// Drop
	ITEM_VORTEX_SHARD,
	ITEM_FIRE_SHARD,
	ITEM_POISON_SHARD,
	ITEM_STAR,
	ITEM_LEATHER,
	ITEM_DEMON_LEATHER,
	ITEM_DRAGON_LEATHER,
	// Ores
	ITEM_COPPER,
	ITEM_IRON,
	ITEM_GOLD,
	ITEM_DIAMOND,
	ITEM_OBSIDIAN,
	ITEM_MITHRIL_ORE,
	ITEM_ORIHALCIUM_ORE,
	ITEM_ADAMANTITE_ORE,
	ITEM_TITANIUM_ORE,
	ITEM_DRAGON_ORE,
	ITEM_ASTRALIUM_ORE,
	// Boxes
	ITEM_FARMER_BOX,
	ITEM_MINER_BOX,
	// Pickaxes
	ITEM_COPPER_PICKAXE,
	ITEM_IRON_PICKAXE,
	ITEM_GOLD_PICKAXE,
	ITEM_DIAMOND_PICKAXE,
	ITEM_OBSIDIAN_PICKAXE,
	ITEM_MITHRIL_PICKAXE,
	ITEM_ORIHALCIUM_PICKAXE,
	ITEM_ADAMANTITE_PICKAXE,
	ITEM_TITANIUM_PICKAXE,
	ITEM_DRAGON_PICKAXE,
	ITEM_ASTRALIUM_PICKAXE,
	// Ingots
	ITEM_MITHRIL_INGOT,
	ITEM_ORIHALCIUM_INGOT,
	ITEM_ADAMANTITE_INGOT,
	ITEM_TITANIUM_INGOT,
	ITEM_DRAGON_INGOT,
	ITEM_ASTRALIUM_INGOT,
	// Weapon
	ITEM_GUN,
	ITEM_GRENADE,
	ITEM_SHOTGUN,
	ITEM_LASER,
	ITEM_SGUN,
	ITEM_AUTO_HAMMER,
	ITEM_AUTO_GUN,
	// Armors
	// Body
	ITEM_COPPER_BODY,
	ITEM_IRON_BODY,
	ITEM_GOLD_BODY,
	ITEM_DIAMOND_BODY,
	ITEM_OBSIDIAN_BODY,
	ITEM_MITHRIL_BODY,
	ITEM_ORIHALCIUM_BODY,
	ITEM_ADAMANTITE_BODY,
	ITEM_TITANIUM_BODY,
	ITEM_DRAGON_BODY,
	ITEM_ASTRALIUM_BODY,
	// Feet
	ITEM_COPPER_FEET,
	ITEM_IRON_FEET,
	ITEM_GOLD_FEET,
	ITEM_DIAMOND_FEET,
	ITEM_OBSIDIAN_FEET,
	ITEM_MITHRIL_FEET,
	ITEM_ORIHALCIUM_FEET,
	ITEM_ADAMANTITE_FEET,
	ITEM_TITANIUM_FEET,
	ITEM_DRAGON_FEET,
	ITEM_ASTRALIUM_FEET,
	// Use
	ITEM_MONEY_BAG,
	ITEM_RESET_SKILLS,
	// Artifacts
	ITEM_RARE_HAMMER,
	ITEM_EMOTE_MODULE,
	ITEM_HOOK_DAMAGE,
	ITEM_HOOK_LIFE_STEAL,
	ITEM_BIG_BOOM,
	ITEM_GHOST_GUN,
	ITEM_GHOST_SHOTGUN,
	ITEM_GHOST_GRENADE,
	ITEM_WHITE_TICKET,
	ITEM_JUMP_IMPULS,
	ITEM_PIZDAMET,
	ITEM_MATERIAL,
	ITEM_EVENT_BOX,
	ITEM_RESET_UPGRADES
};

enum
{
	QUALITY_0, // ☆☆☆☆☆
	QUALITY_1, // ★☆☆☆☆
	QUALITY_2, // ★★☆☆☆
	QUALITY_3, // ★★★☆☆
	QUALITY_4, // ★★★★☆
	QUALITY_5, // ★★★★★
};

enum
{
	RARITY_COMMON, // COMMON
	RARITY_UNCOMMON, // UNCOMMON
	RARITY_RARE, // RARE
	RARITY_EPIC, // EPIC
	RARITY_LEGENDARY // LEGENDARY
};

enum
{
	ITEM_TYPE_PROFESSION,
	ITEM_TYPE_CRAFT,
	ITEM_TYPE_USE,
	ITEM_TYPE_ARTIFACTS,
	ITEM_TYPE_WEAPON,
	ITEM_TYPE_OTHER,

	ITEM_TYPE_ARMOR_BODY,
	ITEM_TYPE_ARMOR_FEET,
};

#endif // GAME_SERVER_MMO_ITEMS_H
