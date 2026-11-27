extends Control
## Winning Eleven 3 Final Ver. Title Screen & Authentic Mode Select Menu.

const WE3_MODES := [
	{"id": "exhibition", "label": "EXHIBITION MATCH", "desc": "1P vs COM / 2P Friendly Match"},
	{"id": "world_cup", "label": "WORLD CUP", "desc": "France '98 International Tournament"},
	{"id": "league", "label": "INTERNATIONAL LEAGUE", "desc": "Round-Robin League Championship"},
	{"id": "cup", "label": "INTERNATIONAL CUP", "desc": "Knockout Cup Tournament"},
	{"id": "pk", "label": "PK SHOOTOUT", "desc": "Penalty Kick Challenge"},
	{"id": "training", "label": "TRAINING", "desc": "Free Practice & Set Pieces"},
	{"id": "all_star", "label": "ALL-STAR MATCH", "desc": "World All-Stars vs European All-Stars"},
	{"id": "option", "label": "OPTION", "desc": "Match Settings, Stadium, Sound & Difficulty"},
]

var _on_title_screen: bool = true
var _selected_idx: int = 0
var _blink_time: float = 0.0

var _title_layer: Control
var _press_start_node: CanvasItem
var _mode_layer: Control
var _mode_buttons: Array[Button] = []
var _desc_lbl: Label

func _ready() -> void:
	_build_ui()
	SoundFX.stop_crowd_ambience()
	SoundFX.play_music("02TITLEV")

func _process(delta: float) -> void:
	if _on_title_screen and _press_start_node != null:
		_blink_time += delta
		_press_start_node.modulate.a = 1.0 if int(_blink_time * 2.4) % 2 == 0 else 0.15

func _input(event: InputEvent) -> void:
	if _on_title_screen:
		if (event is InputEventKey and event.pressed and not event.echo) or (event is InputEventMouseButton and event.pressed):
			_enter_mode_select()
		return

	if event is InputEventKey and event.pressed and not event.echo:
		var ke := event as InputEventKey
		if ke.keycode == KEY_UP or ke.keycode == KEY_W:
			_set_selected((_selected_idx - 1 + WE3_MODES.size()) % WE3_MODES.size())
		elif ke.keycode == KEY_DOWN or ke.keycode == KEY_S:
			_set_selected((_selected_idx + 1) % WE3_MODES.size())
		elif ke.keycode == KEY_ENTER or ke.keycode == KEY_SPACE or ke.keycode == KEY_J:
			_activate_mode(WE3_MODES[_selected_idx]["id"])
		elif ke.keycode == KEY_ESCAPE:
			_return_to_title()

func _enter_mode_select() -> void:
	_on_title_screen = false
	_title_layer.visible = false
	_mode_layer.visible = true
	SoundFX.play_sfx("kick")
	SoundFX.play_music("08SELECT")
	_set_selected(0)

func _return_to_title() -> void:
	_on_title_screen = true
	_title_layer.visible = true
	_mode_layer.visible = false
	SoundFX.play_music("02TITLEV")

func _set_selected(idx: int) -> void:
	_selected_idx = idx
	for i in range(_mode_buttons.size()):
		var btn := _mode_buttons[i]
		var prefix := "▶  " if i == _selected_idx else "    "
		btn.text = prefix + WE3_MODES[i]["label"]
		if i == _selected_idx:
			btn.add_theme_color_override("font_color", Color(1.0, 0.88, 0.22))
		else:
			btn.add_theme_color_override("font_color", Color(0.88, 0.92, 0.98))
	if _desc_lbl != null:
		_desc_lbl.text = WE3_MODES[_selected_idx]["desc"]

func _activate_mode(mode_id: String) -> void:
	GameData.is_ai_vs_ai = false
	GameData.human_side = 0
	GameData.return_scene = "res://scenes/main_menu.tscn"
	SoundFX.play_sfx("whistle")
	match mode_id:
		"exhibition", "world_cup", "league", "cup", "option":
			get_tree().change_scene_to_file("res://scenes/team_select.tscn")
		"all_star":
			GameData.home_team_id = 40
			GameData.away_team_id = 41
			get_tree().change_scene_to_file("res://scenes/match.tscn")
		"pk", "training":
			get_tree().change_scene_to_file("res://scenes/match.tscn")

