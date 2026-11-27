extends Node
## Global database and match configuration for WE3 Redux.

const DB_PATH := "res://assets/database/players_database.json"
const TACTICS_PATH := "res://assets/database/team_tactics.json"
const KIT_DIR := "res://assets/textures/team_kits/"
const EXTRA_KIT_DIR := "res://assets/textures/team_kits_extra/"
const ANIM_INDEX_PATH := "res://assets/3d/animations/INDEX.json"

var teams: Array[Dictionary] = []
var anim_index: Array[Dictionary] = []
var kit_files_by_id: Dictionary = {}
var extra_kit_files_by_id: Dictionary = {}
var tactics_by_id: Dictionary = {}
var _clut_primary_cache: Dictionary = {}
var _clut_secondary_cache: Dictionary = {}

const STADIUM_NAMES := ["GRDM_C", "GRDM_D", "GRDM_E", "GRDM_F", "GRDM_FN", "GRDM_G", "GRDM_H", "GRDM_I", "GRDM_J"]
const FORMATIONS := ["4-4-2", "4-3-3", "3-5-2", "5-3-2", "4-5-1", "3-4-3"]

# Current match configuration (temporarily Brazil vs Argentina for 11v11)
var home_team_id: int = 30  # Brazil
var away_team_id: int = 33  # Argentina
var game_mode: String = "exhibition"
var extra_time_enabled: bool = false
var half_duration_seconds: float = 120.0
var ai_difficulty: int = 1  # 0=Easy, 1=Normal, 2=Hard
var is_ai_vs_ai: bool = false
var is_two_player: bool = false
var selected_stadium: String = "GRDM_C"
var stadium_is_night: bool = false
var stadium_is_rain: bool = false
var home_formation: String = "4-4-2"
var away_formation: String = "4-4-2"
var home_formation_manual: bool = false
var home_tactic_level: int = 1      # func_800691A8 Scratchpad+0x2FF: 0=Def, 1=Normal, 2=Attack
var away_tactic_level: int = 1
var home_offside_trap: bool = false   # func_8002D9F4 Scratchpad+0x307
var away_offside_trap: bool = false
var human_side: int = 0          # 0 = Home, 1 = Away (ignored when is_ai_vs_ai)
var return_scene: String = "res://scenes/main_menu.tscn"   # where F8 / Main Menu goes (Match Preview sets its own)
var preview_speed: float = 1.0
var away_formation_manual: bool = false

# Cup / World Cup knockout progression (0=Quarter-Final, 1=Semi-Final, 2=Final)
var cup_active: bool = false
var cup_round: int = 0
var cup_opponents: Array[int] = []

func start_cup_tournament(player_tid: int) -> void:
	cup_active = true
	cup_round = 0
	cup_opponents.clear()
	extra_time_enabled = true
	var pool: Array[int] = []
	for tid in range(mini(40, teams.size())):
		if tid != player_tid:
			pool.append(tid)
	pool.shuffle()
	for i in range(mini(3, pool.size())):
		cup_opponents.append(pool[i])
	if not cup_opponents.is_empty():
		away_team_id = cup_opponents[0]

func advance_cup_round() -> bool:
	cup_round += 1
	if cup_round < cup_opponents.size():
		away_team_id = cup_opponents[cup_round]
		return true
	cup_active = false
	return false

func get_cup_round_name() -> String:
	var names: Array[String] = ["QUARTER-FINAL", "SEMI-FINAL", "FINAL"]
	return names[clampi(cup_round, 0, 2)]

func _ready() -> void:
	_ensure_p2_input_actions()
	_load_teams()
	_load_tactics()
	_load_kit_map()
	_load_extra_kit_map()
	_load_anim_index()
	_validate_default_teams()

func _ensure_p2_input_actions() -> void:
	var key_map := {
		"p2_move_left": [KEY_KP_4],
		"p2_move_right": [KEY_KP_6],
		"p2_move_up": [KEY_KP_8],
		"p2_move_down": [KEY_KP_2, KEY_KP_5],
		"p2_pass_short": [KEY_KP_1, KEY_END],
		"p2_pass_through": [KEY_KP_3, KEY_PAGEDOWN],
		"p2_pass_loft": [KEY_KP_7, KEY_HOME],
		"p2_shoot": [KEY_KP_0, KEY_INSERT],
		"p2_sprint": [KEY_KP_ENTER, KEY_KP_ADD],
	}
	for act in key_map:
		if not InputMap.has_action(act):
			InputMap.add_action(act)
		for kc in key_map[act]:
			var ev := InputEventKey.new()
			ev.keycode = kc
			if not InputMap.action_has_event(act, ev):
				InputMap.action_add_event(act, ev)
	var joy_axes := {
		"p2_move_left": [JOY_AXIS_LEFT_X, -1.0],
		"p2_move_right": [JOY_AXIS_LEFT_X, 1.0],
		"p2_move_up": [JOY_AXIS_LEFT_Y, -1.0],
		"p2_move_down": [JOY_AXIS_LEFT_Y, 1.0],
	}
	for act in joy_axes:
		var ax := InputEventJoypadMotion.new()
		ax.device = 1
		ax.axis = joy_axes[act][0]
		ax.axis_value = joy_axes[act][1]
		InputMap.action_add_event(act, ax)
	var joy_btns := {
		"p2_pass_short": JOY_BUTTON_A,
		"p2_pass_through": JOY_BUTTON_Y,
		"p2_pass_loft": JOY_BUTTON_B,
		"p2_shoot": JOY_BUTTON_X,
		"p2_sprint": JOY_BUTTON_RIGHT_SHOULDER,
	}
	for act in joy_btns:
		var jb := InputEventJoypadButton.new()
		jb.device = 1
		jb.button_index = joy_btns[act]
		InputMap.action_add_event(act, jb)

