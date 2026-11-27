extends Control
## WE3 Match Preview — widescreen Konami-style quick match & test launcher.
##
## Command line (everything after "--" is read as user args):
##   godot --path . res://test_tools/match_preview.tscn -- --home=Brazil --away=Japan --stadium=GRDM_D \
##         --form-home=3-5-2 --form-away=auto --half=180 --control=away --speed=2 --difficulty=2 \
##         --time=night --weather=rain --autostart
## Last used settings are stored in user://match_preview.cfg.

const CFG_PATH := "user://match_preview.cfg"
const HALF_CHOICES := [60, 120, 180, 300, 600]
const SPEEDS := [0.5, 1.0, 2.0, 4.0]
const CONTROLS := ["1P (Home) vs CPU", "1P (Away) vs CPU", "CPU vs CPU (Watch)", "1P vs 2P (Local)"]
const STADIUM_LABELS := {
	"GRDM_C": "National Stadium",
	"GRDM_D": "Euro Arena",
	"GRDM_E": "South Dome",
	"GRDM_F": "Olympic Park",
	"GRDM_FN": "Night Cup Arena",
	"GRDM_G": "Classic Ground",
	"GRDM_H": "World Bowl",
	"GRDM_I": "Training Pitch",
	"GRDM_J": "Grand Finale",
}

var _home_opt: OptionButton
var _away_opt: OptionButton
var _stad_opt: OptionButton
var _form_home_opt: OptionButton
var _form_away_opt: OptionButton
var _half_opt: OptionButton
var _diff_opt: OptionButton
var _ctrl_opt: OptionButton
var _speed_opt: OptionButton
var _time_opt: OptionButton
var _weather_opt: OptionButton
var _mode_opt: OptionButton
var _et_chk: CheckButton
var _tactic_opt: OptionButton
var _trap_chk: CheckButton
var _home_cond_opt: OptionButton
var _away_cond_opt: OptionButton
var _home_avg_lbl: Label
var _away_avg_lbl: Label
var _home_info: Label
var _away_info: Label
var _home_kit: TextureRect
var _away_kit: TextureRect
var _home_flag: TextureRect
var _away_flag: TextureRect
var _home_pitch: PreviewMiniPitch
var _away_pitch: PreviewMiniPitch
var _stadium_preview: TextureRect
var _stadium_caption: Label

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
func _make_box_style(bg_col: Color, border_col: Color, pad: int = 10) -> StyleBoxFlat:
	var sb := StyleBoxFlat.new()
	sb.bg_color = bg_col
	sb.border_color = border_col
	sb.set_border_width_all(2)
	sb.set_corner_radius_all(2)
	sb.set_content_margin_all(pad)
	return sb