func _build_ui() -> void:
	# Authentic WE3 1998 Title Screen Artwork (decoded from TITLE.BIN)
	var bg_tex := load("res://assets/textures/ui_and_bg/TITLE/we3_title_bg.png") as Texture2D
	if bg_tex != null:
		var bg_img := TextureRect.new()
		bg_img.set_anchors_preset(Control.PRESET_FULL_RECT)
		bg_img.texture = bg_tex
		bg_img.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		bg_img.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		bg_img.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		add_child(bg_img)
	else:
		var bg := ColorRect.new()
		bg.set_anchors_preset(Control.PRESET_FULL_RECT)
		bg.color = Color(0.02, 0.06, 0.18)
		add_child(bg)

	# Subtle dark vignette so WE3 logo & menu text stay razor-sharp
	var dim := ColorRect.new()
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	dim.color = Color(0.02, 0.05, 0.14, 0.32)
	add_child(dim)

	# === STAGE 1: WE3 TITLE SCREEN ===
	_title_layer = Control.new()
	_title_layer.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(_title_layer)

	var t_vbox := VBoxContainer.new()
	t_vbox.set_anchors_preset(Control.PRESET_CENTER)
	t_vbox.offset_left = -380
	t_vbox.offset_right = 380
	t_vbox.offset_top = -180
	t_vbox.offset_bottom = 180
	t_vbox.alignment = BoxContainer.ALIGNMENT_CENTER
	t_vbox.add_theme_constant_override("separation", 18)
	_title_layer.add_child(t_vbox)

	var logo_tex := load("res://assets/textures/ui_and_bg/TITLE/we3_logo_full.png") as Texture2D
	if logo_tex != null:
		var logo_rect := TextureRect.new()
		logo_rect.texture = logo_tex
		logo_rect.custom_minimum_size = Vector2(520, 220)
		logo_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		logo_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		logo_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		t_vbox.add_child(logo_rect)
	else:
		var main_logo := Label.new()
		main_logo.text = "WINNING ELEVEN 3\nFINAL VER."
		main_logo.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		main_logo.add_theme_font_size_override("font_size", 48)
		main_logo.add_theme_color_override("font_color", Color(1.0, 0.84, 0.16))
		t_vbox.add_child(main_logo)

	var spacer := Control.new()
	spacer.custom_minimum_size = Vector2(0, 24)
	t_vbox.add_child(spacer)

	var ps_tex := load("res://assets/textures/ui_and_bg/TITLE/we3_press_start.png") as Texture2D
	if ps_tex != null:
		var ps_rect := TextureRect.new()
		ps_rect.texture = ps_tex
		ps_rect.custom_minimum_size = Vector2(288, 40)
		ps_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		ps_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		ps_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		t_vbox.add_child(ps_rect)
		_press_start_node = ps_rect
	else:
		var ps_lbl := Label.new()
		ps_lbl.text = "PRESS START BUTTON"
		ps_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		ps_lbl.add_theme_font_size_override("font_size", 22)
		ps_lbl.add_theme_color_override("font_color", Color(1.0, 1.0, 1.0))
		t_vbox.add_child(ps_lbl)
		_press_start_node = ps_lbl

	# === STAGE 2: WE3 MAIN MODE SELECT ===
	_mode_layer = Control.new()
	_mode_layer.set_anchors_preset(Control.PRESET_FULL_RECT)
	_mode_layer.visible = false
	add_child(_mode_layer)
	# Mode select uses the real WE3 stadium backdrop (DATBG00)
	if ResourceLoader.exists("res://assets/textures/ui_and_bg/BACKGROUNDS/we3_bg_stadium_day.png"):
		var mbg := TextureRect.new()
		mbg.set_anchors_preset(Control.PRESET_FULL_RECT)
		mbg.texture = load("res://assets/textures/ui_and_bg/BACKGROUNDS/we3_bg_stadium_day.png") as Texture2D
		mbg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		mbg.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		mbg.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		_mode_layer.add_child(mbg)
		var mdim := ColorRect.new()
		mdim.set_anchors_preset(Control.PRESET_FULL_RECT)
		mdim.color = Color(0.02, 0.05, 0.14, 0.38)
		_mode_layer.add_child(mdim)

	var header := Label.new()
	header.text = "MAIN MENU  —  SELECT GAME MODE"
	header.set_anchors_preset(Control.PRESET_TOP_WIDE)
	header.offset_top = 36
	header.offset_bottom = 78
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 28)
	header.add_theme_color_override("font_color", Color(1.0, 0.86, 0.20))
	_mode_layer.add_child(header)

	var menu_box := VBoxContainer.new()
	menu_box.set_anchors_preset(Control.PRESET_CENTER)
	menu_box.offset_left = -220
	menu_box.offset_right = 220
	menu_box.offset_top = -170
	menu_box.offset_bottom = 170
	menu_box.add_theme_constant_override("separation", 10)
	_mode_layer.add_child(menu_box)

	var btn_style := StyleBoxFlat.new()
	btn_style.bg_color = Color(0.05, 0.12, 0.30, 0.90)
	btn_style.border_color = Color(0.82, 0.72, 0.25, 0.85)
	btn_style.set_border_width_all(2)
	btn_style.set_corner_radius_all(3)
	btn_style.content_margin_left = 18
	btn_style.content_margin_top = 8
	btn_style.content_margin_bottom = 8

	for i in range(WE3_MODES.size()):
		var btn := Button.new()
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		btn.custom_minimum_size = Vector2(440, 44)
		btn.add_theme_font_size_override("font_size", 20)
		btn.add_theme_stylebox_override("normal", btn_style)
		btn.add_theme_stylebox_override("hover", btn_style)
		btn.add_theme_stylebox_override("focus", btn_style)
		var idx_cap := i
		btn.mouse_entered.connect(func(): _set_selected(idx_cap))
		btn.pressed.connect(func(): _activate_mode(WE3_MODES[idx_cap]["id"]))
		menu_box.add_child(btn)
		_mode_buttons.append(btn)

	# Bottom WE3 description ticker bar
	var desc_bar := PanelContainer.new()
	desc_bar.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	desc_bar.offset_top = -64
	desc_bar.offset_bottom = -16
	desc_bar.offset_left = 60
	desc_bar.offset_right = -60
	desc_bar.add_theme_stylebox_override("panel", btn_style)
	_mode_layer.add_child(desc_bar)

	_desc_lbl = Label.new()
	_desc_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_desc_lbl.add_theme_font_size_override("font_size", 16)
	_desc_lbl.add_theme_color_override("font_color", Color(0.92, 0.96, 1.0))
	desc_bar.add_child(_desc_lbl)
