extends Node
## Keeper test: fires N shots from random spots/powers at the away keeper and reports goals / saves / spills / misses.
##   godot --headless --fixed-fps 60 --path . res://tests/gk_shot_test.tscn

const N := 60

func _ready() -> void:
	call_deferred("_run")

func _run() -> void:
	GameData.is_ai_vs_ai = true
	GameData.home_team_id = 30
	GameData.away_team_id = 33
	var m := (load("res://scenes/match.tscn") as PackedScene).instantiate() as MatchManager
	get_tree().root.add_child(m)
	await get_tree().physics_frame
	var shooter: Player = m.players[10]
	var gk: Player = null
	for p in m.players:
		if p.team_side == 1 and p.role == Player.Role.GK:
			gk = p
	# park everyone else far away so only shooter vs keeper matters
	for p in m.players:
		if p != shooter and p != gk:
			p.global_position = Vector3(0.0, 0.0, 60.0 if p.team_side == 0 else -60.0)
			p.set_physics_process(false)
	var rng := RandomNumberGenerator.new()
	rng.seed = 7
	var goals := 0
	var saves0 := MatchState.gk_saves[1]
	var spills0 := MatchState.gk_spills[1]
	var rows: PackedStringArray = []
	for t in range(N):
		MatchState.set_phase(MatchState.Phase.OPEN_PLAY)
		m.ball.is_active_play = true
		gk.global_position = Vector3(PitchBuilder.HALF_LEN - 1.5, 0.0, 0.0)
		gk.vel = Vector3.ZERO
		gk.action_lock_timer = 0.0
		var dist := rng.randf_range(10.0, 28.0)
		var zpos := rng.randf_range(-12.0, 12.0)
		shooter.global_position = Vector3(PitchBuilder.HALF_LEN - dist, 0.0, zpos)
		m.ball.global_position = shooter.global_position + Vector3(0.6, 0.2, 0.0)
		m.ball.set_owner_player(shooter, 0)
		shooter.action_lock_timer = 0.0
		var before := MatchState.home_score
		var s0 := MatchState.gk_saves[1]
		var p0 := MatchState.gk_spills[1]
		var power := rng.randf_range(0.5, 1.0)
		var tz := rng.randf_range(-3.3, 3.3)   # aimed corner inside the posts
		var aim_pt := Vector3(PitchBuilder.HALF_LEN, 0.0, tz)
		shooter._perform_shoot((aim_pt - shooter.global_position).normalized(), power, true)
		var result := "miss/other"
		for f in range(240):
			await get_tree().physics_frame
			if MatchState.home_score > before:
				result = "GOAL"
				break
			if MatchState.phase != MatchState.Phase.OPEN_PLAY:
				result = "out"
				break
			if MatchState.gk_saves[1] > s0:
				result = "SAVE"
				break
			if MatchState.gk_spills[1] > p0:
				result = "SPILL"
				# keep watching: a spill can still cross the line
				for f2 in range(120):
					await get_tree().physics_frame
					if MatchState.home_score > before:
						result = "SPILL->GOAL"
						break
				break
		rows.append("shot %2d dist %4.1f z %5.1f pow %.2f -> %s" % [t, dist, zpos, power, result])
		if result.begins_with("GOAL") or result == "SPILL->GOAL":
			goals += 1
		# reset score so goal phase doesn't stall
		MatchState.home_score = before if result != "GOAL" else MatchState.home_score
	for r in rows:
		print(r)
	print("RESULT shots=%d goals=%d saves=%d spills=%d" % [N, goals, MatchState.gk_saves[1] - saves0, MatchState.gk_spills[1] - spills0])
	get_tree().quit(0)
