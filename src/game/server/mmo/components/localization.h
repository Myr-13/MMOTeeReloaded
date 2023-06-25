#ifndef GAME_SERVER_MMO_COMPONENTS_LOCALIZATION_H
#define GAME_SERVER_MMO_COMPONENTS_LOCALIZATION_H

#include <map>

#include <engine/external/xml/pugixml.hpp>
#include <game/server/mmo/component.h>

enum
{
	LANG_EN, // Eng
	LANG_RU, // Rus
	LANG_UA, // Ua
	LANG_GR, // Ger
	LANG_TR, // Tur
	NUM_LANGS
};

class CLocalization : public CServerComponent
{
	bool m_aTranslated[NUM_LANGS];
	std::map<unsigned int, std::string> m_aLocalizeStrings[NUM_LANGS];

public:
	virtual void OnInit() override;

	const char *Localize(int Lang, const char *pText);
};

#endif // GAME_SERVER_MMO_COMPONENTS_LOCALIZATION_H
