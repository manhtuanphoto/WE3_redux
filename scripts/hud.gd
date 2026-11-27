class_name MatchHUD
extends CanvasLayer
## In-match scoreboard, player bar, 2D radar minimap, event banners, and pause/fulltime panels.

signal resume_requested
signal rematch_requested
signal main_menu_requested
signal next_half_requested

var _score_label: Label
var _clock_label: Label
var _pos_badge: Label
var _player_label: Label
var _card_badge: ColorRect
var _stats_label: Label
var _stamina_bar: ProgressBar
var _power_bar: ProgressBar
var _banner_panel: PanelContainer
var _banner_label: Label
var _overlay_panel: PanelContainer
var _overlay_title: Label
var _overlay_stats: Label
var _btn_primary: Button
var _btn_rematch: Button
var _btn_menu: Button
var _radar: RadarWidget
var _log_panel: PanelContainer
var _log_label: Label
var _replay_badge: Label
var _loss_time_rect: TextureRect
var _card_toast: PanelContainer
var _card_toast_rect: ColorRect
var _card_toast_label: Label
var _card_toast_timer: float = 0.0

var _banner_timer: float = 0.0
var _banner_state: int = 0   # func_8004F82C: 0 = slide in, 1 = hold, 2 = slide out
var _banner_y: float = -70.0
var _players_ref: Array[Player] = []
var _ball_ref: Ball = null
var _controlled_ref: Player = null

func _ready() -> void:
	_build_ui()
	MatchState.event_logged.connect(_on_event_logged)
	_refresh_log()

func _on_event_logged(entry: String) -> void:
	_refresh_log()
	if _card_toast == null:
		return
	if "RED CARD!" in entry:
		_card_toast_rect.color = Color(0.95, 0.16, 0.16)
		_card_toast_label.text = entry.substr(entry.find("]") + 2) if "]" in entry else entry
		_card_toast.visible = true
		_card_toast_timer = 2.6
	elif "YELLOW CARD:" in entry:
		_card_toast_rect.color = Color(0.98, 0.84, 0.12)
		_card_toast_label.text = entry.substr(entry.find("]") + 2) if "]" in entry else entry
		_card_toast.visible = true
		_card_toast_timer = 2.2

func _input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		if (event as InputEventKey).keycode == KEY_F12 and _log_panel != null:
			_log_panel.visible = not _log_panel.visible

func _refresh_log() -> void:
	if _log_label != null:
		_log_label.text = "EVENT LOG (F12)\n" + "\n".join(MatchState.event_log)

func bind_entities(players: Array[Player], ball: Ball) -> void:
	_players_ref = players
	_ball_ref = ball
	if _radar != null:
		_radar.players = players
		_radar.ball = ball

func set_controlled_player(p: Player) -> void:
	_controlled_ref = p

func set_replay_badge(active: bool) -> void:
	if _replay_badge != null:
		_replay_badge.visible = active

func show_banner(text: String, duration: float = 2.0) -> void:
	if _banner_panel == null or _banner_label == null:
		return
	_banner_label.text = text
	_banner_panel.visible = true
	_banner_timer = duration
	_banner_state = 0
	_banner_y = -70.0
	_banner_panel.offset_top = _banner_y
	_banner_panel.offset_bottom = _banner_y + 66.0

func show_pause_menu(show_panel: bool) -> void:
	if _overlay_panel == null:
		return
	_overlay_panel.visible = show_panel
	if show_panel:
		_overlay_title.text = "PAUSED"
		_overlay_stats.text = _build_stats_text()
		_btn_primary.text = "Resume Match"
		_btn_primary.visible = true
		_btn_rematch.visible = true

func show_half_or_full_time(is_fulltime: bool) -> void:
	if _overlay_panel == null:
		return
	_overlay_panel.visible = true
	_overlay_title.text = "FULL TIME" if is_fulltime else "HALF TIME"
	_overlay_stats.text = _build_stats_text()
	_btn_primary.text = "Start 2nd Half"
	_btn_primary.visible = not is_fulltime
	_btn_rematch.visible = true

func _build_stats_text() -> String:
	var h_abbr := GameData.get_team_abbr(GameData.home_team_id)
	var a_abbr := GameData.get_team_abbr(GameData.away_team_id)
	return "%s  %d  -  %d  %s\n\nShots: %d  |  %d\nPasses: %d  |  %d\nPossession: %d%%  |  %d%%" % [
		h_abbr, MatchState.home_score, MatchState.away_score, a_abbr,
		MatchState.shots[0], MatchState.shots[1],
		MatchState.passes_completed[0], MatchState.passes_completed[1],
		MatchState.get_possession_percent(0), MatchState.get_possession_percent(1)
	]

