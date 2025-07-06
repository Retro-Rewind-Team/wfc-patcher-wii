#pragma once

#define OpenHostMessages                                                       \
    const wchar_t*                                                             \
        OpenHostPage::s_openHostPromptMessages[RVL::SCLanguageCount] = {       \
            L"オープンホストをゆうこうにしますか？\n" /* Japanese */           \
            "\n" /* Japanese */                                                \
            "お互いにフレンド追加をしなくても\n" /* Japanese */                \
            "あなたのことをフレンドに追加したプレイヤーなら\n" /* Japanese */  \
            "あなたのルームに参加できる機能です",                              \
            L"Enable Open Host?\n" /* English */                               \
            "\n" /* English */                                                 \
            "This feature allows players who\n" /* English */                  \
            "add your friend code to meet up with you,\n" /* English */        \
            "even if you don't add them back.",                                \
            L"Open Host aktivieren?\n" /* German */                            \
            "\n" /* German */                                                  \
            "Diese Funktion erlaubt es Spielern,\n" /* German */               \
            "die deinen Freundescode hinzugefügt haben,\n" /* German */        \
            "sich mit dir zu treffen, auch wenn du sie\n" /* German */         \
            "nicht zurück hinzugefügt hast.",                                  \
            nullptr, /* French_ */                                             \
            L"Activar Open Host?\n" /* Spanish(NTSC) */                        \
            "\n" /* Spanish(NTSC) */                                           \
            "Esto permitira a otros jugadores a\n" /* Spanish(NTSC) */         \
            "a añadirte en su lista de amigos incluso,\n" /* Spanish(NTSC) */  \
            "si usted no le añade de vuelta.",                                 \
            L"Attivare Open Host?\n" /* Italian */                             \
            "\n" /* Italian */                                                 \
            "Questa opzione permette ai giocatori\n" /* Italian */             \
            "che aggiungono il tuo codice amico di\n" /* Italian */            \
            "giocare assieme senza che tu aggiunga loro.",                     \
            L"Schakel Openhost in?\n" /* Dutch */                              \
            "\n" /* Dutch */                                                   \
            "Dit staat spelers toe om met\n" /* Dutch */                       \
            "jou mee te doen door jouw vriendcode,\n" /* Dutch */              \
            "toe te voegen, zelfs als jij dit niet bij hen doet.",             \
            nullptr, /* Chinese (Simplified) */                                \
            L"要啟用 Open Host 嗎？\n" /* Chinese (Traditional) */             \
            "\n" /* Chinese (Traditional) */                                   \
            "此功能可以在你未添加好友的情況下\n" /* Chinese (Traditional) */   \
            "使用你的好友代碼一起玩",                                          \
            L"[개방형 호스트]를 키겠습니까?\n" /* Korean */                    \
            "\n" /* Korean */                                                  \
            "이 기능은 당신의 친구 코드를 등록한\n" /* Korean */               \
            "사람은 당신이 친구 등록을 하지 않아도\n" /* Korean */             \
            "당신의 방에 들어갈 수 있습니다.",                                 \
            L"Zapnout Open Host?\n" /* Czech */                                \
            "\n" /* Czech */                                                   \
            "Tato funkce umožňuje hráčům, kteří\n" /* Czech */                 \
            "přidají tvůj kód kamaráda, aby se s\n" /* Czech */                \
            "tebou setkali, i když je nepřidáš zpět.",                         \
            nullptr, /* Norwegian */                                           \
            L"Сделать вход свободным?\n" /* Russian */                         \
            "\n" /* Russian */                                                 \
            "Таким образом игроки, добавившие ваш\n" /* Russian */             \
            "код друга, смогут заходить к вам,\n" /* Russian */                \
            "даже если вы не добавили их в ответ.",                            \
            nullptr, /* Portuguese_ */                                         \
            nullptr, /* Arabic */                                              \
            L"Open Host açılsın mı?\n" /* Turkish */                           \
            "\n" /* Turkish */                                                 \
            "Bu özellik, sizin arkadaş kodunuzu\n" /* Turkish */               \
            "ekleyen herhangi bir kişinin siz onu\n" /* Turkish */             \
            "eklemeseniz bile onun size\n" /* Turkish */                       \
            "katılabilmesini sağlar.",                                         \
            nullptr, /* Finnish */                                             \
            nullptr, /* EnglishEU */                                           \
            L"Activar Open Host?\n" /* Spanish(EU) */                          \
            "\n" /* Spanish(EU) */                                             \
            "Esto permitira a otros jugadores a\n" /* Spanish(EU) */           \
            "a añadirte en su lista de amigos incluso,\n" /* Spanish(EU) */    \
            "si usted no le añade de vuelta.",                                 \
            L"Activer Open Host?\n" /* French */                               \
            "\n" /* French */                                                  \
            "Cette fonctionnalité permet aux joueurs qui\n" /* French */       \
            "ajoutent votre code ami de vous rejoindre,\n" /* French */        \
            "même si vous ne les avez pas ajoutés.",                           \
            L"Ativar Open Host?\n" /* Portuguese */                            \
            "\n" /* Portuguese */                                              \
            "Esta opção permite jogares que te adicionaram\n" /* Portuguese */ \
            "a sua lista de amigos juntarem -se a ti,\n" /* Portuguese */      \
            "mesmo que não os adiciones de volta.",                            \
    };                                                                         \
                                                                               \
    const wchar_t*                                                             \
        OpenHostPage::s_connectionLostMessages[RVL::SCLanguageCount] = {       \
            L"サーバーからの接続が切断されました\n" /* Japanese */             \
            "\n" /* Japanese */                                                \
            "\n" /* Japanese */                                                \
            "もう一度やり直してください",                                      \
            L"You have lost connection to\n" /* English */                     \
            "the server.\n" /* English */                                      \
            "\n" /* English */                                                 \
            "Please try again later.",                                         \
            L"Du hast die Verbindung\n" /* German */                           \
            "zum Server verloren.\n" /* German */                              \
            "\n" /* German */                                                  \
            "Bitte versuche es später erneut.",                                \
            nullptr, /* French_ */                                             \
            L"Ha perdido la conexión con\n" /* Spanish(NTSC) */                \
            "el servidor.\n" /* Spanish(NTSC) */                               \
            "\n" /* Spanish(NTSC) */                                           \
            "Intente de nuevo más adelante.",                                  \
            L"La connessione al server\n" /* Italian */                        \
            "è stata interrotta.\n" /* Italian */                              \
            "\n" /* Italian */                                                 \
            "Prova nuovamente più tardi.",                                     \
            L"De verbinding met de server\n" /* Dutch */                       \
            "is verbroken.\n" /* Dutch */                                      \
            "\n" /* Dutch */                                                   \
            "Probeer het later opnieuw.",                                      \
            nullptr, /* Chinese (Simplified) */                                \
            L"你已與伺服器失去連線\\n" /* Chinese (Traditional) */             \
            "請稍後再試.",                                                     \
            L"서버와 연결이 끊어졌습니다.\n" /* Korean */                      \
            "\n" /* Korean */                                                  \
            "잠시 후 다시 시도해 주세요.",                                     \
            L"Ztratil jsi spojení\n" /* Czech */                               \
            "se serverem.\n" /* Czech */                                       \
            "\n" /* Czech */                                                   \
            "Zkus to znovu později.",                                          \
            nullptr, /* Norwegian */                                           \
            L"Соединение с сервером\n" /* Russian */                           \
            "потеряно.\n" /* Russian */                                        \
            "\n" /* Russian */                                                 \
            "Повторите попытку.",                                              \
            nullptr, /* Portuguese_ */                                         \
            nullptr, /* Arabic */                                              \
            L"Sunucuyla bağlantınız\n" /* Turkish */                           \
            "kesildi\n" /* Turkish */                                          \
            "\n" /* Turkish */                                                 \
            "Lütfen daha sonra tekrar deneyin.",                               \
            nullptr, /* Finnish */                                             \
            nullptr, /* EnglishEU */                                           \
            L"Ha perdido la conexión con\n" /* Spanish(EU) */                  \
            "el servidor.\n" /* Spanish(EU) */                                 \
            "\n" /* Spanish(EU) */                                             \
            "Intente de nuevo más adelante.",                                  \
            L"Vous avez perdu la connexion\n" /* French */                     \
            "au serveur.\n" /* French */                                       \
            "\n" /* French */                                                  \
            "Veuillez réessayer ultérieurement.",                              \
            L"A conexão com o servidor\n" /* Portuguese */                     \
            "foi interrompida.\n" /* Portuguese */                             \
            "\n" /* Portuguese */                                              \
            "Por favor tenta mais tarde.",                                     \
    };                                                                         \
                                                                               \
    const wchar_t*                                                             \
        OpenHostPage::s_openHostEnabledMessages[RVL::SCLanguageCount] = {      \
            L"オープンホストをゆうこうにしました！",                           \
            L"Open Host is now enabled!",                                      \
            L"Open Host ist nun aktiviert!",                                   \
            nullptr, /* French_ */                                             \
            L"Open Host está ahora activado!",                                 \
            L"Open Host è attivato!",                                          \
            L"Openhost is nu ingeschakeld!",                                   \
            nullptr, /* Chinese (Simplified) */                                \
            L"Open Host 已啟用！",                                             \
            L"[개방형 호스트]가 활성화되었습니다!",                            \
            L"Open Host je teď zapnuto!",                                      \
            nullptr, /* Norwegian */                                           \
            L"Свободный вход включен!",                                        \
            nullptr, /* Portuguese_ */                                         \
            nullptr, /* Arabic */                                              \
            L"Open Host artık açık!",                                          \
            nullptr, /* Finnish */                                             \
            nullptr, /* EnglishEU */                                           \
            L"Open Host está ahora activado!",                                 \
            L"Open Host est maintenant activé!",                               \
            L"Open Host está ativo",                                           \
    };                                                                         \
                                                                               \
    const wchar_t*                                                             \
        OpenHostPage::s_openHostDisabledMessages[RVL::SCLanguageCount] = {     \
            L"オープンホストをむこうにしました！",                             \
            L"Open Host is now disabled!",                                     \
            L"Open Host ist nun deaktiviert!",                                 \
            nullptr, /* French_ */                                             \
            L"Open Host está ahora desactivado!",                              \
            L"Open Host è disattivato!",                                       \
            L"Openhost is nu uitgeschakeld!",                                  \
            nullptr, /* Chinese (Simplified) */                                \
            L"Open Host 已停用！",                                             \
            L"[개방형 호스트]가 비활성화되었습니다!",                          \
            L"Open Host je teď vypnuto!",                                      \
            nullptr, /* Norwegian */                                           \
            L"Свободный вход отключен!",                                       \
            nullptr, /* Portuguese_ */                                         \
            nullptr, /* Arabic */                                              \
            L"Open Host artık kapalı!",                                        \
            nullptr, /* Finnish */                                             \
            nullptr, /* EnglishEU */                                           \
            L"Open Host está ahora desactivado!",                              \
            L"Open Host est maintenant désactivé!",                            \
            L"Open Host está desativado",                                      \
    }
