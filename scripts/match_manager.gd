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
	"5-3-2": [
		{"role": Player.Role.GK, "x": -49.5, "z": 0.0},
		{"role": Player.Role.DF, "x": -33.0, "z": -23.0},
		{"role": Player.Role.DF, "x": -36.0, "z": -11.5},
		{"role": Player.Role.DF, "x": -38.0, "z": 0.0},
		{"role": Player.Role.DF, "x": -36.0, "z": 11.5},
		{"role": Player.Role.DF, "x": -33.0, "z": 23.0},
		{"role": Player.Role.MF, "x": -19.0, "z": -12.0},
		{"role": Player.Role.MF, "x": -16.0, "z": 0.0},
		{"role": Player.Role.MF, "x": -19.0, "z": 12.0},
		{"role": Player.Role.FW, "x": -4.0, "z": -6.0},
		{"role": Player.Role.FW, "x": -4.0, "z": 6.0},
	],
	"4-5-1": [
		{"role": Player.Role.GK, "x": -49.5, "z": 0.0},
		{"role": Player.Role.DF, "x": -34.0, "z": -18.0},
		{"role": Player.Role.DF, "x": -36.0, "z": -6.0},
		{"role": Player.Role.DF, "x": -36.0, "z": 6.0},
		{"role": Player.Role.DF, "x": -34.0, "z": 18.0},
		{"role": Player.Role.MF, "x": -16.0, "z": -24.0},
		{"role": Player.Role.MF, "x": -19.0, "z": -11.0},
		{"role": Player.Role.MF, "x": -14.0, "z": 0.0},
		{"role": Player.Role.MF, "x": -19.0, "z": 11.0},
		{"role": Player.Role.MF, "x": -16.0, "z": 24.0},
		{"role": Player.Role.FW, "x": -4.0, "z": 0.0},
	],
	"3-4-3": [
		{"role": Player.Role.GK, "x": -49.5, "z": 0.0},
		{"role": Player.Role.DF, "x": -36.0, "z": -13.0},
		{"role": Player.Role.DF, "x": -37.0, "z": 0.0},
		{"role": Player.Role.DF, "x": -36.0, "z": 13.0},
		{"role": Player.Role.MF, "x": -17.0, "z": -20.0},
		{"role": Player.Role.MF, "x": -19.0, "z": -6.5},
		{"role": Player.Role.MF, "x": -19.0, "z": 6.5},
		{"role": Player.Role.MF, "x": -17.0, "z": 20.0},
		{"role": Player.Role.FW, "x": -5.0, "z": -16.0},
		{"role": Player.Role.FW, "x": -3.5, "z": 0.0},
		{"role": Player.Role.FW, "x": -5.0, "z": 16.0},
	],
}

## Maps the authentic roster (22 entries, first 11 = starters, rest = bench) onto a formation template.
## Each slot takes a player whose WE3 position group (GK/DF/MF/FW) matches; the DB order inside a
## group is centre-first, so the earliest picks go to the most central slots, later ones to the flanks.
## Returns roster indices (one per template slot).
static func assign_lineup(roster: Array, template: Array) -> Array[int]:
	const GROUPS := ["GK", "DF", "MF", "FW"]
	var pools := {"GK": [], "DF": [], "MF": [], "FW": []}
	for i in range(roster.size()):
		var g := str(roster[i].get("pos_group", "MF")) if typeof(roster[i]) == TYPE_DICTIONARY else "MF"
		if pools.has(g):
			pools[g].append(i)  # starters (0..10) naturally precede bench (11..21)
	var out: Array[int] = []
	out.resize(template.size())
	out.fill(-1)
	var used := {}
	for role_idx in range(4):
		var slot_ids: Array = []
		for s in range(template.size()):
			if int(template[s]["role"]) == role_idx:
				slot_ids.append(s)
		# centre-first ordering by |z|
		slot_ids.sort_custom(func(a, b): return absf(float(template[a]["z"])) < absf(float(template[b]["z"])) or (absf(float(template[a]["z"])) == absf(float(template[b]["z"])) and float(template[a]["z"]) < float(template[b]["z"])))
		var fallback_order: Array = [GROUPS[role_idx]]
		match role_idx:
			1: fallback_order.append_array(["MF", "FW"])
			2: fallback_order.append_array(["DF", "FW"])
			3: fallback_order.append_array(["MF", "DF"])
			0: fallback_order.append_array(["DF", "MF"])
		for s in slot_ids:
			for g in fallback_order:
				var picked := -1
				for idx in pools[g]:
					if not used.has(idx):
						picked = idx
						break
				if picked >= 0:
					used[picked] = true
					out[s] = picked
					break
	# Safety net: any unfilled slot takes the next unused roster entry
	for s in range(out.size()):
		if out[s] < 0:
			for i in range(roster.size()):
				if not used.has(i):
					used[i] = true
					out[s] = i
					break
	return out

