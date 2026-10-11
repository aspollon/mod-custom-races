-- mod-custom-races: the new races on the race choice of the character creation.
-- Loaded after CharacterCreate.xml (GlueXML.toc). The client lists every race its ChrRaces.dbc marks as playable (the
-- client patch marks the races the realm chose, with the faction of their base race); the character creation has
-- buttons for eleven. This file adds buttons for more, lays them out by faction - Alliance on the left, Horde on the
-- right - and gives the new races their portrait, their text, the racial abilities and background scene of their base
-- race, and keeps races without a female model male.

-- The races the realm chose, as AFK Realm read them from this game client (mod_custom_races.conf): one line each.
local RACES = {}
-- @RACES@

-- @KNOWN BEGIN (written by tools/races.py)
local KNOWN = {
	Goblin = { portrait = { kind = "old", male = { 0.625, 0.75, 0.25, 0.5 }, female = { 0.625, 0.75, 0.75, 1.0 } } },
	ZandalariTroll = { portrait = { kind = "icon", male = "Interface\\Icons\\achievement_alliedrace_zandalaritroll", female = "Interface\\Icons\\achievement_alliedrace_zandalaritroll" } },
	Worgen = { portrait = { kind = "old", male = { 0.625, 0.75, 0.0, 0.25 }, female = { 0.625, 0.75, 0.5, 0.75 } } },
	KulTiran = { portrait = { kind = "icon", male = "Interface\\Icons\\achievement_alliedrace_kultiranhuman", female = "Interface\\Icons\\achievement_alliedrace_kultiranhuman" } },
	Vulpera = { portrait = { kind = "icon", male = "Interface\\Icons\\achievement_alliedrace_vulpera", female = "Interface\\Icons\\achievement_alliedrace_vulpera" } },
	Pandaren = { portrait = { kind = "icon", male = "Interface\\Icons\\achievement_character_pandaren_female", female = "Interface\\Icons\\achievement_character_pandaren_female" } },
	HighElf = { portrait = { kind = "round", key = "BLOODELF" } },
	FelOrc = { portrait = { kind = "icon", male = "Interface\\Icons\\achievement_boss_hellfire_felorccouncil", female = "Interface\\Icons\\achievement_boss_hellfire_felorccouncil" } },
	Broken = { portrait = { kind = "icon", male = "Interface\\Icons\\achievement_boss_triumvirate_voidbrokenbrute", female = "Interface\\Icons\\achievement_boss_triumvirate_voidbrokenbrute" } },
	Earthen = { portrait = nil },
	DrakkariTroll = { portrait = nil },
	Vrykul = { portrait = { kind = "icon", male = "Interface\\Icons\\inv_misc_head_vrykul", female = "Interface\\Icons\\inv_misc_head_vrykul" } },
	Tuskarr = { portrait = { kind = "icon", male = "Interface\\Icons\\inv_misc_head_tuskarr", female = "Interface\\Icons\\inv_misc_head_tuskarr" } },
	Taunka = { portrait = nil },
	ForestTroll = { portrait = nil },
	IceTroll = { portrait = nil },
	Skeleton = { portrait = nil },
	Naga = { portrait = { kind = "icon", male = "Interface\\Icons\\achievement_boss_nagazone", female = "Interface\\Icons\\achievement_boss_nagazone" } },
	NorthrendSkeleton = { portrait = nil },
	LeperTroll = { portrait = nil },
	Troglodyte = { portrait = { kind = "icon", male = "Interface\\Icons\\inv_misc_head_kobold_01", female = "Interface\\Icons\\inv_misc_head_kobold_01" } },
	ThinHuman = { portrait = nil },
	HumanCultist = { portrait = nil },
}
-- The stock races a race can lean on: name in the settings -> file name of the client.
local BASE_FILES = { Human = "Human", Orc = "Orc", Dwarf = "Dwarf", NightElf = "NightElf", Undead = "Scourge", Tauren = "Tauren", Gnome = "Gnome", Troll = "Troll", BloodElf = "BloodElf", Draenei = "Draenei" }
-- @KNOWN END

if not CharCreateRaceButtonsFrame or not CharCreateRaceButtonMixin or not CharacterCreateEnumerateRaces then
	return
end

local BUTTONS = 32
local ROUND = "Interface\\Glues\\CharacterCreate\\UI-CHARACTERCREATE-RACES_Round"
local OLDROUND = "Interface\\Glues\\CharacterCreate\\UI-CHARACTERCREATE-RACESROUND"

