class_name MatchHUD
extends CanvasLayer
## In-match scoreboard, player bar, 2D radar minimap, event banners, and pause/fulltime panels.

signal resume_requested
signal rematch_requested
signal main_menu_requested
signal next_half_requested
signal next_cup_round_requested
signal substitution_requested(starter_slot: int, bench_idx: int)

var _score_label: Label
var _home_abbr_lbl: Label
var _away_abbr_lbl: Label
var _home_digit_rect: TextureRect
var _away_digit_rect: TextureRect
var _digits_tex: Texture2D = null
var _clock_label: Label
var _pos_badge: Label
var _player_label: Label
var _card_badge: ColorRect
var _stats_label: Label
var _stamina_bar: ProgressBar
var _power_bar: ProgressBar
var _p2_panel: PanelContainer
var _p2_pos_badge: Label
var _p2_player_label: Label
var _p2_card_badge: ColorRect
var _p2_stamina_bar: ProgressBar
var _p2_power_bar: ProgressBar
var _banner_panel: PanelContainer
var _banner_label: Label
var _overlay_panel: PanelContainer
var _overlay_title: Label
var _overlay_stats: Label
var _btn_primary: Button
var _btn_rematch: Button
var _btn_menu: Button
var _sub_box: HBoxContainer
var _sub_out_opt: OptionButton
var _sub_in_opt: OptionButton
var _btn_sub: Button
var _radar: RadarWidget
var _log_panel: PanelContainer
var _log_label: Label
var _replay_badge: Label
var _tactic_badge: Label
var _loss_time_rect: TextureRect
var _pk_panel: PanelContainer
var _pk_home_lbl: Label
var _pk_away_lbl: Label
var _prematch_panel: PanelContainer
var _prematch_label: Label
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
var _controlled_p2_ref: Player = null

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

func set_controlled_player_p2(p: Player) -> void:
	_controlled_p2_ref = p

func show_prematch_card(home_name: String, away_name: String, home_form: String, away_form: String, stadium_name: String) -> void:
	if _prematch_panel == null or _prematch_label == null:
		return
	var arrows: Array[String] = ["↓", "↘", "→", "↗", "↑"]
	var hc: String = arrows[clampi(GameData.team_condition[0], 0, 4)]
	var ac: String = arrows[clampi(GameData.team_condition[1], 0, 4)]
	_prematch_label.text = "%s [%s %s]  VS  %s [%s %s]\nSTADIUM: %s" % [home_name, home_form, hc, away_name, away_form, ac, stadium_name]
	_prematch_panel.visible = true

func hide_prematch_card() -> void:
	if _prematch_panel != null:
		_prematch_panel.visible = false

func set_replay_badge(active: bool) -> void:
	if _replay_badge != null:
		_replay_badge.visible = active

func update_tactics(tactic_level: int, offside_trap: bool) -> void:
	if _tactic_badge == null:
		return
	var tags: Array[String] = ["DEF", "BAL", "ATK"]
	var txt: String = tags[clampi(tactic_level, 0, 2)]
	if offside_trap:
		txt += "+TRAP"
	_tactic_badge.text = txt

func update_pk_board(results: Array, active: bool) -> void:
	if _pk_panel == null:
		return
	_pk_panel.visible = active
	if not active or results.size() < 2:
		return
	var h_res: Array = results[0]
	var a_res: Array = results[1]
	var slots := maxi(5, maxi(h_res.size(), a_res.size()))
	var h_abbr := GameData.get_team_abbr(GameData.home_team_id)
	var a_abbr := GameData.get_team_abbr(GameData.away_team_id)
	_pk_home_lbl.text = "%-3s %s  (%d)" % [h_abbr, _format_pk_dots(h_res, slots), h_res.count("O")]
	_pk_away_lbl.text = "%-3s %s  (%d)" % [a_abbr, _format_pk_dots(a_res, slots), a_res.count("O")]

func _format_pk_dots(res: Array, slots: int) -> String:
	var out: Array[String] = []
	for i in range(slots):
		if i < res.size():
			out.append("●" if res[i] == "O" else "✕")
		else:
			out.append("○")
	return " ".join(out)

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
		_refresh_sub_controls(true)

