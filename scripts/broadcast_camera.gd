class_name BroadcastCamera
extends Camera3D
## Winning Eleven style wide broadcast camera with smooth ball tracking and mode toggle.

enum Mode { WIDE, DYNAMIC, OVERHEAD }

var target_ball: Node3D = null
var mode: Mode = Mode.WIDE
var _look_target: Vector3 = Vector3.ZERO

func _ready() -> void:
	fov = 42.0
	near = 0.5
	far = 300.0
	_snap_to_target()

func _process(delta: float) -> void:
	if Input.is_action_just_pressed("camera_toggle"):
		mode = ((mode + 1) % 3) as Mode

	if target_ball == null or not is_instance_valid(target_ball):
		return

	var bpos := target_ball.global_position
	var clamped_x := clampf(bpos.x * 0.82, -38.0, 38.0)
	var clamped_z := clampf(bpos.z * 0.55, -18.0, 18.0)

	var desired_pos := Vector3.ZERO
	var desired_look := Vector3(clamped_x, 2.0, clamped_z - 4.0)

	if MatchState.phase == MatchState.Phase.KICKOFF:
		# Pre-match stadium entrance sweep (enter overlay func_8018957C)
		desired_pos = Vector3(clamped_x - 8.0, 11.5, 24.0)
		desired_look = Vector3(0.0, 1.0, 0.0)
		fov = lerpf(fov, 40.0, delta * 3.5)
	elif MatchState.phase == MatchState.Phase.GOAL_CELEBRATION:
		desired_pos = Vector3(clamped_x * 0.9, 9.5, clamped_z + 14.0)
		desired_look = Vector3(clamped_x, 1.0, clamped_z)
		fov = lerpf(fov, 36.0, delta * 4.0)
	else:
		match mode:
			Mode.WIDE:
				desired_pos = Vector3(clamped_x * 0.80, 17.5, clamped_z + 34.0)
				fov = lerpf(fov, 48.0, delta * 5.0)
			Mode.DYNAMIC:
				desired_pos = Vector3(clamped_x - 4.0, 13.5, clamped_z + 22.0)
				desired_look = Vector3(clamped_x, 1.2, clamped_z - 2.0)
				fov = lerpf(fov, 50.0, delta * 5.0)
			Mode.OVERHEAD:
				desired_pos = Vector3(clamped_x * 0.7, 38.0, clamped_z + 12.0)
				desired_look = Vector3(clamped_x, 0.5, clamped_z)
				fov = lerpf(fov, 48.0, delta * 5.0)

	global_position = global_position.lerp(desired_pos, clampf(delta * 5.5, 0.0, 1.0))
	_look_target = _look_target.lerp(desired_look, clampf(delta * 7.5, 0.0, 1.0))
	look_at(_look_target, Vector3.UP)

func _snap_to_target() -> void:
	global_position = Vector3(0.0, 21.0, 29.0)
	look_at(Vector3.ZERO, Vector3.UP)
