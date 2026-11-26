extends Control
## WE3 Match Preview - quick match launcher (inspired by PES6_Match_Preview).
## Pick teams / stadium / formations / control / speed and jump straight into a match.
##
## Command line (everything after "--" is read as user args):
##   godot --path . res://test_tools/match_preview.tscn -- --home=Brazil --away=Japan --stadium=GRDM_D \
##         --form-home=3-5-2 --form-away=auto --half=180 --control=away --speed=2 --difficulty=2 --autostart
##   --control = home | away | cpu    --home/--away = team id or (part of) team name
## Last used settings are stored in user://match_preview.cfg.

const CFG_PATH := "user://match_preview.cfg"
const HALF_CHOICES := [60, 120, 180, 300, 600]
const SPEEDS := [0.5, 1.0, 2.0, 4.0]
const CONTROLS := ["Home (you) vs CPU", "Away (you) vs CPU", "CPU vs CPU (watch)"]

var _home_opt: OptionButton
var _away_opt: OptionButton
var _stad_opt: OptionButton
var _form_home_opt: OptionButton
var _form_away_opt: OptionButton
var _half_opt: OptionButton
var _diff_opt: OptionButton
var _ctrl_opt: OptionButton
var _speed_opt: OptionButton
var _home_info: Label
var _away_info: Label
var _home_kit: TextureRect
var _away_kit: TextureRect
var _status: Label

func _ready() -> void:
	_build_ui()
	_load_settings()
	_apply_cmdline()
	_refresh_info()
	SoundFX.stop_crowd_ambience()
	SoundFX.stop_music()
	if _has_flag("--autostart"):
		call_deferred("_start_match")

# --------------------------------------------------------------------- UI
func _build_ui() -> void:
	var bg := ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.04, 0.07, 0.15)
	add_child(bg)

	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(center)

	var panel := PanelContainer.new()
	var sb := StyleBoxFlat.new()
	sb.bg_color = Color(0.08, 0.13, 0.25, 0.97)
	sb.border_color = Color(0.95, 0.80, 0.18)
	sb.set_border_width_all(2)
	sb.set_corner_radius_all(8)
	sb.set_content_margin_all(18)
	panel.add_theme_stylebox_override("panel", sb)
	center.add_child(panel)

	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 10)
	panel.add_child(root)

	var title := Label.new()
	title.text = "WE3 MATCH PREVIEW  -  quick match tool"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 24)
	title.add_theme_color_override("font_color", Color(0.98, 0.86, 0.22))
	root.add_child(title)

	var teams_row := HBoxContainer.new()
	teams_row.add_theme_constant_override("separation", 28)
	teams_row.alignment = BoxContainer.ALIGNMENT_CENTER
	root.add_child(teams_row)

	var home_col := _team_column("HOME", true)
	var away_col := _team_column("AWAY", false)
	teams_row.add_child(home_col)
	var swap := Button.new()
	swap.text = "⇄\nSWAP"
	swap.custom_minimum_size = Vector2(64, 64)
	swap.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	swap.pressed.connect(_swap_teams)
	teams_row.add_child(swap)
	teams_row.add_child(away_col)

	var grid := GridContainer.new()
	grid.columns = 4
	grid.add_theme_constant_override("h_separation", 14)
	grid.add_theme_constant_override("v_separation", 8)
	root.add_child(grid)

	_stad_opt = _option_row(grid, "Stadium")
	for s_name in GameData.STADIUM_NAMES:
		_stad_opt.add_item(s_name)
	_half_opt = _option_row(grid, "Half length")
	for sec in HALF_CHOICES:
		_half_opt.add_item("%d:%02d min" % [sec / 60, sec % 60], sec)
	_form_home_opt = _option_row(grid, "Home formation")
	_form_away_opt = _option_row(grid, "Away formation")
	for fo in [_form_home_opt, _form_away_opt]:
		fo.add_item("Auto (real XI)")
		for f_name in GameData.FORMATIONS:
			fo.add_item(f_name)
		fo.item_selected.connect(func(_i): _refresh_info())
	_diff_opt = _option_row(grid, "CPU difficulty")
	for d_name in ["Easy", "Normal", "Hard"]:
		_diff_opt.add_item(d_name)
	_ctrl_opt = _option_row(grid, "Control")
	for c_name in CONTROLS:
		_ctrl_opt.add_item(c_name)
	_speed_opt = _option_row(grid, "Game speed")
	for sp in SPEEDS:
		_speed_opt.add_item("%sx" % str(sp))

	var btn_row := HBoxContainer.new()
	btn_row.alignment = BoxContainer.ALIGNMENT_CENTER
	btn_row.add_theme_constant_override("separation", 14)
	root.add_child(btn_row)
	var b_rand := Button.new()
	b_rand.text = "Random teams"
	b_rand.pressed.connect(_random_teams)
	btn_row.add_child(b_rand)
	var b_menu := Button.new()
	b_menu.text = "Main menu"
	b_menu.pressed.connect(func():
		GameData.return_scene = "res://scenes/main_menu.tscn"
		get_tree().change_scene_to_file("res://scenes/main_menu.tscn"))
	btn_row.add_child(b_menu)
	var b_go := Button.new()
	b_go.text = "▶  START MATCH  (Enter)"
	b_go.custom_minimum_size = Vector2(240, 44)
	b_go.add_theme_font_size_override("font_size", 18)
	b_go.pressed.connect(_start_match)
	btn_row.add_child(b_go)

	_status = Label.new()
	_status.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_status.add_theme_font_size_override("font_size", 11)
	_status.add_theme_color_override("font_color", Color(0.6, 0.72, 0.9))
	_status.text = "In match:  F8 = back to this preview   F5 = restart   Tab = 2x speed   F12 = event log   M = mute"
	root.add_child(_status)

