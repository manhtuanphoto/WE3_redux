class_name MatchManager
extends Node3D
## Orchestrates the 11v11 match, formations, kickoff/set-pieces, player switching, and clock.

const FORMATION_TEMPLATES := {
	"4-4-2": [
		{"role": Player.Role.GK, "x": -49.5, "z": 0.0},
		{"role": Player.Role.DF, "x": -34.0, "z": -18.0},
		{"role": Player.Role.DF, "x": -36.0, "z": -6.0},
		{"role": Player.Role.DF, "x": -36.0, "z": 6.0},
		{"role": Player.Role.DF, "x": -34.0, "z": 18.0},
		{"role": Player.Role.MF, "x": -15.0, "z": -20.0},
		{"role": Player.Role.MF, "x": -18.0, "z": -6.5},
		{"role": Player.Role.MF, "x": -18.0, "z": 6.5},
		{"role": Player.Role.MF, "x": -15.0, "z": 20.0},
		{"role": Player.Role.FW, "x": -4.0, "z": -6.0},
		{"role": Player.Role.FW, "x": -4.0, "z": 6.0},
	],
	"4-3-3": [
		{"role": Player.Role.GK, "x": -49.5, "z": 0.0},
		{"role": Player.Role.DF, "x": -34.0, "z": -18.0},
		{"role": Player.Role.DF, "x": -36.0, "z": -6.0},
		{"role": Player.Role.DF, "x": -36.0, "z": 6.0},
		{"role": Player.Role.DF, "x": -34.0, "z": 18.0},
		{"role": Player.Role.MF, "x": -17.0, "z": -12.0},
		{"role": Player.Role.MF, "x": -20.0, "z": 0.0},
		{"role": Player.Role.MF, "x": -17.0, "z": 12.0},
		{"role": Player.Role.FW, "x": -5.0, "z": -16.0},
		{"role": Player.Role.FW, "x": -3.5, "z": 0.0},
		{"role": Player.Role.FW, "x": -5.0, "z": 16.0},
	],
	"3-5-2": [
		{"role": Player.Role.GK, "x": -49.5, "z": 0.0},
		{"role": Player.Role.DF, "x": -36.0, "z": -13.0},
		{"role": Player.Role.DF, "x": -37.0, "z": 0.0},
		{"role": Player.Role.DF, "x": -36.0, "z": 13.0},
		{"role": Player.Role.MF, "x": -16.0, "z": -22.0},
		{"role": Player.Role.MF, "x": -20.0, "z": -8.0},
		{"role": Player.Role.MF, "x": -14.0, "z": 0.0},
		{"role": Player.Role.MF, "x": -20.0, "z": 8.0},
		{"role": Player.Role.MF, "x": -16.0, "z": 22.0},
		{"role": Player.Role.FW, "x": -4.0, "z": -6.0},
		{"role": Player.Role.FW, "x": -4.0, "z": 6.0},
	],
}

var pitch: PitchBuilder
var ball: Ball
var cam: BroadcastCamera
var hud: MatchHUD
var players: Array[Player] = []
var controlled_player: Player = null

var _phase_timer: float = 0.0
var _pending_restart_pos: Vector3 = Vector3.ZERO
var _pending_restart_team: int = 0
var _pre_pause_phase: MatchState.Phase = MatchState.Phase.OPEN_PLAY
var _injury_stall_timer: float = 0.0
var _crowd_excited: bool = false
var _replay_buffer: Array[Dictionary] = []
var _is_replaying: bool = false
var _replay_cursor: float = 0.0