func _process(delta: float) -> void:
	var h_abbr := GameData.get_team_abbr(GameData.home_team_id)
	var a_abbr := GameData.get_team_abbr(GameData.away_team_id)
	_score_label.text = "%s  %d  -  %d  %s" % [h_abbr, MatchState.home_score, MatchState.away_score, a_abbr]
	var half_str := "1ST" if MatchState.current_half == 1 else "2ND"
	var spd_str := " [2X]" if Engine.time_scale > 1.5 else ""
	var added_str := "+" if (MatchState.match_elapsed_real > GameData.half_duration_seconds and MatchState.phase == MatchState.Phase.OPEN_PLAY) else ""
	_clock_label.text = "%s %02d'%s%s" % [half_str, MatchState.get_display_minutes(), added_str, spd_str]
	if _loss_time_rect != null:
		_loss_time_rect.visible = added_str != ""

	var shown_player: Player = _controlled_ref
	if (shown_player == null or not is_instance_valid(shown_player)) and _ball_ref != null:
		if _ball_ref.owner_player is Player and is_instance_valid(_ball_ref.owner_player):
			shown_player = _ball_ref.owner_player as Player
		elif _ball_ref.last_touch_player is Player and is_instance_valid(_ball_ref.last_touch_player):
			shown_player = _ball_ref.last_touch_player as Player

	if shown_player != null and is_instance_valid(shown_player):
		var role_names := ["GK", "DF", "MF", "FW"]
		var role_cols := [Color(0.98, 0.82, 0.18), Color(0.28, 0.60, 0.98), Color(0.26, 0.88, 0.42), Color(0.96, 0.30, 0.24)]
		var r_idx := clampi(shown_player.role, 0, 3)
		if _pos_badge != null:
			_pos_badge.visible = true
			_pos_badge.text = "%s %02d" % [role_names[r_idx], shown_player.shirt_number if shown_player.shirt_number > 0 else shown_player.slot_index + 1]
			_pos_badge.add_theme_color_override("font_color", role_cols[r_idx])
		_player_label.text = shown_player.player_name.to_upper()
		if _card_badge != null:
			_card_badge.visible = shown_player.yellow_cards > 0
		if _stats_label != null:
			_stats_label.visible = true
			_stats_label.text = "SPD %d  ACC %d  KCK %d  TEC %d  STA %d" % [
				shown_player.stats["speed"], shown_player.stats["acceleration"], shown_player.stats["kick_power"],
				shown_player.stats["technique"], shown_player.stats["stamina"]]
		_stamina_bar.visible = true
		_stamina_bar.value = shown_player.stamina
		_stamina_bar.modulate = Color(0.25, 0.90, 0.35) if shown_player.stamina > 50.0 else (Color(0.98, 0.80, 0.15) if shown_player.stamina > 22.0 else Color(0.95, 0.25, 0.20))
		_power_bar.visible = shown_player.is_charging_shot
		_power_bar.value = shown_player.shot_charge * 100.0
	elif GameData.is_ai_vs_ai:
		if _pos_badge != null:
			_pos_badge.visible = true
			_pos_badge.text = "CPU"
			_pos_badge.add_theme_color_override("font_color", Color(0.95, 0.82, 0.22))
		_player_label.text = "AI vs AI"
		if _stats_label != null:
			_stats_label.visible = false
		if _card_badge != null:
			_card_badge.visible = false
		_stamina_bar.visible = false
		_power_bar.visible = false

	# func_8004F82C: 3-state vertical slide-in / hold / slide-out HUD banner
	if _banner_panel != null and _banner_panel.visible:
		match _banner_state:
			0:
				_banner_y = move_toward(_banner_y, 90.0, 960.0 * delta)
				if _banner_y >= 89.5:
					_banner_y = 90.0
					_banner_state = 1
			1:
				_banner_timer -= delta
				if _banner_timer <= 0.0:
					_banner_state = 2
			2:
				_banner_y = move_toward(_banner_y, -70.0, 960.0 * delta)
				if _banner_y <= -69.5:
					_banner_panel.visible = false
		_banner_panel.offset_top = _banner_y
		_banner_panel.offset_bottom = _banner_y + 66.0

	if _card_toast_timer > 0.0:
		_card_toast_timer -= delta
		if _card_toast_timer <= 0.0 and _card_toast != null:
			_card_toast.visible = false

	if _radar != null:
		_radar.queue_redraw()