func _team_column(label_text: String, is_home: bool) -> VBoxContainer:
	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 6)
	col.custom_minimum_size = Vector2(330, 0)
	var lbl := Label.new()
	lbl.text = label_text
	lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	lbl.add_theme_color_override("font_color", Color(0.98, 0.86, 0.22))
	col.add_child(lbl)
	var opt := OptionButton.new()
	opt.custom_minimum_size = Vector2(300, 34)
	for t in GameData.teams:
		opt.add_item(str(t.get("team_name", "?")), int(t.get("team_id", 0)))
	opt.item_selected.connect(func(_i): _refresh_info())
	col.add_child(opt)
	var kit := TextureRect.new()
	kit.custom_minimum_size = Vector2(96, 96)
	kit.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	kit.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	kit.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	col.add_child(kit)
	var info := Label.new()
	info.add_theme_font_size_override("font_size", 11)
	info.add_theme_color_override("font_color", Color(0.82, 0.88, 0.95))
	col.add_child(info)
	if is_home:
		_home_opt = opt
		_home_kit = kit
		_home_info = info
	else:
		_away_opt = opt
		_away_kit = kit
		_away_info = info
	return col

func _option_row(grid: GridContainer, text: String) -> OptionButton:
	var lbl := Label.new()
	lbl.text = text
	grid.add_child(lbl)
	var opt := OptionButton.new()
	opt.custom_minimum_size = Vector2(190, 30)
	grid.add_child(opt)
	return opt

# ------------------------------------------------------------- behaviour
func _input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		var k := (event as InputEventKey).keycode
		if k == KEY_ENTER or k == KEY_KP_ENTER:
			_start_match()
		elif k == KEY_R:
			_random_teams()
		elif k == KEY_S:
			_swap_teams()

func _team_id(opt: OptionButton) -> int:
	return opt.get_selected_id()

func _formation_choice(opt: OptionButton) -> String:
	return "" if opt.selected <= 0 else GameData.FORMATIONS[opt.selected - 1]

