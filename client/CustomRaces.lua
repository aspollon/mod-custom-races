-- mod-custom-races: the new races on the race choice of the character creation.
-- Loaded after CharacterCreate.xml (GlueXML.toc). The client lists every race its ChrRaces.dbc marks as playable (the
-- client patch marks the races the realm chose, with the faction of their base race); the character creation has
-- buttons for eleven. This file adds buttons for more, lays them out by faction - Alliance on the left, Horde on the
-- right - and gives the new races their portrait, their text, the racial abilities and background scene of their base
-- race, and keeps races without a female model male.

-- @POOL BEGIN (written by tools/pool.py)
local ACTIVE = {} -- @RACES@
local BASES = {}
BASES.Goblin = "Orc" -- @BASE_GOBLIN@
BASES.Zandalari = "Troll" -- @BASE_ZANDALARI@
BASES.Worgen = "Human" -- @BASE_WORGEN@
BASES.KulTiran = "Human" -- @BASE_KULTIRAN@
BASES.Vulpera = "Tauren" -- @BASE_VULPERA@
BASES.Pandaren = "Human" -- @BASE_PANDAREN@
BASES.HighElf = "Human" -- @BASE_HIGHELF@
BASES.FelOrc = "Orc" -- @BASE_FELORC@
BASES.Broken = "Draenei" -- @BASE_BROKEN@
BASES.Earthen = "Dwarf" -- @BASE_EARTHEN@
BASES.Drakkari = "Troll" -- @BASE_DRAKKARI@
BASES.Vrykul = "Human" -- @BASE_VRYKUL@
BASES.Tuskarr = "Dwarf" -- @BASE_TUSKARR@
BASES.Taunka = "Tauren" -- @BASE_TAUNKA@
BASES.ForestTroll = "Troll" -- @BASE_FORESTTROLL@
BASES.IceTroll = "Troll" -- @BASE_ICETROLL@
BASES.Skeleton = "Undead" -- @BASE_SKELETON@
BASES.Naga = "Undead" -- @BASE_NAGA@
BASES.NorthrendSkeleton = "Undead" -- @BASE_NORTHRENDSKELETON@
BASES.LeperTroll = "Troll" -- @BASE_LEPERTROLL@
BASES.Troglodyte = "Dwarf" -- @BASE_TROGLODYTE@
BASES.ThinHuman = "Human" -- @BASE_THINHUMAN@
BASES.HumanCultist = "Human" -- @BASE_HUMANCULTIST@
local POOL = {
	{ key = "Goblin", file = "GOBLIN", name = "Goblin", maleOnly = false, portrait = { kind = "old", male = { 0.625, 0.75, 0.25, 0.5 }, female = { 0.625, 0.75, 0.75, 1.0 } },
		info = "Goblins are cunning merchants and inventors, and the best of them sell to everyone." },
	{ key = "Zandalari", file = "ZANDALARI TROLL", name = "Zandalari Troll", maleOnly = false, portrait = { kind = "icon", male = "Interface\\Icons\\achievement_alliedrace_zandalaritroll", female = "Interface\\Icons\\achievement_alliedrace_zandalaritroll" },
		info = "The Zandalari are the oldest of the trolls, proud keepers of their gods and their empire." },
	{ key = "Worgen", file = "WORGEN", name = "Worgen", maleOnly = false, portrait = { kind = "old", male = { 0.625, 0.75, 0.0, 0.25 }, female = { 0.625, 0.75, 0.5, 0.75 } },
		info = "Cursed people of Gilneas who keep their minds in the shape of a wolf." },
	{ key = "KulTiran", file = "KULTIRAN", name = "Kul Tiran", maleOnly = false, portrait = { kind = "icon", male = "Interface\\Icons\\achievement_alliedrace_kultiranhuman", female = "Interface\\Icons\\achievement_alliedrace_kultiranhuman" },
		info = "Sturdy sailors of the island kingdom of Kul Tiras." },
	{ key = "Vulpera", file = "VULPERA", name = "Vulpera", maleOnly = false, portrait = { kind = "icon", male = "Interface\\Icons\\achievement_alliedrace_vulpera", female = "Interface\\Icons\\achievement_alliedrace_vulpera" },
		info = "Small desert foxes, scavengers and travellers who make a home wherever their caravan stops." },
	{ key = "Pandaren", file = "PANDAREN", name = "Pandaren", maleOnly = false, portrait = { kind = "icon", male = "Interface\\Icons\\achievement_character_pandaren_female", female = "Interface\\Icons\\achievement_character_pandaren_female" },
		info = "Wanderers from the Wandering Isle who seek balance - and good food." },
	{ key = "HighElf", file = "BLOODELF", name = "High Elf", maleOnly = false, portrait = { kind = "round", key = "BLOODELF" },
		info = "The quel'dorei who stayed true to the Alliance after the fall of Quel'Thalas." },
	{ key = "FelOrc", file = "FELORC", name = "Fel Orc", maleOnly = true, portrait = { kind = "icon", male = "Interface\\Icons\\achievement_boss_hellfire_felorccouncil", female = "Interface\\Icons\\achievement_boss_hellfire_felorccouncil" },
		info = "Orcs who drank the blood of demons, and kept its fury." },
	{ key = "Broken", file = "BROKEN", name = "Broken", maleOnly = true, portrait = { kind = "icon", male = "Interface\\Icons\\achievement_boss_triumvirate_voidbrokenbrute", female = "Interface\\Icons\\achievement_boss_triumvirate_voidbrokenbrute" },
		info = "Draenei changed by the fel magic of the orcs, who still remember who they were." },
	{ key = "Earthen", file = "NPC_EARTHEN", name = "Earthen", maleOnly = false, portrait = { kind = "round", key = "DWARF" },
		info = "Living stone shaped by the titans, the forefathers of the dwarves." },
	{ key = "Drakkari", file = "DRAKKARITROLL", name = "Drakkari Troll", maleOnly = false, portrait = { kind = "round", key = "TROLL" },
		info = "The ice trolls of Zul'Drak, an empire that fell to the Scourge." },
	{ key = "Vrykul", file = "VRYKUL", name = "Vrykul", maleOnly = true, portrait = { kind = "icon", male = "Interface\\Icons\\inv_misc_head_vrykul", female = "Interface\\Icons\\inv_misc_head_vrykul" },
		info = "Giant warriors of Northrend, made by the titans and fond of battle." },
	{ key = "Tuskarr", file = "TUSKARR", name = "Tuskarr", maleOnly = true, portrait = { kind = "icon", male = "Interface\\Icons\\inv_misc_head_tuskarr", female = "Interface\\Icons\\inv_misc_head_tuskarr" },
		info = "Fishers and hunters of the Northrend coasts, friendly to all who come in peace." },
	{ key = "Taunka", file = "TAUNKA", name = "Taunka", maleOnly = true, portrait = { kind = "round", key = "TAUREN" },
		info = "The northern cousins of the tauren, hardened by the cold of Northrend." },
	{ key = "ForestTroll", file = "FORESTTROLL", name = "Forest Troll", maleOnly = true, portrait = { kind = "round", key = "TROLL" },
		info = "The trolls of the forests of Lordaeron and the Hinterlands." },
	{ key = "IceTroll", file = "ICETROLL", name = "Ice Troll", maleOnly = true, portrait = { kind = "round", key = "TROLL" },
		info = "The trolls of the frozen north, tough as the ice they live on." },
	{ key = "Skeleton", file = "SKELETON", name = "Skeleton", maleOnly = true, portrait = { kind = "round", key = "SCOURGE" },
		info = "Bones that walk on their own, free of the Lich King." },
	{ key = "Naga", file = "NAGA_", name = "Naga", maleOnly = false, portrait = { kind = "icon", male = "Interface\\Icons\\achievement_boss_nagazone", female = "Interface\\Icons\\achievement_boss_nagazone" },
		info = "Serpent people of the deep sea, once night elves of Zin-Azshari." },
	{ key = "NorthrendSkeleton", file = "NORTHRENDSKELETON", name = "Northrend Skeleton", maleOnly = true, portrait = { kind = "round", key = "SCOURGE" },
		info = "The bones of Northrend, risen in the cold and left without a master." },
	{ key = "LeperTroll", file = "LEPERTROLL", name = "Leper Troll", maleOnly = false, portrait = { kind = "round", key = "TROLL" },
		info = "Trolls marked by a wasting sickness, and the harder for it." },
	{ key = "Troglodyte", file = "TROGLODYTE", name = "Troglodyte", maleOnly = true, portrait = { kind = "icon", male = "Interface\\Icons\\inv_misc_head_kobold_01", female = "Interface\\Icons\\inv_misc_head_kobold_01" },
		info = "Cave dwellers of the deep places of the world." },
	{ key = "ThinHuman", file = "THINHUMAN", name = "Thin Human", maleOnly = true, portrait = { kind = "round", key = "HUMAN" },
		info = "A lean folk of humans, quick and wiry." },
	{ key = "HumanCultist", file = "NPC_HUMANCULTIST", name = "Human Cultist", maleOnly = false, portrait = { kind = "round", key = "HUMAN" },
		info = "Humans who gave themselves to dark powers - and lived to tell of it." },
}
-- The stock races a race can lean on: name in the settings -> file name of the client.
local BASE_FILES = { Human = "Human", Orc = "Orc", Dwarf = "Dwarf", NightElf = "NightElf", Undead = "Scourge", Tauren = "Tauren", Gnome = "Gnome", Troll = "Troll", BloodElf = "BloodElf", Draenei = "Draenei" }
-- @POOL END