func _ready() -> void:
	MatchState.reset_match()

	pitch = PitchBuilder.new()
	add_child(pitch)

	ball = Ball.new()
	ball.goal_detected.connect(_on_goal_detected)
	ball.out_of_bounds_detected.connect(_on_out_of_bounds)
	add_child(ball)

	cam = BroadcastCamera.new()
	cam.target_ball = ball
	add_child(cam)

	_spawn_teams()
	_spawn_referees()
	_apply_stadium_lighting()

	hud = MatchHUD.new()
	add_child(hud)
	hud.bind_entities(players, ball)
	hud.resume_requested.connect(_toggle_pause)
	hud.rematch_requested.connect(_restart_match)
	hud.main_menu_requested.connect(_go_main_menu)
	hud.next_half_requested.connect(_start_second_half)

	_setup_kickoff(0)
	# Authentic WE3: no BGM during live match (crowd ambience, SFX, and commentary only)
	SoundFX.stop_music()
	SoundFX.start_crowd_ambience()

func _spawn_teams() -> void:
	_spawn_team_side(0, GameData.home_team_id, not GameData.is_ai_vs_ai)
	_spawn_team_side(1, GameData.away_team_id, false)
	if not GameData.is_ai_vs_ai:
		_select_controlled_player(_get_closest_human_outfielder(Vector3.ZERO))
	else:
		_select_controlled_player(null)

func _spawn_referees() -> void:
	var main_ref := Referee.new()
	main_ref.setup(ball, false, 0.0)
	main_ref.position = Vector3(-4.0, 0.0, -8.0)
	add_child(main_ref)

	var line_north := Referee.new()
	line_north.setup(ball, true, -PitchBuilder.HALF_WID - 0.9)
	line_north.position = Vector3(12.0, 0.0, -PitchBuilder.HALF_WID - 0.9)
	add_child(line_north)

	var line_south := Referee.new()
	line_south.setup(ball, true, PitchBuilder.HALF_WID + 0.9)
	line_south.position = Vector3(-12.0, 0.0, PitchBuilder.HALF_WID + 0.9)
	add_child(line_south)

func _spawn_team_side(side: int, tid: int, is_human_team: bool) -> void:
	var team_data := GameData.get_team(tid)
	var roster: Array = team_data.get("players", [])
	var form_name := GameData.home_formation if side == 0 else GameData.away_formation
	var template: Array = FORMATION_TEMPLATES.get(form_name, FORMATION_TEMPLATES["4-4-2"])
	for i in range(11):
		var slot: Dictionary = template[i]
		var px := float(slot["x"]) if side == 0 else -float(slot["x"])
		var pz := float(slot["z"]) if side == 0 else -float(slot["z"])
		var anchor := Vector3(px, 0.0, pz)
		var pname := "Player %d" % (i + 1)
		if i < roster.size() and typeof(roster[i]) == TYPE_DICTIONARY:
			pname = str(roster[i].get("name", pname))
		var p := Player.new()
		p.setup(side, tid, i, pname, slot["role"], anchor, ball, players)
		p.is_human = is_human_team
		p.position = anchor
		add_child(p)
		players.append(p)

func _setup_kickoff(kicking_team: int) -> void:
	ball.reset_to_center()
	for p in players:
		p.vel = Vector3.ZERO
		p.action_lock_timer = 0.0
		p.tackle_timer = 0.0
		# Keep players on their own half for kickoff
		var ax := p.home_anchor.x
		if p.team_side == 0 and ax > -1.0:
			ax = -2.0
		elif p.team_side == 1 and ax < 1.0:
			ax = 2.0
		p.global_position = Vector3(ax, 0.0, p.home_anchor.z)
		p.rotation.y = -PI * 0.5 if p.team_side == 0 else PI * 0.5

	# Place striker of kicking team near center spot
	for p in players:
		if p.team_side == kicking_team and p.slot_index == 10:
			p.global_position = Vector3(-0.65 if kicking_team == 0 else 0.65, 0.0, 0.0)
			if p.is_human:
				_select_controlled_player(p)
			break

	MatchState.set_phase(MatchState.Phase.KICKOFF)
	_phase_timer = 1.2
	var h_abbr := GameData.get_team_abbr(GameData.home_team_id)
	var a_abbr := GameData.get_team_abbr(GameData.away_team_id)
	hud.show_banner("KICK OFF!  %s vs %s" % [h_abbr, a_abbr], 1.2)
	SoundFX.play_sfx("whistle")

