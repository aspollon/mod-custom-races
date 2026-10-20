-- mod-custom-races: the new races on the race choice of the character creation.
-- Loaded after CharacterCreate.xml (GlueXML.toc). The client lists every race its ChrRaces.dbc marks as playable (the
-- client patch marks the races the realm chose, with the faction of their base race); the character creation has
-- buttons for eleven. This file adds buttons for more, lays them out by faction - Alliance on the left, Horde on the
-- right - and gives the new races their portrait, their text, the racial abilities and background scene of their base
-- race, and keeps races without a female model male.
-- The stock races keep one column at their edge; the added races fill columns of their own towards the middle, from
-- the top, and give way to the class choice in the lower middle (a column that would reach it keeps only the rows
-- above it). A side with more races than its columns hold scrolls: the mouse wheel, or the bar next to the last
-- column.

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
	VulperaEsteria = { portrait = { kind = "icon", male = "Interface\\Icons\\achievement_alliedrace_vulpera", female = "Interface\\Icons\\achievement_alliedrace_vulpera" } },
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

local BUTTONS = 64                 -- race buttons: with the 64-race patch the client lists up to 63 races
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

-- A short text for the tooltip of races the client has none for (RACE_INFO_<race>), by the race's key or name without
-- spaces; a race not listed here shows its name, as before.
local DESCRIPTIONS = {
	Worgen = "Gilneans cursed with the form of the wolf, who learned to keep their minds while the beast runs free.",
	Goblin = "Cunning traders and tinkerers from Kezan. Everything has a price, and every price can be beaten.",
	ZandalariTroll = "The proud trolls of Zandalar, eldest of their kind, guided by the loa of their ancient empire.",
	KulTiran = "Broad-shouldered sailors of the island kingdom of Kul Tiras, at home on every sea.",
	Vulpera = "Nimble desert foxes of Vol'dun who wander in caravans and always have something useful in the bag.",
	Pandaren = "Wandering monks, brewers and philosophers from the mists of Pandaria, seeking balance in all things.",
	HighElf = "Quel'dorei who never gave up the Light of the Sunwell, keepers of an old and proud tradition.",
	FelOrc = "Orcs who drank demon blood, stronger and angrier than their brothers, marked by the fel forever.",
	Broken = "Draenei twisted by the fel who still carry a spark of the Naaru's Light.",
	Earthen = "Titan-forged guardians of living stone, woken again to protect the depths of the world.",
	DrakkariTroll = "Ice trolls of the fallen Drakkari empire of Zul'Drak, who once drank the blood of their own gods.",
	Vrykul = "Towering warriors of Northrend, forged by the titans and feared on every shore.",
	Tuskarr = "Hardy fisher-folk of Northrend's coasts, patient, generous and unshakable.",
	Taunka = "The northern kin of the Tauren, hardened by the long winters of the Borean Tundra.",
	Naga = "Serpentine children of the deep, once highborne, now servants of the tides.",
	ForestTroll = "Trolls of the Hinterlands and Eversong, ferocious hunters who heal from almost anything.",
	IceTroll = "Trolls of the frozen north, hardened by ice and hunger.",
	Skeleton = "The restless dead, risen once more and bound to a will of their own.",
	Murloc = "Mrglglgl! Creatures of the shallows, slippery, loud and never alone for long.",
	Arakkoa = "Bird-folk of the spires of Skettis and Draenor, keepers of ancient secrets and dark magic.",
	HighmountainTauren = "Tauren of the mountains of Highmountain, mighty and patient as the great moose they ride.",
	Mechagnome = "Gnomes of Mechagon who replaced flesh with metal, always improving, always tinkering.",
	Haranir = "Root-folk of the Harandar, who draw life from the great roots beneath the world.",
	Skyborne = "A people of the winds and high places, never quite at home on solid ground.",
	DarkIronDwarf = "Dwarves of the Shadowforge, masters of fire and forge, tempered in the flames of Blackrock.",
	MagharOrc = "Uncorrupted orcs of Draenor, who never drank the demon's blood and kept the old ways.",
	LightforgedDraenei = "Draenei who fought the Legion for ages at the side of the Army of the Light.",
	Nightborne = "The elves of Suramar, masters of the arcane, freed after ten thousand years under the dome.",
	VoidElf = "Elves touched by the Void, who learned to master the shadow instead of fearing it.",
	Dracthyr = "Dragon-kin of the Dragon Isles, created to be the perfect weapon and now free to choose their path.",
	Eredar = "The ancient people of Argus, gifted and proud, whose brilliance made them a prize for the Legion.",
	Harpy = "Winged hunters of the cliffs and storms, quick to strike and quicker to vanish.",
	IllidariBloodElf = "Blood elves who followed Illidan, trading part of themselves for the power to hunt demons.",
	IllidariNightElf = "Night elves who followed Illidan and chose the hunt over the safety of their kin.",
	UndeadGnome = "Gnomes who did not let death stop their tinkering.",
	Ogre = "Big, strong and not always clever, but no one ever argues with an ogre twice.",
	Furbolg = "Bear-folk of the deep forests, gentle friends and terrible foes.",
	Jinyu = "Fish-folk of Pandaria who read the will of the waters.",
	Darkfallen = "San'layn of Northrend, elves of blood and shadow who serve no king anymore.",
	Gnoll = "Hyena-folk of the wild lands, cunning in the pack and dangerous alone.",
	Drakonid = "Mighty dragon-kin, bred for war in the shadow of the great flights.",
	Drakkin = "Mortals with the blood of dragons in their veins, who carry the gifts of their flight.",
	Grummle = "Small, cheerful traders of the mountains who never leave home without their pack and a lucky charm.",
	Sethrak = "Snake-folk of Vol'dun, proud and wary, guardians of the secrets of the desert.",
}

