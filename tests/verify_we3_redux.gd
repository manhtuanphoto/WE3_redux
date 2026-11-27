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
	PlayerAnimCache._cached_library = null

	if _failures == 0:
		print("ALL TESTS PASSED!")
		get_tree().quit(0)
	else:
		print("TESTS FAILED: ", _failures)
		get_tree().quit(1)
