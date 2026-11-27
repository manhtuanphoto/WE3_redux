extends Node
## AI vs AI telemetry harness (headless). Run:
##   godot --headless --fixed-fps 60 --path . res://tests/sim_telemetry.tscn
## Prints movement/pass/shot statistics so AI & motion-model fidelity changes can be compared numerically.

const FRAMES := 21600   # 360 s of match time at 60 Hz

func _ready() -> void:
	call_deferred("_run")

func _run() -> void:
	GameData.is_ai_vs_ai = true
	GameData.half_duration_seconds = 400.0
	GameData.home_team_id = 30
	GameData.away_team_id = 33
	var packed := load("res://scenes/match.tscn") as PackedScene
	var m := packed.instantiate() as MatchManager
	get_tree().root.add_child(m)
	await get_tree().physics_frame
	MatchState.set_phase(MatchState.Phase.OPEN_PLAY)

	var max_speed := {}
	var sum_speed := 0.0
	var samples := 0
	var ball_travel := 0.0
	var last_ball := m.ball.global_position
	var near_ball_max := 0
	var stack_pairs := 0
	var sprint_frames := 0
	var moving_frames := 0
	var phase_counts := {}
	var own_zone := [0, 0]
	var owner_changes := 0
	var last_owner: Node3D = null
	var box_possessions := [0, 0]
	var in_zone_prev := [false, false]
	var final_third: Array[int] = [0, 0]
	var min_goal_dist: Array[float] = [99.0, 99.0]
	for f in range(FRAMES):
		await get_tree().physics_frame
		phase_counts[MatchState.phase] = int(phase_counts.get(MatchState.phase, 0)) + 1
		var bx := m.ball.global_position.x
		if bx > 17.5:
			final_third[0] += 1
		if bx < -17.5:
			final_third[1] += 1
		min_goal_dist[0] = minf(min_goal_dist[0], m.ball.global_position.distance_to(Vector3(PitchBuilder.HALF_LEN, 0, 0)))
		min_goal_dist[1] = minf(min_goal_dist[1], m.ball.global_position.distance_to(Vector3(-PitchBuilder.HALF_LEN, 0, 0)))
		if OS.get_environment("WE3_VERBOSE") != "" and f % 30 == 0 and f < 2400:
			var ow: Node3D = m.ball.owner_player
			print("t=%5.1f ball(%6.1f,%5.1f,y%.1f) v=%.1f owner=%s" % [f / 60.0, m.ball.global_position.x, m.ball.global_position.z, m.ball.global_position.y, m.ball.vel.length(), (str((ow as Player).player_name) + "/" + str((ow as Player).team_side)) if ow != null else "-"])
		var cur_owner: Node3D = m.ball.owner_player
		if cur_owner != last_owner and cur_owner != null:
			owner_changes += 1
		last_owner = cur_owner if cur_owner != null else last_owner
		for side_i in range(2):
			var atk_goal_x := PitchBuilder.HALF_LEN if side_i == 0 else -PitchBuilder.HALF_LEN
			var zone_now := cur_owner != null and (cur_owner as Player).team_side == side_i and absf(cur_owner.global_position.x - atk_goal_x) < 24.0
			if zone_now:
				own_zone[side_i] += 1
			if zone_now and not in_zone_prev[side_i]:
				box_possessions[side_i] += 1
			in_zone_prev[side_i] = zone_now
		ball_travel += m.ball.global_position.distance_to(last_ball)
		last_ball = m.ball.global_position
		if f % 6 == 0:
			var near := 0
			for p in m.players:
				var sp := Vector2(p.vel.x, p.vel.z).length()
				max_speed[p.player_name] = maxf(float(max_speed.get(p.player_name, 0.0)), sp)
				if p.role != Player.Role.GK:
					sum_speed += sp
					samples += 1
					if sp > 0.5:
						moving_frames += 1
					if sp > 6.6:
						sprint_frames += 1
				if p.global_position.distance_to(m.ball.global_position) < 6.0:
					near += 1
			near_ball_max = maxi(near_ball_max, near)
			# stacking: two same-team outfielders closer than 1.0 m
			for i in range(m.players.size()):
				for j in range(i + 1, m.players.size()):
					var a: Player = m.players[i]
					var b: Player = m.players[j]
					if a.team_side == b.team_side and a.global_position.distance_to(b.global_position) < 1.0:
						stack_pairs += 1
	var top: Array = []
	for k in max_speed.keys():
		top.append([k, max_speed[k]])
	top.sort_custom(func(a, b): return a[1] > b[1])
	print("=== TELEMETRY %d frames ===" % FRAMES)
	print("avg outfield speed %.2f m/s | moving %.0f%% | sprint-ish %.1f%%" % [sum_speed / maxf(1, samples), 100.0 * moving_frames / maxf(1, samples), 100.0 * sprint_frames / maxf(1, samples)])
	print("top speeds: ", top.slice(0, 5))
	print("ball travelled %.0f m | max players within 6m of ball: %d | same-team stack samples: %d" % [ball_travel, near_ball_max, stack_pairs])
	print("GK saves %s spills %s" % [MatchState.gk_saves, MatchState.gk_spills])
	print("score %d-%d | shots %s (attempts %s) | passes %s | poss %d%%/%d%%" % [MatchState.home_score, MatchState.away_score, MatchState.shots, MatchState.shot_attempts, MatchState.passes_completed, MatchState.get_possession_percent(0), MatchState.get_possession_percent(1)])
	print("ball in attacking third (frames) home %d away %d | closest ball-to-goal home-att %.1f m away-att %.1f m" % [final_third[0], final_third[1], min_goal_dist[0], min_goal_dist[1]])
	print("phase frames: ", phase_counts)
	print("owner changes %d | frames owner in shooting zone (<24m) home %d away %d | zone entries home %d away %d" % [owner_changes, own_zone[0], own_zone[1], box_possessions[0], box_possessions[1]])
	get_tree().quit(0)
