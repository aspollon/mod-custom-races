-- CoA's AppearanceUI picks the Wardrobe background from the race name:
--     "transmog-background-race-" .. UnitRace("player"):lower()
-- The stock races have an atlas for that name; custom races (Highmountain
-- Tauren, Gnoll, ...) do not. SetAtlas then throws in OnLoad before the
-- category callbacks are registered, so item changes stop updating the
-- preview for those characters.
--
-- Keep the original function for every race that has an atlas and only fall
-- back to a stock background when the atlas is missing. This runs before the
-- LoadOnDemand AppearanceUI is opened and also catches it when it is loaded
-- by another add-on first.

local FALLBACK = "transmog-background-race-human"

local function Fix()
    if AppearanceUtil and AppearanceUtil._afkBackgroundAtlasFixed then
        return true
    end
    if not AppearanceUtil
        or type(AppearanceUtil.GetBackgroundAtlas) ~= "function"
        or not AtlasUtil
        or type(AtlasUtil.AtlasExists) ~= "function" then
        return false
    end

    local original = AppearanceUtil.GetBackgroundAtlas
    AppearanceUtil.GetBackgroundAtlas = function()
        local atlas = original()
        if atlas and AtlasUtil:AtlasExists(atlas) then
            return atlas
        end
        return FALLBACK
    end

    AppearanceUtil._afkBackgroundAtlasFixed = true
    return true
end

local watcher = CreateFrame("Frame")
watcher:RegisterEvent("ADDON_LOADED")
watcher:RegisterEvent("PLAYER_LOGIN")
watcher:SetScript("OnEvent", function(self, event, name)
    if event == "ADDON_LOADED" and name ~= "Ascension_AppearanceUI" then
        return
    end
    if Fix() then
        self:UnregisterAllEvents()
    end
end)

Fix()
