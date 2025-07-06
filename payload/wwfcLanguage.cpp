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
    case 'J':
        exLang = RVL::SCLanguageJapanese;
        break;
    // SCLanguageEnglish Unsupported. Handled by vanilla
    case 'G':
        exLang = RVL::SCLanguageGerman;
        break;
    // SCLanguageFrench Unsupported. We only have FrenchEU
    case 'U':
        exLang = RVL::SCLanguageSpanish;
        break;
    case 'I':
        exLang = RVL::SCLanguageItalian;
        break;
    case 'D':
        exLang = RVL::SCLanguageDutch;
        break;
    case 'W':
        exLang = RVL::SCLanguageSimplifiedChinese;
        break;
    case 'Z':
        exLang = RVL::SCLanguageTraditionalChinese;
        break;
    case 'K':
        exLang = RVL::SCLanguageKorean;
        break;
    case 'C':
        exLang = RVL::SCLanguageCzech;
        break;
    case 'O': // 'O'
        exLang = RVL::SCLanguageNorwegian;
        break;
    case 'A':
        exLang = RVL::SCLanguageRussian;
        break;
    // SCLanguagePortuguese Unsupported. We only have PortugueseEU
    case 'B':
        exLang = RVL::SCLanguageArabic;
        break;
    case 'T':
        exLang = RVL::SCLanguageTurkish;
        break;
    case 'N':
        exLang = RVL::SCLanguageFinnish;
        break;
    // SCLanguageEnglishEU Unsupported. Just use English
    case 'F':
        exLang = RVL::SCLanguageFrenchEU;
        break;
    case 'E':
        exLang = RVL::SCLanguageSpanishEU;
        break;
    case 'P':
        exLang = RVL::SCLanguagePortugueseEU;
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
