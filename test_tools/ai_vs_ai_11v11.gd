extends Node
## Standalone Test Tool: 11vs11 AI vs AI Match (Brazil vs Argentina by default).

func _ready() -> void:
	GameData.home_team_id = 30  # Brazil
	GameData.away_team_id = 33  # Argentina
	GameData.is_ai_vs_ai = true
	GameData.selected_stadium = "GRDM_C"
	get_tree().change_scene_to_file.call_deferred("res://scenes/match.tscn")