-- Texts, racial abilities, scene, light, sound and camera of the base race.
local function Lend(race, upper, file)
	local base = race.baseFile:upper()
	-- a text the client has stays; one that only repeats the race's name is no description
	local have = _G["RACE_INFO_" .. upper]
	if type(have) ~= "string" or Plain(have) == "" or Plain(have) == Plain(race.name) or Plain(have) == Plain(upper) or Plain(have) == Plain(race.key) then
		local wanted = Plain(race.key)
		local text
		for key, value in pairs(DESCRIPTIONS) do
			if Plain(key) == wanted or Plain(key) == Plain(race.name) or Plain(key) == Plain(upper) then
				text = value
			end
		end
		_G["RACE_INFO_" .. upper] = text or race.name
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

-- The tooltip of a race shows the racial abilities of its base race (lent above) and, below them, its own: the spells
-- AFK Realm gives it (CustomRaces.<race>.Racials), with icon, name and text from the client's spell table - the same
-- way the game shows those of the stock races. A spell the client does not know is left out.
local function OwnRacials(self, race)
	if not race or not race.racials or race.racials == "" or type(self.tooltipText) ~= "table" or not GetSpellData then
		return
	end
	local lines = {}
	for id in race.racials:gmatch("%d+") do
		local ok, name, text, icon = pcall(GetSpellData, tonumber(id), false, true)
		if ok and name and name ~= "" then
			text = type(text) == "string" and text:gsub("[\r\n]+", " ") or ""
			if text:find("%$") then
				text = ""                                 -- a text with the client's $ variables left unresolved
			end
			local line = (icon and icon ~= "" and ("|T" .. icon .. ":24:24:0:0|t") or "") .. "|cffffffff" .. name .. "|r"
			if text ~= "" then
				line = line .. " - " .. text
			end
			lines[#lines + 1] = line .. "\n"
		end
	end
	if #lines > 0 then
		tinsert(self.tooltipText, "|cffffd100" .. (CUSTOM_RACES_OWN_RACIALS or "Racial abilities of its own:") .. "|r\n")
		for _, line in ipairs(lines) do
			tinsert(self.tooltipText, line)
		end
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
	OwnRacials(self, race)
	return result
end
CharCreateRaceButtonMixin.SetRace = SetRace

-- A side with more races than its columns hold scrolls: its races move by the mouse wheel over the buttons, or by
-- its bar. The offset lives here, so it keeps its place when the races are laid out again.
local SCROLL = { Alliance = { offset = 0 }, Horde = { offset = 0 } }
local BARS = {}
local ZONE
local LAST_COUNT = 0
local Layout, Watch

local function ScrollWheel(faction, delta)
	local state = SCROLL[faction] or SCROLL.Alliance
	if state.scrolling then
		state.offset = (state.offset or 0) + (delta > 0 and -1 or 1)
		Layout(LAST_COUNT)
	end
end

local function OnRaceWheel(self, delta)
	ScrollWheel(self.faction or "Alliance", delta)
end

MAX_RACES = BUTTONS
for i = 1, BUTTONS do
	local button = _G["CharCreateRaceButton" .. i]
	if not button then
		button = CreateFrame("CheckButton", "CharCreateRaceButton" .. i, CharCreateRaceButtonsFrame, "CharCreateRaceButtonTemplate")
		button:SetID(i)
		button:Hide()
	end
	button.SetRace = SetRace
	button:EnableMouseWheel(true)
	button:SetScript("OnMouseWheel", OnRaceWheel)
end

-- What the character creation shows for the class choice - its frame and the class buttons of the client in it -
-- as one rectangle: what a race column must stay out of. Left and right in screen units, top upwards.
local function ClassZone()
	local W = CharCreateRaceButtonsFrame:GetWidth() or 1024
	local l, r, top = W / 2 - 400, W / 2 + 400, 292
	local frame = CharCreateClassFrame
	if frame and frame:IsShown() then
		local fl, fr, ft = frame:GetLeft(), frame:GetRight(), frame:GetTop()
		if fl and fr and ft and fr - fl > 100 then
			l, r, top = fl, fr, ft
		end
		local function Eat(item)
			local kind = item:GetObjectType()
			if (kind == "Button" or kind == "CheckButton") and item:IsShown() then
				local il, ir, it = item:GetLeft(), item:GetRight(), item:GetTop()
				if il and ir and it and ir - il > 8 then
					if il < l then
						l = il
					end
					if ir > r then
						r = ir
					end
					if it > top then
						top = it
					end
				end
			end
		end
		local function Walk(item, depth)
			if depth > 3 then
				return
			end
			for _, child in ipairs({ item:GetChildren() }) do
				Eat(child)
				Walk(child, depth + 1)
			end
		end
		Walk(frame, 0)
	end
	return l, r, top
end

-- Scroll one side and lay the sides out anew.
local function ScrollTo(faction, offset)
	SCROLL[faction].offset = offset
	Layout(LAST_COUNT)
end

-- The bar of a side: a thin track with a thumb to drag, the mouse wheel over it. Shown when a side has more races
-- than its columns hold.
local function CreateBar(faction)
	local track = CreateFrame("Frame", nil, CharCreateRaceButtonsFrame)
	track:SetWidth(6)
	track:EnableMouse(true)
	track:EnableMouseWheel(true)
	local back = track:CreateTexture(nil, "BACKGROUND")
	back:SetAllPoints(track)
	back:SetTexture("Interface\\Tooltips\\UI-Tooltip-Background")
	back:SetVertexColor(0, 0, 0, 0.4)
	local thumb = CreateFrame("Frame", nil, track)
	thumb:SetWidth(6)
	thumb:EnableMouse(true)
	local knob = thumb:CreateTexture(nil, "ARTWORK")
	knob:SetAllPoints(thumb)
	knob:SetTexture("Interface\\Tooltips\\UI-Tooltip-Background")
	knob:SetVertexColor(0.85, 0.85, 0.85, 0.9)
	local drag
	thumb:SetScript("OnMouseDown", function()
		local _, y = GetCursorPosition()
		drag = { y = y, offset = SCROLL[faction].offset or 0 }
	end)
	thumb:SetScript("OnMouseUp", function()
		drag = nil
	end)
	thumb:SetScript("OnUpdate", function()
		local state = SCROLL[faction]
		local bar = BARS[faction]
		if not drag or not state.scrolling or not bar then
			return
		end
		local span = (bar.track:GetHeight() or 0) - (bar.thumb:GetHeight() or 0)
		local rows = (state.total or 0) - (state.rows or 0)
		if span <= 0 or rows <= 0 then
			return
		end
		local _, y = GetCursorPosition()
		ScrollTo(faction, drag.offset + (drag.y - y) / (track:GetEffectiveScale() or 1) * rows / span)
	end)
	track:SetScript("OnMouseWheel", function(_, delta)
		ScrollWheel(faction, delta)
	end)
	return { track = track, thumb = thumb }
end

-- The race buttons stand as a grid of portraits: smaller than the game's (SCALE), without their names and the coloured
-- band behind them - the name is in the tooltip - so that many races fit next to each other.
local SCALE, PITCH, GAP = 0.8, 60, 12

-- Alliance on the left, Horde on the right. The stock races keep one column at the edge; the added races fill
-- columns of their own towards the middle, from the top. A column whose buttons would stand over the class choice
-- of the lower middle keeps only the rows above it; where its name is, the class choice is left alone, too. When a
-- side has more races than its columns hold, the rest scrolls.
Layout = function(count)
	local frame = CharCreateRaceButtonsFrame
	local W = frame:GetWidth() or 1024
	local H = frame:GetHeight() or 768
	local frameTop = frame:GetTop() or H
	local frameBottom = frame:GetBottom() or 0
	local zoneL, zoneR, zoneTop = ClassZone()
	zoneTop = zoneTop + 12
	ZONE = { l = zoneL, r = zoneR, top = zoneTop }
	local size = (CharCreateRaceButton1 and CharCreateRaceButton1:GetHeight() or 52) * SCALE
	local step = size + GAP
	local room = math.max(300, H - 120)
	local fit = math.max(1, math.floor((room + 12) / step))
	local sides = { Alliance = { stock = {}, added = {} }, Horde = { stock = {}, added = {} } }
	for i = 1, count do
		local _, faction = GetFactionForRace(i)
		if faction ~= "Horde" then
			faction = "Alliance"
		end
		table.insert(listed[i] and sides[faction].added or sides[faction].stock, i)
	end
	-- What a column of added races may hold: the first row that would stand over the class choice (or below the
	-- foot of the screen) ends the column. Columns end before the middle of the screen.
	local base = 0
	local caps, last = {}, -1
	for column = 0, 15 do
		local x = 32 + column * PITCH
		if x + size > W / 2 - 140 then                 -- the middle stays free for the character
			break
		end
		local rows = 0
		for row = 0, fit - 1 do
			local bottom = frameTop - (75 + row * step + size)
			if bottom < frameBottom + 150 then             -- above Back and Help Me Choose
				break
			end
			if bottom < zoneTop and x + size > zoneL - 12 and x < zoneR + 12 then
				break
			end
			rows = rows + 1
		end
		if rows == 0 then
			break
		end
		caps[column] = rows
		last = column
	end
	local capacity = 0
	for column = base, last do
		capacity = capacity + (caps[column] or 0)
	end
	local columns = last - base + 1
	-- the offsets of a scaled button count in its own scale: the place on the frame divided by it
	local function place(button, faction, column, row)
		button:SetScale(SCALE)
		button:ClearAllPoints()
		if faction == "Alliance" then
			button:SetPoint("TOPLEFT", frame, "TOPLEFT", (32 + column * PITCH) / SCALE, (-75 - row * step) / SCALE)
		else
			button:SetPoint("TOPRIGHT", frame, "TOPRIGHT", (-32 - column * PITCH) / SCALE, (-75 - row * step) / SCALE)
		end
		button:SetFaction(faction)
		local text = button.Text or _G[button:GetName() .. "Text"]
		if text then
			text:Hide()
		end
		if button.Shadow then
			button.Shadow:Hide()
		end
		button:Show()
	end
	for faction, side in pairs(sides) do
		-- the stock races first, then the added ones, column by column from the edge: none reaches the buttons below
		local added, state = {}, SCROLL[faction]
		for _, i in ipairs(side.stock) do
			added[#added + 1] = i
		end
		for _, i in ipairs(side.added) do
			added[#added + 1] = i
		end
		local rows, total, offset = capacity, 0, 0
		if #added > capacity and columns > 0 then
			rows = caps[base]
			for column = base, last do
				rows = math.min(rows, caps[column])
			end
			total = math.ceil(#added / columns)
			offset = math.max(0, math.min(total - rows, math.floor((state.offset or 0) + 0.5)))
		end
		state.offset, state.rows, state.total = offset, rows, total
		state.scrolling = total > rows
		for k, i in ipairs(added) do
			local button = _G["CharCreateRaceButton" .. i]
			local column, row
			if state.scrolling then
				column = base + math.floor((k - 1) / total)
				row = (k - 1) % total - offset
				if row < 0 or row >= rows then
					column = nil
				end
			else
				column, row = base, k - 1
				while column <= last and row >= (caps[column] or 0) do
					row = row - (caps[column] or 0)
					column = column + 1
				end
				if column > last then
					column = nil
				end
			end
			if column then
				place(button, faction, column, row)
			else
				button:Hide()
			end
		end
		local bar = BARS[faction]
		if state.scrolling then
			bar = bar or CreateBar(faction)
			BARS[faction] = bar
			local height = (rows - 1) * step + size
			local thumb = math.max(24, height * rows / total)
			bar.track:ClearAllPoints()
			if faction == "Alliance" then
				bar.track:SetPoint("TOPLEFT", frame, "TOPLEFT", 32 + last * PITCH + size + 6, -75)
			else
				bar.track:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -(32 + last * PITCH + size + 6), -75)
			end
			bar.track:SetHeight(height)
			bar.thumb:ClearAllPoints()
			bar.thumb:SetPoint("TOP", 0, -(height - thumb) * offset / (total - rows))
			bar.thumb:SetHeight(thumb)
			bar.track:Show()
		elseif bar then
			bar.track:Hide()
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
	LAST_COUNT = math.min(CharacterCreate.numRaces or 0, BUTTONS)
	Layout(LAST_COUNT)
	if Watch then
		Watch.wait = 0.5
		Watch:Show()
	end
end

-- The class buttons of the client arrive after the races, and are built anew whenever the classes shown change:
-- lay the races out again after them.
if CharacterCreate_RefreshClassButtons then
	local refresh = CharacterCreate_RefreshClassButtons
	function CharacterCreate_RefreshClassButtons(self, ...)
		local result = refresh(self, ...)
		Layout(LAST_COUNT)
		return result
	end
end

-- The class choice moves while the character creation is open (the buttons arrive late, a race shows other
-- classes): watch its place and lay the races out again when it moved.
local watch = CreateFrame("Frame", nil, CharCreateRaceButtonsFrame)
watch:Hide()
watch.wait = 0
watch:SetScript("OnUpdate", function(self, elapsed)
	if not CharacterCreate:IsShown() then
		self:Hide()
		return
	end
	self.wait = self.wait - (elapsed or 0)
	if self.wait > 0 then
		return
	end
	self.wait = 0.5
	local l, r, top = ClassZone()
	local zone = ZONE
	if not zone or math.abs(zone.l - l) + math.abs(zone.r - r) + math.abs(zone.top - (top + 12)) > 6 then
		Layout(LAST_COUNT)
	end
end)
Watch = watch

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

-- The camera of the character creation is set for the base race's scene: a race much taller than its base race (the
-- Highmountain Tauren next to the Tauren) showed only in part. AFK Realm writes how tall the models stand
-- ("{height:RACE:SEX}", in yards, measured from their model files); the camera follows how much taller the race is.
local BASE_HEIGHT = {
	Human = { "{height:1:0}", "{height:1:1}" }, Orc = { "{height:2:0}", "{height:2:1}" }, Dwarf = { "{height:3:0}", "{height:3:1}" },
	NightElf = { "{height:4:0}", "{height:4:1}" }, Undead = { "{height:5:0}", "{height:5:1}" }, Tauren = { "{height:6:0}", "{height:6:1}" },
	Gnome = { "{height:7:0}", "{height:7:1}" }, Troll = { "{height:8:0}", "{height:8:1}" }, BloodElf = { "{height:10:0}", "{height:10:1}" },
	Draenei = { "{height:11:0}", "{height:11:1}" },
}

-- How much taller (or smaller) a race stands than its base race; 1 when it is not known.
local function SizeOf(race, sex)
	if not race then
		return 1, 0
	end
	local i = sex == SEX_FEMALE and 2 or 1
	local own = race.height and tonumber(race.height[i])
	local base = BASE_HEIGHT[race.base] and tonumber(BASE_HEIGHT[race.base][i])
	if not own or not base or own <= 0 or base <= 0 then
		return 1, 0
	end
	-- only a race taller than its base race moves the camera: for a smaller one the camera of the base race stays (it
	-- went down by the yards the race is smaller and ended in the ground, a Vulpera leaning on the Tauren for example)
	if own <= base then
		return 1, 0
	end
	return math.min(3, own / base), own - base
end

-- How the camera follows the size (set by eye on the Highmountain Tauren, men and women): per scene, back by a share of
-- how much taller the race is (0.3: a third of it) and up by a share of how many yards taller it is; the zoom onto the
-- face goes a share less far than for the base race (-0.3), so that a taller head stays in view.
local camera = { size = 1, taller = 0, zoom = -0.3,
	scenes = { race = { back = 0.3, up = 0.4 }, custom = { back = 0.8, up = 0.4 } } }

local function Scene()
	local steps = Enum and Enum.CharCreateSteps
	if CharacterCreateFrame and steps and steps.Customization and CharacterCreateFrame.state == steps.Customization then
		return "custom"
	end
	return "race"
end
local cc = C_CharacterCreate

local function Sized(x, y, z, size)
	local scene = camera.scenes[Scene()]
	local back = 1 + (size - 1) * scene.back
	return x * back, y * back, z + camera.taller * scene.up
end

-- The client places its camera a frame after it is told to (a reset, a race): its position is read two frames later,
-- once, and kept. The camera is always set from that position - never from one read after setting it ourselves, or
-- every setting would grow on the last one.
local function Creating()
	return CharacterCreate and CharacterCreate:IsShown()
end

local function Apply()
	if not Creating() then
		camera.size, camera.taller = 1, 0
		return
	end
	local race = listed[GetSelectedRace and GetSelectedRace() or 0]
	local size, taller = SizeOf(race, GetSelectedSex and GetSelectedSex())
	camera.size, camera.taller = size, (taller or 0)
	local f = camera.from
	if cc and cc.SetCameraPosition and f and f[1] and not IS_ZOOMED and math.abs(camera.size - 1) > 0.02 then
		cc.SetCameraPosition(Sized(f[1], f[2], f[3], camera.size))
		camera.moved = true
	end
end

local later = CreateFrame("Frame")
later:Hide()
later:SetScript("OnUpdate", function(self)
	self.frames = self.frames - 1
	if self.frames > 0 then
		return
	end
	self:Hide()
	if cc and cc.GetCameraPosition then
		camera.from = { cc.GetCameraPosition() }
		camera.moved = false
	end
	Apply()
end)
local function ReadLater()
	if not Creating() then
		camera.size, camera.taller, camera.moved = 1, 0, false
		return
	end
	later.frames = 2
	later:Show()
end

if cc and cc.ResetCameraPosition and cc.GetCameraPosition and cc.SetCameraPosition then
	local reset = cc.ResetCameraPosition
	cc.ResetCameraPosition = function(...)
		local result = reset(...)
		ReadLater()
		return result
	end
	camera.reset = reset
	-- the zoom onto the face of a taller race goes less far (camera.zoom), so that its head stays in view
	if cc.ZoomIn and cc.ZoomOut then
		local zoomIn, zoomOut = cc.ZoomIn, cc.ZoomOut
		local function factor() return Creating() and math.max(0.2, 1 + (camera.size - 1) * camera.zoom) or 1 end
		cc.ZoomIn = function(step, ...) return zoomIn((step or 0) * factor(), ...) end
		cc.ZoomOut = function(step, ...) return zoomOut((step or 0) * factor(), ...) end
	end
end

-- a race or sex chosen: the camera of the client first, then sized - for races of another size, or when the camera
-- was moved for the race before
local function Resize()
	if not camera.reset or IS_ZOOMED or not Creating() then
		return
	end
	local race = listed[GetSelectedRace and GetSelectedRace() or 0]
	local size = SizeOf(race, GetSelectedSex and GetSelectedSex())
	if math.abs(size - 1) > 0.02 or camera.moved then
		camera.reset()
		ReadLater()
	end
end
if SetCharacterRace then
	local setRace = SetCharacterRace
	function SetCharacterRace(...)
		local result = setRace(...)
		Resize()
		return result
	end
end
if SetCharacterGender then
	local setGender = SetCharacterGender
	function SetCharacterGender(...)
		local result = setGender(...)
		Resize()
		return result
	end
end

-- The character selection shows a character in the scene of its race (a new race: of its base race), with that
-- scene's camera: a much taller race stood half out of the picture there too. The camera follows its size as above.
-- set by eye on the Highmountain Tauren: a little more back than in the creation, hardly up
local chosen = { scene = { back = 0.6, up = 0.1 }, moved = false }

local function Selected()
	local index = CharacterSelect and CharacterSelect.selectedIndex
	if not index or index == 0 or not GetCharacterInfo then
		return nil
	end
	local id = GetCharIDFromIndex and GetCharIDFromIndex(index) or index
	local _, raceName, _, _, _, sex = GetCharacterInfo(id)
	return raceName and Find(raceName, raceName), sex == SEX_FEMALE and SEX_FEMALE or SEX_MALE
end

local function SelectApply()
	local race, sex = Selected()
	local size, taller = SizeOf(race, sex)
	chosen.size, chosen.taller = size, taller or 0
	local f = chosen.from
	if cc and cc.SetCameraPosition and f and f[1] then
		if math.abs(size - 1) > 0.02 then
			local back = 1 + (size - 1) * chosen.scene.back
			pcall(cc.SetCameraPosition, f[1] * back, f[2] * back, f[3] + chosen.taller * chosen.scene.up)
			chosen.moved = true
		elseif chosen.moved then
			pcall(cc.SetCameraPosition, f[1], f[2], f[3])
			chosen.moved = false
		end
	end
end

local selectLater = CreateFrame("Frame")
selectLater:Hide()
selectLater:SetScript("OnUpdate", function(self)
	self.frames = self.frames - 1
	if self.frames > 0 then
		return
	end
	self:Hide()
	local ok, x, y, z = pcall(function() return cc.GetCameraPosition() end)
	chosen.from = ok and x and { x, y, z } or nil
	SelectApply()
end)

-- another character chosen: the camera as the client sets it for that scene, read a few frames later, then sized
local function SelectChanged()
	if Creating() or not CharacterSelect or not CharacterSelect:IsShown() then
		return
	end
	if chosen.moved and camera.reset then
		pcall(camera.reset)
	end
	chosen.moved = false
	selectLater.frames = 5
	selectLater:Show()
end
if CharacterSelect_SelectCharacter then
	local selectCharacter = CharacterSelect_SelectCharacter
	function CharacterSelect_SelectCharacter(...)
		local result = selectCharacter(...)
		SelectChanged()
		return result
	end
end


-- The races of EsteriaAppearance.dll (CoA Custom's game, CustomRaces.HighRaces): their many choices of looks. The DLL
-- knows them by CoA Custom's race numbers and answers "EA_" calls of CycleCharCustomization; the stock buttons of the
-- customization cycle only the five stock choices, which mix up the DLL's packed values (a choice that does nothing,
-- or one that changes another). For these races the buttons go to the DLL, and there are as many as the race has
-- choices. Taken over from CoA Custom (scripts/esteria/esteria_options.lua, from Esteria's CharacterCreate.lua).
do
	-- CoA Custom's race number -> Esteria's, as the DLL's choices are told apart. The file name is the safer key:
	-- an imported race keeps its CoA file (VulperaEsteria) but may get another number in this client.
	local ESTERIA = { [66] = 46, [67] = 47, [68] = 48, [69] = 49, [70] = 51, [71] = 50, [72] = 53, [77] = 52, [74] = 20 }
	local ESTERIA_FILE = { NAGA = 54, HIGHMOUNTAINTAUREN = 46, MECHAGNOME = 47, EARTHEN = 48, EARTHENHORDE = 49,
		HARANIR = 50, HARANIRHORDE = 51, VULPERAESTERIA = 20, SETHRAKALLIANCE = 62, GNOLLHORDE = 63, SABERONHORDE = 64 }
	local NAMED = { [20] = true, [46] = true, [48] = true, [49] = true, [50] = true, [51] = true }
	local RANDOM = NAMED
	local LABELS = { "Skin Color", "Face", "Hair Style", "Hair Color", "Eye Color", "Eyebrow Style",
		"Feathers", "Feather Color", "Facial Hair", "Ears" }
	local MECH_LABELS = { "Skin Color", "Face", "Hair Style", "Hair Color", "Facial Hair", "Arm Upgrade", "Leg Upgrade",
		"Modification", "Eye Color", "Paint", "Eyesight", "Eye Style" }
	local MAX = 29
	local saved, wasActive, lastRace

	local function EsteriaRace()
		local race = listed[GetSelectedRace and GetSelectedRace() or 0]
		if not race then
			return nil
		end
		local byFile = ESTERIA_FILE[Plain(race.file)]
		if byFile then
			return byFile
		end
		return ESTERIA[tonumber(race.id) or 0]
	end

	local function Get(i)
		local ok, value, count, label = pcall(CycleCharCustomization, "EA_GET", i)
		if ok then
			return value, count, label
		end
	end

	-- the customization buttons of the client: Conquest of Azeroth's (CharCreateCustomizationButton1-5, a label in .text),
	-- or the stock ones of older screens (CharacterCustomizationButtonFrame1-5)
	local NAME, TEMPLATE, KIND = "CharCreateCustomizationButton", "CharCreateCustomizationButtonTemplate", "CheckButton"
	if not _G[NAME .. "1"] then
		NAME, TEMPLATE, KIND = "CharacterCustomizationButtonFrame", "CharacterCustomizationFrameTemplate", "Frame"
	end
	local ROWS, STEP, COLUMN = 10, 46, 250

	local function Label(frame)
		return frame and (frame.text or _G[frame:GetName() .. "Text"])
	end

	-- where the first button sits in its frame, taken once while the stock layout stands
	local function Origin()
		local first = _G[NAME .. "1"]
		if not first or not saved or saved.x or wasActive then
			return
		end
		local parent = first:GetParent()
		if first:GetLeft() and parent:GetLeft() then
			saved.x = first:GetLeft() - parent:GetLeft()
			saved.y = first:GetTop() - parent:GetTop()
		end
	end

	-- the game's own names on the five buttons, as its character creation sets them (CHAR_CUSTOMIZATION<n>_DESC, then
	-- the hair and beard names of the race)
	local function StockLabels()
		for i = 1, 5 do
			local text = Label(_G[NAME .. i])
			local desc = _G["CHAR_CUSTOMIZATION" .. i .. "_DESC"]
			if text and desc then
				text:SetText(desc)
			end
		end
		if CharCreateCustomizationFrame_OnShow then pcall(CharCreateCustomizationFrame_OnShow) end
		if CharacterCreate_UpdateHairCustomization then pcall(CharacterCreate_UpdateHairCustomization) end
		if CharacterCreate_UpdateFacialHairCustomization then pcall(CharacterCreate_UpdateFacialHairCustomization) end
	end

	local function Refresh()
		local first = _G[NAME .. "1"]
		if not first then
			return
		end
		saved = saved or {}
		if not saved[1] and not wasActive then
			for i = 1, 5 do
				local frame = _G[NAME .. i]
				if frame and frame:GetNumPoints() > 0 then
					saved[i] = { frame:GetPoint(1) }
				end
			end
		end
		Origin()
		local parent = first:GetParent()
		for i = 6, MAX do
			if not _G[NAME .. i] then
				local frame = CreateFrame(KIND, NAME .. i, parent, TEMPLATE)
				frame:SetID(i)
				frame:Hide()
			end
		end
		local race = EsteriaRace()
		if race and race ~= lastRace then
			-- their first choices are bald: start from a random look (as Esteria's screen does), else put the hair on by
			-- stepping the hair style once and back
			if RANDOM[race] then
				pcall(CycleCharCustomization, "EA_RANDOM", 1)
			else
				pcall(CycleCharCustomization, "EA_CYCLE", 3, 1)
				pcall(CycleCharCustomization, "EA_CYCLE", 3, -1)
			end
		end
		lastRace = race
		if race then
			wasActive = true
			local row = 0
			local lastOfFirst
			for i = 1, MAX do
				local frame = _G[NAME .. i]
				local value, count, label = Get(i)
				if count and count > 1 then
					local column = math.floor(row / ROWS)
					frame:ClearAllPoints()
					frame:SetPoint("TOPLEFT", parent, "TOPLEFT", (saved.x or 0) - column * COLUMN, (saved.y or 0) - (row % ROWS) * STEP)
					local name = NAMED[race] and (label or "") or race == 47 and MECH_LABELS[i] or LABELS[i] or ""
					local text = Label(frame)
					if text then
						if (race == 48 or race == 49) and label == "Belt" then
							text:SetText(name .. (value == 0 and "  None" or "  Gem"))
						else
							text:SetText(name .. "  " .. (value + 1) .. "/" .. count)
						end
					end
					frame:Show()
					if column == 0 then
						lastOfFirst = frame
					end
					row = row + 1
				elseif frame then
					frame:Hide()
				end
			end
			-- the dice and "Show equipment" hang below the fifth button: they go below the last of the first column
			local arrow = lastOfFirst and _G[lastOfFirst:GetName() .. "RightButton"]
			if arrow and CharCreateRandomizeButton then
				CharCreateRandomizeButton:ClearAllPoints()
				CharCreateRandomizeButton:SetPoint("TOP", arrow, "BOTTOM", -8, -12)
			end
		elseif wasActive then
			wasActive = nil
			if CharCreateRandomizeButton and _G[NAME .. "5RightButton"] then
				CharCreateRandomizeButton:ClearAllPoints()
				CharCreateRandomizeButton:SetPoint("TOP", _G[NAME .. "5RightButton"], "BOTTOM", -8, 0)
			end
			for i = 6, MAX do
				_G[NAME .. i]:Hide()
			end
			for i = 1, 5 do
				local frame = _G[NAME .. i]
				if frame and saved[i] then
					frame:ClearAllPoints()
					frame:SetPoint(unpack(saved[i]))
				end
			end
			StockLabels()
		else
			-- a label still showing the DLL's count ("Face  4/4") belongs to a race of the DLL seen before: the game's
			-- own buttons show the names only, so they are put back
			for i = 1, 5 do
				local text = Label(_G[NAME .. i])
				local shown = text and text:GetText()
				if shown and (shown:find("  %d+/%d+$") or shown:find("  None$") or shown:find("  Gem$")) then
					StockLabels()
					break
				end
			end
		end
	end

	local left, right, randomize = CharacterCustomization_Left, CharacterCustomization_Right, CharacterCreate_Randomize
	function CharacterCustomization_Left(id)
		if EsteriaRace() then
			PlaySound("gsCharacterCreationLook")
			pcall(CycleCharCustomization, "EA_CYCLE", id, -1)
			Refresh()
		elseif left then
			left(id)
		end
	end
	function CharacterCustomization_Right(id)
		if EsteriaRace() then
			PlaySound("gsCharacterCreationLook")
			pcall(CycleCharCustomization, "EA_CYCLE", id, 1)
			Refresh()
		elseif right then
			right(id)
		end
	end
	function CharacterCreate_Randomize(...)
		local race = EsteriaRace()
		if race and RANDOM[race] then
			PlaySound("gsCharacterCreationLook")
			pcall(CycleCharCustomization, "EA_RANDOM", 1)
			Refresh()
		elseif randomize then
			return randomize(...)
		end
	end

	local ticker = CreateFrame("Frame")
	ticker:SetScript("OnUpdate", function(self, elapsed)
		self.elapsed = (self.elapsed or 0) + elapsed
		if self.elapsed > 0.15 then
			self.elapsed = 0
			if CharacterCreate and CharacterCreate:IsShown() then
				Refresh()
			end
		end
	end)
end