func _build_ui() -> void:
	# WE3 classic bevelled dark-navy & gold broadcast style
	var sb_style := StyleBoxFlat.new()
	sb_style.bg_color = Color(0.04, 0.08, 0.20, 0.88)
	sb_style.border_color = Color(0.88, 0.74, 0.22, 0.95)
	sb_style.set_border_width_all(2)
	sb_style.set_corner_radius_all(4)
	sb_style.content_margin_left = 12
	sb_style.content_margin_right = 12
	sb_style.content_margin_top = 6
	sb_style.content_margin_bottom = 6

	# Top-left WE3 Scoreboard + Clock
	var top_box := PanelContainer.new()
	top_box.position = Vector2(20, 16)
	top_box.add_theme_stylebox_override("panel", sb_style)
	add_child(top_box)

	var hbox := HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 10)
	top_box.add_child(hbox)

	var home_swatch := ColorRect.new()
	home_swatch.color = GameData.get_team_primary_color(GameData.home_team_id)
	home_swatch.custom_minimum_size = Vector2(8, 22)
	hbox.add_child(home_swatch)

	_score_label = Label.new()
	_score_label.add_theme_font_size_override("font_size", 22)
	_score_label.add_theme_color_override("font_color", Color(0.98, 0.98, 1.0))
	hbox.add_child(_score_label)

	var away_swatch := ColorRect.new()
	away_swatch.color = GameData.get_team_primary_color(GameData.away_team_id)
	away_swatch.custom_minimum_size = Vector2(8, 22)
	hbox.add_child(away_swatch)

	var sep := VSeparator.new()
	hbox.add_child(sep)

	_clock_label = Label.new()
	_clock_label.add_theme_font_size_override("font_size", 20)
	_clock_label.add_theme_color_override("font_color", Color(0.98, 0.86, 0.22))
	hbox.add_child(_clock_label)

	# Authentic WE3 "LOSS TIME" sprite (hud_03), shown while injury time is being played
	if ResourceLoader.exists("res://assets/textures/hud/we3_loss_time.png"):
		_loss_time_rect = TextureRect.new()
		_loss_time_rect.texture = load("res://assets/textures/hud/we3_loss_time.png") as Texture2D
		_loss_time_rect.custom_minimum_size = Vector2(98, 20)
		_loss_time_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		_loss_time_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		_loss_time_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		_loss_time_rect.visible = false
		hbox.add_child(_loss_time_rect)

	_replay_badge = Label.new()
	_replay_badge.text = "● REPLAY"
	_replay_badge.add_theme_font_size_override("font_size", 18)
	_replay_badge.add_theme_color_override("font_color", Color(1.0, 0.25, 0.20))
	_replay_badge.visible = false
	hbox.add_child(_replay_badge)

	# Top-right live event log (hidden by default, F12 to toggle)
	_log_panel = PanelContainer.new()
	_log_panel.set_anchors_preset(Control.PRESET_TOP_RIGHT)
	_log_panel.offset_left = -330
	_log_panel.offset_right = -16
	_log_panel.offset_top = 16
	_log_panel.offset_bottom = 260
	_log_panel.visible = false
	_log_panel.add_theme_stylebox_override("panel", sb_style)
	add_child(_log_panel)

	_log_label = Label.new()
	_log_label.add_theme_font_size_override("font_size", 11)
	_log_label.add_theme_color_override("font_color", Color(0.82, 0.96, 0.84))
	_log_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_log_panel.add_child(_log_label)

	# Bottom-left WE3 active player nameplate (Slot 0x30/0x31)
	var bot_left := PanelContainer.new()
	bot_left.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	bot_left.offset_left = 20
	bot_left.offset_right = 276
	bot_left.offset_top = -96
	bot_left.offset_bottom = -16
	bot_left.add_theme_stylebox_override("panel", sb_style)
	add_child(bot_left)

	var vbox_pl := VBoxContainer.new()
	vbox_pl.add_theme_constant_override("separation", 4)
	bot_left.add_child(vbox_pl)

	var pl_row := HBoxContainer.new()
	pl_row.add_theme_constant_override("separation", 10)
	vbox_pl.add_child(pl_row)

	_pos_badge = Label.new()
	_pos_badge.text = "FW 10"
	_pos_badge.add_theme_font_size_override("font_size", 16)
	pl_row.add_child(_pos_badge)

	_player_label = Label.new()
	_player_label.text = "PLAYER"
	_player_label.add_theme_font_size_override("font_size", 18)
	_player_label.add_theme_color_override("font_color", Color(0.98, 0.98, 1.0))
	pl_row.add_child(_player_label)

	_card_badge = ColorRect.new()
	_card_badge.color = Color(0.98, 0.84, 0.12)
	_card_badge.custom_minimum_size = Vector2(10, 14)
	_card_badge.visible = false
	pl_row.add_child(_card_badge)

	_stats_label = Label.new()
	_stats_label.add_theme_font_size_override("font_size", 10)
	_stats_label.add_theme_color_override("font_color", Color(0.72, 0.84, 0.98))
	_stats_label.visible = false
	vbox_pl.add_child(_stats_label)

	_stamina_bar = ProgressBar.new()
	_stamina_bar.max_value = 100.0
	_stamina_bar.value = 100.0
	_stamina_bar.show_percentage = false
	_stamina_bar.custom_minimum_size = Vector2(200, 8)
	vbox_pl.add_child(_stamina_bar)

	_power_bar = ProgressBar.new()
	_power_bar.max_value = 100.0
	_power_bar.value = 0.0
	_power_bar.show_percentage = false
	_power_bar.custom_minimum_size = Vector2(200, 10)
	_power_bar.modulate = Color(1.0, 0.45, 0.12)
	_power_bar.visible = false
	vbox_pl.add_child(_power_bar)

	# Bottom-center 2D Radar
	_radar = RadarWidget.new()
	_radar.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	_radar.custom_minimum_size = Vector2(240, 130)
	_radar.anchor_left = 0.5
	_radar.anchor_right = 0.5
	_radar.anchor_top = 1.0
	_radar.anchor_bottom = 1.0
	_radar.offset_left = -120
	_radar.offset_right = 120
	_radar.offset_top = -146
	_radar.offset_bottom = -16
	add_child(_radar)

	# Center event banner (GOAL / THROW IN / CORNER)
	_banner_panel = PanelContainer.new()
	_banner_panel.set_anchors_preset(Control.PRESET_CENTER_TOP)
	_banner_panel.offset_left = -220
	_banner_panel.offset_right = 220
	_banner_panel.offset_top = 90
	_banner_panel.offset_bottom = 156
	_banner_panel.visible = false
	var ban_style := StyleBoxFlat.new()
	ban_style.bg_color = Color(0.1, 0.15, 0.28, 0.92)
	ban_style.border_color = Color(0.95, 0.8, 0.2)
	ban_style.set_border_width_all(2)
	ban_style.set_corner_radius_all(8)
	_banner_panel.add_theme_stylebox_override("panel", ban_style)
	add_child(_banner_panel)

	_banner_label = Label.new()
	_banner_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_banner_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_banner_label.add_theme_font_size_override("font_size", 28)
	_banner_panel.add_child(_banner_label)

	# Yellow / Red Card Toast (top-right)
	_card_toast = PanelContainer.new()
	_card_toast.set_anchors_preset(Control.PRESET_TOP_RIGHT)
	_card_toast.offset_left = -330
	_card_toast.offset_right = -20
	_card_toast.offset_top = 16
	_card_toast.offset_bottom = 58
	_card_toast.visible = false
	_card_toast.add_theme_stylebox_override("panel", sb_style)
	add_child(_card_toast)
	var ct_row := HBoxContainer.new()
	ct_row.add_theme_constant_override("separation", 10)
	_card_toast.add_child(ct_row)
	_card_toast_rect = ColorRect.new()
	_card_toast_rect.color = Color(0.98, 0.84, 0.12)
	_card_toast_rect.custom_minimum_size = Vector2(14, 20)
	ct_row.add_child(_card_toast_rect)
	_card_toast_label = Label.new()
	_card_toast_label.add_theme_font_size_override("font_size", 15)
	_card_toast_label.add_theme_color_override("font_color", Color(0.98, 0.98, 1.0))
	ct_row.add_child(_card_toast_label)

	# Pause / Halftime / Fulltime Modal
	_overlay_panel = PanelContainer.new()
	_overlay_panel.set_anchors_preset(Control.PRESET_CENTER)
	_overlay_panel.offset_left = -210
	_overlay_panel.offset_right = 210
	_overlay_panel.offset_top = -170
	_overlay_panel.offset_bottom = 170
	_overlay_panel.visible = false
	_overlay_panel.add_theme_stylebox_override("panel", ban_style)
	add_child(_overlay_panel)

	var ov_box := VBoxContainer.new()
	ov_box.add_theme_constant_override("separation", 12)
	_overlay_panel.add_child(ov_box)

	_overlay_title = Label.new()
	_overlay_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_overlay_title.add_theme_font_size_override("font_size", 28)
	ov_box.add_child(_overlay_title)

	_overlay_stats = Label.new()
	_overlay_stats.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_overlay_stats.add_theme_font_size_override("font_size", 16)
	ov_box.add_child(_overlay_stats)

	_btn_primary = Button.new()
	_btn_primary.text = "Resume"
	_btn_primary.pressed.connect(func():
		if MatchState.phase == MatchState.Phase.HALFTIME:
			next_half_requested.emit()
		else:
			resume_requested.emit()
	)
	ov_box.add_child(_btn_primary)

	_btn_rematch = Button.new()
	_btn_rematch.text = "Restart Match"
	_btn_rematch.pressed.connect(func(): rematch_requested.emit())
	ov_box.add_child(_btn_rematch)

	_btn_menu = Button.new()
	_btn_menu.text = "Main Menu"
	_btn_menu.pressed.connect(func(): main_menu_requested.emit())
	ov_box.add_child(_btn_menu)