func show_half_or_full_time(is_fulltime: bool) -> void:
	if _overlay_panel == null:
		return
	_overlay_panel.visible = true
	var can_advance_cup := false
	if is_fulltime and GameData.cup_active:
		var won := MatchState.home_score > MatchState.away_score
		if won and GameData.cup_round < 2:
			_overlay_title.text = "%s WIN!" % GameData.get_cup_round_name()
			can_advance_cup = true
		elif won:
			_overlay_title.text = "CUP CHAMPION!!"
		else:
			_overlay_title.text = "ELIMINATED (%s)" % GameData.get_cup_round_name()
	elif is_fulltime:
		_overlay_title.text = "FULL TIME"
	elif MatchState.current_half == 2:
		_overlay_title.text = "END OF 90 MIN — EXTRA TIME"
	elif MatchState.current_half == 3:
		_overlay_title.text = "EXTRA TIME HALF"
	else:
		_overlay_title.text = "HALF TIME"
	_overlay_stats.text = _build_stats_text()
	if can_advance_cup:
		_btn_primary.text = "Next Round"
		_btn_primary.visible = true
	else:
		_btn_primary.text = "Start Extra Time" if (not is_fulltime and MatchState.current_half >= 2) else "Start 2nd Half"
		_btn_primary.visible = not is_fulltime
	_btn_rematch.visible = true
	_refresh_sub_controls(not is_fulltime)

func _refresh_sub_controls(allow: bool) -> void:
	if _sub_box == null:
		return
	var can_sub := allow and not GameData.is_ai_vs_ai and MatchState.subs_used[0] < 3 and not _players_ref.is_empty()
	_sub_box.visible = can_sub
	if not can_sub:
		return
	_sub_out_opt.clear()
	for p in _players_ref:
		if p.team_side == 0 and not p.is_sent_off:
			_sub_out_opt.add_item("%d. %s (%d%%)" % [p.shirt_number, p.player_name, int(p.stamina)], p.slot_index)
	_sub_in_opt.clear()
	var roster: Array = GameData.get_team(GameData.home_team_id).get("players", [])
	for idx in range(11, roster.size()):
		var rec: Dictionary = roster[idx]
		_sub_in_opt.add_item("%d. %s (%s)" % [int(rec.get("shirt_number", idx + 1)), str(rec.get("name", "")), str(rec.get("pos_group", ""))], idx)
	_btn_sub.text = "Sub (%d/3)" % MatchState.subs_used[0]

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
	if _digits_tex != null and MatchState.home_score <= 7 and MatchState.away_score <= 7:
		_home_abbr_lbl.visible = true
		_away_abbr_lbl.visible = true
		_home_abbr_lbl.text = h_abbr
		_away_abbr_lbl.text = a_abbr
		_home_digit_rect.visible = true
		_away_digit_rect.visible = true
		(_home_digit_rect.texture as AtlasTexture).region = Rect2(MatchState.home_score * 16, 0, 16, 22)
		(_away_digit_rect.texture as AtlasTexture).region = Rect2(MatchState.away_score * 16, 0, 16, 22)
		_score_label.text = "-"
	else:
		_home_abbr_lbl.visible = false
		_away_abbr_lbl.visible = false
		_home_digit_rect.visible = false
		_away_digit_rect.visible = false
		_score_label.text = "%s  %d  -  %d  %s" % [h_abbr, MatchState.home_score, MatchState.away_score, a_abbr]
	var half_names := {1: "1ST", 2: "2ND", 3: "ET1", 4: "ET2"}
	var half_str: String = half_names.get(MatchState.current_half, "2ND")
	var spd_str := " [2X]" if Engine.time_scale > 1.5 else ""
	var half_dur := GameData.half_duration_seconds * (0.3333 if MatchState.current_half >= 3 else 1.0)
	var added_str := "+" if (MatchState.match_elapsed_real > half_dur and MatchState.phase == MatchState.Phase.OPEN_PLAY) else ""
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
		_update_nameplate(shown_player, _pos_badge, _player_label, _card_badge, _stats_label, _stamina_bar, _power_bar)
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

	if _p2_panel != null:
		_p2_panel.visible = GameData.is_two_player and _controlled_p2_ref != null and is_instance_valid(_controlled_p2_ref)
		if _p2_panel.visible:
			_update_nameplate(_controlled_p2_ref, _p2_pos_badge, _p2_player_label, _p2_card_badge, null, _p2_stamina_bar, _p2_power_bar)

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

