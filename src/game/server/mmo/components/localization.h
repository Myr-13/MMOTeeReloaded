#ifndef GAME_SERVER_MMO_COMPONENTS_LOCALIZATION_H
#define GAME_SERVER_MMO_COMPONENTS_LOCALIZATION_H

#include <game/server/mmo/component.h>

enum
{
	LANG_EN, // Eng
	LANG_RU, // Rus
	LANG_UA, // Ua
	LANG_GR, // Ger
	LANG_TR  // Tur
};

class CLocalization : public CServerComponent
{
public:
	virtual void OnInit() override;
};

#endif // GAME_SERVER_MMO_COMPONENTS_LOCALIZATION_H
