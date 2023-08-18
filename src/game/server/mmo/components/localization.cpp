#include "localization.h"

#include <base/system.h>

using namespace pugi;

void CLocalization::OnConsoleInit()
{
	
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