func _process(delta: float) -> void:
	if Input.is_action_just_pressed("ui_focus_next"):
		Engine.time_scale = 2.0 if Engine.time_scale < 1.5 else 1.0

	if Input.is_action_just_pressed("pause"):
		if MatchState.phase == MatchState.Phase.OPEN_PLAY or MatchState.phase == MatchState.Phase.KICKOFF or MatchState.phase == MatchState.Phase.PAUSED:
			_toggle_pause()
			return

	if MatchState.phase == MatchState.Phase.PAUSED or MatchState.phase == MatchState.Phase.HALFTIME or MatchState.phase == MatchState.Phase.FULLTIME:
		return

	if MatchState.phase == MatchState.Phase.KICKOFF:
		_phase_timer -= delta
		if _phase_timer <= 0.0:
			MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
		return

	if MatchState.phase == MatchState.Phase.GOAL_CELEBRATION:
		if not _is_replaying:
			_phase_timer -= delta
			if _phase_timer <= 0.0:
				if _replay_buffer.size() >= 30:
					_is_replaying = true
					_replay_cursor = 0.0
					hud.set_replay_badge(true)
				else:
					_setup_kickoff(MatchState.kickoff_team)
		return

	if MatchState.phase in [MatchState.Phase.OUT_THROW_IN, MatchState.Phase.OUT_CORNER, MatchState.Phase.OUT_GOAL_KICK, MatchState.Phase.OUT_FREE_KICK]:
		_phase_timer -= delta
		if _phase_timer <= 0.0:
			_execute_set_piece_restart()
		return

	# OPEN_PLAY
	MatchState.match_elapsed_real += delta
	if ball.owner_player != null:
		MatchState.possession_time[ball.owner_player.team_side] += delta

	# Record 180-frame (3s) ring buffer for Goal Instant Replay
	var p_pos: Array[Vector3] = []
	var p_yaw: Array[float] = []
	for p in players:
		p_pos.append(p.global_position)
		p_yaw.append(p.rotation.y)
	_replay_buffer.append({"ball": ball.global_position, "p_pos": p_pos, "p_yaw": p_yaw})
	if _replay_buffer.size() > 180:
		_replay_buffer.pop_front()

	# WE3 danger-zone crowd excitement (func_80076E7C.c / func_80055CD4.c)
	var bx_abs := absf(ball.global_position.x)
	if not _crowd_excited and bx_abs >= 36.0:
		_crowd_excited = true
		SoundFX.play_sfx("cheer")
	elif _crowd_excited and bx_abs < 26.0:
		_crowd_excited = false

	_update_human_switching()

	# WE3 smart end-of-half / injury-time whistle (func_80076E7C.c):
	# Blow immediately in midfield (|x| < 22m), but allow attacking plays in final third until stalled > 2.2s
	if MatchState.match_elapsed_real >= GameData.half_duration_seconds:
		var should_whistle := false
		if bx_abs < 22.0:
			should_whistle = true
		else:
			var attacking := ball.global_position.y > 0.46
			if ball.owner_player != null and (ball.owner_player as Player).vel.length() > 0.5:
				var p := ball.owner_player as Player
				var atk_dir := 1.0 if p.team_side == 0 else -1.0
				if (p.vel.x * atk_dir) > 0.2 and (ball.global_position.x * atk_dir) > 0.0:
					attacking = true
			if attacking:
				_injury_stall_timer = 0.0
			else:
				_injury_stall_timer += delta
				if _injury_stall_timer >= 2.2:
					should_whistle = true

		if should_whistle:
			_injury_stall_timer = 0.0
			Engine.time_scale = 1.0
			SoundFX.play_sfx("whistle_long")
			if MatchState.current_half == 1:
				MatchState.set_phase(MatchState.Phase.HALFTIME)
				hud.show_half_or_full_time(false)
				SoundFX.stop_crowd_ambience()
				SoundFX.play_music("07RESULT")
			else:
				MatchState.set_phase(MatchState.Phase.FULLTIME)
				hud.show_half_or_full_time(true)
				SoundFX.stop_crowd_ambience()
				var lost := (not GameData.is_ai_vs_ai) and (MatchState.home_score < MatchState.away_score)
				SoundFX.play_music("03G_OVER" if lost else "07RESULT")

