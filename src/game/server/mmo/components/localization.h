#ifndef GAME_SERVER_MMO_COMPONENTS_LOCALIZATION_H
#define GAME_SERVER_MMO_COMPONENTS_LOCALIZATION_H

#include <map>

#include <engine/external/xml/pugixml.hpp>
#include <game/server/mmo/component.h>
#include <engine/console.h>

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

	// TODO: Move auto detect of this shit to xml parsing
	std::map<const char *, int> m_aLangIDs = {
		{"en", LANG_EN},
		{"ru", LANG_RU},
	};

	static void ChatLang(IConsole::IResult *pResult, void *pUserData);

public:
	void OnInit() override;
	void OnConsoleInit() override;

	const char *Localize(int Lang, const char *pText);
};

#endif // GAME_SERVER_MMO_COMPONENTS_LOCALIZATION_H