var pitch: PitchBuilder
var ball: Ball
var cam: BroadcastCamera
var hud: MatchHUD
var players: Array[Player] = []
var controlled_player: Player = null
var controlled_player_p2: Player = null

var _phase_timer: float = 0.0
var _pending_restart_pos: Vector3 = Vector3.ZERO
var _pending_restart_team: int = 0
var _pending_restart_type: String = ""
var _pre_pause_phase: MatchState.Phase = MatchState.Phase.OPEN_PLAY
var _injury_stall_timer: float = 0.0
var _crowd_excited: bool = false
var _replay_buffer: Array[Dictionary] = []
var _is_replaying: bool = false
var _replay_cursor: float = 0.0

# PK Shootout state (func_800820A0)
var _pk_active: bool = false
var _pk_kick_index: int = 0
var _pk_kicker_order_idx: Array[int] = [0, 0]
var _pk_results: Array = [[], []]
var _pk_ai_shoot_timer: float = 0.0
var _pk_wait_next_timer: float = 0.0
var _pk_shot_taken: bool = false
var _pk_shot_elapsed: float = 0.0
var _pk_kicker: Player = null
var _pk_gk: Player = null

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
	hud.next_cup_round_requested.connect(_on_next_cup_round)
	hud.substitution_requested.connect(_on_substitution_requested)

	if GameData.game_mode == "pk":
		_start_pk_shootout()
	else:
		_setup_kickoff(0)
	# Authentic WE3: no BGM during live match (crowd ambience, SFX, and commentary only)
	SoundFX.stop_music()
	SoundFX.start_crowd_ambience()

func _spawn_teams() -> void:
	if GameData.is_ai_vs_ai:
		_spawn_team_side(0, GameData.home_team_id, false, 1)
		_spawn_team_side(1, GameData.away_team_id, false, 1)
		_select_controlled_player(null)
		_select_controlled_player_p2(null)
	elif GameData.is_two_player:
		_spawn_team_side(0, GameData.home_team_id, true, 1)
		_spawn_team_side(1, GameData.away_team_id, true, 2)
		_select_controlled_player(_get_closest_human_outfielder(Vector3.ZERO, null, 1))
		_select_controlled_player_p2(_get_closest_human_outfielder(Vector3.ZERO, null, 2))
	else:
		var hs := GameData.human_side
		_spawn_team_side(0, GameData.home_team_id, hs == 0, 1)
		_spawn_team_side(1, GameData.away_team_id, hs == 1, 1)
		_select_controlled_player(_get_closest_human_outfielder(Vector3.ZERO, null, 1))
		_select_controlled_player_p2(null)

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

func _spawn_team_side(side: int, tid: int, is_human_team: bool, slot_num: int = 1) -> void:
	var team_data := GameData.get_team(tid)
	var roster: Array = team_data.get("players", [])
	var form_name := GameData.resolve_formation(side)
	var template: Array = FORMATION_TEMPLATES.get(form_name, FORMATION_TEMPLATES["4-4-2"])
	var lineup: Array[int] = assign_lineup(roster, template)
	for i in range(11):
		var slot: Dictionary = template[i]
		var px := float(slot["x"]) if side == 0 else -float(slot["x"])
		var pz := float(slot["z"]) if side == 0 else -float(slot["z"])
		var anchor := Vector3(px, 0.0, pz)
		var pname := "Player %d" % (i + 1)
		var rec: Dictionary = {}
		var r_idx := lineup[i] if i < lineup.size() else i
		if r_idx >= 0 and r_idx < roster.size() and typeof(roster[r_idx]) == TYPE_DICTIONARY:
			rec = roster[r_idx]
			pname = str(rec.get("name", pname))
		var p := Player.new()
		p.setup(side, tid, i, pname, slot["role"], anchor, ball, players)
		p.apply_profile(rec, r_idx)
		p.is_human = is_human_team
		p.human_slot = slot_num
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
	var prefix := (GameData.get_cup_round_name() + " — ") if GameData.cup_active else ""
	hud.show_banner("%sKICK OFF!  %s vs %s" % [prefix, h_abbr, a_abbr], 1.4)
	SoundFX.play_sfx("whistle")
	SoundFX.play_commentary("kickoff")