class RadarWidget extends Control:
	var players: Array[Player] = []
	var ball: Ball = null
	var _pitch_tex: Texture2D = null

	func _ready() -> void:
		if ResourceLoader.exists("res://assets/textures/hud/we3_radar_pitch.png"):
			_pitch_tex = load("res://assets/textures/hud/we3_radar_pitch.png") as Texture2D
		texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST

	func _draw() -> void:
		var rect := Rect2(Vector2.ZERO, size)
		# Classic WE3 translucent dark-green radar pitch backed by real WE3 hud_03 radar sprite
		draw_rect(rect, Color(0.03, 0.16, 0.06, 0.56), true)
		if _pitch_tex != null:
			draw_texture_rect(_pitch_tex, rect, false)
		else:
			var line_col := Color(0.92, 0.96, 0.92, 0.65)
			draw_rect(rect, line_col, false, 1.5)
			draw_line(Vector2(size.x * 0.5, 0.0), Vector2(size.x * 0.5, size.y), line_col, 1.0)
			draw_arc(Vector2(size.x * 0.5, size.y * 0.5), size.y * 0.14, 0.0, TAU, 24, line_col, 1.0)

		var c0 := GameData.get_team_primary_color(GameData.home_team_id)
		var c1 := GameData.get_team_primary_color(GameData.away_team_id)
		for p in players:
			if p == null or not is_instance_valid(p) or p.is_sent_off:
				continue
			var u := (p.global_position.x + PitchBuilder.HALF_LEN) / PitchBuilder.PITCH_LENGTH
			var v := (p.global_position.z + PitchBuilder.HALF_WID) / PitchBuilder.PITCH_WIDTH
			var pt := Vector2(clampf(u, 0.02, 0.98) * size.x, clampf(v, 0.02, 0.98) * size.y)
			var col := Color(0.95, 0.85, 0.2) if p.role == Player.Role.GK else (c0 if p.team_side == 0 else c1)
			draw_circle(pt, 4.0, Color(0.0, 0.0, 0.0, 0.85))
			draw_circle(pt, 3.0, col)
			if p.is_controlled or (ball != null and ball.owner_player == p):
				draw_arc(pt, 5.4, 0.0, TAU, 16, Color.WHITE, 1.4)

		if ball != null and is_instance_valid(ball):
			var bu := (ball.global_position.x + PitchBuilder.HALF_LEN) / PitchBuilder.PITCH_LENGTH
			var bv := (ball.global_position.z + PitchBuilder.HALF_WID) / PitchBuilder.PITCH_WIDTH
			var bpt := Vector2(clampf(bu, 0.01, 0.99) * size.x, clampf(bv, 0.01, 0.99) * size.y)
			draw_circle(bpt, 3.6, Color(0.0, 0.0, 0.0, 0.9))
			draw_circle(bpt, 2.6, Color(1.0, 0.96, 0.35))
