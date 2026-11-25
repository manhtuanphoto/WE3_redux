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