func _process(delta: float) -> void:
	if Input.is_action_just_pressed("ui_focus_next"):
		Engine.time_scale = 2.0 if Engine.time_scale < 1.5 else 1.0

	if Input.is_action_just_pressed("pause"):
		if MatchState.phase == MatchState.Phase.OPEN_PLAY or MatchState.phase == MatchState.Phase.KICKOFF or MatchState.phase == MatchState.Phase.PAUSED:
			_toggle_pause()
			return

	if MatchState.phase == MatchState.Phase.OPEN_PLAY and not GameData.is_ai_vs_ai:
		var tac_changed := false
		if Input.is_key_pressed(KEY_1) and GameData.home_tactic_level != 0:
			GameData.home_tactic_level = 0
			tac_changed = true
		elif Input.is_key_pressed(KEY_2) and GameData.home_tactic_level != 1:
			GameData.home_tactic_level = 1
			tac_changed = true
		elif Input.is_key_pressed(KEY_3) and GameData.home_tactic_level != 2:
			GameData.home_tactic_level = 2
			tac_changed = true
		if tac_changed:
			var t_names := ["DEFENSIVE", "NORMAL", "ATTACKING"]
			hud.update_tactics(GameData.home_tactic_level, GameData.home_offside_trap)
			hud.show_banner("TACTIC: %s" % t_names[GameData.home_tactic_level], 1.1)

	if MatchState.phase == MatchState.Phase.PAUSED or MatchState.phase == MatchState.Phase.HALFTIME or MatchState.phase == MatchState.Phase.FULLTIME:
		return

	if _pk_active:
		_process_pk_shootout(delta)
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
					ball.owner_player = null
					ball.vel = Vector3.ZERO
					ball.is_active_play = false
					hud.set_replay_badge(true)
				else:
					_setup_kickoff(MatchState.kickoff_team)
		elif Input.is_action_just_pressed("pass_short") or Input.is_action_just_pressed("shoot") or Input.is_action_just_pressed("p2_pass_short"):
			_is_replaying = false
			hud.set_replay_badge(false)
			_replay_buffer.clear()
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

	# Record 300-frame (5s) ring buffer for Goal Instant Replay
	var p_pos: Array[Vector3] = []
	var p_yaw: Array[float] = []
	var p_anim: Array[String] = []
	for p in players:
		p_pos.append(p.global_position)
		p_yaw.append(p.rotation.y)
		p_anim.append(p._current_anim)
	_replay_buffer.append({"ball": ball.global_position, "p_pos": p_pos, "p_yaw": p_yaw, "p_anim": p_anim})
	if _replay_buffer.size() > 300:
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
	var half_dur := GameData.half_duration_seconds * (0.3333 if MatchState.current_half >= 3 else 1.0)
	if GameData.game_mode != "training" and MatchState.match_elapsed_real >= half_dur:
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
			if MatchState.current_half == 1 or MatchState.current_half == 3:
				MatchState.set_phase(MatchState.Phase.HALFTIME)
				hud.show_half_or_full_time(false)
				SoundFX.stop_crowd_ambience()
				SoundFX.play_music("07RESULT")
			elif MatchState.current_half == 2 and GameData.extra_time_enabled and MatchState.home_score == MatchState.away_score:
				MatchState.set_phase(MatchState.Phase.HALFTIME)
				hud.show_half_or_full_time(false)
				SoundFX.stop_crowd_ambience()
				SoundFX.play_music("07RESULT")
			elif MatchState.current_half == 4 and GameData.extra_time_enabled and MatchState.home_score == MatchState.away_score:
				hud.show_banner("EXTRA TIME TIED — PK SHOOTOUT!", 2.0)
				_start_pk_shootout()
			else:
				MatchState.set_phase(MatchState.Phase.FULLTIME)
				hud.show_half_or_full_time(true)
				SoundFX.stop_crowd_ambience()
				var lost := (not GameData.is_ai_vs_ai) and (MatchState.home_score < MatchState.away_score)
				SoundFX.play_music("03G_OVER" if lost else "07RESULT")
				for pl in players:
					if pl.is_sent_off:
						continue
					if MatchState.home_score == MatchState.away_score:
						pl._play_anim("ending_bow", 0.12)
					elif (pl.team_side == 0 and MatchState.home_score > MatchState.away_score) or (pl.team_side == 1 and MatchState.away_score > MatchState.home_score):
						pl._play_anim("ending_celebrate", 0.12)
					else:
						pl._play_anim("ending_idle", 0.12)

