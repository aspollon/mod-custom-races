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
	local size = CharCreateRaceButton1 and CharCreateRaceButton1:GetHeight() or 52
	local step = size + 12
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
	local base = (#sides.Alliance.stock > 0 or #sides.Horde.stock > 0) and 1 or 0
	local caps, last = {}, base - 1
	for column = base, base + 7 do
		local x = 32 + column * 170
		if x + size > W / 2 - 20 then
			break
		end
		local rows = 0
		for row = 0, fit - 1 do
			local bottom = frameTop - (75 + row * step + size)
			if bottom < frameBottom + 84 then
				break
			end
			if bottom < zoneTop and x + size + 150 > zoneL - 12 and x < zoneR + 12 then
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
	local function place(button, faction, column, row, gap)
		button:ClearAllPoints()
		if faction == "Alliance" then
			button:SetPoint("TOPLEFT", frame, "TOPLEFT", 32 + column * 170, -75 - row * (size + gap))
		else
			button:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -32 - column * 170, -75 - row * (size + gap))
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
		local added, state = side.added, SCROLL[faction]
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
				place(button, faction, column, row, 12)
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
				bar.track:SetPoint("TOPLEFT", frame, "TOPLEFT", 89 + last * 170, -75)
			else
				bar.track:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -(89 + last * 170), -75)
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