func _refresh_info() -> void:
	if _home_opt == null or _away_opt == null:
		return
	for side in range(2):
		var opt := _home_opt if side == 0 else _away_opt
		var info := _home_info if side == 0 else _away_info
		var kit := _home_kit if side == 0 else _away_kit
		var tid := _team_id(opt)
		kit.texture = GameData.get_team_kit_texture(tid)
		var choice := _formation_choice(_form_home_opt if side == 0 else _form_away_opt)
		var auto_form := GameData.get_team_default_formation(tid)
		var players: Array = GameData.get_team(tid).get("players", [])
		var sums := {"offense": 0, "defense": 0, "speed": 0, "technique": 0, "stamina": 0}
		var lines: PackedStringArray = []
		var n := mini(11, players.size())
		for i in range(n):
			var p: Dictionary = players[i]
			var st: Dictionary = p.get("stats", {})
			for k in sums.keys():
				sums[k] += int(st.get(k, 0))
			lines.append("#%-2d %-9s %s  OFF%d DEF%d SPD%d" % [int(p.get("shirt_number", i + 1)), str(p.get("name", "")), str(p.get("pos_group", "")), int(st.get("offense", 0)), int(st.get("defense", 0)), int(st.get("speed", 0))])
		var head := "Formation: %s%s  |  avg OFF %.1f DEF %.1f SPD %.1f TEC %.1f STA %.1f\n" % [choice if choice != "" else auto_form, " (auto)" if choice == "" else "", sums["offense"] / float(maxi(n, 1)), sums["defense"] / float(maxi(n, 1)), sums["speed"] / float(maxi(n, 1)), sums["technique"] / float(maxi(n, 1)), sums["stamina"] / float(maxi(n, 1))]
		info.text = head + "\n".join(lines)

func _swap_teams() -> void:
	var h := _home_opt.selected
	_home_opt.select(_away_opt.selected)
	_away_opt.select(h)
	var fh := _form_home_opt.selected
	_form_home_opt.select(_form_away_opt.selected)
	_form_away_opt.select(fh)
	_refresh_info()

func _random_teams() -> void:
	var n := _home_opt.item_count
	if n < 2:
		return
	var a := randi() % n
	var b := (a + 1 + randi() % (n - 1)) % n
	_home_opt.select(a)
	_away_opt.select(b)
	_refresh_info()

func _start_match() -> void:
	_save_settings()
	var home_id := _team_id(_home_opt)
	var away_id := _team_id(_away_opt)
	if home_id == away_id:
		away_id = (away_id + 1) % GameData.teams.size()
	GameData.home_team_id = home_id
	GameData.away_team_id = away_id
	GameData.selected_stadium = GameData.STADIUM_NAMES[_stad_opt.selected]
	GameData.half_duration_seconds = float(_half_opt.get_selected_id())
	GameData.ai_difficulty = _diff_opt.selected
	var fh := _formation_choice(_form_home_opt)
	var fa := _formation_choice(_form_away_opt)
	GameData.home_formation_manual = fh != ""
	GameData.away_formation_manual = fa != ""
	if fh != "":
		GameData.home_formation = fh
	if fa != "":
		GameData.away_formation = fa
	match _ctrl_opt.selected:
		0:
			GameData.is_ai_vs_ai = false
			GameData.human_side = 0
		1:
			GameData.is_ai_vs_ai = false
			GameData.human_side = 1
		_:
			GameData.is_ai_vs_ai = true
			GameData.human_side = -1
	Engine.time_scale = float(SPEEDS[_speed_opt.selected])
	GameData.preview_speed = Engine.time_scale
	GameData.return_scene = "res://test_tools/match_preview.tscn"
	MatchState.reset_match()
	get_tree().change_scene_to_file("res://scenes/match.tscn")

# -------------------------------------------------------------- settings
func _save_settings() -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("preview", "home", _home_opt.selected)
	cfg.set_value("preview", "away", _away_opt.selected)
	cfg.set_value("preview", "stadium", _stad_opt.selected)
	cfg.set_value("preview", "half", _half_opt.selected)
	cfg.set_value("preview", "form_home", _form_home_opt.selected)
	cfg.set_value("preview", "form_away", _form_away_opt.selected)
	cfg.set_value("preview", "difficulty", _diff_opt.selected)
	cfg.set_value("preview", "control", _ctrl_opt.selected)
	cfg.set_value("preview", "speed", _speed_opt.selected)
	cfg.save(CFG_PATH)