func _update_human_switching() -> void:
	if GameData.is_ai_vs_ai:
		return
	# Auto-switch if a human teammate gains possession (P1 or P2)
	if ball.owner_player != null and ball.owner_player.is_human:
		if ball.owner_player.human_slot == 1 and ball.owner_player != controlled_player:
			_select_controlled_player(ball.owner_player)
		elif ball.owner_player.human_slot == 2 and ball.owner_player != controlled_player_p2:
			_select_controlled_player_p2(ball.owner_player)

	# Manual switch when defending / chasing loose ball (P1)
	if Input.is_action_just_pressed("pass_short"):
		if controlled_player == null or not controlled_player.has_ball():
			var next_p := _get_closest_human_outfielder(ball.global_position, controlled_player, 1)
			if next_p != null:
				_select_controlled_player(next_p)

	# Manual switch when defending / chasing loose ball (P2)
	if GameData.is_two_player and Input.is_action_just_pressed("p2_pass_short"):
		if controlled_player_p2 == null or not controlled_player_p2.has_ball():
			var next_p2 := _get_closest_human_outfielder(ball.global_position, controlled_player_p2, 2)
			if next_p2 != null:
				_select_controlled_player_p2(next_p2)

func _select_controlled_player(p: Player) -> void:
	if controlled_player != null and is_instance_valid(controlled_player):
		controlled_player.set_controlled(false)
	controlled_player = p
	if controlled_player != null:
		controlled_player.set_controlled(true)
	if hud != null:
		hud.set_controlled_player(controlled_player)

func _select_controlled_player_p2(p: Player) -> void:
	if controlled_player_p2 != null and is_instance_valid(controlled_player_p2):
		controlled_player_p2.set_controlled(false)
	controlled_player_p2 = p
	if controlled_player_p2 != null:
		controlled_player_p2.set_controlled(true)

func _get_closest_human_outfielder(to_pos: Vector3, exclude: Player = null, slot_num: int = 1) -> Player:
	var best: Player = null
	var best_d2 := INF
	for p in players:
		if p.is_human and p.human_slot == slot_num and p.role != Player.Role.GK and p != exclude and not p.is_sent_off:
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
		var p_anim: Array = frame.get("p_anim", [])
		for i in range(mini(players.size(), p_pos.size())):
			players[i].global_position = p_pos[i]
			players[i].rotation.y = p_yaw[i]
			if i < p_anim.size() and p_anim[i] != "":
				players[i]._play_anim(p_anim[i], 0.04, 0.55)
		_replay_cursor += 0.55 # Slow-motion 0.55x replay!

func _on_goal_detected(scoring_team: int) -> void:
	if _pk_active:
		_resolve_pk_turn(true)
		return
	if MatchState.phase != MatchState.Phase.OPEN_PLAY:
		return
	var scorer := MatchState.last_touch_player_name
	if scorer == "":
		scorer = GameData.get_team_abbr(GameData.home_team_id if scoring_team == 0 else GameData.away_team_id)
	MatchState.record_goal(scoring_team, scorer)
	_is_replaying = false
	_phase_timer = 1.6
	const CELEB_ANIMS := ["celebrate", "celebrate_alt", "celebrate_jump", "celebrate_3", "celebrate_4"]
	for i in range(players.size()):
		var pl := players[i]
		if not pl.is_sent_off and pl.team_side == scoring_team:
			pl._play_anim(CELEB_ANIMS[i % CELEB_ANIMS.size()], 0.08)
	var goal_min := MatchState.get_display_minutes()
	if pitch != null:
		pitch.trigger_goal_net_ripple(scoring_team)
	hud.show_banner("GOAL!!  %s (%d')  [%d - %d]" % [scorer, goal_min, MatchState.home_score, MatchState.away_score], 2.5)
	SoundFX.play_sfx("whistle")
	SoundFX.play_sfx("cheer")
	SoundFX.play_commentary("goal", scorer)