func _build_ui() -> void:
	var bg := ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.03, 0.06, 0.15)
	add_child(bg)
	var bg_path := "res://assets/textures/ui_and_bg/BACKGROUNDS/we3_bg_flags.png"
	if not ResourceLoader.exists(bg_path):
		bg_path = "res://assets/textures/ui_and_bg/BACKGROUNDS/we3_bg_stadium_night.png"
	_add_we3_background(bg_path, 0.52)

	var margin := MarginContainer.new()
	margin.set_anchors_preset(Control.PRESET_FULL_RECT)
	margin.add_theme_constant_override("margin_left", 18)
	margin.add_theme_constant_override("margin_right", 18)
	margin.add_theme_constant_override("margin_top", 14)
	margin.add_theme_constant_override("margin_bottom", 12)
	add_child(margin)

	var outer := VBoxContainer.new()
	outer.add_theme_constant_override("separation", 10)
	margin.add_child(outer)

	# === TOP KONAMI HEADER BAR ===
	var hdr_panel := PanelContainer.new()
	hdr_panel.add_theme_stylebox_override("panel", _make_box_style(Color(0.06, 0.12, 0.30, 0.94), Color(0.75, 0.85, 1.0, 0.9), 8))
	outer.add_child(hdr_panel)

	var hdr_row := HBoxContainer.new()
	hdr_row.add_theme_constant_override("separation", 10)
	hdr_panel.add_child(hdr_row)

	var title := Label.new()
	title.text = "EXHIBITION & MATCH PREVIEW"
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", Color(0.98, 0.88, 0.25))
	hdr_row.add_child(title)

	var b_rand := Button.new()
	b_rand.text = "Random [R]"
	b_rand.pressed.connect(_random_teams)
	hdr_row.add_child(b_rand)

	var b_swap := Button.new()
	b_swap.text = "⇄ Swap [S]"
	b_swap.pressed.connect(_swap_teams)
	hdr_row.add_child(b_swap)

	var b_menu := Button.new()
	b_menu.text = "Main Menu"
	b_menu.pressed.connect(func():
		GameData.return_scene = "res://scenes/main_menu.tscn"
		get_tree().change_scene_to_file("res://scenes/main_menu.tscn"))
	hdr_row.add_child(b_menu)

	var b_go := Button.new()
	b_go.text = "▶  KICK OFF  [Enter]"
	b_go.custom_minimum_size = Vector2(200, 36)
	b_go.add_theme_font_size_override("font_size", 16)
	b_go.add_theme_color_override("font_color", Color(1.0, 0.95, 0.35))
	b_go.pressed.connect(_start_match)
	hdr_row.add_child(b_go)

	# === MAIN 3-COLUMN WIDESCREEN BODY ===
	var body := HBoxContainer.new()
	body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	body.add_theme_constant_override("separation", 12)
	outer.add_child(body)

	body.add_child(_build_team_card("1P  /  HOME TEAM", true))
	body.add_child(_build_center_settings())
	body.add_child(_build_team_card("CPU / 2P  /  AWAY TEAM", false))

	# === FOOTER ===
	var footer := Label.new()
	footer.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	footer.add_theme_font_size_override("font_size", 12)
	footer.add_theme_color_override("font_color", Color(0.76, 0.86, 0.98))
	footer.text = "In-Match:  F8 = Back to Preview   |   F5 = Restart   |   Tab = 2x Speed   |   1/2/3 = Tactic   |   F12 = Event Log   |   M = Mute"
	outer.add_child(footer)

func _build_team_card(header_text: String, is_home: bool) -> PanelContainer:
	var card := PanelContainer.new()
	card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	card.size_flags_vertical = Control.SIZE_EXPAND_FILL
	var border_col := Color(0.25, 0.88, 1.0, 0.9) if is_home else Color(1.0, 0.38, 0.28, 0.9)
	card.add_theme_stylebox_override("panel", _make_box_style(Color(0.04, 0.08, 0.20, 0.92), border_col, 10))

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 6)
	card.add_child(col)

	var lbl := Label.new()
	lbl.text = header_text
	lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	lbl.add_theme_font_size_override("font_size", 14)
	lbl.add_theme_color_override("font_color", Color(0.35, 0.95, 1.0) if is_home else Color(1.0, 0.55, 0.42))
	col.add_child(lbl)

	var opt := OptionButton.new()
	opt.custom_minimum_size = Vector2(0, 32)
	for t in GameData.teams:
		opt.add_item(str(t.get("team_name", "?")), int(t.get("team_id", 0)))
	opt.item_selected.connect(func(_i): _refresh_info())
	col.add_child(opt)

	var vis_row := HBoxContainer.new()
	vis_row.alignment = BoxContainer.ALIGNMENT_CENTER
	vis_row.add_theme_constant_override("separation", 10)
	col.add_child(vis_row)

	var flag := TextureRect.new()
	flag.custom_minimum_size = Vector2(64, 40)
	flag.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	flag.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	flag.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	vis_row.add_child(flag)

	var kit := TextureRect.new()
	kit.custom_minimum_size = Vector2(78, 78)
	kit.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	kit.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	kit.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	vis_row.add_child(kit)

	var mini_p := PreviewMiniPitch.new()
	mini_p.custom_minimum_size = Vector2(118, 62)
	vis_row.add_child(mini_p)

	var sub_row := HBoxContainer.new()
	sub_row.add_theme_constant_override("separation", 6)
	col.add_child(sub_row)

	var form_opt := OptionButton.new()
	form_opt.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	form_opt.add_item("Form: Auto")
	for f_name in GameData.FORMATIONS:
		form_opt.add_item(f_name)
	form_opt.item_selected.connect(func(_i): _refresh_info())
	sub_row.add_child(form_opt)

	var cond_opt := OptionButton.new()
	cond_opt.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	for ci in ["↓ BAD", "↘ POOR", "→ NORM", "↗ GOOD", "↑ TOP"]:
		cond_opt.add_item(ci)
	cond_opt.select(2)
	sub_row.add_child(cond_opt)

	var avg_lbl := Label.new()
	avg_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	avg_lbl.add_theme_font_size_override("font_size", 12)
	avg_lbl.add_theme_color_override("font_color", Color(0.98, 0.86, 0.25))
	col.add_child(avg_lbl)

	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	col.add_child(scroll)

	var info := Label.new()
	info.add_theme_font_size_override("font_size", 12)
	info.add_theme_color_override("font_color", Color(0.86, 0.92, 0.98))
	scroll.add_child(info)

	if is_home:
		_home_opt = opt
		_home_kit = kit
		_home_flag = flag
		_home_pitch = mini_p
		_form_home_opt = form_opt
		_home_cond_opt = cond_opt
		_home_avg_lbl = avg_lbl
		_home_info = info
	else:
		_away_opt = opt
		_away_kit = kit
		_away_flag = flag
		_away_pitch = mini_p
		_form_away_opt = form_opt
		_away_cond_opt = cond_opt
		_away_avg_lbl = avg_lbl
		_away_info = info
	return card

