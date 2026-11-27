extends Control
## Team Select screen for choosing Home & Away teams from all 43 national teams.

var _home_opt: OptionButton
var _away_opt: OptionButton
var _half_opt: OptionButton
var _home_kit_rect: TextureRect
var _away_kit_rect: TextureRect
var _home_flag_rect: TextureRect
var _away_flag_rect: TextureRect
var _home_swatch: ColorRect
var _away_swatch: ColorRect
var _home_rating_label: Label
var _away_rating_label: Label
var _home_roster_label: Label
var _away_roster_label: Label
var _formation_preview: FormationMiniPitch
var _form_opt: OptionButton
var _home_cond_opt: OptionButton
var _away_cond_opt: OptionButton

func _ready() -> void:
	_build_ui()
	SoundFX.play_music("08SELECT")

func _build_ui() -> void:
	var bg := ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.03, 0.06, 0.16)
	add_child(bg)
	var bg_path := "res://assets/textures/ui_and_bg/BACKGROUNDS/we3_bg_flags.png"
	if not ResourceLoader.exists(bg_path):
		bg_path = "res://assets/textures/ui_and_bg/BACKGROUNDS/we3_bg_stadium_night.png"
	_add_we3_background(bg_path, 0.45)

	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(center)

	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(760, 0)
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.04, 0.08, 0.22, 0.94)
	style.border_color = Color(0.96, 0.82, 0.20)
	style.set_border_width_all(2)
	style.set_corner_radius_all(2)
	style.set_content_margin_all(14)
	panel.add_theme_stylebox_override("panel", style)
	center.add_child(panel)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 8)
	panel.add_child(vbox)

	var title := Label.new()
	var mode_titles := {
		"world_cup": "WORLD CUP '98 — TEAM SELECT",
		"cup": "INTERNATIONAL CUP — TEAM SELECT",
		"league": "INTERNATIONAL LEAGUE — TEAM SELECT",
		"option": "OPTION & MATCH SETTINGS",
	}
	title.text = mode_titles.get(GameData.game_mode, "EXHIBITION MATCH SETUP")
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 26)
	title.add_theme_color_override("font_color", Color(0.98, 0.86, 0.22))
	vbox.add_child(title)

	var hbox := HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 36)
	hbox.alignment = BoxContainer.ALIGNMENT_CENTER
	vbox.add_child(hbox)

	# Home side
	var home_col := VBoxContainer.new()
	home_col.add_theme_constant_override("separation", 4)
	hbox.add_child(home_col)
	var h_lbl := Label.new()
	h_lbl.text = "HOME (YOU)"
	h_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	home_col.add_child(h_lbl)
	_home_opt = OptionButton.new()
	_home_opt.custom_minimum_size = Vector2(240, 38)
	home_col.add_child(_home_opt)
	var h_kit_row := HBoxContainer.new()
	h_kit_row.alignment = BoxContainer.ALIGNMENT_CENTER
	h_kit_row.add_theme_constant_override("separation", 10)
	home_col.add_child(h_kit_row)
	_home_swatch = ColorRect.new()
	_home_swatch.custom_minimum_size = Vector2(12, 64)
	h_kit_row.add_child(_home_swatch)
	_home_kit_rect = TextureRect.new()
	_home_kit_rect.custom_minimum_size = Vector2(88, 88)
	_home_kit_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_home_kit_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	h_kit_row.add_child(_home_kit_rect)
	_home_flag_rect = TextureRect.new()
	_home_flag_rect.custom_minimum_size = Vector2(64, 40)
	_home_flag_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_home_flag_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_home_flag_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	h_kit_row.add_child(_home_flag_rect)
	_home_rating_label = Label.new()
	_home_rating_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_home_rating_label.add_theme_font_size_override("font_size", 12)
	_home_rating_label.add_theme_color_override("font_color", Color(0.96, 0.84, 0.22))
	home_col.add_child(_home_rating_label)
	_home_cond_opt = _build_condition_selector(0)
	home_col.add_child(_home_cond_opt)
	_home_roster_label = Label.new()
	_home_roster_label.add_theme_font_size_override("font_size", 11)
	_home_roster_label.add_theme_color_override("font_color", Color(0.82, 0.88, 0.95))
	home_col.add_child(_home_roster_label)

	# Away side
	var away_col := VBoxContainer.new()
	away_col.add_theme_constant_override("separation", 4)
	hbox.add_child(away_col)
	var a_lbl := Label.new()
	a_lbl.text = "AWAY (CPU)"
	a_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	away_col.add_child(a_lbl)
	_away_opt = OptionButton.new()
	_away_opt.custom_minimum_size = Vector2(240, 38)
	away_col.add_child(_away_opt)
	var a_kit_row := HBoxContainer.new()
	a_kit_row.alignment = BoxContainer.ALIGNMENT_CENTER
	a_kit_row.add_theme_constant_override("separation", 10)
	away_col.add_child(a_kit_row)
	_away_swatch = ColorRect.new()
	_away_swatch.custom_minimum_size = Vector2(12, 64)
	a_kit_row.add_child(_away_swatch)
	_away_kit_rect = TextureRect.new()
	_away_kit_rect.custom_minimum_size = Vector2(88, 88)
	_away_kit_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_away_kit_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	a_kit_row.add_child(_away_kit_rect)
	_away_flag_rect = TextureRect.new()
	_away_flag_rect.custom_minimum_size = Vector2(64, 40)
	_away_flag_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_away_flag_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_away_flag_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	a_kit_row.add_child(_away_flag_rect)
	_away_rating_label = Label.new()
	_away_rating_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_away_rating_label.add_theme_font_size_override("font_size", 12)
	_away_rating_label.add_theme_color_override("font_color", Color(0.96, 0.84, 0.22))
	away_col.add_child(_away_rating_label)
	_away_cond_opt = _build_condition_selector(1)
	away_col.add_child(_away_cond_opt)
	_away_roster_label = Label.new()
	_away_roster_label.add_theme_font_size_override("font_size", 11)
	_away_roster_label.add_theme_color_override("font_color", Color(0.82, 0.88, 0.95))
	away_col.add_child(_away_roster_label)

	for t in GameData.teams:
		var tid := int(t.get("team_id", 0))
		var tname := str(t.get("team_name", "Team"))
		_home_opt.add_item(tname, tid)
		_away_opt.add_item(tname, tid)

	_home_opt.select(_find_item_idx(_home_opt, GameData.home_team_id))
	_away_opt.select(_find_item_idx(_away_opt, GameData.away_team_id))
	_home_opt.item_selected.connect(func(_i): _sync_selection())
	_away_opt.item_selected.connect(func(_i): _sync_selection())
	_sync_selection()

	# Half duration & Difficulty
	var opt_row := HBoxContainer.new()
	opt_row.alignment = BoxContainer.ALIGNMENT_CENTER
	opt_row.add_theme_constant_override("separation", 14)
	vbox.add_child(opt_row)
	var dur_lbl := Label.new()
	dur_lbl.text = "Half Length:"
	opt_row.add_child(dur_lbl)
	_half_opt = OptionButton.new()
	_half_opt.add_item("1 Minute", 60)
	_half_opt.add_item("2 Minutes", 120)
	_half_opt.add_item("3 Minutes", 180)
	_half_opt.select(1)
	_half_opt.item_selected.connect(func(idx): GameData.half_duration_seconds = float(_half_opt.get_item_id(idx)))
	opt_row.add_child(_half_opt)

	var diff_lbl := Label.new()
	diff_lbl.text = "Difficulty:"
	opt_row.add_child(diff_lbl)
	var diff_opt := OptionButton.new()
	diff_opt.add_item("Easy", 0)
	diff_opt.add_item("Normal", 1)
	diff_opt.add_item("Hard", 2)
	diff_opt.select(GameData.ai_difficulty)
	diff_opt.item_selected.connect(func(idx): GameData.ai_difficulty = diff_opt.get_item_id(idx))
	opt_row.add_child(diff_opt)

	var stad_lbl := Label.new()
	stad_lbl.text = "Stadium:"
	opt_row.add_child(stad_lbl)
	var stad_opt := OptionButton.new()
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
	for s_name in GameData.STADIUM_NAMES:
		stad_opt.add_item(STADIUM_LABELS.get(s_name, s_name))
	stad_opt.select(maxi(0, GameData.STADIUM_NAMES.find(GameData.selected_stadium)))
	stad_opt.item_selected.connect(func(idx): GameData.selected_stadium = GameData.STADIUM_NAMES[idx])
	opt_row.add_child(stad_opt)

	var time_opt := OptionButton.new()
	time_opt.add_item("Day", 0)
	time_opt.add_item("Night", 1)
	time_opt.select(1 if GameData.stadium_is_night else 0)
	time_opt.item_selected.connect(func(idx): GameData.stadium_is_night = (idx == 1))
	opt_row.add_child(time_opt)

	var weather_opt := OptionButton.new()
	weather_opt.add_item("Clear", 0)
	weather_opt.add_item("Rain", 1)
	weather_opt.select(1 if GameData.stadium_is_rain else 0)
	weather_opt.item_selected.connect(func(idx): GameData.stadium_is_rain = (idx == 1))
	opt_row.add_child(weather_opt)

	var form_row := HBoxContainer.new()
	form_row.alignment = BoxContainer.ALIGNMENT_CENTER
	form_row.add_theme_constant_override("separation", 18)
	vbox.add_child(form_row)

	var form_lbl := Label.new()
	form_lbl.text = "Formation:"
	form_row.add_child(form_lbl)
	var form_opt := OptionButton.new()
	_form_opt = form_opt
	for f_name in GameData.FORMATIONS:
		form_opt.add_item(f_name)
	form_opt.select(maxi(0, GameData.FORMATIONS.find(GameData.resolve_formation(0))))
	form_opt.item_selected.connect(func(idx):
		GameData.home_formation = GameData.FORMATIONS[idx]
		GameData.home_formation_manual = true
		if _formation_preview != null:
			_formation_preview.formation_name = GameData.home_formation
			_formation_preview.queue_redraw()
	)
	form_row.add_child(form_opt)

	_formation_preview = FormationMiniPitch.new()
	_formation_preview.custom_minimum_size = Vector2(200, 62)
	_formation_preview.formation_name = GameData.resolve_formation(0)
	_formation_preview.team_color = GameData.get_team_primary_color(GameData.home_team_id)
	form_row.add_child(_formation_preview)

	# selform tactics: Tactic Level (0=Def, 1=Normal, 2=Atk) & Offside Trap
	var tac_lbl := Label.new()
	tac_lbl.text = "Tactic:"
	form_row.add_child(tac_lbl)
	var tac_opt := OptionButton.new()
	tac_opt.add_item("Defensive", 0)
	tac_opt.add_item("Normal", 1)
	tac_opt.add_item("Attacking", 2)
	tac_opt.select(clampi(GameData.home_tactic_level, 0, 2))
	tac_opt.item_selected.connect(func(idx): GameData.home_tactic_level = idx)
	form_row.add_child(tac_opt)

	var trap_chk := CheckButton.new()
	trap_chk.text = "Offside Trap"
	trap_chk.button_pressed = GameData.home_offside_trap
	trap_chk.toggled.connect(func(on): GameData.home_offside_trap = on)
	form_row.add_child(trap_chk)

	var mode_opt := OptionButton.new()
	mode_opt.add_item("1P vs COM", 0)
	mode_opt.add_item("1P vs 2P", 1)
	mode_opt.add_item("COM vs COM", 2)
	var cur_mode := 2 if GameData.is_ai_vs_ai else (1 if GameData.is_two_player else 0)
	mode_opt.select(cur_mode)
	mode_opt.item_selected.connect(func(idx):
		GameData.is_two_player = (idx == 1)
		GameData.is_ai_vs_ai = (idx == 2)
	)
	form_row.add_child(mode_opt)

	var btn_row := HBoxContainer.new()
	btn_row.alignment = BoxContainer.ALIGNMENT_CENTER
	btn_row.add_theme_constant_override("separation", 20)
	vbox.add_child(btn_row)

	var btn_back := Button.new()
	btn_back.text = "Back"
	btn_back.custom_minimum_size = Vector2(140, 44)
	btn_back.pressed.connect(func(): get_tree().change_scene_to_file("res://scenes/main_menu.tscn"))
	btn_row.add_child(btn_back)

	var btn_kickoff := Button.new()
	btn_kickoff.text = "KICK OFF!"
	btn_kickoff.custom_minimum_size = Vector2(200, 44)
	btn_kickoff.pressed.connect(func():
		GameData.human_side = 0
		GameData.return_scene = "res://scenes/main_menu.tscn"
		GameData.preview_speed = 1.0
		if GameData.game_mode in ["cup", "world_cup"] and not GameData.cup_active:
			GameData.start_cup_tournament(GameData.home_team_id)
		get_tree().change_scene_to_file("res://scenes/match.tscn")
	)
	btn_row.add_child(btn_kickoff)

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