func _update_nameplate(p: Player, pos_lbl: Label, name_lbl: Label, card_rect: ColorRect, stats_lbl: Label, stam_bar: ProgressBar, pow_bar: ProgressBar) -> void:
	var role_names: Array[String] = ["GK", "DF", "MF", "FW"]
	var role_cols: Array[Color] = [Color(0.98, 0.82, 0.18), Color(0.28, 0.60, 0.98), Color(0.26, 0.88, 0.42), Color(0.96, 0.30, 0.24)]
	var r_idx := clampi(p.role, 0, 3)
	if pos_lbl != null:
		pos_lbl.visible = true
		pos_lbl.text = "%s %02d" % [role_names[r_idx], p.shirt_number if p.shirt_number > 0 else p.slot_index + 1]
		pos_lbl.add_theme_color_override("font_color", role_cols[r_idx])
	name_lbl.text = p.player_name.to_upper()
	if card_rect != null:
		card_rect.visible = p.yellow_cards > 0
	if stats_lbl != null:
		stats_lbl.visible = true
		stats_lbl.text = "SPD %d  ACC %d  KCK %d  TEC %d  STA %d" % [
			p.stats["speed"], p.stats["acceleration"], p.stats["kick_power"],
			p.stats["technique"], p.stats["stamina"]]
	stam_bar.visible = true
	stam_bar.value = p.stamina
	stam_bar.modulate = Color(0.25, 0.90, 0.35) if p.stamina > 50.0 else (Color(0.98, 0.80, 0.15) if p.stamina > 22.0 else Color(0.95, 0.25, 0.20))
	pow_bar.visible = p.is_charging_shot
	pow_bar.value = p.shot_charge * 100.0
	pow_bar.modulate = Color(0.28, 0.92, 0.38) if p.shot_charge < 0.45 else (Color(0.98, 0.82, 0.16) if p.shot_charge < 0.78 else Color(1.0, 0.26, 0.18))