func _build_center_settings() -> PanelContainer:
	var card := PanelContainer.new()
	card.custom_minimum_size = Vector2(390, 0)
	card.size_flags_vertical = Control.SIZE_EXPAND_FILL
	card.add_theme_stylebox_override("panel", _make_box_style(Color(0.05, 0.09, 0.24, 0.94), Color(0.96, 0.82, 0.20, 0.95), 10))

	var col := VBoxContainer.new()
	col.add_theme_constant_override("separation", 8)
	card.add_child(col)

	# Stadium Preview Box (like WE3 Exhibition Mode right-top preview)
	var stad_frame := PanelContainer.new()
	stad_frame.add_theme_stylebox_override("panel", _make_box_style(Color(0.02, 0.04, 0.10, 1.0), Color(0.65, 0.78, 0.95, 0.8), 2))
	col.add_child(stad_frame)

	var stad_overlay := Control.new()
	stad_overlay.custom_minimum_size = Vector2(0, 112)
	stad_frame.add_child(stad_overlay)

	_stadium_preview = TextureRect.new()
	_stadium_preview.set_anchors_preset(Control.PRESET_FULL_RECT)
	_stadium_preview.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_stadium_preview.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	_stadium_preview.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	stad_overlay.add_child(_stadium_preview)

	_stadium_caption = Label.new()
	_stadium_caption.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	_stadium_caption.offset_top = -24
	_stadium_caption.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_stadium_caption.add_theme_font_size_override("font_size", 13)
	_stadium_caption.add_theme_color_override("font_color", Color(1.0, 0.96, 0.75))
	_stadium_caption.add_theme_color_override("font_outline_color", Color(0.0, 0.0, 0.0, 0.9))
	_stadium_caption.add_theme_constant_override("outline_size", 3)
	stad_overlay.add_child(_stadium_caption)

	# Konami 2-column match settings table
	var grid := GridContainer.new()
	grid.columns = 2
	grid.add_theme_constant_override("h_separation", 10)
	grid.add_theme_constant_override("v_separation", 6)
	col.add_child(grid)

	_mode_opt = _option_row(grid, "Match Mode")
	_mode_opt.add_item("Exhibition", 0)
	_mode_opt.add_item("PK Shootout", 1)
	_mode_opt.add_item("Training", 2)

	_ctrl_opt = _option_row(grid, "Control")
	for c_name in CONTROLS:
		_ctrl_opt.add_item(c_name)

	_stad_opt = _option_row(grid, "Stadium")
	for s_name in GameData.STADIUM_NAMES:
		_stad_opt.add_item(STADIUM_LABELS.get(s_name, s_name))
	_stad_opt.item_selected.connect(func(_i): _refresh_stadium_preview())

	_time_opt = _option_row(grid, "Time of Day")
	_time_opt.add_item("Day", 0)
	_time_opt.add_item("Night", 1)
	_time_opt.item_selected.connect(func(_i): _refresh_stadium_preview())

	_weather_opt = _option_row(grid, "Weather")
	_weather_opt.add_item("Clear", 0)
	_weather_opt.add_item("Rain", 1)
	_weather_opt.item_selected.connect(func(_i): _refresh_stadium_preview())

	_half_opt = _option_row(grid, "Half Length")
	for sec in HALF_CHOICES:
		_half_opt.add_item("%d:%02d min" % [sec / 60, sec % 60], sec)

	_diff_opt = _option_row(grid, "CPU Level")
	for d_name in ["Easy", "Normal", "Hard"]:
		_diff_opt.add_item(d_name)

	_tactic_opt = _option_row(grid, "Home Tactic")
	_tactic_opt.add_item("Defensive", 0)
	_tactic_opt.add_item("Normal", 1)
	_tactic_opt.add_item("Attacking", 2)

	_speed_opt = _option_row(grid, "Game Speed")
	for sp in SPEEDS:
		_speed_opt.add_item("%sx" % str(sp))

	var chk_row := HBoxContainer.new()
	chk_row.alignment = BoxContainer.ALIGNMENT_CENTER
	chk_row.add_theme_constant_override("separation", 14)
	col.add_child(chk_row)

	_et_chk = CheckButton.new()
	_et_chk.text = "Extra Time + PK"
	chk_row.add_child(_et_chk)

	_trap_chk = CheckButton.new()
	_trap_chk.text = "Offside Trap"
	chk_row.add_child(_trap_chk)

	return card