func _apply_stadium_lighting() -> void:
	var is_night := GameData.stadium_is_night or GameData.selected_stadium == "GRDM_FN"
	var is_rain := GameData.stadium_is_rain
	var sun := get_node_or_null("StadiumSun") as DirectionalLight3D
	if sun != null:
		if is_night:
			sun.light_energy = 0.78 if is_rain else 0.85
			sun.light_color = Color(0.76, 0.82, 0.96) if is_rain else Color(0.82, 0.88, 1.0)
		else:
			sun.light_energy = 1.05 if is_rain else 1.35
			sun.light_color = Color(0.84, 0.88, 0.94) if is_rain else Color(1.0, 0.98, 0.94)
	var env_node := get_node_or_null("WorldEnvironment") as WorldEnvironment
	if env_node != null and env_node.environment != null and env_node.environment.sky != null:
		var sky_mat := env_node.environment.sky.sky_material as ProceduralSkyMaterial
		if sky_mat != null:
			if is_night:
				sky_mat.sky_top_color = Color(0.03, 0.05, 0.14)
				sky_mat.sky_horizon_color = Color(0.10, 0.14, 0.26)
			elif is_rain:
				sky_mat.sky_top_color = Color(0.24, 0.30, 0.38)
				sky_mat.sky_horizon_color = Color(0.46, 0.52, 0.60)
			else:
				sky_mat.sky_top_color = Color(0.12, 0.25, 0.52)
				sky_mat.sky_horizon_color = Color(0.55, 0.72, 0.88)