if not CharCreateRaceButtonsFrame or not CharCreateRaceButtonMixin or not CharacterCreateEnumerateRaces then
	return
end

local BUTTONS = 32
local ROUND = "Interface\\Glues\\CharacterCreate\\UI-CHARACTERCREATE-RACES_Round"
local OLDROUND = "Interface\\Glues\\CharacterCreate\\UI-CHARACTERCREATE-RACESROUND"

local function Plain(text)
	return ((text or ""):upper():gsub("[^%w]", ""))
end

-- The races of the pool by their name and by their file name (both as the client gives them).
local BY_NAME, BY_FILE = {}, {}
for _, race in ipairs(POOL) do
	race.base = BASE_FILES[BASES[race.key]] and BASES[race.key] or "Human"
	race.baseFile = BASE_FILES[race.base]
	BY_NAME[Plain(race.name)] = race
	BY_FILE[Plain(race.file)] = race
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
		_G["RACE_INFO_" .. upper] = race.info
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
		if race and race.key ~= "HighElf" then
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

-- The portrait: a round one of the stock races or of the older portraits, or an icon of the game.
local function Portrait(self, race, gender)
	local file, coords = ROUND, { 0, 1, 0, 1 }
	local p = race and race.portrait
	if not p then
		return
	elseif p.kind == "round" and RACE_ICON_TCOORDS[p.key .. "_" .. gender] then
		coords = RACE_ICON_TCOORDS[p.key .. "_" .. gender]
	elseif p.kind == "old" then
		file, coords = OLDROUND, (gender == "FEMALE" and p.female or p.male)
	elseif p.kind == "icon" then
		file = gender == "FEMALE" and p.female or p.male
	end
	for _, texture in ipairs({ self:GetNormalTexture(), self:GetPushedTexture() }) do
		texture:SetTexture(file)
		texture:SetTexCoord(unpack(coords))
	end