func _option_row(grid: GridContainer, text: String) -> OptionButton:
	var lbl := Label.new()
	lbl.text = text
	lbl.custom_minimum_size = Vector2(110, 0)
	lbl.add_theme_font_size_override("font_size", 13)
	lbl.add_theme_color_override("font_color", Color(0.82, 0.90, 1.0))
	grid.add_child(lbl)
	var opt := OptionButton.new()
	opt.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	opt.custom_minimum_size = Vector2(210, 28)
	grid.add_child(opt)
	return opt

func _add_we3_background(path: String, dim_alpha: float) -> void:
	if ResourceLoader.exists(path):
		var tex := TextureRect.new()
		tex.set_anchors_preset(Control.PRESET_FULL_RECT)
		tex.texture = load(path) as Texture2D
		tex.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tex.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		tex.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		add_child(tex)
		var dim := ColorRect.new()
		dim.set_anchors_preset(Control.PRESET_FULL_RECT)
		dim.color = Color(0.02, 0.05, 0.14, dim_alpha)
		add_child(dim)

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

func _refresh_stadium_preview() -> void:
	if _stadium_preview == null or _stad_opt == null:
		return
	var s_code: String = GameData.STADIUM_NAMES[clampi(_stad_opt.selected, 0, GameData.STADIUM_NAMES.size() - 1)]
	var is_night: bool = (_time_opt.selected == 1) or (s_code == "GRDM_FN")
	var img_path := "res://assets/textures/ui_and_bg/BACKGROUNDS/we3_bg_stadium_night.png" if is_night else "res://assets/textures/ui_and_bg/BACKGROUNDS/we3_bg_stadium_day.png"
	if ResourceLoader.exists(img_path):
		_stadium_preview.texture = load(img_path) as Texture2D
	var w_txt := "RAIN" if _weather_opt.selected == 1 else "CLEAR"
	var t_txt := "NIGHT" if is_night else "DAY"
	_stadium_caption.text = "%s  •  %s / %s" % [STADIUM_LABELS.get(s_code, s_code), t_txt, w_txt]