func _update_human_switching() -> void:
	if GameData.is_ai_vs_ai:
		return
	# Auto-switch if a human teammate gains possession
	if ball.owner_player != null and ball.owner_player.is_human and ball.owner_player != controlled_player:
		_select_controlled_player(ball.owner_player)
		return

	# Manual switch when defending / chasing loose ball
	if Input.is_action_just_pressed("pass_short"):
		if controlled_player == null or not controlled_player.has_ball():
			var next_p := _get_closest_human_outfielder(ball.global_position, controlled_player)
			if next_p != null:
				_select_controlled_player(next_p)

func _select_controlled_player(p: Player) -> void:
	if controlled_player != null and is_instance_valid(controlled_player):
		controlled_player.set_controlled(false)
	controlled_player = p
	if controlled_player != null:
		controlled_player.set_controlled(true)
	if hud != null:
		hud.set_controlled_player(controlled_player)

func _get_closest_human_outfielder(to_pos: Vector3, exclude: Player = null) -> Player:
	var best: Player = null
	var best_d2 := INF
	for p in players:
		if p.is_human and p.role != Player.Role.GK and p != exclude and not p.is_sent_off:
			var d2 := p.global_position.distance_squared_to(to_pos)
			if d2 < best_d2:
				best_d2 = d2
				best = p
	return best

func _physics_process(_delta: float) -> void:
	if MatchState.phase == MatchState.Phase.GOAL_CELEBRATION and _is_replaying:
		var idx := int(_replay_cursor)
		if idx >= _replay_buffer.size():
			_is_replaying = false
			hud.set_replay_badge(false)
			_replay_buffer.clear()
			_setup_kickoff(MatchState.kickoff_team)
			return
		var frame: Dictionary = _replay_buffer[idx]
		ball.global_position = frame["ball"]
		var p_pos: Array = frame["p_pos"]
		var p_yaw: Array = frame["p_yaw"]
		for i in range(mini(players.size(), p_pos.size())):
			players[i].global_position = p_pos[i]
			players[i].rotation.y = p_yaw[i]
		_replay_cursor += 0.55 # Slow-motion 0.55x replay!

func _on_goal_detected(scoring_team: int) -> void:
	if MatchState.phase != MatchState.Phase.OPEN_PLAY:
		return
	var scorer := MatchState.last_touch_player_name
	if scorer == "":
		scorer = GameData.get_team_abbr(GameData.home_team_id if scoring_team == 0 else GameData.away_team_id)
	MatchState.record_goal(scoring_team, scorer)
	_is_replaying = false
	_phase_timer = 1.6
	var goal_min := MatchState.get_display_minutes()
	hud.show_banner("GOAL!!  %s (%d')  [%d - %d]" % [scorer, goal_min, MatchState.home_score, MatchState.away_score], 2.5)
	SoundFX.play_sfx("whistle")
	SoundFX.play_sfx("cheer")
	SoundFX.play_commentary("goal")

func _apply_stadium_lighting() -> void:
	var is_night := GameData.selected_stadium == "GRDM_FN"
	var sun := get_node_or_null("StadiumSun") as DirectionalLight3D
	if sun != null:
		sun.light_energy = 0.85 if is_night else 1.35
		sun.light_color = Color(0.82, 0.88, 1.0) if is_night else Color(1.0, 0.98, 0.94)
	var env_node := get_node_or_null("WorldEnvironment") as WorldEnvironment
	if env_node != null and env_node.environment != null and env_node.environment.sky != null:
		var sky_mat := env_node.environment.sky.sky_material as ProceduralSkyMaterial
		if sky_mat != null:
			sky_mat.sky_top_color = Color(0.03, 0.05, 0.14) if is_night else Color(0.12, 0.25, 0.52)
			sky_mat.sky_horizon_color = Color(0.10, 0.14, 0.26) if is_night else Color(0.55, 0.72, 0.88)

