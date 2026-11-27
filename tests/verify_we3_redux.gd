extends Node
## Headless integration test runner for WE3 Redux.

var _failures: int = 0

func _assert(cond: bool, msg: String) -> void:
	if not cond:
		push_error("ASSERT FAILED: " + msg)
		_failures += 1
	else:
		print("PASS: ", msg)

func _ready() -> void:
	call_deferred("_run_tests")

func _run_tests() -> void:
	# 1. Verify Main Menu & Team Select scenes instantiate cleanly
	for sc_path in ["res://scenes/main_menu.tscn", "res://scenes/team_select.tscn"]:
		var sc := load(sc_path) as PackedScene
		_assert(sc != null, "Load scene " + sc_path)
		if sc != null:
			var inst := sc.instantiate()
			get_tree().root.add_child(inst)
			await get_tree().process_frame
			inst.queue_free()
			await get_tree().process_frame

	# 2. Verify GameData
	_assert(GameData.teams.size() == 43, "Loaded 43 national teams (got %d)" % GameData.teams.size())
	_assert(GameData.get_team_kit_texture(0) != null, "Loaded kit texture for team 0")
	var bra_col := GameData.get_team_primary_color(30)
	_assert(bra_col.r > 0.6 and bra_col.g > 0.5 and bra_col.b < 0.3, "Brazil CLUT-sampled primary color is yellow (got %s)" % str(bra_col))

	# 2b. Authentic WE3 player stat database (SLPM_861.62 stat words) + roster-driven lineups
	var ronaldo := GameData.get_player_record(30, 10)
	_assert(str(ronaldo.get("name", "")) == "Ronaldo" and int(ronaldo.get("stats", {}).get("speed", 0)) == 9, "Authentic stats: Ronaldo speed level 9 (#%d)" % int(ronaldo.get("shirt_number", 0)))
	_assert(GameData.get_team_default_formation(30) == "4-4-2", "Brazil authentic formation derived from starting XI = 4-4-2")
	var lineup_ok := true
	var shapes := {}
	for t in GameData.teams:
		var tid := int(t["team_id"])
		var fname := GameData.get_team_default_formation(tid)
		shapes[fname] = true
		var tmpl: Array = MatchManager.FORMATION_TEMPLATES[fname]
		var lu: Array[int] = MatchManager.assign_lineup(t["players"], tmpl)
		var seen := {}
		for s in range(lu.size()):
			if lu[s] < 0 or seen.has(lu[s]):
				lineup_ok = false
			seen[lu[s]] = true
		if str(t["players"][lu[0]].get("pos_group", "")) != "GK":
			lineup_ok = false
	_assert(lineup_ok, "All 43 teams build valid unique lineups with a real GK (formations: %s)" % ", ".join(shapes.keys()))

	# 3. Verify PlayerAnimCache & 8 WE3 Audio Tracks
	var lib := PlayerAnimCache.get_library()
	_assert(lib != null and lib.get_animation_list().size() == 36, "Loaded 36 WE3 animations into library (got %d)" % (lib.get_animation_list().size() if lib else 0))

	for trk in ["02TITLEV", "03G_OVER", "04STAFF", "05G_DEMO", "06P_DEMO", "07RESULT", "08SELECT", "09WINRUN"]:
		var st := load("res://assets/audio/%s.wav" % trk) as AudioStream
		_assert(st != null, "Audio track %s.wav loads cleanly" % trk)

	# 4. Instantiate full MatchScene and simulate ticks
	var match_sc := load("res://scenes/match.tscn") as PackedScene
	_assert(match_sc != null, "Load match.tscn")
	var match_inst := match_sc.instantiate() as MatchManager
	_assert(match_inst != null, "Instantiate MatchManager")
	get_tree().root.add_child(match_inst)
	await get_tree().physics_frame
	await get_tree().physics_frame

	_assert(match_inst.players.size() == 22, "Spawned 22 players (got %d)" % match_inst.players.size())
	_assert(match_inst.ball != null, "Ball instantiated")
	var ron: Player = null
	var ald: Player = null
	for pl in match_inst.players:
		if pl.player_name == "Ronaldo" and pl.team_side == 0:
			ron = pl
		elif pl.player_name == "Aldair" and pl.team_side == 0:
			ald = pl
	_assert(ron != null and ald != null and ron._sprint_speed > ald._sprint_speed and ron.shirt_number == 9, "Player stats drive speed: Ronaldo (spd 9) faster than Aldair (spd 6), shirt #9")
	_assert(match_inst.players[0].pos_group == "GK" and match_inst.players[0].role == Player.Role.GK, "Slot 0 is the authentic goalkeeper")
	match_inst.ball.kick(Vector3(12.0, 5.0, 0.0), null, 0)
	var pred: Vector3 = match_inst.ball.predict_landing_point(30)
	_assert(pred.x > 1.0, "Ball landing predictor projects forward (x=%.2f)" % pred.x)
	_assert(match_inst.controlled_player != null, "Controlled human player assigned")
	var stad := match_inst.pitch.get_node_or_null("ExtractedStadium_GRDM_C")
	_assert(stad != null and stad.get_child_count() >= 6, "Extracted 3D stadium GRDM_C loaded (%d parts)" % (stad.get_child_count() if stad else 0))

	# 5. Simulate ball crossing Home's attack goal line (+X) during OPEN_PLAY
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
	match_inst.ball.is_active_play = true
	match_inst.ball.owner_player = null
	match_inst.ball.global_position = Vector3(PitchBuilder.HALF_LEN + 0.5, 0.5, 0.0)
	match_inst.ball.vel = Vector3(10.0, 0.0, 0.0)
	await get_tree().physics_frame
	await get_tree().physics_frame
	_assert(MatchState.home_score == 1, "Goal detection increments Home score (got %d)" % MatchState.home_score)

	# 6. Simulate out-of-bounds throw-in
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
	match_inst.ball.is_active_play = true
	match_inst.ball.owner_player = null
	match_inst.ball.global_position = Vector3(10.0, 0.2, PitchBuilder.HALF_WID + 0.5)
	match_inst.ball.vel = Vector3(0.0, 0.0, 5.0)
	await get_tree().physics_frame
	await get_tree().physics_frame
	_assert(MatchState.phase == MatchState.Phase.OUT_THROW_IN, "Touchline out-of-bounds triggers THROW_IN phase")

	# 6b. Verify authentic WE3 foul & card rules (func_80052720)
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
	var home_gk: Player = match_inst.players[0]
	var home_def: Player = match_inst.players[1]
	var away_fwd: Player = match_inst.players[21]
	away_fwd.global_position = Vector3(0.0, 0.0, 0.0)
	away_fwd.rotation.y = 0.0   # facing -Z
	home_def.global_position = Vector3(0.0, 0.0, 3.0)   # directly behind away_fwd (+Z -> -Z)
	_assert(home_gk._eval_we3_foul_severity(away_fwd) == 0, "GK challenge never triggers foul (func_80052720)")
	_assert(home_def._eval_we3_foul_severity(home_gk) == 0, "Same-team challenge never triggers foul")
	var behind_sev := home_def._eval_we3_foul_severity(away_fwd)
	_assert(behind_sev >= 1 and behind_sev <= 3, "Slide straight from behind always fouls (got %d)" % behind_sev)
	for bi in range(2, 6):
		match_inst.players[bi].yellow_cards = 1
	_assert(home_def._eval_we3_foul_severity(away_fwd) == 1, "Team >=4 booked players caps new card to foul-only (1)")
	for bi in range(2, 6):
		match_inst.players[bi].yellow_cards = 0

	# 6c. Verify free-kick goal-area clamping (func_800772F8) & penalty box clearance
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
	match_inst._on_out_of_bounds("foul", Vector3(PitchBuilder.HALF_LEN - 1.0, 0.0, 2.0), 0)
	_assert(absf(match_inst._pending_restart_pos.x) <= 47.14 and absf(match_inst._pending_restart_pos.z) >= 8.91, "Free-kick spot inside goal area clamped out (func_800772F8)")
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
	match_inst._on_out_of_bounds("penalty", Vector3(PitchBuilder.HALF_LEN - 11.0, 0.0, 0.0), 0)
	match_inst._execute_set_piece_restart()
	var away_gk: Player = match_inst.players[11]
	_assert(absf(away_gk.global_position.x - (PitchBuilder.HALF_LEN - 0.4)) < 0.2, "Penalty kick places defending GK on goal line")

	# 6d. Verify aerial header contest (func_80035A5C)
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
	var home_fw: Player = match_inst.players[10]
	var prev_shots := MatchState.shot_attempts[0]
	home_fw.action_lock_timer = 0.0
	home_fw.global_position = Vector3(PitchBuilder.HALF_LEN - 10.0, 0.0, 0.0)
	match_inst.ball.owner_player = null
	match_inst.ball.pickup_cooldown = 0.0
	match_inst.ball.global_position = Vector3(PitchBuilder.HALF_LEN - 10.0, 1.65, 0.0)
	match_inst.ball.vel = Vector3(0.0, -1.0, 0.0)
	home_fw._try_pickup_ball()
	_assert(MatchState.shot_attempts[0] == prev_shots + 1 and match_inst.ball.vel.x > 8.0, "Aerial ball at 1.65m triggers header shot toward goal (func_80035A5C)")

	match_inst.queue_free()
	await get_tree().process_frame

	# 7. Verify AI vs AI mode spawns cleanly and simulates 60 physics frames without errors
	GameData.is_ai_vs_ai = true
	var ai_match := match_sc.instantiate() as MatchManager
	get_tree().root.add_child(ai_match)
	await get_tree().physics_frame
	await get_tree().physics_frame
	_assert(ai_match.players.size() == 22, "AI vs AI spawned 22 players")
	_assert(ai_match.controlled_player == null, "AI vs AI has no human controlled player")
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
	for _f in range(60):
		await get_tree().physics_frame
	var moved := ai_match.players[10].global_position.distance_to(Vector3(-0.65, 0.0, 0.0)) > 0.1
	_assert(moved, "AI vs AI striker moves actively during OPEN_PLAY simulation")
	ai_match.queue_free()
	await get_tree().process_frame
	GameData.is_ai_vs_ai = false

	# 8. Verify PK Shootout mode (func_800820A0)
	GameData.game_mode = "pk"
	var pk_match := match_sc.instantiate() as MatchManager
	get_tree().root.add_child(pk_match)
	await get_tree().physics_frame
	await get_tree().physics_frame
	_assert(pk_match._pk_active and pk_match._pk_kicker != null and pk_match._pk_gk != null, "PK Shootout initializes kicker & GK on penalty spot (func_800820A0)")
	pk_match._resolve_pk_turn(true)
	_assert(MatchState.home_score == 1 and pk_match._pk_results[0].size() == 1, "PK Shootout scores goal and logs round result")

	# 8b. Verify ball perimeter adboard bounce (func_800A8F3C) & goal-net containment
	pk_match.ball.is_active_play = false
	pk_match.ball.owner_player = null
	pk_match.ball.vel = Vector3(0.0, 0.0, 10.0)
	var bounced := pk_match.ball._collide_goal_net_and_adboards(Vector3(10.0, 0.2, 40.5))
	_assert(bounced.z < 40.0 and pk_match.ball.vel.z < 0.0, "Adboard collision reflects ball & halves speed (func_800A8F3C)")
	pk_match.ball.vel = Vector3(12.0, 0.0, 0.0)
	var netted := pk_match.ball._collide_goal_net_and_adboards(Vector3(PitchBuilder.HALF_LEN + 2.5, 1.0, 0.0))
	_assert(netted.x < PitchBuilder.HALF_LEN + 2.2 and pk_match.ball.vel.x < 0.0, "Goal net catches & damps ball inside net pocket")

	pk_match.queue_free()
	await get_tree().process_frame
	GameData.game_mode = "exhibition"

	# 9. Verify Night + Rain stadium mode (floodlights + rain particles + VRAM texture swap)
	GameData.stadium_is_night = true
	GameData.stadium_is_rain = true
	var nr_match := match_sc.instantiate() as MatchManager
	get_tree().root.add_child(nr_match)
	await get_tree().physics_frame
	_assert(nr_match.pitch.get_node_or_null("StadiumFloodlights") != null, "Night mode spawns StadiumFloodlights")
	_assert(nr_match.pitch.get_node_or_null("StadiumRain") != null, "Rain mode spawns StadiumRain particles")
	_assert(nr_match.players[0]._shadow_mesh.get_child_count() == 4, "Night mode uses 4 floodlight shadow quads per player (func_8001A968)")
	nr_match.ball.reset_to_center()
	nr_match.ball.kick(Vector3(10.0, 2.5, 0.0), null, 0, 2.0, 1)
	nr_match.ball.pickup_cooldown = 5.0
	var p0_spin: int = nr_match.ball._spin_period
	for _bf in range(36):
		await get_tree().physics_frame
	_assert(p0_spin > 0 and nr_match.ball._spin_period >= p0_spin + 2, "Ball bounce increments spin_period by +2 (func_8004BAA4)")

	# 9b. Verify high-speed ball deflection (func_8004DEFC)
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
	var blk_df: Player = nr_match.players[1]
	blk_df.action_lock_timer = 0.0
	blk_df.stats["technique"] = 3
	blk_df.global_position = Vector3(0.0, 0.0, 0.0)
	nr_match.ball.owner_player = null
	nr_match.ball.pickup_cooldown = 0.0
	nr_match.ball.flight_frames = 4
	nr_match.ball.global_position = Vector3(-0.3, 0.2, 0.0)
	nr_match.ball.vel = Vector3(17.0, 0.0, 0.0)
	blk_df._try_pickup_ball()
	_assert(nr_match.ball.owner_player == null and nr_match.ball.vel.length() < 10.0, "High-speed shot (>thresh) deflects off low-technique defender (func_8004DEFC)")

	# 9c. Verify in-match substitution (bench player replaces starter with 100% stamina)
	nr_match.players[10].stamina = 22.0
	var old_st_name: String = nr_match.players[10].player_name
	nr_match._on_substitution_requested(10, 11)
	_assert(MatchState.subs_used[0] == 1 and nr_match.players[10].stamina == 100.0 and nr_match.players[10].player_name != old_st_name, "Substitution replaces starter with bench player at 100% stamina")

	# 9d. Verify free-kick defensive wall forms at ~9.15m for direct free kicks near goal
	var fk_pos := Vector3(PitchBuilder.HALF_LEN - 20.0, 0.0, 4.0)
	nr_match._pending_restart_pos = fk_pos
	nr_match._pending_restart_team = 0
	nr_match._pending_restart_type = "foul"
	MatchState.set_phase(MatchState.Phase.OUT_FREE_KICK)
	nr_match._execute_set_piece_restart()
	var wall_count := 0
	for p in nr_match.players:
		if p.team_side == 1 and p.role != Player.Role.GK:
			var dist_fk := p.global_position.distance_to(fk_pos)
			if dist_fk >= 8.8 and dist_fk <= 9.6:
				wall_count += 1
	_assert(wall_count >= 3, "Direct free kick near goal forms defensive wall at 9.15m (got %d)" % wall_count)

	# 9e. Verify PK Shootout HUD board becomes visible and formats dots
	nr_match._start_pk_shootout()
	nr_match._resolve_pk_turn(true)
	_assert(nr_match.hud._pk_panel.visible and "●" in nr_match.hud._pk_home_lbl.text, "PK Shootout HUD scoreboard displays round dots")

	# 9f. Verify 2-Player Local Versus (1P vs 2P) and Instant Replay playback
	GameData.is_two_player = true
	GameData.is_ai_vs_ai = false
	var vs_match := (load("res://scenes/match.tscn") as PackedScene).instantiate() as MatchManager
	add_child(vs_match)
	await get_tree().process_frame
	_assert(InputMap.has_action("p2_shoot") and vs_match.controlled_player != null and vs_match.controlled_player_p2 != null and vs_match.controlled_player.human_slot == 1 and vs_match.controlled_player_p2.human_slot == 2, "1P vs 2P assigns P1 (slot 1) & P2 (slot 2) controlled players and P2 InputMap")
	vs_match.hud._process(0.02)
	_assert(vs_match.hud._p2_panel.visible and vs_match.hud._p2_player_label.text == vs_match.controlled_player_p2.player_name.to_upper(), "2P HUD nameplate displays active 2P player in 1P vs 2P")
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
	for _rf in range(35):
		await get_tree().physics_frame
		vs_match._process(0.016)
	vs_match._on_goal_detected(0)
	vs_match._phase_timer = 0.01
	vs_match._process(0.02)
	_assert(vs_match._is_replaying and vs_match.hud._replay_badge.visible, "Goal Instant Replay activates with REPLAY badge after celebration")
	vs_match.pitch._process(0.05)
	_assert(vs_match.pitch._net_ripple_t[1] >= 0.0 and vs_match.pitch._net_right.position.x > vs_match.pitch._net_right_x, "Goal Net Ripple 3D animates back net mesh on goal")
	vs_match.hud.update_tactics(2, true)
	_assert(vs_match.hud._tactic_badge.text == "ATK+TRAP", "In-match Tactics HUD badge updates to ATK+TRAP")
	vs_match.queue_free()

	# 9g. Verify Cup Knockout Tournament progression (Quarter-Final -> Semi-Final -> Final)
	GameData.start_cup_tournament(30)
	_assert(GameData.cup_active and GameData.get_cup_round_name() == "QUARTER-FINAL" and GameData.cup_opponents.size() == 3, "Cup tournament initializes Quarter-Final with 3 opponents")
	var adv1 := GameData.advance_cup_round()
	var adv2 := GameData.advance_cup_round()
	var adv3 := GameData.advance_cup_round()
	_assert(adv1 and adv2 and not adv3 and not GameData.cup_active, "Cup tournament advances QF -> SF -> Final -> Champion")

	# 9h. Verify Pre-match PS1 Matchup Card & Condition Arrows
	GameData.team_condition = [4, 0]
	nr_match.hud.show_prematch_card("Brazil", "Argentina", "4-4-2", "4-4-2", "GRDM_C")
	_assert(nr_match.hud._prematch_panel.visible and "↑" in nr_match.hud._prematch_label.text and "↓" in nr_match.hud._prematch_label.text, "Pre-match PS1 card displays teams, formation, and condition arrows")
	nr_match.hud.hide_prematch_card()
	_assert(not nr_match.hud._prematch_panel.visible, "Pre-match PS1 card hides on OPEN_PLAY")

	# 9i. Verify Condition Form speed scaling, GK Rush, and One-Two run
	var p_cond: Player = nr_match.players[10]
	GameData.team_condition[0] = 0
	p_cond.apply_profile(GameData.get_team(GameData.home_team_id)["players"][10], 10)
	var spd_bad := p_cond._sprint_speed
	GameData.team_condition[0] = 4
	p_cond.apply_profile(GameData.get_team(GameData.home_team_id)["players"][10], 10)
	_assert(p_cond._sprint_speed > spd_bad, "Condition Form arrow TOP boosts sprint speed vs BAD")
	GameData.team_condition = [2, 2]
	MatchState.gk_rush[0] = true
	var gk0: Player = nr_match.players[0]
	gk0.global_position = Vector3(-50.0, 0.0, 0.0)
	gk0.vel = Vector3.ZERO
	gk0.action_lock_timer = 0.0
	nr_match.ball.owner_player = null
	nr_match.ball.global_position = Vector3(-30.0, 0.2, 0.0)
	for _gt in range(6):
		gk0._process_gk_ai(0.035)
	_assert(gk0.vel.x > 0.1, "GK Rush sprints keeper toward ball")
	MatchState.gk_rush[0] = false

	# 9j. Verify extracted flag_shapes.json + HATA_CSR produces 64x40 flag textures
	var f_bra := GameData.get_team_flag_texture(30)
	_assert(f_bra != null and f_bra.get_width() == 64 and f_bra.get_height() == 40, "Extracted HATA_CSR + flag_shapes.json produces 64x40 flag texture")

	# 9k. Verify Free-Kick Wall Jump & Deflection (func_8007B39C)
	var wall_p: Player = nr_match.players[12]
	wall_p.global_position = Vector3(10.0, 0.0, 0.0)
	wall_p.trigger_wall_jump()
	nr_match.ball.owner_player = null
	nr_match.ball.last_touch_team = 0
	nr_match.ball.global_position = Vector3(10.0, 1.8, 0.0)
	nr_match.ball.vel = Vector3(18.0, 0.0, 0.0)
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
	wall_p._physics_process(0.15)
	_assert(wall_p.position.y > 0.2 and nr_match.ball.vel.x < 0.0, "Free-kick wall player jumps and deflects shot (func_8007B39C)")

	# 9l. Verify func_8002D9F4 weak-foot 16/17 power penalty
	var shooter: Player = nr_match.players[10]
	shooter.global_position = Vector3(30.0, 0.0, 0.0)
	shooter.rotation.y = -PI * 0.5 # facing +X
	shooter.stats["technique"] = 9
	nr_match.ball.global_position = Vector3(30.2, 0.1, 0.5) # right foot (+X local)
	shooter._perform_shoot(Vector3(1.0, 0.0, 0.0), 1.0, true)
	var spd_strong := shooter._pending_kick_vel.length()
	nr_match.ball.global_position = Vector3(30.2, 0.1, -0.5) # left/weak foot (-X local)
	shooter._perform_shoot(Vector3(1.0, 0.0, 0.0), 1.0, true)
	var spd_weak := shooter._pending_kick_vel.length()
	_assert(spd_weak < spd_strong, "Weak-foot shot applies 16/17 power penalty (func_8002D9F4)")

	# 9m. Verify func_80066150 wing/flank switch evaluation
	for i in range(1, 11):
		nr_match.players[i].global_position = Vector3(-25.0, 0.0, 0.0)
	var passer: Player = nr_match.players[6]
	passer.global_position = Vector3(-5.0, 0.0, 0.0)
	var winger: Player = nr_match.players[5]
	winger.global_position = Vector3(-1.0, 0.0, 24.0)
	nr_match.ball.global_position = Vector3(-5.0, 0.1, 0.0)
	_assert(passer._find_wing_switch_target(1.0) == winger, "Wing switch evaluator picks open winger at |z|=24m (func_80066150)")

	# 9n. Verify func_8002F2B0 iterative pass power solver
	var solved_spd := passer._solve_pass_speed(20.0, 3.0, winger)
	_assert(solved_spd >= 6.0 and solved_spd <= 16.0, "Ball-flight pass solver returns valid speed for 20m pass (func_8002F2B0)")

	# 9o. Verify func_8008D358 GK high-ball jump/punch
	gk0.global_position = Vector3(-52.0, 0.0, 0.0)
	gk0._gk_start_dive(Vector3(-52.0, 2.0, 0.5))
	gk0._process_ai(0.10)
	_assert(gk0._gk_jump_height > 0.2 and gk0.position.y > 0.1, "GK high-ball jump leaps vertically for aerial shots (func_8008D358)")
	gk0._gk_dive_finish()

	# 9p. Verify func_80031024 dribble ball snap-back when ball falls behind carrier
	var drib: Player = nr_match.players[10]
	drib.global_position = Vector3(0.0, 0.0, 0.0)
	drib.rotation.y = -PI * 0.5 # facing +X
	nr_match.ball.set_owner_player(drib, 0)
	nr_match.ball.global_position = Vector3(-1.2, Ball.BALL_RADIUS, 0.0) # behind
	nr_match.ball._physics_process(0.033)
	_assert(nr_match.ball.global_position.x > -1.0, "Dribble snap-back pulls ball in front of carrier (func_80031024)")
	GameData.is_two_player = false

	nr_match.queue_free()
	await get_tree().process_frame
	GameData.stadium_is_night = false
	GameData.stadium_is_rain = false
	PlayerAnimCache._cached_library = null

	if _failures == 0:
		print("ALL TESTS PASSED!")
		get_tree().quit(0)
	else:
		print("TESTS FAILED: ", _failures)
		get_tree().quit(1)
