extends Control
## Team Select screen for choosing Home & Away teams from all 43 national teams.

var _home_opt: OptionButton
var _away_opt: OptionButton
var _half_opt: OptionButton
var _home_kit_rect: TextureRect
var _away_kit_rect: TextureRect
var _home_roster_label: Label
var _away_roster_label: Label
var _formation_preview: FormationMiniPitch

func _ready() -> void:
	_build_ui()
	SoundFX.play_music("08SELECT")

func _build_ui() -> void:
	var bg := ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.05, 0.09, 0.18)
	add_child(bg)

	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(center)

	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(720, 460)
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.09, 0.14, 0.26, 0.95)
	style.border_color = Color(0.95, 0.80, 0.18)
	style.set_border_width_all(2)
	style.set_corner_radius_all(10)
	style.set_content_margin_all(24)
	panel.add_theme_stylebox_override("panel", style)
	center.add_child(panel)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 18)
	panel.add_child(vbox)

	var title := Label.new()
	title.text = "EXHIBITION MATCH SETUP"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 28)
	title.add_theme_color_override("font_color", Color(0.96, 0.84, 0.22))
	vbox.add_child(title)

	var hbox := HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 36)
	hbox.alignment = BoxContainer.ALIGNMENT_CENTER
	vbox.add_child(hbox)

	# Home side
	var home_col := VBoxContainer.new()
	home_col.add_theme_constant_override("separation", 10)
	hbox.add_child(home_col)
	var h_lbl := Label.new()
	h_lbl.text = "HOME (YOU)"
	h_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	home_col.add_child(h_lbl)
	_home_opt = OptionButton.new()
	_home_opt.custom_minimum_size = Vector2(240, 38)
	home_col.add_child(_home_opt)
	_home_kit_rect = TextureRect.new()
	_home_kit_rect.custom_minimum_size = Vector2(128, 128)
	_home_kit_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_home_kit_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	home_col.add_child(_home_kit_rect)
	_home_roster_label = Label.new()
	_home_roster_label.add_theme_font_size_override("font_size", 11)
	_home_roster_label.add_theme_color_override("font_color", Color(0.82, 0.88, 0.95))
	home_col.add_child(_home_roster_label)

	# Away side
	var away_col := VBoxContainer.new()
	away_col.add_theme_constant_override("separation", 10)
	hbox.add_child(away_col)
	var a_lbl := Label.new()
	a_lbl.text = "AWAY (CPU)"
	a_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	away_col.add_child(a_lbl)
	_away_opt = OptionButton.new()
	_away_opt.custom_minimum_size = Vector2(240, 38)
	away_col.add_child(_away_opt)
	_away_kit_rect = TextureRect.new()
	_away_kit_rect.custom_minimum_size = Vector2(128, 128)
	_away_kit_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_away_kit_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	away_col.add_child(_away_kit_rect)
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
	for s_name in GameData.STADIUM_NAMES:
		stad_opt.add_item(s_name)
	stad_opt.select(maxi(0, GameData.STADIUM_NAMES.find(GameData.selected_stadium)))
	stad_opt.item_selected.connect(func(idx): GameData.selected_stadium = GameData.STADIUM_NAMES[idx])
	opt_row.add_child(stad_opt)

	var form_row := HBoxContainer.new()
	form_row.alignment = BoxContainer.ALIGNMENT_CENTER
	form_row.add_theme_constant_override("separation", 18)
	vbox.add_child(form_row)

	var form_lbl := Label.new()
	form_lbl.text = "Formation:"
	form_row.add_child(form_lbl)
	var form_opt := OptionButton.new()
	for f_name in GameData.FORMATIONS:
		form_opt.add_item(f_name)
	form_opt.select(maxi(0, GameData.FORMATIONS.find(GameData.home_formation)))
	form_opt.item_selected.connect(func(idx):
		GameData.home_formation = GameData.FORMATIONS[idx]
		if _formation_preview != null:
			_formation_preview.formation_name = GameData.home_formation
			_formation_preview.queue_redraw()
	)
	form_row.add_child(form_opt)

	_formation_preview = FormationMiniPitch.new()
	_formation_preview.custom_minimum_size = Vector2(220, 92)
	_formation_preview.formation_name = GameData.home_formation
	form_row.add_child(_formation_preview)

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
		GameData.is_ai_vs_ai = false
		get_tree().change_scene_to_file("res://scenes/match.tscn")
	)
	btn_row.add_child(btn_kickoff)

func _find_item_idx(opt: OptionButton, id: int) -> int:
	for i in range(opt.item_count):
		if opt.get_item_id(i) == id:
			return i
	return 0

func _sync_selection() -> void:
	if _home_opt.item_count > 0:
		GameData.home_team_id = _home_opt.get_selected_id()
		_home_kit_rect.texture = GameData.get_team_kit_texture(GameData.home_team_id)
		_home_roster_label.text = _format_starting_xi(GameData.home_team_id)
	if _away_opt.item_count > 0:
		GameData.away_team_id = _away_opt.get_selected_id()
		_away_kit_rect.texture = GameData.get_team_kit_texture(GameData.away_team_id)
		_away_roster_label.text = _format_starting_xi(GameData.away_team_id)

func _format_starting_xi(tid: int) -> String:
	var t := GameData.get_team(tid)
	var players: Array = t.get("players", [])
	var lines: PackedStringArray = []
	for i in range(mini(11, players.size())):
		var p: Dictionary = players[i]
		lines.append("%2d. %s" % [i + 1, str(p.get("name", ""))])
	return "\n".join(lines)


class FormationMiniPitch extends Control:
	var formation_name: String = "4-4-2"

	func _draw() -> void:
		draw_rect(Rect2(Vector2.ZERO, size), Color(0.08, 0.34, 0.12), true)
		draw_rect(Rect2(Vector2.ZERO, size), Color(0.9, 0.95, 0.9), false, 1.5)
		draw_line(Vector2(size.x * 0.5, 0), Vector2(size.x * 0.5, size.y), Color(0.9, 0.95, 0.9, 0.6), 1.0)
		var pts := {
			"4-4-2": [Vector2(0.08,0.5), Vector2(0.25,0.2), Vector2(0.22,0.4), Vector2(0.22,0.6), Vector2(0.25,0.8), Vector2(0.5,0.18), Vector2(0.46,0.4), Vector2(0.46,0.6), Vector2(0.5,0.82), Vector2(0.76,0.38), Vector2(0.76,0.62)],
			"4-3-3": [Vector2(0.08,0.5), Vector2(0.25,0.2), Vector2(0.22,0.4), Vector2(0.22,0.6), Vector2(0.25,0.8), Vector2(0.48,0.28), Vector2(0.44,0.5), Vector2(0.48,0.72), Vector2(0.78,0.2), Vector2(0.82,0.5), Vector2(0.78,0.8)],
			"3-5-2": [Vector2(0.08,0.5), Vector2(0.24,0.26), Vector2(0.22,0.5), Vector2(0.24,0.74), Vector2(0.52,0.14), Vector2(0.46,0.34), Vector2(0.54,0.5), Vector2(0.46,0.66), Vector2(0.52,0.86), Vector2(0.76,0.38), Vector2(0.76,0.62)],
		}
		var list: Array = pts.get(formation_name, pts["4-4-2"])
		for i in range(list.size()):
			var pt: Vector2 = Vector2(list[i].x * size.x, list[i].y * size.y)
			draw_circle(pt, 4.0, Color.YELLOW if i == 0 else Color.CYAN)
