#include "nf/Plugin.h"

namespace nf {

Plugin::Plugin(const PluginStartupInfo& psi) : psi_(psi) {}

void Plugin::SayHello() const {
    // Для FMSG_MB_* в FAR SDK Items[0] — это заголовок,
    // Items[1..] — строки тела сообщения.
    const wchar_t* items[] = {
        L"nf plugin",
        L"Hello from nf!",
    };
    psi_.Message(psi_.ModuleNumber, FMSG_MB_OK,
                 nullptr, // HelpTopic
                 items,
                 2, // ItemsNumber
                 0  // DefaultButton
    );
}

int Plugin::ShowMenu() {
    const FarMenuItem items[] = {
        {L"Say Hello", 0, 0, 0},
        {L"Exit", 0, 0, 0},
    };

    // FARAPIMENU: 12 аргументов
    //  PluginNumber, X, Y, MaxHeight, Flags, Title, Bottom, HelpTopic,
    //  BreakKeys, BreakCode, Items, ItemsNumber
    const int choice = psi_.Menu(psi_.ModuleNumber, -1, -1, // X, Y
                                 0,                         // MaxHeight
                                 FMENU_WRAPMODE,            // Flags
                                 L"nf plugin",              // Title
                                 L"Choose an action",       // Bottom
                                 L"nf",                     // HelpTopic
                                 nullptr,                   // BreakKeys
                                 nullptr,                   // BreakCode
                                 items,
                                 2 // ItemsNumber
    );

    switch (choice) {
    case 0:
        SayHello();
        break;
    case 1:
    default:
        break;
    }
    return choice;
}

} // namespace nf