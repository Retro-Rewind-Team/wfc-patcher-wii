local http_request = require("http.request")
local utils = require("./utils")

local SHEETS_CSV_URL =
    [[https://docs.google.com/spreadsheets/d/1kas1J6RcIePcaRRxtTluPZm8C8kydpaoQBtRg15M-zM/export?format=tsv&gid=1517055494#gid=1517055494]]

local ORDERED_MESSAGES = {
    "s_openHostPromptMessages",
    "s_connectionLostMessages",
    "s_openHostEnabledMessages",
    "s_openHostDisabledMessages",
}

-- TODO: Payload only supports default 10 languages
local ORDERED_LANGUAGES = {
    "Japanese",
    "English",
    "German",
    "French",
    "Spanish",
    "Italian",
    "Dutch",
    "Chinese (Simplified)",
    "Korean",

    -- -- Custom
    -- "Czech",
    -- "Norwegian",
    -- "Russian",
    -- "Arabic",
    -- "Turkish",
    -- "Finnish",

    -- -- EU
    -- "French",
    -- -- Custom
    -- "Portuguese",
}

local headers, stream = assert(http_request.new_from_uri(SHEETS_CSV_URL):go())
local body = assert(stream:get_body_as_string())
if headers:get(":status") ~= "200" then
    error(body)
end

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
    local message = messages[message_name]
    assert(message, "Missing message for " .. message_name)
    table.insert(output_lines, "    const wchar_t* \\")
    table.insert(output_lines, string.format("        OpenHostPage::%s[RVL::SCLanguageCount] = { \\", message_name))

    for _, lang in ipairs(ORDERED_LANGUAGES) do
        local translation = message.langs[lang]
        if translation ~= "" then
            local splits = utils.split_by_pattern(translation, "\n")
            for _, v in ipairs(splits) do
                local segment = utils.trim(v)
                -- Sheet has incorrect symbols. Whatever
                segment = segment:gsub(";", ",")
                table.insert(output_lines, string.format("            %s \\", segment))
            end
        else
            table.insert(output_lines, "            nullptr, \\")
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