func _input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		var ke := event as InputEventKey
		if ke.keycode == KEY_F5:
			reload_current_game_scene()
		elif ke.keycode == KEY_F8:
			return_to_main_menu()

func reload_current_game_scene() -> void:
	Engine.time_scale = preview_speed
	if get_tree() != null:
		get_tree().reload_current_scene()

func return_to_main_menu() -> void:
	Engine.time_scale = 1.0
	preview_speed = 1.0
	if get_tree() != null:
		get_tree().change_scene_to_file(return_scene)

func _load_teams() -> void:
	if not FileAccess.file_exists(DB_PATH):
		push_error("GameData: Missing players_database.json at %s" % DB_PATH)
		return
	var text := FileAccess.get_file_as_string(DB_PATH)
	var parsed: Variant = JSON.parse_string(text)
	if typeof(parsed) == TYPE_DICTIONARY and parsed.has("teams"):
		for t in parsed["teams"]:
			if typeof(t) == TYPE_DICTIONARY:
				teams.append(t)

func _load_kit_map() -> void:
	var dir := DirAccess.open(KIT_DIR)
	if dir == null:
		return
	dir.list_dir_begin()
	var fname := dir.get_next()
	while fname != "":
		if not dir.current_is_dir() and fname.ends_with(".png") and not fname.ends_with(".import"):
			# Format: tex_01_Germany.png -> 1-based index maps to team_id = index - 1
			var parts := fname.split("_")
			if parts.size() >= 2 and parts[1].is_valid_int():
				var tid := parts[1].to_int() - 1
				kit_files_by_id[tid] = KIT_DIR + fname
		fname = dir.get_next()
	dir.list_dir_end()

func _load_anim_index() -> void:
	if not FileAccess.file_exists(ANIM_INDEX_PATH):
		return
	var text := FileAccess.get_file_as_string(ANIM_INDEX_PATH)
	var parsed: Variant = JSON.parse_string(text)
	if typeof(parsed) == TYPE_DICTIONARY and parsed.has("animations"):
		for a in parsed["animations"]:
			if typeof(a) == TYPE_DICTIONARY:
				anim_index.append(a)

func _validate_default_teams() -> void:
	if teams.is_empty():
		return
	if home_team_id < 0 or home_team_id >= teams.size():
		home_team_id = 0
	if away_team_id < 0 or away_team_id >= teams.size() or away_team_id == home_team_id:
		away_team_id = (home_team_id + 1) % teams.size()

func get_team(team_id: int) -> Dictionary:
	for t in teams:
		if int(t.get("team_id", -1)) == team_id:
			return t
	if not teams.is_empty():
		return teams[0]
	return {"team_id": 0, "team_name": "Team A", "players": []}

func get_team_abbr(team_id: int) -> String:
	var t := get_team(team_id)
	var name: String = str(t.get("team_name", "TEA"))
	return name.substr(0, 3).to_upper()

func _load_tactics() -> void:
	if not FileAccess.file_exists(TACTICS_PATH):
		return
	var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(TACTICS_PATH))
	if typeof(parsed) == TYPE_DICTIONARY and parsed.has("teams"):
		for t in parsed["teams"]:
			if typeof(t) == TYPE_DICTIONARY and t.has("team_id"):
				tactics_by_id[int(t["team_id"])] = t

func _load_extra_kit_map() -> void:
	var dir := DirAccess.open(EXTRA_KIT_DIR)
	if dir == null:
		return
	dir.list_dir_begin()
	var fname := dir.get_next()
	while fname != "":
		if not dir.current_is_dir() and fname.ends_with(".png"):
			var parts := fname.split("_")
			if parts.size() >= 2 and parts[1].is_valid_int():
				var tid := parts[1].to_int() - 1
				extra_kit_files_by_id[tid] = EXTRA_KIT_DIR + fname
		fname = dir.get_next()
	dir.list_dir_end()

func get_team_extra_kit_texture(team_id: int) -> Texture2D:
	if extra_kit_files_by_id.has(team_id):
		var path: String = extra_kit_files_by_id[team_id]
		if ResourceLoader.exists(path):
			return load(path) as Texture2D
	return null