func _on_out_of_bounds(event_type: String, restart_pos: Vector3, restart_team: int) -> void:
	if MatchState.phase != MatchState.Phase.OPEN_PLAY:
		return
	_pending_restart_pos = restart_pos
	_pending_restart_team = restart_team
	_phase_timer = 1.1
	SoundFX.play_sfx("whistle")
	match event_type:
		"throw_in":
			MatchState.set_phase(MatchState.Phase.OUT_THROW_IN)
			hud.show_banner("THROW IN", 1.1)
		"corner":
			MatchState.set_phase(MatchState.Phase.OUT_CORNER)
			hud.show_banner("CORNER KICK", 1.1)
		"goal_kick":
			MatchState.set_phase(MatchState.Phase.OUT_GOAL_KICK)
			hud.show_banner("GOAL KICK", 1.1)
		"foul":
			MatchState.set_phase(MatchState.Phase.OUT_FREE_KICK)
			hud.show_banner("FOUL! FREE KICK", 1.3)
		"penalty":
			MatchState.set_phase(MatchState.Phase.OUT_FREE_KICK)
			hud.show_banner("PENALTY KICK!", 1.5)
		"offside":
			MatchState.set_phase(MatchState.Phase.OUT_FREE_KICK)
			hud.show_banner("OFFSIDE!", 1.3)

func _execute_set_piece_restart() -> void:
	ball.place_at(_pending_restart_pos)
	ball.is_active_play = true
	# Move closest player of restart_team to take it
	var taker: Player = null
	var best_d2 := INF
	for p in players:
		if p.team_side == _pending_restart_team:
			if MatchState.phase == MatchState.Phase.OUT_GOAL_KICK and p.role == Player.Role.GK:
				taker = p
				break
			elif p.role != Player.Role.GK:
				var d2 := p.global_position.distance_squared_to(_pending_restart_pos)
				if d2 < best_d2:
					best_d2 = d2
					taker = p
	if taker != null:
		taker.global_position = _pending_restart_pos + Vector3(-0.6 if _pending_restart_team == 0 else 0.6, 0.0, 0.0)
		ball.set_owner_player(taker, _pending_restart_team)
		if MatchState.phase == MatchState.Phase.OUT_THROW_IN:
			taker._play_anim("throw_in", 0.05)
			taker.action_lock_timer = 0.45
			if not taker.is_human:
				var inward_z := -signf(_pending_restart_pos.z)
				taker._perform_pass(Vector3(0.0, 0.0, inward_z).normalized(), false, false)
		elif MatchState.phase == MatchState.Phase.OUT_CORNER and not taker.is_human:
			var attack_x := PitchBuilder.HALF_LEN if _pending_restart_team == 0 else -PitchBuilder.HALF_LEN
			var box_target := Vector3(attack_x - signf(attack_x) * 10.0, 0.0, 0.0)
			taker._perform_pass((box_target - taker.global_position).normalized(), false, true)
		if taker.is_human:
			_select_controlled_player(taker)
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)

func _toggle_pause() -> void:
	if MatchState.phase == MatchState.Phase.PAUSED:
		MatchState.set_phase(_pre_pause_phase)
		hud.show_pause_menu(false)
	else:
		Engine.time_scale = 1.0
		_pre_pause_phase = MatchState.phase
		MatchState.set_phase(MatchState.Phase.PAUSED)
		hud.show_pause_menu(true)

func _start_second_half() -> void:
	hud.show_pause_menu(false)
	MatchState.current_half = 2
	MatchState.match_elapsed_real = 0.0
	MatchState.half_changed.emit(2)
	_setup_kickoff(1)
	SoundFX.stop_music()
	SoundFX.start_crowd_ambience()

func _restart_match() -> void:
	Engine.time_scale = 1.0
	get_tree().reload_current_scene()

func _go_main_menu() -> void:
	Engine.time_scale = 1.0
	get_tree().change_scene_to_file("res://scenes/main_menu.tscn")
