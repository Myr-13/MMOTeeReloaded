#ifndef MACRO_CONFIG_INT
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Save, Desc) ;
#endif

#define MAX_INT 0x7fffffff

// Discord
MACRO_CONFIG_STR(MmoDiscordLink, mmo_discord_link, 64, "https://discord.gg/PbyMdPvkhd", CFGFLAG_SERVER, "Link to discord server")
MACRO_CONFIG_INT(MmoDiscordSpamTime, mmo_discord_spam_time, 300, 0, MAX_INT, CFGFLAG_SERVER, "Time between spamming discord links in chat (in seconds)")

#undef MAX_INT
