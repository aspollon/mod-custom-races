-- mod-custom-races: the new races on the race choice of the character creation.
-- Loaded after CharacterCreate.xml (GlueXML.toc). The client lists every race its ChrRaces.dbc marks as playable
-- (the client patch marks the new ones); the character creation has buttons for eleven of them. This file adds
-- buttons for more, lays them out by faction - Alliance on the left, Horde on the right - and gives the new races
-- their icon, their text and the racial abilities of their base race.

-- The new races, by the file name of their row in ChrRaces.dbc. male, female: where the race's portraits sit on
-- Interface\Glues\CharacterCreate\UI-CharacterCreate-Races (left, right, top, bottom); base: the stock race whose
-- racial abilities it has.
local CUSTOM_RACES = {
	GOBLIN = {
		base = "ORC",
		male = { 0.625, 0.75, 0.25, 0.5 },
		female = { 0.625, 0.75, 0.75, 1.0 },
		info = "Goblins are cunning merchants and inventors. They stand with the Horde, which they serve with their " ..
			"wits, their gold and their explosives.",
	},
}

local BUTTONS = 24

if not CharCreateRaceButtonsFrame or not CharCreateRaceButtonMixin or not CharacterCreateEnumerateRaces then
	return
end

-- Icons, texts and racial abilities.
for file, race in pairs(CUSTOM_RACES) do
	if RACE_ICON_TCOORDS then
		RACE_ICON_TCOORDS[file .. "_MALE"] = RACE_ICON_TCOORDS[file .. "_MALE"] or race.male
		RACE_ICON_TCOORDS[file .. "_FEMALE"] = RACE_ICON_TCOORDS[file .. "_FEMALE"] or race.female
	end
	if not _G["RACE_INFO_" .. file] or _G["RACE_INFO_" .. file] == "" then
		_G["RACE_INFO_" .. file] = race.info
	end
	local prefix = "ABILITY_INFO_" .. race.base
	local copies = {}
	for key, value in pairs(_G) do
		if type(key) == "string" and type(value) == "string" and key:sub(1, #prefix) == prefix then
			copies["ABILITY_INFO_" .. file .. key:sub(#prefix + 1)] = value
		end
	end
	for key, value in pairs(copies) do
		if _G[key] == nil then
			_G[key] = value
		end
	end
	if CHAR_CREATE_PASSIVES and CHAR_CREATE_PASSIVES[race.base] and not CHAR_CREATE_PASSIVES[file] then
		CHAR_CREATE_PASSIVES[file] = CHAR_CREATE_PASSIVES[race.base]
	end
end

-- A button made here has no race yet when it loads.
local onLoad = CharCreateRaceButtonMixin.OnLoad
function CharCreateRaceButtonMixin:OnLoad()
	if self:GetID() > 0 then
		return onLoad(self)
	end
	self:GetNormalTexture():SetDrawLayer("BACKGROUND")
	self:GetPushedTexture():SetDrawLayer("BACKGROUND")
	self:SetFaction("Alliance")
end

-- The race buttons show the round portraits of UI-CHARACTERCREATE-RACES_Round, which has none of the new races; a new
-- race shows its square portrait of UI-CharacterCreate-Races instead. A race without a portrait or a text of its own
-- still gets a button.
local ROUND = "Interface\\Glues\\CharacterCreate\\UI-CHARACTERCREATE-RACES_Round"
local SQUARE = "Interface\\Glues\\CharacterCreate\\UI-CharacterCreate-Races"
local setRace = CharCreateRaceButtonMixin.SetRace
local function SetRace(self, raceName, raceEnglishName, gender, enabled)
	local custom = CUSTOM_RACES[raceEnglishName]
	if RACE_ICON_TCOORDS and not RACE_ICON_TCOORDS[raceEnglishName .. "_" .. gender] then
		RACE_ICON_TCOORDS[raceEnglishName .. "_" .. gender] = RACE_ICON_TCOORDS["HUMAN_" .. gender]
	end
	if not GetFlavorText("RACE_INFO_" .. raceEnglishName, gender) then
		_G["RACE_INFO_" .. raceEnglishName] = raceName
	end
	local file = custom and SQUARE or ROUND
	self:GetNormalTexture():SetTexture(file)
	self:GetPushedTexture():SetTexture(file)
	return setRace(self, raceName, raceEnglishName, gender, enabled)
end
CharCreateRaceButtonMixin.SetRace = SetRace

MAX_RACES = BUTTONS
for i = 1, BUTTONS do
	local button = _G["CharCreateRaceButton" .. i]
	if not button then
		button = CreateFrame("CheckButton", "CharCreateRaceButton" .. i, CharCreateRaceButtonsFrame, "CharCreateRaceButtonTemplate")
		button:SetID(i)
		button:Hide()
	end
	button.SetRace = SetRace
end

-- Alliance on the left, Horde on the right, each in a column; a long column continues in a second one inside it.
local function Layout(count)
	local placed = { Alliance = 0, Horde = 0 }
	local first = CharCreateRaceButton1
	local height = (first and first:GetHeight() or 50) + 30
	local perColumn = math.max(1, math.floor(((CharCreateRaceButtonsFrame:GetHeight() or 0) - 100) / height))
	if perColumn < 5 then
		perColumn = 6
	end
	for i = 1, count do
		local button = _G["CharCreateRaceButton" .. i]
		local _, faction = GetFactionForRace(i)
		if faction ~= "Horde" then
			faction = "Alliance"
		end
		local k = placed[faction]
		placed[faction] = k + 1
		local column, row = math.floor(k / perColumn), k % perColumn
		button:ClearAllPoints()
		if faction == "Alliance" then
			button:SetPoint("TOPLEFT", CharCreateRaceButtonsFrame, "TOPLEFT", 32 + column * 170, -75 - row * height)
		else
			button:SetPoint("TOPRIGHT", CharCreateRaceButtonsFrame, "TOPRIGHT", -32 - column * 170, -75 - row * height)
		end
		button:SetFaction(faction)
		button:Show()
	end
end

local enumerate = CharacterCreateEnumerateRaces
function CharacterCreateEnumerateRaces(...)
	enumerate(...)
	Layout(math.min(CharacterCreate.numRaces or 0, BUTTONS))
end