## Authentic WE3 starting XI = first 11 roster slots. Their position groups (GK/DF/MF/FW)
## define each national team's real default formation (e.g. 3-5-2, 5-3-2, 4-5-1 ...).
func get_team_default_formation(team_id: int) -> String:
	var counts := {"DF": 0, "MF": 0, "FW": 0}
	var players: Array = get_team(team_id).get("players", [])
	for i in range(mini(11, players.size())):
		var g := str(players[i].get("pos_group", ""))
		if counts.has(g):
			counts[g] += 1
	var key := "%d-%d-%d" % [counts["DF"], counts["MF"], counts["FW"]]
	if FORMATIONS.has(key):
		return key
	# Unknown shape -> nearest known formation by defender count
	for f in FORMATIONS:
		if f.begins_with(str(counts["DF"])):
			return f
	return "4-4-2"

## Formation actually used for a side: manual override from Team Select, else the team's authentic one.
func resolve_formation(side: int) -> String:
	if side == 0:
		return home_formation if home_formation_manual else get_team_default_formation(home_team_id)
	return away_formation if away_formation_manual else get_team_default_formation(away_team_id)

## Authentic per-player record (stats, shirt number, position, height) for roster index 0..21.
func get_player_record(team_id: int, roster_idx: int) -> Dictionary:
	var players: Array = get_team(team_id).get("players", [])
	if roster_idx >= 0 and roster_idx < players.size() and typeof(players[roster_idx]) == TYPE_DICTIONARY:
		return players[roster_idx]
	return {}

func get_team_kit_texture(team_id: int) -> Texture2D:
	if kit_files_by_id.has(team_id):
		var path: String = kit_files_by_id[team_id]
		if ResourceLoader.exists(path):
			return load(path) as Texture2D
	return null

func get_team_primary_color(team_id: int) -> Color:
	if _clut_primary_cache.has(team_id):
		return _clut_primary_cache[team_id]
	# Sample authentic PS1 CLUT-decoded jersey chest patch (x=10..26, y=36..52)
	var tex := get_team_kit_texture(team_id)
	if tex != null:
		var img := tex.get_image()
		if img != null and img.get_width() >= 32 and img.get_height() >= 56:
			var sum_c := Color(0, 0, 0, 0)
			var cnt := 0
			for y in range(36, 52, 2):
				for x in range(10, 26, 2):
					sum_c += img.get_pixel(x, y)
					cnt += 1
			var col := Color(sum_c.r / cnt, sum_c.g / cnt, sum_c.b / cnt, 1.0)
			_clut_primary_cache[team_id] = col
			return col
	const PALETTE := [
		Color(0.94, 0.94, 0.97), # 0 Germany (White)
		Color(0.82, 0.12, 0.14), # 1 Spain (Red)
		Color(0.96, 0.82, 0.12), # 2 Romania (Yellow)
		Color(0.95, 0.95, 0.98), # 3 England (White)
		Color(0.10, 0.24, 0.72), # 4 France (Blue)
		Color(0.84, 0.12, 0.18), # 5 Denmark (Red)
		Color(0.12, 0.36, 0.82), # 6 Italy (Azzurri)
		Color(0.98, 0.44, 0.08), # 7 Netherlands (Orange)
		Color(0.82, 0.14, 0.16), # 8 Norway (Red)
		Color(0.96, 0.82, 0.10), # 9 Sweden (Yellow)
	]
	if team_id >= 0 and team_id < PALETTE.size():
		return PALETTE[team_id]
	var hue := fmod(float(team_id) * 0.61803398875, 1.0)
	return Color.from_hsv(hue, 0.75, 0.90)

func get_team_secondary_color(team_id: int) -> Color:
	if _clut_secondary_cache.has(team_id):
		return _clut_secondary_cache[team_id]
	# Sample authentic PS1 CLUT-decoded shorts patch (x=24..40, y=76..88)
	var tex := get_team_kit_texture(team_id)
	if tex != null:
		var img := tex.get_image()
		if img != null and img.get_width() >= 48 and img.get_height() >= 96:
			var sum_c := Color(0, 0, 0, 0)
			var cnt := 0
			for y in range(76, 88, 2):
				for x in range(24, 40, 2):
					sum_c += img.get_pixel(x, y)
					cnt += 1
			var col := Color(sum_c.r / cnt, sum_c.g / cnt, sum_c.b / cnt, 1.0)
			_clut_secondary_cache[team_id] = col
			return col
	const SHORTS := [
		Color(0.12, 0.12, 0.15), # Germany (Black)
		Color(0.10, 0.18, 0.48), # Spain (Navy)
		Color(0.92, 0.76, 0.10), # Romania (Yellow)
		Color(0.10, 0.16, 0.38), # England (Navy)
		Color(0.95, 0.95, 0.98), # France (White)
		Color(0.95, 0.95, 0.98), # Denmark (White)
		Color(0.95, 0.95, 0.98), # Italy (White)
		Color(0.12, 0.12, 0.15), # Netherlands (Black)
		Color(0.95, 0.95, 0.98), # Norway (White)
		Color(0.10, 0.28, 0.72), # Sweden (Blue)
	]
	if team_id >= 0 and team_id < SHORTS.size():
		return SHORTS[team_id]
	return Color(0.14, 0.16, 0.22)
