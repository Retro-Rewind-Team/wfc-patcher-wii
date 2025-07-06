#include "wwfcLanguage.hpp"
#include "import/revolution.h"
#include "wwfcUtil.h"

namespace wwfc::Language
{

typedef enum {
    LangEnglishEU = 0x81,
    LangFrenchEU = 0x83,
    LangSpanishEU = 0x84,

    // Custom Languages
    LangPortugueseEU = 0x85
} ServerLanguage;

u8 GetExLang(void)
{
    const u8 raceRename = *reinterpret_cast<const u8*>(
        RMCXD_PORT(0x80897ddf, 0x80891037, 0x8089742f, 0x80886707)
    );

    u8 exLang = 0;

    switch (raceRename) {
    case 0x4A: // 'J'
        exLang = RVL::SCLanguageJapanese;
        break;
    case 0x46: // 'F'
        exLang = RVL::SCLanguageFrenchEU;
        break;
    case 0x47: // 'G'
        exLang = RVL::SCLanguageGerman;
        break;
    case 0x44: // 'D'
        exLang = RVL::SCLanguageDutch;
        break;
    case 0x55: // 'U'
        exLang = RVL::SCLanguageSpanish;
        break;
    case 0x45: // 'E'
        exLang = RVL::SCLanguageSpanishEU;
        break;
    case 0x4E: // 'N'
        exLang = RVL::SCLanguageFinnish;
        break;
    case 0x49: // 'I'
        exLang = RVL::SCLanguageItalian;
        break;
    case 0x4B: // 'K'
        exLang = RVL::SCLanguageKorean;
        break;
    case 0x41: // 'A'
        exLang = RVL::SCLanguageRussian;
        break;
    case 0x54: // 'T'
        exLang = RVL::SCLanguageTurkish;
        break;
    case 0x43: // 'C'
        exLang = RVL::SCLanguageCzech;
        break;
    default:
        // Unknown, fall back to RVL System Language
        exLang = RVL::SCGetLanguage();
        break;
    }

    return exLang;
}

// Accounts for EU Langs beginning at 0x81 on the server.
u8 ExLangToServerLang(u8 exLang)
{
    u8 sLang = 0;
    switch (exLang) {
    case RVL::SCLanguageEnglishEU:
        sLang = LangEnglishEU;
        break;
    case RVL::SCLanguageFrenchEU:
        sLang = LangFrenchEU;
        break;
    case RVL::SCLanguageSpanishEU:
        sLang = LangSpanishEU;
        break;
    case RVL::SCLanguagePortugueseEU:
        sLang = LangPortugueseEU;
        break;
    default:
        sLang = exLang;
        break;
    }

    return sLang;
}

} // namespace wwfc::Language
