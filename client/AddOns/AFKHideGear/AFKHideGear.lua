-- AFK Hide Gear: a button in the character window that toggles .hidearmor
-- (mod-custom-races). The server remembers the choice per character.

local function Label(hidden)
    if GetLocale() == "deDE" then
        return hidden and "Ausrüstung anzeigen" or "Ausrüstung ausblenden"
    end
    return hidden and "Show gear" or "Hide gear"
end

local hidden = false
local button

local function Update()
    if button then
        button:SetText(Label(hidden))
    end
end

local function Toggle()
    SendChatMessage(".hidearmor", "SAY")
end

local hooked

local function CreateButton()
    if button then
        return
    end

    -- Ascension uses its own character frame; older clients use PaperDollFrame.
    local parent = _G["AscensionCharacterFrame"] or _G["CharacterFrame"] or _G["PaperDollFrame"]
    if not parent then
        return
    end

    button = CreateFrame("Button", "AFKHideGearButton", parent, "UIPanelButtonTemplate")
    button:SetSize(132, 22)
    button:SetPoint("TOPLEFT", parent, "BOTTOMLEFT", 8, -4)
    button:SetFrameStrata("DIALOG")
    button:SetFrameLevel((parent:GetFrameLevel() or 1) + 30)
    button:SetClampedToScreen(true)
    button:SetText(Label(false))
    button:SetScript("OnClick", Toggle)
    button:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetText("Hide gear", 1, 1, 1)
        GameTooltip:AddLine("Hides every armor piece for everyone; weapons stay visible. Saved per character.", 1, 1, 1, true)
        GameTooltip:Show()
    end)
    button:SetScript("OnLeave", function() GameTooltip:Hide() end)
    button:Show()

    if not hooked and parent.HookScript then
        hooked = true
        parent:HookScript("OnShow", CreateButton)
    end
end

local frame = CreateFrame("Frame")
frame:RegisterEvent("ADDON_LOADED")
frame:RegisterEvent("PLAYER_LOGIN")
frame:RegisterEvent("PLAYER_ENTERING_WORLD")
frame:RegisterEvent("CHAT_MSG_SYSTEM")
frame:SetScript("OnEvent", function(_, event, message)
    if event == "ADDON_LOADED" then
        CreateButton()
    elseif event == "PLAYER_LOGIN" or event == "PLAYER_ENTERING_WORLD" then
        CreateButton()
        SendChatMessage(".hidearmor status", "SAY")
    elseif event == "CHAT_MSG_SYSTEM" then
        if message == "Armor hidden." then
            hidden = true
            Update()
        elseif message == "Armor shown again." then
            hidden = false
            Update()
        end
    end
end)

SLASH_AFKHIDEGEAR1 = "/hidegear"
SlashCmdList.AFKHIDEGEAR = Toggle
