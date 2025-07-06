local http_request = require("http.request")
local utils = require("./utils")

local SHEETS_TSV_URL =
    [[https://docs.google.com/spreadsheets/d/1kas1J6RcIePcaRRxtTluPZm8C8kydpaoQBtRg15M-zM/export?format=tsv&gid=1517055494#gid=1517055494]]

local ORDERED_MESSAGES = {
    "s_openHostPromptMessages",
    "s_connectionLostMessages",
    "s_openHostEnabledMessages",
    "s_openHostDisabledMessages",
}

local ORDERED_LANGUAGES = {
    "Japanese",
    "English",
    "German",
    "French_", -- Placeholder, won't be found. For proper array offsets.
    "Spanish(NTSC)",
    "Italian",
    "Dutch",
    "Chinese (Simplified)",
    "Chinese (Traditional)",
    "Korean",

    -- Custom
    "Czech",
    "Norwegian",
    "Russian",
    "Portuguese_", -- Placeholder, won't be found. For proper array offsets.
    "Arabic",
    "Turkish",
    "Finnish",

    -- EU
    "EnglishEU", -- Placeholder, won't be found. For proper array offsets.
    "Spanish(EU)",
    "French",
    -- Custom
    "Portuguese",
}

print("Downloading sheet as tsv")
local headers, stream = assert(http_request.new_from_uri(SHEETS_TSV_URL):go())
local body = assert(stream:get_body_as_string())
if headers:get(":status") ~= "200" then
    error(body)
end
print("Downloaded sheet")

-- local infd = io.open("./WhWz & RR Translation - RR_ Server-Side Text.tsv")
-- assert(infd, "Please download the spreadsheet as 'WhWz & RR Translation - RR_ Server-Side Text.tsv'")
-- local body = infd:read("*a")
-- infd:close()

local split = utils.split_by_pattern(body, "\n")

-- Remove percentages, we don't care about them.
table.remove(split, 1)

local langs = {}

-- List of languages
local langs_split = utils.split_by_pattern(table.remove(split, 1), "\t")
-- Remove 'Fields' marker at start
table.remove(langs_split, 1)
-- Remove 'Error Code' marker at start
table.remove(langs_split, 1)
for i, lang in ipairs(langs_split) do
    if lang ~= "" then
        langs[i] = lang
    end
end

local messages = {}

print("Processing sheet into message objects")
for _, line in ipairs(split) do
    local translations = utils.split_by_pattern(line, "\t")
    local message = table.remove(translations, 1)
    local error_code = table.remove(translations, 1)

    messages[message] = {
        error_code = error_code,
        langs = {},
    }

    for i, translation in ipairs(translations) do
        if langs[i] then
            messages[message].langs[langs[i]] = translation
        end
    end
end

local output_lines = {
    "#pragma once",
    "",
    "#define OpenHostMessages \\",
}

for i, message_name in ipairs(ORDERED_MESSAGES) do
    print("Adding message " .. message_name)
    local message = messages[message_name]
    assert(message, "Missing message for " .. message_name)
    table.insert(output_lines, "    const wchar_t* \\")
    table.insert(output_lines, string.format("        OpenHostPage::%s[RVL::SCLanguageCount] = { \\", message_name))

    for _, lang in ipairs(ORDERED_LANGUAGES) do
        local translation = message.langs[lang]
        if translation and translation ~= "" then
            print(("    %s: Present"):format(lang))
            local splits = utils.split_by_pattern(translation, "\n")
            for _, v in ipairs(splits) do
                local segment = utils.trim(v)
                segment = segment:gsub("%s*+%s*", ('\\n" /* %s */ \\\n"'):format(lang))
                segment = ('L"%s",'):format(segment)
                table.insert(output_lines, string.format("            %s \\", segment))
            end
        else
            print(("    %s: Missing"):format(lang))
            table.insert(output_lines, ("            nullptr, /* %s */ \\"):format(lang))
        end
    end

    if i ~= #ORDERED_MESSAGES then
        table.insert(output_lines, "    }; \\")
        table.insert(output_lines, "\\")
    else
        table.insert(output_lines, "    }")
    end
end

-- Extra newline to make the compiler happy
table.insert(output_lines, "")

local output = table.concat(output_lines, "\n")

local fd = io.open("../payload/wwfcOpenHostMessages.hpp", "w+")
assert(fd, "Unable to open wwfcOpenHostMessages")

fd:write(output)
fd:close()

local handle = io.popen("clang-format -i ../payload/wwfcOpenHostMessages.hpp")
assert(handle, "Unable to open clang-format")
print(handle:read("*a"))
handle:close()