local function Plain(text)
	return ((text or ""):upper():gsub("[^%w]", ""))
end

-- The races by their name and by their file name (both as the client gives them). A race copied from a stock race
-- (the High Elf from the Blood Elf) shares its file name; only its name finds it.
local STOCK_FILES = {}
for _, file in pairs(BASE_FILES) do
	STOCK_FILES[Plain(file)] = true
end
local BY_NAME, BY_FILE = {}, {}
for _, race in ipairs(RACES) do
	local known = KNOWN[race.key] or {}
	race.portrait = known.portrait
	race.base = BASE_FILES[race.base] and race.base or "Human"
	race.baseFile = BASE_FILES[race.base]
	BY_NAME[Plain(race.name)] = race
	if not STOCK_FILES[Plain(race.file)] then
		BY_FILE[Plain(race.file)] = race
	end
end

local function Find(raceName, raceEnglishName)
	return BY_NAME[Plain(raceName)] or BY_FILE[Plain(raceEnglishName)]
end

-- The races as the client listed them last, by their place in the list.
local listed = {}

local function Copy(list, from, to)
	if type(list) == "table" and list[from] ~= nil and list[to] == nil then
		list[to] = list[from]
	end
end

-- Texts, racial abilities, scene, light, sound and camera of the base race.
local function Lend(race, upper, file)
	local base = race.baseFile:upper()
	if not _G["RACE_INFO_" .. upper] or _G["RACE_INFO_" .. upper] == "" then
		_G["RACE_INFO_" .. upper] = race.name
	end
	local prefix = "ABILITY_INFO_" .. base
	local copies = {}
	for key, value in pairs(_G) do
		if type(key) == "string" and type(value) == "string" and key:sub(1, #prefix) == prefix then
			copies["ABILITY_INFO_" .. upper .. key:sub(#prefix + 1)] = value
		end
	end
	for key, value in pairs(copies) do
		if _G[key] == nil then
			_G[key] = value
		end
	end
	Copy(CHAR_CREATE_PASSIVES, base, upper)
	Copy(GlueAmbienceTracks, base, upper)
	Copy(CharModelFogInfo, base, upper)
	Copy(RaceLights, base, upper)
	Copy(CHAR_CREATE_CAMERA_ZOOM_IN, race.baseFile, file)
	Copy(CHAR_CREATE_CAMERA_ZOOM_OUTS, race.baseFile, file)
end

-- The character creation and selection show the scene of a race from Interface\Glues\Models\UI_<race>; a new race
-- shows the one of its base race.
if SetBackgroundModel then
	local setBackgroundModel = SetBackgroundModel
	function SetBackgroundModel(model, name)
		local upper = name and strupper(name) or ""
		local file, zoom = upper:match("^(.-)(_ZOOM)$")
		file = file or upper
		local race = BY_FILE[Plain(file)]
		if race then
			name = race.baseFile:upper() .. (zoom or "")
		end
		return setBackgroundModel(model, name)
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

-- The portrait, always round like those of the stock races: a portrait of the older round set, an icon of the game cut
-- round, or the round portrait of a stock race - its base race when nothing better is known.
local function Portrait(self, race, gender)
	if not race then
		return
	end
	local p = race.portrait or {}
	if race.icon and race.icon ~= "" then
		p = { kind = "icon", male = race.icon, female = race.icon }
	end
	local textures = { self:GetNormalTexture(), self:GetPushedTexture() }
	if p.kind == "icon" and SetPortraitToTexture then
		local file = gender == "FEMALE" and p.female or p.male
		for _, texture in ipairs(textures) do
			texture:SetTexCoord(0, 1, 0, 1)
			SetPortraitToTexture(texture, file)
		end
		return
	end
	local file, coords = ROUND, nil
	if p.kind == "old" then
		file, coords = OLDROUND, (gender == "FEMALE" and p.female or p.male)
	else
		local key = (p.kind == "round" and p.key or race.baseFile:upper()) .. "_" .. gender
		coords = RACE_ICON_TCOORDS and (RACE_ICON_TCOORDS[key] or RACE_ICON_TCOORDS[race.baseFile:upper() .. "_" .. gender])
	end
	if not coords then
		return
	end
	for _, texture in ipairs(textures) do
		texture:SetTexture(file)
		texture:SetTexCoord(unpack(coords))
	end
end

local setRace = CharCreateRaceButtonMixin.SetRace
local function SetRace(self, raceName, raceEnglishName, gender, enabled)
	local race = Find(raceName, raceEnglishName)
	local key = raceEnglishName .. "_" .. gender
	if RACE_ICON_TCOORDS and not RACE_ICON_TCOORDS[key] then
		RACE_ICON_TCOORDS[key] = RACE_ICON_TCOORDS[(race and race.baseFile:upper() or "HUMAN") .. "_" .. gender] or RACE_ICON_TCOORDS["HUMAN_" .. gender]
	end
	if race then
		Lend(race, raceEnglishName, raceEnglishName:sub(1, 1) .. raceEnglishName:sub(2):lower())
	elseif not GetFlavorText("RACE_INFO_" .. raceEnglishName, gender) then
		_G["RACE_INFO_" .. raceEnglishName] = raceName
	end
	for _, texture in ipairs({ self:GetNormalTexture(), self:GetPushedTexture() }) do
		texture:SetTexture(ROUND)
	end
	local result = setRace(self, raceName, raceEnglishName, gender, enabled)
	Portrait(self, race, gender)
	return result
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

-- Alliance on the left, Horde on the right. The stock races keep one column at the edge; the added races get columns
-- of their own next to it, towards the middle, from the top - not below the stock races, where buttons and tooltips
-- of the character creation are.
local function Layout(count)
	local sides = { Alliance = { stock = {}, added = {} }, Horde = { stock = {}, added = {} } }
	for i = 1, count do
		local _, faction = GetFactionForRace(i)
		if faction ~= "Horde" then
			faction = "Alliance"
		end
		table.insert(listed[i] and sides[faction].added or sides[faction].stock, i)
	end
	local size = CharCreateRaceButton1 and CharCreateRaceButton1:GetHeight() or 52
	local room = math.max(300, (CharCreateRaceButtonsFrame:GetHeight() or 620) - 120)
	local fit = math.max(1, math.floor((room + 12) / (size + 12)))
	local function place(button, faction, column, row, gap)
		button:ClearAllPoints()
		if faction == "Alliance" then
			button:SetPoint("TOPLEFT", CharCreateRaceButtonsFrame, "TOPLEFT", 32 + column * 170, -75 - row * (size + gap))
		else
			button:SetPoint("TOPRIGHT", CharCreateRaceButtonsFrame, "TOPRIGHT", -32 - column * 170, -75 - row * (size + gap))
		end
		button:SetFaction(faction)
		button:Show()
	end
	for faction, side in pairs(sides) do
		local n = #side.stock
		local gap = n > 1 and math.min(30, math.floor((room - n * size) / (n - 1))) or 30
		for k, i in ipairs(side.stock) do
			place(_G["CharCreateRaceButton" .. i], faction, 0, k - 1, gap)
		end
		local first = n > 0 and 1 or 0
		for k, i in ipairs(side.added) do
			place(_G["CharCreateRaceButton" .. i], faction, first + math.floor((k - 1) / fit), (k - 1) % fit, 12)
		end
	end
end

local enumerate = CharacterCreateEnumerateRaces
function CharacterCreateEnumerateRaces(...)
	listed = {}
	for i = 1, select("#", ...), 3 do
		local raceName, raceEnglishName = select(i, ...)
		listed[#listed + 1] = Find(raceName, raceEnglishName) or false
	end
	enumerate(...)
	Layout(math.min(CharacterCreate.numRaces or 0, BUTTONS))
end

-- Races without a female model are male only.
local function MaleOnly()
	local race = listed[GetSelectedRace and GetSelectedRace() or 0]
	return race and race.maleOnly
end

if SetCharacterGender and SetCharacterRace and CharCreateFemaleButton then
	local setGender = SetCharacterGender
	function SetCharacterGender(sex)
		if sex == SEX_FEMALE and MaleOnly() then
			sex = SEX_MALE
		end
		return setGender(sex)
	end

	-- The game builds the model of the chosen race and sex at once: a woman of a race without a female body crashes
	-- it. A click on such a race switches to male first, while the race shown before is still the chosen one.
	local setCharacterRace = SetCharacterRace
	function SetCharacterRace(id)
		local race = listed[id]
		if race and race.maleOnly and GetSelectedSex() == SEX_FEMALE then
			setGender(SEX_MALE)
		end
		local result = setCharacterRace(id)
		if MaleOnly() then
			CharCreateFemaleButton:Disable()
			if GetSelectedSex() == SEX_FEMALE then
				SetCharacterGender(SEX_MALE)
			end
		else
			CharCreateFemaleButton:Enable()
		end
		return result
	end
end
