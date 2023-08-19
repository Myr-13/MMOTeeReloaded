#include "localization.h"

#include <base/system.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

using namespace pugi;

void CLocalization::OnConsoleInit()
{
	Console()->Register("lang", "s[lang_name]", CFGFLAG_GAME | CFGFLAG_CHAT, ChatLang, this, "Change your localization");
}

void CLocalization::OnInit()
{
	for(bool &Elem : m_aTranslated)
		Elem = false;

	// Load all localization files
	xml_document Document;

	xml_parse_result ParseResult = Document.load_file("mmo/localization/index.xml");

	if (!ParseResult)
	{
		dbg_msg("xml", "source file 'mmo/localization/index.xml' parsed with errors!");
		dbg_msg("xml", "error: %s", ParseResult.description());

		dbg_break();
	}

	xml_node Root = Document.child("Localization");

	for (xml_node Node : Root)
	{
		int ID = Node.attribute("ID").as_int(-1);

		if(ID < 0 || ID >= NUM_LANGS)
		{
			dbg_msg("localization", "invalid language id %d", ID);
			continue;
		}

		std::string LangPath = std::string("mmo/localization/") + std::string(Node.child("File").attribute("Path").as_string());

		xml_document Lang;
		xml_parse_result PR = Lang.load_file(LangPath.c_str());

		if(!PR)
		{
			dbg_msg("xml", "source file '%s' parsed with errors!", LangPath.c_str());
			dbg_msg("xml", "error: %s", ParseResult.description());

			dbg_break();
		}

		m_aTranslated[ID] = true;
		xml_node LR = Lang.child("Language");

		for(xml_node Node2 : LR)
		{
			unsigned int Hash = str_quickhash(Node2.attribute("Text").as_string());
			std::string Text = Node2.child_value();

			m_aLocalizeStrings[ID].insert({Hash, Text});
		}

		dbg_msg("localization", "loaded %s. Lang ID: %d", LangPath.c_str(), ID);
	}
}

void CLocalization::ChatLang(IConsole::IResult *pResult, void *pUserData)
{
	CLocalization *pSelf = (CLocalization *)pUserData;
	const char *pLang = pResult->GetString(0);

	int ClientID = pResult->m_ClientID;
	if(ClientID < 0 || ClientID >= MAX_PLAYERS)
		return;
	CPlayer *pPly = pSelf->GameServer()->m_apPlayers[ClientID];
	if(!pPly || !pPly->m_LoggedIn)
		return;

	for(auto &Entry : pSelf->m_aLangIDs)
	{
		if(!str_comp(Entry.first, pLang))
		{
			pPly->m_AccData.m_Lang = Entry.second;

			pSelf->GameServer()->SendChatLocalize(ClientID, "Localization changed.");
			return;
		}
	}

	char aBuf[128];
	mem_zero(aBuf, sizeof(aBuf));

	for(auto &Entry : pSelf->m_aLangIDs)
	{
		str_append(aBuf, Entry.first, sizeof(aBuf));
		str_append(aBuf, ", ", sizeof(aBuf));
	}

	aBuf[str_length(aBuf) - 1] = '\0';

	pSelf->GameServer()->SendChatLocalize(ClientID, "Wrong lang_name. Supported languages: %s", aBuf);
}

const char *CLocalization::Localize(int Lang, const char *pText)
{
	// Skip english, cuz server is already in english
	if(Lang == LANG_EN)
		return pText;

	// Skip invalid languages
	if(Lang < 0 || Lang >= NUM_LANGS)
		return pText;

	// Skip non translated languages
	if(!m_aTranslated[Lang])
		return pText;

	unsigned int Hash = str_quickhash(pText);

	auto Elem = m_aLocalizeStrings[Lang].find(Hash);

	if(Elem != m_aLocalizeStrings[Lang].end())
		return Elem->second.c_str();

	return pText;
}
