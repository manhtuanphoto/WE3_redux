extends Node
## Tracks live match score, clock, phase, and statistics.

signal score_changed(home_score: int, away_score: int)
signal phase_changed(new_phase: Phase)
signal half_changed(half_num: int)
signal goal_scored(scoring_team: int, scorer_name: String)
signal event_logged(entry: String)

enum Phase {
	KICKOFF,
	OPEN_PLAY,
	GOAL_CELEBRATION,
	OUT_THROW_IN,
	OUT_CORNER,
	OUT_GOAL_KICK,
	OUT_FREE_KICK,
	HALFTIME,
	FULLTIME,
	PAUSED
}

var home_score: int = 0
var away_score: int = 0
var current_half: int = 1
var match_elapsed_real: float = 0.0
var phase: Phase = Phase.KICKOFF
var kickoff_team: int = 0  # 0 = Home, 1 = Away
var last_touch_team: int = 0
var last_touch_player_name: String = ""

# Match stats [home, away]
var shots: Array[int] = [0, 0]
var shots_on_target: Array[int] = [0, 0]
var passes_completed: Array[int] = [0, 0]
var possession_time: Array[float] = [0.0, 0.0]
var event_log: Array[String] = []

func log_event(msg: String) -> void:
	var line := "[%02d'] %s" % [get_display_minutes(), msg]
	event_log.append(line)
	if event_log.size() > 14:
		event_log.pop_front()
	event_logged.emit(line)
	print("[WE3_LOG] ", line)

func reset_match() -> void:
	home_score = 0
	away_score = 0
	current_half = 1
	match_elapsed_real = 0.0
	phase = Phase.KICKOFF
	kickoff_team = 0
	last_touch_team = 0
	last_touch_player_name = ""
	shots = [0, 0]
	shots_on_target = [0, 0]
	passes_completed = [0, 0]
	possession_time = [0.0, 0.0]
	event_log.clear()
	score_changed.emit(home_score, away_score)
	half_changed.emit(current_half)
	phase_changed.emit(phase)

func set_phase(new_phase: Phase) -> void:
	if phase != new_phase:
		phase = new_phase
		phase_changed.emit(phase)
		log_event("Phase -> %s" % Phase.keys()[new_phase])

func record_goal(scoring_team: int, scorer_name: String) -> void:
	if scoring_team == 0:
		home_score += 1
	else:
		away_score += 1
	kickoff_team = 1 - scoring_team
	score_changed.emit(home_score, away_score)
	goal_scored.emit(scoring_team, scorer_name)
	log_event("GOAL Team %d by %s (%d-%d)" % [scoring_team, scorer_name, home_score, away_score])
	set_phase(Phase.GOAL_CELEBRATION)

func get_display_minutes() -> int:
	var half_dur := maxf(GameData.half_duration_seconds, 10.0)
	var ratio := clampf(match_elapsed_real / half_dur, 0.0, 1.0)
	var base_min := 0 if current_half == 1 else 45
	return base_min + int(floor(ratio * 45.0))

func get_possession_percent(team_idx: int) -> int:
	var total := possession_time[0] + possession_time[1]
	if total <= 0.01:
		return 50
	return int(round((possession_time[team_idx] / total) * 100.0))