func _style_ps1_bar(bar: ProgressBar) -> void:
	var bg := StyleBoxFlat.new()
	bg.bg_color = Color(0.04, 0.07, 0.16, 0.95)
	bg.border_color = Color(0.45, 0.55, 0.72, 0.85)
	bg.set_border_width_all(1)
	bg.set_corner_radius_all(0)
	var fg := StyleBoxFlat.new()
	fg.bg_color = Color(1.0, 1.0, 1.0, 0.95)
	fg.set_corner_radius_all(0)
	bar.add_theme_stylebox_override("background", bg)
	bar.add_theme_stylebox_override("fill", fg)

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

	if ResourceLoader.exists("res://assets/textures/hud/we3_score_digits.png"):
		_digits_tex = load("res://assets/textures/hud/we3_score_digits.png") as Texture2D

	_home_abbr_lbl = Label.new()
	_home_abbr_lbl.add_theme_font_size_override("font_size", 20)
	_home_abbr_lbl.add_theme_color_override("font_color", Color(0.98, 0.98, 1.0))
	hbox.add_child(_home_abbr_lbl)

	_home_digit_rect = TextureRect.new()
	_home_digit_rect.custom_minimum_size = Vector2(18, 24)
	_home_digit_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_home_digit_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_home_digit_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	var at_h := AtlasTexture.new()
	at_h.atlas = _digits_tex
	at_h.region = Rect2(0, 0, 16, 22)
	_home_digit_rect.texture = at_h
	hbox.add_child(_home_digit_rect)

	_score_label = Label.new()
	_score_label.add_theme_font_size_override("font_size", 22)
	_score_label.add_theme_color_override("font_color", Color(0.98, 0.98, 1.0))
	hbox.add_child(_score_label)

	_away_digit_rect = TextureRect.new()
	_away_digit_rect.custom_minimum_size = Vector2(18, 24)
	_away_digit_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_away_digit_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_away_digit_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	var at_a := AtlasTexture.new()
	at_a.atlas = _digits_tex
	at_a.region = Rect2(0, 0, 16, 22)
	_away_digit_rect.texture = at_a
	hbox.add_child(_away_digit_rect)

	_away_abbr_lbl = Label.new()
	_away_abbr_lbl.add_theme_font_size_override("font_size", 20)
	_away_abbr_lbl.add_theme_color_override("font_color", Color(0.98, 0.98, 1.0))
	hbox.add_child(_away_abbr_lbl)

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

	_tactic_badge = Label.new()
	_tactic_badge.add_theme_font_size_override("font_size", 13)
	_tactic_badge.add_theme_color_override("font_color", Color(0.65, 0.88, 1.0))
	hbox.add_child(_tactic_badge)
	update_tactics(GameData.home_tactic_level, GameData.home_offside_trap)

	# PK Shootout scoreboard (shown under main scoreboard during PK tiebreaker)
	_pk_panel = PanelContainer.new()
	_pk_panel.set_anchors_preset(Control.PRESET_CENTER_TOP)
	_pk_panel.offset_left = -165
	_pk_panel.offset_right = 165
	_pk_panel.offset_top = 66
	_pk_panel.offset_bottom = 122
	_pk_panel.visible = false
	_pk_panel.add_theme_stylebox_override("panel", sb_style)
	add_child(_pk_panel)

	var pk_vbox := VBoxContainer.new()
	pk_vbox.add_theme_constant_override("separation", 2)
	_pk_panel.add_child(pk_vbox)

	_pk_home_lbl = Label.new()
	_pk_home_lbl.add_theme_font_size_override("font_size", 15)
	_pk_home_lbl.add_theme_color_override("font_color", Color(0.95, 0.96, 1.0))
	pk_vbox.add_child(_pk_home_lbl)

	_pk_away_lbl = Label.new()
	_pk_away_lbl.add_theme_font_size_override("font_size", 15)
	_pk_away_lbl.add_theme_color_override("font_color", Color(0.95, 0.96, 1.0))
	pk_vbox.add_child(_pk_away_lbl)

	# Pre-match PS1 Matchup & Lineup Card
	_prematch_panel = PanelContainer.new()
	_prematch_panel.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	_prematch_panel.offset_left = -280
	_prematch_panel.offset_right = 280
	_prematch_panel.offset_top = -175
	_prematch_panel.offset_bottom = -105
	_prematch_panel.visible = false
	var pm_style := StyleBoxFlat.new()
	pm_style.bg_color = Color(0.04, 0.08, 0.22, 0.92)
	pm_style.border_color = Color(0.98, 0.84, 0.22)
	pm_style.set_border_width_all(2)
	pm_style.set_corner_radius_all(2)
	pm_style.set_content_margin_all(10)
	_prematch_panel.add_theme_stylebox_override("panel", pm_style)
	add_child(_prematch_panel)

	_prematch_label = Label.new()
	_prematch_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_prematch_label.add_theme_font_size_override("font_size", 16)
	_prematch_label.add_theme_color_override("font_color", Color(0.98, 0.96, 0.88))
	_prematch_panel.add_child(_prematch_label)

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
	if ResourceLoader.exists("res://assets/textures/hud/we3_nameplate_frame.png"):
		var np_style := StyleBoxTexture.new()
		np_style.texture = load("res://assets/textures/hud/we3_nameplate_frame.png") as Texture2D
		np_style.texture_margin_left = 10
		np_style.texture_margin_right = 10
		np_style.texture_margin_top = 6
		np_style.texture_margin_bottom = 6
		np_style.content_margin_left = 12
		np_style.content_margin_right = 12
		np_style.content_margin_top = 6
		np_style.content_margin_bottom = 6
		bot_left.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		bot_left.add_theme_stylebox_override("panel", np_style)
	else:
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
	_style_ps1_bar(_stamina_bar)
	vbox_pl.add_child(_stamina_bar)

	_power_bar = ProgressBar.new()
	_power_bar.max_value = 100.0
	_power_bar.value = 0.0
	_power_bar.show_percentage = false
	_power_bar.custom_minimum_size = Vector2(200, 10)
	_power_bar.modulate = Color(1.0, 0.45, 0.12)
	_power_bar.visible = false
	_style_ps1_bar(_power_bar)
	vbox_pl.add_child(_power_bar)

	# Bottom-right WE3 2P active player nameplate (visible in 1P vs 2P)
	_p2_panel = PanelContainer.new()
	_p2_panel.set_anchors_preset(Control.PRESET_BOTTOM_RIGHT)
	_p2_panel.offset_left = -276
	_p2_panel.offset_right = -20
	_p2_panel.offset_top = -96
	_p2_panel.offset_bottom = -16
	_p2_panel.visible = false
	_p2_panel.add_theme_stylebox_override("panel", bot_left.get_theme_stylebox("panel"))
	add_child(_p2_panel)

	var vbox_p2 := VBoxContainer.new()
	vbox_p2.add_theme_constant_override("separation", 4)
	_p2_panel.add_child(vbox_p2)

	var p2_row := HBoxContainer.new()
	p2_row.add_theme_constant_override("separation", 10)
	vbox_p2.add_child(p2_row)

	_p2_pos_badge = Label.new()
	_p2_pos_badge.text = "FW 10"
	_p2_pos_badge.add_theme_font_size_override("font_size", 16)
	p2_row.add_child(_p2_pos_badge)

	_p2_player_label = Label.new()
	_p2_player_label.text = "2P"
	_p2_player_label.add_theme_font_size_override("font_size", 18)
	_p2_player_label.add_theme_color_override("font_color", Color(1.0, 0.85, 0.80))
	p2_row.add_child(_p2_player_label)

	_p2_card_badge = ColorRect.new()
	_p2_card_badge.color = Color(0.98, 0.84, 0.12)
	_p2_card_badge.custom_minimum_size = Vector2(10, 14)
	_p2_card_badge.visible = false
	p2_row.add_child(_p2_card_badge)

	_p2_stamina_bar = ProgressBar.new()
	_p2_stamina_bar.max_value = 100.0
	_p2_stamina_bar.value = 100.0
	_p2_stamina_bar.show_percentage = false
	_p2_stamina_bar.custom_minimum_size = Vector2(200, 8)
	_style_ps1_bar(_p2_stamina_bar)
	vbox_p2.add_child(_p2_stamina_bar)

	_p2_power_bar = ProgressBar.new()
	_p2_power_bar.max_value = 100.0
	_p2_power_bar.value = 0.0
	_p2_power_bar.show_percentage = false
	_p2_power_bar.custom_minimum_size = Vector2(200, 10)
	_p2_power_bar.visible = false
	_style_ps1_bar(_p2_power_bar)
	vbox_p2.add_child(_p2_power_bar)

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
		elif MatchState.phase == MatchState.Phase.FULLTIME and GameData.cup_active:
			next_cup_round_requested.emit()
		else:
			resume_requested.emit()
	)
	ov_box.add_child(_btn_primary)

	_sub_box = HBoxContainer.new()
	_sub_box.add_theme_constant_override("separation", 6)
	_sub_box.visible = false
	ov_box.add_child(_sub_box)
	_sub_out_opt = OptionButton.new()
	_sub_out_opt.custom_minimum_size = Vector2(130, 30)
	_sub_box.add_child(_sub_out_opt)
	_sub_in_opt = OptionButton.new()
	_sub_in_opt.custom_minimum_size = Vector2(130, 30)
	_sub_box.add_child(_sub_in_opt)
	_btn_sub = Button.new()
	_btn_sub.text = "Sub (0/3)"
	_btn_sub.pressed.connect(func():
		if _sub_out_opt.item_count > 0 and _sub_in_opt.item_count > 0:
			substitution_requested.emit(_sub_out_opt.get_selected_id(), _sub_in_opt.get_selected_id())
			_refresh_sub_controls(true)
	)
	_sub_box.add_child(_btn_sub)

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
		# Goalposts on left/right edges
		var gw := (PitchBuilder.GOAL_WIDTH / PitchBuilder.PITCH_WIDTH) * size.y
		draw_rect(Rect2(0.0, size.y * 0.5 - gw * 0.5, 3.0, gw), Color(0.95, 0.95, 0.95, 0.85))
		draw_rect(Rect2(size.x - 3.0, size.y * 0.5 - gw * 0.5, 3.0, gw), Color(0.95, 0.95, 0.95, 0.85))
		for p in players:
			if p == null or not is_instance_valid(p) or p.is_sent_off:
				continue
			var u := (p.global_position.x + PitchBuilder.HALF_LEN) / PitchBuilder.PITCH_LENGTH
			var v := (p.global_position.z + PitchBuilder.HALF_WID) / PitchBuilder.PITCH_WIDTH
			var pt := Vector2(clampf(u, 0.02, 0.98) * size.x, clampf(v, 0.02, 0.98) * size.y)
			var col := Color(0.95, 0.85, 0.2) if p.role == Player.Role.GK else (c0 if p.team_side == 0 else c1)
			draw_rect(Rect2(pt.x - 3.0, pt.y - 3.0, 6.0, 6.0), Color(0.0, 0.0, 0.0, 0.90))
			draw_rect(Rect2(pt.x - 2.0, pt.y - 2.0, 4.0, 4.0), col)
			if p.is_controlled or (ball != null and ball.owner_player == p):
				var hi := Color(1.0, 0.35, 0.25) if (p.is_human and p.human_slot == 2) else Color(0.25, 0.95, 1.0)
				draw_rect(Rect2(pt.x - 4.5, pt.y - 4.5, 9.0, 9.0), hi, false, 1.2)

		if ball != null and is_instance_valid(ball):
			var bu := (ball.global_position.x + PitchBuilder.HALF_LEN) / PitchBuilder.PITCH_LENGTH
			var bv := (ball.global_position.z + PitchBuilder.HALF_WID) / PitchBuilder.PITCH_WIDTH
			var bpt := Vector2(clampf(bu, 0.01, 0.99) * size.x, clampf(bv, 0.01, 0.99) * size.y)
			draw_rect(Rect2(bpt.x - 3.0, bpt.y - 3.0, 6.0, 6.0), Color(0.0, 0.0, 0.0, 0.95))
			draw_rect(Rect2(bpt.x - 2.0, bpt.y - 2.0, 4.0, 4.0), Color(1.0, 0.96, 0.35))