func _refresh_info() -> void:
	if _home_opt == null or _away_opt == null:
		return
	_refresh_stadium_preview()
	for side in range(2):
		var opt := _home_opt if side == 0 else _away_opt
		var avg_lbl := _home_avg_lbl if side == 0 else _away_avg_lbl
		var info := _home_info if side == 0 else _away_info
		var kit := _home_kit if side == 0 else _away_kit
		var flag := _home_flag if side == 0 else _away_flag
		var mini_p := _home_pitch if side == 0 else _away_pitch
		var tid := _team_id(opt)
		kit.texture = GameData.get_team_kit_texture(tid)
		flag.texture = GameData.get_team_flag_texture(tid)
		var choice := _formation_choice(_form_home_opt if side == 0 else _form_away_opt)
		var auto_form := GameData.get_team_default_formation(tid)
		var active_form := choice if choice != "" else auto_form
		mini_p.formation_name = active_form
		mini_p.team_color = GameData.get_team_primary_color(tid)
		mini_p.queue_redraw()
		var players: Array = GameData.get_team(tid).get("players", [])
		var sums := {"offense": 0, "defense": 0, "speed": 0, "technique": 0}
		var lines: PackedStringArray = []
		var n := mini(11, players.size())
		for i in range(n):
			var p: Dictionary = players[i]
			var st: Dictionary = p.get("stats", {})
			for k in sums.keys():
				sums[k] += int(st.get(k, 0))
			lines.append("#%-2d %-10s %s  O%d D%d S%d" % [int(p.get("shirt_number", i + 1)), str(p.get("name", "")), str(p.get("pos_group", "")), int(st.get("offense", 0)), int(st.get("defense", 0)), int(st.get("speed", 0))])
		avg_lbl.text = "%s  |  OFF %.1f  DEF %.1f  SPD %.1f  TEC %.1f" % [active_form, sums["offense"] / float(maxi(n, 1)), sums["defense"] / float(maxi(n, 1)), sums["speed"] / float(maxi(n, 1)), sums["technique"] / float(maxi(n, 1))]
		info.text = "\n".join(lines)

func _swap_teams() -> void:
	var h := _home_opt.selected
	_home_opt.select(_away_opt.selected)
	_away_opt.select(h)
	var fh := _form_home_opt.selected
	_form_home_opt.select(_form_away_opt.selected)
	_form_away_opt.select(fh)
	var ch := _home_cond_opt.selected
	_home_cond_opt.select(_away_cond_opt.selected)
	_away_cond_opt.select(ch)
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
	GameData.stadium_is_night = (_time_opt.selected == 1)
	GameData.stadium_is_rain = (_weather_opt.selected == 1)
	GameData.extra_time_enabled = _et_chk.button_pressed
	GameData.home_tactic_level = _tactic_opt.selected
	GameData.home_offside_trap = _trap_chk.button_pressed
	GameData.team_condition = [_home_cond_opt.selected, _away_cond_opt.selected]
	var mode_ids := ["exhibition", "pk", "training"]
	GameData.game_mode = mode_ids[clampi(_mode_opt.selected, 0, 2)]
	GameData.cup_active = false
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
			GameData.is_two_player = false
			GameData.human_side = 0
		1:
			GameData.is_ai_vs_ai = false
			GameData.is_two_player = false
			GameData.human_side = 1
		3:
			GameData.is_ai_vs_ai = false
			GameData.is_two_player = true
			GameData.human_side = 0
		_:
			GameData.is_ai_vs_ai = true
			GameData.is_two_player = false
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
	cfg.set_value("preview", "time", _time_opt.selected)
	cfg.set_value("preview", "weather", _weather_opt.selected)
	cfg.set_value("preview", "mode", _mode_opt.selected)
	cfg.set_value("preview", "et", _et_chk.button_pressed)
	cfg.set_value("preview", "tactic", _tactic_opt.selected)
	cfg.set_value("preview", "trap", _trap_chk.button_pressed)
	cfg.set_value("preview", "cond_h", _home_cond_opt.selected)
	cfg.set_value("preview", "cond_a", _away_cond_opt.selected)
	cfg.save(CFG_PATH)