func _build_condition_selector(side: int) -> OptionButton:
	var opt := OptionButton.new()
	opt.add_item("Form: ↓ BAD", 0)
	opt.add_item("Form: ↘ POOR", 1)
	opt.add_item("Form: → NORMAL", 2)
	opt.add_item("Form: ↗ GOOD", 3)
	opt.add_item("Form: ↑ TOP", 4)
	opt.select(clampi(GameData.team_condition[side], 0, 4))
	opt.item_selected.connect(func(idx): GameData.team_condition[side] = idx)
	return opt

func _find_item_idx(opt: OptionButton, id: int) -> int:
	for i in range(opt.item_count):
		if opt.get_item_id(i) == id:
			return i
	return 0

func _sync_selection() -> void:
	if _home_opt.item_count > 0:
		var new_home := _home_opt.get_selected_id()
		if new_home != GameData.home_team_id:
			GameData.home_formation_manual = false
		GameData.home_team_id = new_home
		_home_kit_rect.texture = GameData.get_team_kit_texture(GameData.home_team_id)
		if _home_flag_rect != null:
			_home_flag_rect.texture = GameData.get_team_flag_texture(GameData.home_team_id)
		if _home_swatch != null:
			_home_swatch.color = GameData.get_team_primary_color(GameData.home_team_id)
		if _home_rating_label != null:
			_home_rating_label.text = _format_team_rating(GameData.home_team_id)
		_home_roster_label.text = _format_starting_xi(GameData.home_team_id)
	if _away_opt.item_count > 0:
		var new_away := _away_opt.get_selected_id()
		if new_away != GameData.away_team_id:
			GameData.away_formation_manual = false
		GameData.away_team_id = new_away
		_away_kit_rect.texture = GameData.get_team_kit_texture(GameData.away_team_id)
		if _away_flag_rect != null:
			_away_flag_rect.texture = GameData.get_team_flag_texture(GameData.away_team_id)
		if _away_swatch != null:
			_away_swatch.color = GameData.get_team_primary_color(GameData.away_team_id)
		if _away_rating_label != null:
			_away_rating_label.text = _format_team_rating(GameData.away_team_id)
		_away_roster_label.text = _format_starting_xi(GameData.away_team_id)
	# Show the authentic formation of the selected home team (derived from its real starting XI)
	if _form_opt != null:
		var f := GameData.resolve_formation(0)
		_form_opt.select(maxi(0, GameData.FORMATIONS.find(f)))
		if _formation_preview != null:
			_formation_preview.formation_name = f
			_formation_preview.team_color = GameData.get_team_primary_color(GameData.home_team_id)
			_formation_preview.queue_redraw()

