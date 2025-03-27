#pragma once

#define OpenHostMessages                                                       \
    const wchar_t*                                                             \
        OpenHostPage::s_openHostPromptMessages[RVL::SCLanguageCount] = {       \
            L"オープンホストをゆうこうにしますか？\n\n"                        \
            L"お互いにフレンド追加をしなくても\n"                              \
            L"あなたのことをフレンドに追加したプレイヤーなら\n"                \
            L"あなたのルームに参加できる機能です",                             \
            L"Enable Open Host?\n\n"                                           \
            L"This feature allows players who\n"                               \
            L"add your friend code to meet up with you,\n"                     \
            L"even if you don't add them back.",                               \
            L"Offenen Host aktivieren?\n\n"                                    \
            L"Diese Funktion erlaubt es Spielern,\n"                           \
            L"die deinen Freundescode hinzugefügt haben,\n"                    \
            L"sich mit dir zu treffen, auch wenn du sie\n"                     \
            L"nicht zurück hinzugefügt hast.",                                 \
            L"Activer l'Open Host?\n\n"                                        \
            L"Cette fonctionnalité permet aux joueurs qui\n"                   \
            L"ajoutent votre code ami de vous rejoindre,\n"                    \
            L"même si vous ne les avez pas ajoutés.",                          \
            nullptr,                                                           \
            L"Attivare Open Host?\n\n"                                         \
            L"Questa opzione permette ai giocatori\n"                          \
            L"che aggiungono il tuo codice amico di\n"                         \
            L"giocare assieme senza che tu aggiunga loro.",                    \
            L"Schakel Openhost in?\n\n"                                        \
            L"Dit staat spelers toe om met\n"                                  \
            L"jou mee te doen door jouw vriendcode,\n"                         \
            L"toe te voegen, zelfs als jij dit niet bij hen doet.",            \
            nullptr,                                                           \
            L"[개방형 호스트]를 키겠습니까?\n\n"                               \
            L"이 기능은 당신의 친구 코드를 등록한\n"                           \
            L"사람은 당신이 친구 등록을 하지 않아도\n"                         \
            L"당신의 방에 들어갈 수 있습니다.",                                \
    };                                                                         \
                                                                               \
    const wchar_t*                                                             \
        OpenHostPage::s_connectionLostMessages[RVL::SCLanguageCount] = {       \
            L"サーバーからの接続が切断されました\n\n\n"                        \
            L"もう一度やり直してください",                                     \
            L"You have lost connection to\n"                                   \
            L"the server.\n\n"                                                 \
            L"Please try again later.",                                        \
            L"Du hast die Verbindung\n"                                        \
            L"zum Server verloren.\n\n"                                        \
            L"Bitte versuche es später erneut.",                               \
            L"Vous avez perdu la connexion\n"                                  \
            L"au serveur.\n\n"                                                 \
            L"Veuillez réessayer ultérieurement.",                             \
            nullptr,                                                           \
            L"La connessione al server\n"                                      \
            L"è stata interrotta.\n\n"                                         \
            L"Prova nuovamente più tardi.",                                    \
            L"De verbinding met de server\n"                                   \
            L"is verbroken.\n\n"                                               \
            L"Probeer het later opnieuw.",                                     \
            nullptr,                                                           \
            L"서버와 연결이 끊어졌습니다.\n\n"                                 \
            L"잠시 후 다시 시도해 주세요.",                                    \
    };                                                                         \
                                                                               \
    const wchar_t*                                                             \
        OpenHostPage::s_openHostEnabledMessages[RVL::SCLanguageCount] = {      \
            L"オープンホストをゆうこうにしました！",                           \
            L"Open Host is now enabled!",                                      \
            L"Offener Host ist nun aktiviert!",                                \
            L"L'Open Host est maintenant activé!",                             \
            nullptr,                                                           \
            L"Open Host è attivato!",                                          \
            L"Openhost is nu ingeschakeld!",                                   \
            nullptr,                                                           \
            L"[개방형 호스트]가 활성화되었습니다!",                            \
    };                                                                         \
                                                                               \
    const wchar_t*                                                             \
        OpenHostPage::s_openHostDisabledMessages[RVL::SCLanguageCount] = {     \
            L"オープンホストをむこうにしました！",                             \
            L"Open Host is now disabled!",                                     \
            L"Offener Host ist nun deaktiviert!",                              \
            L"L'Open Host est maintenant désactivé!",                          \
            nullptr,                                                           \
            L"Open Host è disattivato!",                                       \
            L"Openhost is nu uitgeschakeld!",                                  \
            nullptr,                                                           \
            L"[개방형 호스트]가 비활성화되었습니다!",                          \
    }
