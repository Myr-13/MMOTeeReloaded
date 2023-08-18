#ifndef GAME_SERVER_MMO_MMO_ERRORS_MACROS_H
#define GAME_SERVER_MMO_MMO_ERRORS_MACROS_H

#define LOG_ERROR(CID, Message) \
	GameServer()->SendChatLocalize(CID, "Error message at %s:%d - %s", __FILE__, __LINE__, Message);

#endif // GAME_SERVER_MMO_MMO_ERRORS_MACROS_H
