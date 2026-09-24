#pragma once

#include <cstdint>
#include <expected>

namespace nf {

enum class MsgID : std::uint8_t {
    PluginTitle = 0,           // "nf — directory aliases"
    AliasPanelTitle,           // "nf — aliases"
    AliasCreated,              // "Alias '{0}' -> {1}"
    DeleteAliasTitle,          // "Delete alias"
    DeleteAliasQuestion,       // "Delete alias '{0}'?"
    ErrorTitle,                // "nf — error"
    AliasNotFound,             // "Alias not found: '{0}'"
    AliasAlreadyExists,        // "Alias already exists: '{0}'"
    InvalidAliasNameEmpty,     // "Alias name must not be empty"
    InvalidAliasNameColon,     // "Alias name must not contain ':'"
    StorageReadFailed,         // "Cannot read aliases file: {0}"
    StorageWriteFailed,        // "Cannot write aliases file: {0}"
    CannotDetermineCurrentDir, // "Cannot determine current panel directory"
    NoMatchingAliases,         // "No aliases match: '{0}'"
    MatchMenuTitle,            // "nf — matching aliases"
    MatchMenuFooter,           // "Select an alias"
    Ok,                        // "&OK"
    Cancel,                    // "&Cancel"
};

} // namespace nf