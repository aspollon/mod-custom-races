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

local function CreateButton()
    if button or not PaperDollFrame then
        return
    end

    button = CreateFrame("Button", "AFKHideGearButton", PaperDollFrame, "UIPanelButtonTemplate")
    button:SetSize(132, 22)
    button:SetPoint("BOTTOMLEFT", PaperDollFrame, "BOTTOMLEFT", 8, 8)
    button:SetText(Label(false))
    button:SetScript("OnClick", Toggle)
    button:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetText("Hide gear", 1, 1, 1)
        GameTooltip:AddLine("Hides every armor piece for everyone; weapons stay visible. Saved per character.", 1, 1, 1, true)
        GameTooltip:Show()
    end)
    button:SetScript("OnLeave", function() GameTooltip:Hide() end)
end

local frame = CreateFrame("Frame")
frame:RegisterEvent("PLAYER_LOGIN")
frame:RegisterEvent("CHAT_MSG_SYSTEM")
frame:SetScript("OnEvent", function(_, event, message)
    if event == "PLAYER_LOGIN" then
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