func _on_out_of_bounds(event_type: String, restart_pos: Vector3, restart_team: int) -> void:
	if _pk_active:
		_resolve_pk_turn(false)
		return
	if MatchState.phase != MatchState.Phase.OPEN_PLAY:
		return
	_pending_restart_type = event_type
	_pending_restart_pos = restart_pos
	if event_type == "foul":
		# func_800772F8: clamp free-kick spot out of goal area (|x| > 47.136, |z| < 8.92) & inside pitch
		var def_goal_x := PitchBuilder.HALF_LEN if restart_team == 0 else -PitchBuilder.HALF_LEN
		if absf(_pending_restart_pos.z) < 8.92 and absf(_pending_restart_pos.x - def_goal_x) < (PitchBuilder.HALF_LEN - 47.136):
			_pending_restart_pos.x = 47.136 if def_goal_x > 0.0 else -47.136
			_pending_restart_pos.z = 8.92 if _pending_restart_pos.z >= 0.0 else -8.92
		_pending_restart_pos.x = clampf(_pending_restart_pos.x, -52.096, 52.096)
		_pending_restart_pos.z = clampf(_pending_restart_pos.z, -33.536, 33.536)
	_pending_restart_team = restart_team
	_phase_timer = 1.1
	SoundFX.play_sfx("whistle")
	SoundFX.play_commentary(event_type)
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
	var is_penalty := (_pending_restart_type == "penalty")
	var atk_goal_x := PitchBuilder.HALF_LEN if _pending_restart_team == 0 else -PitchBuilder.HALF_LEN

	# Select taker (skip sent-off players; func_800A5340 prefers MF/FW for corners)
	var taker: Player = null
	var best_score := INF
	for p in players:
		if p.is_sent_off or p.team_side != _pending_restart_team:
			continue
		if MatchState.phase == MatchState.Phase.OUT_GOAL_KICK and p.role == Player.Role.GK:
			taker = p
			break
		elif p.role != Player.Role.GK:
			var sc := p.global_position.distance_squared_to(_pending_restart_pos)
			if MatchState.phase == MatchState.Phase.OUT_CORNER or is_penalty:
				# Prefer FW/MF specialists over defenders
				var role_bias := 0.0 if p.role == Player.Role.FW else (40.0 if p.role == Player.Role.MF else 400.0)
				sc = role_bias + float(10 - int(p.stats.get("kick_power", 5))) * 10.0
			if sc < best_score:
				best_score = sc
				taker = p

	if is_penalty:
		# Authentic penalty setup: defending GK on goal line, all other players outside 18-yard box
		var def_team := 1 - _pending_restart_team
		for p in players:
			if p.is_sent_off or p == taker:
				continue
			if p.team_side == def_team and p.role == Player.Role.GK:
				p.global_position = Vector3(atk_goal_x - signf(atk_goal_x) * 0.4, 0.0, 0.0)
				p.vel = Vector3.ZERO
				p._face_direction((_pending_restart_pos - p.global_position).normalized(), 1.0)
			else:
				var in_box_x := absf(p.global_position.x - atk_goal_x) < 17.5
				var in_box_z := absf(p.global_position.z) < 20.5
				if in_box_x and in_box_z:
					p.global_position.x = atk_goal_x - signf(atk_goal_x) * (18.5 + randf() * 4.0)
					p.vel = Vector3.ZERO
	elif MatchState.phase == MatchState.Phase.OUT_FREE_KICK:
		# func_800772F8: form defensive wall at 9.15m on direct free kicks near goal, push others >= 9.15m
		var to_goal := (Vector3(atk_goal_x, 0.0, 0.0) - _pending_restart_pos)
		to_goal.y = 0.0
		var dist_to_goal := to_goal.length()
		var goal_dir := to_goal.normalized() if dist_to_goal > 0.01 else Vector3(signf(atk_goal_x), 0.0, 0.0)
		var perp_dir := Vector3(-goal_dir.z, 0.0, goal_dir.x)
		var wall_players: Array[Player] = []
		if _pending_restart_type == "foul" and dist_to_goal < 34.0:
			var wall_count := 4 if dist_to_goal < 22.0 else (3 if dist_to_goal < 28.0 else 2)
			var cands: Array[Player] = []
			for p in players:
				if not p.is_sent_off and p.team_side != _pending_restart_team and p.role != Player.Role.GK:
					cands.append(p)
			cands.sort_custom(func(a, b): return a.global_position.distance_squared_to(_pending_restart_pos) < b.global_position.distance_squared_to(_pending_restart_pos))
			var n := mini(wall_count, cands.size())
			var wall_center := _pending_restart_pos + goal_dir * 9.15
			for wi in range(n):
				var wp: Player = cands[wi]
				var lat := (float(wi) - float(n - 1) * 0.5) * 0.78
				wp.global_position = wall_center + perp_dir * lat
				wp.global_position.x = clampf(wp.global_position.x, -PitchBuilder.HALF_LEN + 1.0, PitchBuilder.HALF_LEN - 1.0)
				wp.global_position.z = clampf(wp.global_position.z, -PitchBuilder.HALF_WID + 1.0, PitchBuilder.HALF_WID - 1.0)
				wp.vel = Vector3.ZERO
				wp._face_direction(-goal_dir, 1.0)
				wall_players.append(wp)
		for p in players:
			if p.is_sent_off or p.team_side == _pending_restart_team or p.role == Player.Role.GK or p in wall_players:
				continue
			var d := p.global_position.distance_to(_pending_restart_pos)
			if d < 9.15:
				p.global_position = _pending_restart_pos + goal_dir * 9.5
				p.global_position.x = clampf(p.global_position.x, -PitchBuilder.HALF_LEN + 1.0, PitchBuilder.HALF_LEN - 1.0)
				p.vel = Vector3.ZERO

	if taker != null:
		var stand_off := 2.8 if is_penalty else (1.2 if MatchState.phase == MatchState.Phase.OUT_FREE_KICK else 0.6)
		var back_dir := -1.0 if _pending_restart_team == 0 else 1.0
		taker.global_position = _pending_restart_pos + Vector3(back_dir * stand_off, 0.0, 0.0)
		taker.vel = Vector3.ZERO
		ball.set_owner_player(taker, _pending_restart_team)
		if MatchState.phase == MatchState.Phase.OUT_THROW_IN:
			taker._play_anim("throw_in", 0.05)
			taker.action_lock_timer = 0.45
			if not taker.is_human:
				var mate := taker._find_best_open_teammate(1.0 if _pending_restart_team == 0 else -1.0)
				if mate != null:
					taker._kick_pass_to_target(mate, false, false)
				else:
					var inward_z := -signf(_pending_restart_pos.z)
					taker._perform_pass(Vector3(0.0, 0.0, inward_z).normalized(), false, false)
		elif MatchState.phase == MatchState.Phase.OUT_GOAL_KICK and not taker.is_human:
			# func_80083F68: AI goalkeeper launches goal kick upfield toward midfield/flank
			var up_dir := Vector3(signf(atk_goal_x), 0.0, randf_range(-0.35, 0.35)).normalized()
			taker._perform_pass(up_dir, false, true)
		elif MatchState.phase == MatchState.Phase.OUT_CORNER and not taker.is_human:
			var box_target := Vector3(atk_goal_x - signf(atk_goal_x) * 10.0, 0.0, 0.0)
			taker._perform_pass((box_target - taker.global_position).normalized(), false, true)
		elif is_penalty and not taker.is_human:
			var pen_aim := Vector3(atk_goal_x, 0.0, randf_range(-2.6, 2.6))
			taker._perform_shoot((pen_aim - taker.global_position).normalized(), 0.85, true)
		elif MatchState.phase == MatchState.Phase.OUT_FREE_KICK and not taker.is_human:
			var dist_goal := taker.global_position.distance_to(Vector3(atk_goal_x, 0.0, 0.0))
			if dist_goal < 28.0 and _pending_restart_type == "foul":
				var fk_aim := Vector3(atk_goal_x, 0.0, randf_range(-2.4, 2.4))
				taker._perform_shoot((fk_aim - taker.global_position).normalized(), 0.78, true)
			else:
				var mate := taker._find_best_open_teammate(signf(atk_goal_x))
				if mate != null:
					taker._kick_pass_to_target(mate, false, false)
				else:
					taker._perform_pass(Vector3(signf(atk_goal_x), 0.0, 0.0), false, false)
		if taker.is_human:
			if taker.human_slot == 2:
				_select_controlled_player_p2(taker)
			else:
				_select_controlled_player(taker)
	_pending_restart_type = ""
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