func _load_settings() -> void:
	_home_opt.select(_idx_of_team(30))
	_away_opt.select(_idx_of_team(33))
	_half_opt.select(1)
	_diff_opt.select(1)
	_speed_opt.select(1)
	_tactic_opt.select(1)
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
	_select_clamped(_time_opt, int(cfg.get_value("preview", "time", 0)))
	_select_clamped(_weather_opt, int(cfg.get_value("preview", "weather", 0)))
	_select_clamped(_mode_opt, int(cfg.get_value("preview", "mode", 0)))
	_et_chk.button_pressed = bool(cfg.get_value("preview", "et", false))
	_select_clamped(_tactic_opt, int(cfg.get_value("preview", "tactic", 1)))
	_trap_chk.button_pressed = bool(cfg.get_value("preview", "trap", false))
	_select_clamped(_home_cond_opt, int(cfg.get_value("preview", "cond_h", 2)))
	_select_clamped(_away_cond_opt, int(cfg.get_value("preview", "cond_a", 2)))

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
			"2p": _ctrl_opt.select(3)
	if args.has("speed"):
		var spd := float(args["speed"])
		for i in range(SPEEDS.size()):
			if is_equal_approx(SPEEDS[i], spd):
				_speed_opt.select(i)
	if args.has("time"):
		_time_opt.select(1 if str(args["time"]).to_lower() == "night" else 0)
	if args.has("weather"):
		_weather_opt.select(1 if str(args["weather"]).to_lower() == "rain" else 0)
	if args.has("pk"):
		_mode_opt.select(1)
	if args.has("extra-time"):
		_et_chk.button_pressed = true


class PreviewMiniPitch extends Control:
	var formation_name: String = "4-4-2"
	var team_color: Color = Color.CYAN

	func _draw() -> void:
		draw_rect(Rect2(Vector2.ZERO, size), Color(0.06, 0.28, 0.10), true)
		var lc := Color(0.9, 0.95, 0.9, 0.72)
		draw_rect(Rect2(Vector2.ZERO, size), lc, false, 1.2)
		draw_line(Vector2(size.x * 0.5, 0), Vector2(size.x * 0.5, size.y), lc, 1.0)
		var pts := {
			"4-4-2": [Vector2(0.08,0.5), Vector2(0.25,0.2), Vector2(0.22,0.4), Vector2(0.22,0.6), Vector2(0.25,0.8), Vector2(0.5,0.18), Vector2(0.46,0.4), Vector2(0.46,0.6), Vector2(0.5,0.82), Vector2(0.76,0.38), Vector2(0.76,0.62)],
			"4-3-3": [Vector2(0.08,0.5), Vector2(0.25,0.2), Vector2(0.22,0.4), Vector2(0.22,0.6), Vector2(0.25,0.8), Vector2(0.48,0.28), Vector2(0.44,0.5), Vector2(0.48,0.72), Vector2(0.78,0.2), Vector2(0.82,0.5), Vector2(0.78,0.8)],
			"3-5-2": [Vector2(0.08,0.5), Vector2(0.24,0.26), Vector2(0.22,0.5), Vector2(0.24,0.74), Vector2(0.52,0.14), Vector2(0.46,0.34), Vector2(0.54,0.5), Vector2(0.46,0.66), Vector2(0.52,0.86), Vector2(0.76,0.38), Vector2(0.76,0.62)],
			"5-3-2": [Vector2(0.08,0.5), Vector2(0.28,0.12), Vector2(0.24,0.31), Vector2(0.22,0.5), Vector2(0.24,0.69), Vector2(0.28,0.88), Vector2(0.48,0.28), Vector2(0.44,0.5), Vector2(0.48,0.72), Vector2(0.76,0.38), Vector2(0.76,0.62)],
			"4-5-1": [Vector2(0.08,0.5), Vector2(0.25,0.2), Vector2(0.22,0.4), Vector2(0.22,0.6), Vector2(0.25,0.8), Vector2(0.50,0.12), Vector2(0.46,0.31), Vector2(0.54,0.5), Vector2(0.46,0.69), Vector2(0.50,0.88), Vector2(0.80,0.5)],
			"3-4-3": [Vector2(0.08,0.5), Vector2(0.24,0.26), Vector2(0.22,0.5), Vector2(0.24,0.74), Vector2(0.50,0.16), Vector2(0.46,0.39), Vector2(0.46,0.61), Vector2(0.50,0.84), Vector2(0.78,0.2), Vector2(0.82,0.5), Vector2(0.78,0.8)],
		}
		var list: Array = pts.get(formation_name, pts["4-4-2"])
		for i in range(list.size()):
			var pt: Vector2 = Vector2(list[i].x * size.x, list[i].y * size.y)
			draw_circle(pt, 3.0, Color.YELLOW if i == 0 else team_color)