func _load_settings() -> void:
	# defaults: Brazil vs Argentina, Normal, 2:00 halves, you control Home
	_home_opt.select(_idx_of_team(30))
	_away_opt.select(_idx_of_team(33))
	_half_opt.select(1)
	_diff_opt.select(1)
	_speed_opt.select(1)
	var cfg := ConfigFile.new()
	if cfg.load(CFG_PATH) != OK:
		return
	_select_clamped(_home_opt, int(cfg.get_value("preview", "home", _home_opt.selected)))
	_select_clamped(_away_opt, int(cfg.get_value("preview", "away", _away_opt.selected)))
	_select_clamped(_stad_opt, int(cfg.get_value("preview", "stadium", 0)))
	_select_clamped(_half_opt, int(cfg.get_value("preview", "half", 1)))
	_select_clamped(_form_home_opt, int(cfg.get_value("preview", "form_home", 0)))
	_select_clamped(_form_away_opt, int(cfg.get_value("preview", "form_away", 0)))
	_select_clamped(_diff_opt, int(cfg.get_value("preview", "difficulty", 1)))
	_select_clamped(_ctrl_opt, int(cfg.get_value("preview", "control", 0)))
	_select_clamped(_speed_opt, int(cfg.get_value("preview", "speed", 1)))

func _select_clamped(opt: OptionButton, idx: int) -> void:
	if opt.item_count > 0:
		opt.select(clampi(idx, 0, opt.item_count - 1))

func _idx_of_team(tid: int) -> int:
	for i in range(_home_opt.item_count):
		if _home_opt.get_item_id(i) == tid:
			return i
	return 0

# ------------------------------------------------------------- cmdline
func _cmd_args() -> Dictionary:
	var out := {}
	for a in OS.get_cmdline_user_args():
		var s := str(a)
		if s.begins_with("--"):
			var kv := s.substr(2).split("=", true, 1)
			out[kv[0]] = kv[1] if kv.size() > 1 else "true"
	return out

func _has_flag(name: String) -> bool:
	return _cmd_args().has(name.trim_prefix("--"))

func _find_team(token: String) -> int:
	if token.is_valid_int():
		return clampi(token.to_int(), 0, GameData.teams.size() - 1)
	var low := token.to_lower()
	for t in GameData.teams:
		if str(t.get("team_name", "")).to_lower() == low:
			return int(t["team_id"])
	for t in GameData.teams:
		if str(t.get("team_name", "")).to_lower().contains(low):
			return int(t["team_id"])
	return -1

func _apply_cmdline() -> void:
	var args := _cmd_args()
	if args.has("home"):
		var id := _find_team(str(args["home"]))
		if id >= 0:
			_home_opt.select(_idx_of_team(id))
	if args.has("away"):
		var id2 := _find_team(str(args["away"]))
		if id2 >= 0:
			_away_opt.select(_idx_of_team(id2))
	if args.has("stadium"):
		var si := GameData.STADIUM_NAMES.find(str(args["stadium"]).to_upper())
		if si >= 0:
			_stad_opt.select(si)
	for key in ["form-home", "form-away"]:
		if args.has(key):
			var v := str(args[key])
			var fi := 0 if v == "auto" else GameData.FORMATIONS.find(v) + 1
			if fi >= 0:
				(_form_home_opt if key == "form-home" else _form_away_opt).select(fi)
	if args.has("half"):
		var secs := int(args["half"])
		for i in range(_half_opt.item_count):
			if _half_opt.get_item_id(i) == secs:
				_half_opt.select(i)
	if args.has("difficulty"):
		_select_clamped(_diff_opt, int(args["difficulty"]))
	if args.has("control"):
		match str(args["control"]):
			"home": _ctrl_opt.select(0)
			"away": _ctrl_opt.select(1)
			"cpu": _ctrl_opt.select(2)
	if args.has("speed"):
		var spd := float(args["speed"])
		for i in range(SPEEDS.size()):
			if is_equal_approx(SPEEDS[i], spd):
				_speed_opt.select(i)