func _on_next_cup_round() -> void:
	if GameData.advance_cup_round():
		get_tree().change_scene_to_file("res://scenes/match.tscn")
	else:
		get_tree().change_scene_to_file(GameData.return_scene)

func _start_second_half() -> void:
	hud.show_pause_menu(false)
	MatchState.current_half += 1
	MatchState.match_elapsed_real = 0.0
	MatchState.half_changed.emit(MatchState.current_half)
	_setup_kickoff(1 if (MatchState.current_half % 2 == 0) else 0)
	SoundFX.stop_music()
	SoundFX.start_crowd_ambience()

func _on_substitution_requested(starter_slot: int, bench_idx: int) -> void:
	if MatchState.subs_used[0] >= 3:
		return
	var roster: Array = GameData.get_team(GameData.home_team_id).get("players", [])
	if bench_idx < 11 or bench_idx >= roster.size():
		return
	var target_p: Player = null
	for p in players:
		if p.team_side == 0 and p.slot_index == starter_slot and not p.is_sent_off:
			target_p = p
			break
	if target_p == null:
		return
	var old_name := target_p.player_name
	var rec: Dictionary = roster[bench_idx]
	target_p.player_name = str(rec.get("name", "Sub"))
	target_p.apply_profile(rec, bench_idx)
	target_p.stamina = 100.0
	target_p.yellow_cards = 0
	MatchState.subs_used[0] += 1
	MatchState.log_event("SUB (%d/3): OUT %s -> IN %s" % [MatchState.subs_used[0], old_name, target_p.player_name])

func _restart_match() -> void:
	Engine.time_scale = 1.0
	get_tree().reload_current_scene()

func _go_main_menu() -> void:
	Engine.time_scale = 1.0
	GameData.preview_speed = 1.0
	get_tree().change_scene_to_file(GameData.return_scene)

# === PK SHOOTOUT MODE (func_800820A0) ===
func _start_pk_shootout() -> void:
	_pk_active = true
	_pk_kick_index = 0
	_pk_kicker_order_idx = [0, 0]
	_pk_results = [[], []]
	_pk_wait_next_timer = 0.0
	_setup_pk_turn()

func _setup_pk_turn() -> void:
	var team := _pk_kick_index & 1
	var def_team := 1 - team
	var goal_x := PitchBuilder.HALF_LEN
	var spot_x := goal_x - 11.0
	var spot := Vector3(spot_x, 0.0, 0.0)

	# Pick next kicker from slots 10..1 (FW -> MF -> DF)
	var team_players: Array[Player] = []
	var def_gk: Player = null
	for p in players:
		if p.team_side == team and p.role != Player.Role.GK:
			team_players.append(p)
		elif p.team_side == def_team and p.role == Player.Role.GK:
			def_gk = p
	team_players.sort_custom(func(a, b): return a.slot_index > b.slot_index)
	var k_idx := _pk_kicker_order_idx[team] % maxi(1, team_players.size())
	_pk_kicker_order_idx[team] += 1
	_pk_kicker = team_players[k_idx] if not team_players.is_empty() else null
	_pk_gk = def_gk
	_pk_shot_taken = false
	_pk_shot_elapsed = 0.0
	_pk_ai_shoot_timer = 0.9

	# Park all other players at midfield
	var mid_idx := 0
	for p in players:
		p.vel = Vector3.ZERO
		p.action_lock_timer = 0.0
		p.tackle_timer = 0.0
		if p == _pk_kicker or p == _pk_gk:
			continue
		var row := mid_idx / 2
		var side_z := -1.0 if (mid_idx % 2 == 0) else 1.0
		p.global_position = Vector3(-4.0 + float(row) * 1.1, 0.0, side_z * 10.0)
		p._face_direction(Vector3(1.0, 0.0, 0.0), 1.0)
		p._play_anim("idle", 0.08)
		mid_idx += 1

	if _pk_gk != null:
		_pk_gk.global_position = Vector3(goal_x - 0.4, 0.0, 0.0)
		_pk_gk._face_direction(Vector3(-1.0, 0.0, 0.0), 1.0)
		_pk_gk._play_anim("idle", 0.08)

	ball.place_at(spot)
	ball.is_active_play = true
	if _pk_kicker != null:
		_pk_kicker.global_position = spot + Vector3(-2.6, 0.0, 0.0)
		_pk_kicker._face_direction(Vector3(1.0, 0.0, 0.0), 1.0)
		ball.set_owner_player(_pk_kicker, team)
		if not GameData.is_ai_vs_ai:
			if GameData.is_two_player:
				_select_controlled_player(_pk_kicker if team == 0 else _pk_gk)
				_select_controlled_player_p2(_pk_gk if team == 0 else _pk_kicker)
			else:
				_select_controlled_player(_pk_kicker if team == 0 else _pk_gk)

	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
	var round_num := (_pk_kick_index / 2) + 1
	var t_abbr := GameData.get_team_abbr(GameData.home_team_id if team == 0 else GameData.away_team_id)
	hud.update_pk_board(_pk_results, true)
	hud.show_banner("PK ROUND %d — %s" % [round_num, t_abbr], 1.4)
	SoundFX.play_sfx("whistle")