end

local setRace = CharCreateRaceButtonMixin.SetRace
local function SetRace(self, raceName, raceEnglishName, gender, enabled)
	local race = Find(raceName, raceEnglishName)
	local key = raceEnglishName .. "_" .. gender
	if RACE_ICON_TCOORDS and not RACE_ICON_TCOORDS[key] then
		RACE_ICON_TCOORDS[key] = RACE_ICON_TCOORDS["HUMAN_" .. gender]
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

-- Alliance on the left, Horde on the right. A faction with more races than fit below each other gets more columns,
-- towards the middle, and the rows come closer together.
local function Layout(count)
	local sides = { Alliance = {}, Horde = {} }
	for i = 1, count do
		local _, faction = GetFactionForRace(i)
		if faction ~= "Horde" then
			faction = "Alliance"
		end
		table.insert(sides[faction], i)
	end
	local size = CharCreateRaceButton1 and CharCreateRaceButton1:GetHeight() or 52
	local room = math.max(300, (CharCreateRaceButtonsFrame:GetHeight() or 620) - 120)
	for faction, list in pairs(sides) do
		local n = #list
		local fit = math.max(1, math.floor((room + 12) / (size + 12)))
		local columns = math.max(1, math.ceil(n / fit))
		local rows = math.ceil(n / columns)
		local gap = rows > 1 and math.min(30, math.floor((room - rows * size) / (rows - 1))) or 30
		for k, i in ipairs(list) do
			local button = _G["CharCreateRaceButton" .. i]
			local column, row = math.floor((k - 1) / rows), (k - 1) % rows
			button:ClearAllPoints()
			if faction == "Alliance" then
				button:SetPoint("TOPLEFT", CharCreateRaceButtonsFrame, "TOPLEFT", 32 + column * 170, -75 - row * (size + gap))
			else
				button:SetPoint("TOPRIGHT", CharCreateRaceButtonsFrame, "TOPRIGHT", -32 - column * 170, -75 - row * (size + gap))
			end
			button:SetFaction(faction)
			button:Show()
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

	local setCharacterRace = SetCharacterRace
	function SetCharacterRace(id)
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