func _format_team_rating(tid: int) -> String:
	var t := GameData.get_team(tid)
	var players: Array = t.get("players", [])
	var n := mini(11, players.size())
	if n == 0:
		return ""
	var sum_off := 0.0
	var sum_def := 0.0
	var sum_spd := 0.0
	for i in range(n):
		var st: Dictionary = players[i].get("stats", {})
		sum_off += float(st.get("offense", 5))
		sum_def += float(st.get("defense", 5))
		sum_spd += float(st.get("speed", 5))
	return "OFF %.1f  |  DEF %.1f  |  SPD %.1f" % [sum_off / n, sum_def / n, sum_spd / n]

func _format_starting_xi(tid: int) -> String:
	var t := GameData.get_team(tid)
	var players: Array = t.get("players", [])
	var lines: PackedStringArray = []
	for i in range(mini(11, players.size())):
		var p: Dictionary = players[i]
		lines.append("%2d. %-9s %s  SPD %d" % [int(p.get("shirt_number", i + 1)), str(p.get("name", "")), str(p.get("pos_group", "")), int(p.get("stats", {}).get("speed", 0))])
	return "\n".join(lines)


class FormationMiniPitch extends Control:
	var formation_name: String = "4-4-2"
	var team_color: Color = Color.CYAN

	func _draw() -> void:
		draw_rect(Rect2(Vector2.ZERO, size), Color(0.08, 0.34, 0.12), true)
		var line_col := Color(0.9, 0.95, 0.9, 0.75)
		draw_rect(Rect2(Vector2.ZERO, size), line_col, false, 1.5)
		draw_line(Vector2(size.x * 0.5, 0), Vector2(size.x * 0.5, size.y), line_col, 1.0)
		draw_arc(Vector2(size.x * 0.5, size.y * 0.5), size.y * 0.18, 0.0, TAU, 20, line_col, 1.0)
		draw_rect(Rect2(0, size.y * 0.22, size.x * 0.14, size.y * 0.56), line_col, false, 1.0)
		draw_rect(Rect2(size.x * 0.86, size.y * 0.22, size.x * 0.14, size.y * 0.56), line_col, false, 1.0)
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
			draw_circle(pt, 4.8, Color(0.05, 0.08, 0.12))
			draw_circle(pt, 3.6, Color.YELLOW if i == 0 else team_color)