func _process_pk_shootout(delta: float) -> void:
	if _pk_wait_next_timer > 0.0:
		_pk_wait_next_timer -= delta
		if _pk_wait_next_timer <= 0.0:
			_pk_kick_index += 1
			_setup_pk_turn()
		return

	if not _pk_shot_taken:
		if _pk_kicker != null and ball.owner_player == _pk_kicker:
			if not _pk_kicker.is_human:
				_pk_ai_shoot_timer -= delta
				if _pk_ai_shoot_timer <= 0.0:
					var aim := Vector3(PitchBuilder.HALF_LEN, 0.0, randf_range(-2.9, 2.9))
					_pk_kicker._perform_shoot((aim - _pk_kicker.global_position).normalized(), randf_range(0.72, 0.94), true)
		else:
			_pk_shot_taken = true
			_pk_shot_elapsed = 0.0
	else:
		_pk_shot_elapsed += delta
		if _pk_gk != null and ball.owner_player == _pk_gk:
			_resolve_pk_turn(false)
		elif _pk_shot_elapsed > 3.2 or (_pk_shot_elapsed > 0.6 and ball.vel.length() < 0.35):
			_resolve_pk_turn(false)

func _resolve_pk_turn(scored: bool) -> void:
	if _pk_wait_next_timer > 0.0:
		return
	var team := _pk_kick_index & 1
	_pk_results[team].append("O" if scored else "X")
	if scored:
		if team == 0:
			MatchState.home_score += 1
		else:
			MatchState.away_score += 1
		MatchState.score_changed.emit(MatchState.home_score, MatchState.away_score)
		SoundFX.play_sfx("cheer")
		SoundFX.play_commentary("goal")
	else:
		SoundFX.play_sfx("whistle")

	var h_str := "".join(_pk_results[0])
	var a_str := "".join(_pk_results[1])
	hud.update_pk_board(_pk_results, true)
	hud.show_banner("%s!  PK: %d [%s] - %d [%s]" % ["GOAL" if scored else "SAVED / MISS", MatchState.home_score, h_str, MatchState.away_score, a_str], 1.7)

	# Check shootout completion (5 rounds + sudden death)
	var h_taken: int = _pk_results[0].size()
	var a_taken: int = _pk_results[1].size()
	var finished := false
	if h_taken <= 5 and a_taken <= 5:
		var h_rem := 5 - h_taken
		var a_rem := 5 - a_taken
		if MatchState.home_score > MatchState.away_score + a_rem or MatchState.away_score > MatchState.home_score + h_rem:
			finished = true
		elif h_taken == 5 and a_taken == 5 and MatchState.home_score != MatchState.away_score:
			finished = true
	elif h_taken == a_taken and MatchState.home_score != MatchState.away_score:
		finished = true

	if finished:
		MatchState.set_phase(MatchState.Phase.FULLTIME)
		hud.show_half_or_full_time(true)
		SoundFX.stop_crowd_ambience()
		var lost := (not GameData.is_ai_vs_ai) and (MatchState.home_score < MatchState.away_score)
		SoundFX.play_music("03G_OVER" if lost else "07RESULT")
	else:
		_pk_wait_next_timer = 1.8
