class_name Ball
extends Node3D
## Arcade-simulation soccer ball with gravity, bounce, spin, goalpost collision, and dribble lock.

signal goal_detected(scoring_team: int)
signal out_of_bounds_detected(event_type: String, restart_pos: Vector3, restart_team: int)

enum BallState { STOPPED, AIRBORNE, ROLLING, ATTACHED }

const BALL_RADIUS := 0.16
const GRAVITY := 10.35   # 736 raw/frame^2 (func_8004BAA4 case 0): 2.875 u * 0.12 m/s per 30 Hz frame
const GRAVITY_BY_TYPE := [10.35, 9.90, 11.25, 11.5875]   # func_8004BAA4: 0x2E0, 0x2C0, 0x320, 0x338
const TICK := 1.0 / 30.0  # WE3 logic frame
const DRAG_K := 6881.0    # ((s3*h)>>21)/7 expressed in m/s per frame: dv = s3*v/6881
const GROUND_FRICTION := 1.65
const AIR_DRAG := 0.28

var state: BallState = BallState.STOPPED
var vel: Vector3 = Vector3.ZERO
var angular_spin_y: float = 0.0
var flight_type: int = 0         # func_8004BAA4 Scratch 0xBC (0=normal, 1=loft, 2=driven, 3=heavy)
var _spin_period: int = 0        # func_8004BAA4 Scratch 0xB2 (0=none, +2 per bounce)
var _spin_dir: float = 0.0       # func_8004BAA4 Scratch 0xB3 (+1 / -1)
var owner_player: Node3D = null
var last_touch_player: Node3D = null
var last_touch_team: int = 0
var pickup_cooldown: float = 0.0
var is_active_play: bool = true
var offside_target: Node3D = null
var offside_defending_team: int = 0

var _tick_acc: float = 0.0
var flight_frames: int = 0   # WE3 scratch+0x31C: logic frames since the ball was last touched
var _mesh_node: MeshInstance3D
var _shadow_node: MeshInstance3D

func _ready() -> void:
	_build_visuals()
	reset_to_center()

func _build_visuals() -> void:
	_mesh_node = MeshInstance3D.new()
	_mesh_node.name = "BallMesh"
	var obj_res: Variant = load("res://assets/3d/ball.obj")
	if obj_res is Mesh:
		_mesh_node.mesh = obj_res
	else:
		var sphere := SphereMesh.new()
		sphere.radius = BALL_RADIUS
		sphere.height = BALL_RADIUS * 2.0
		_mesh_node.mesh = sphere

	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	var ball_tex_path := "res://assets/3d/ball_tex_equirect.png" if ResourceLoader.exists("res://assets/3d/ball_tex_equirect.png") else "res://assets/3d/ball_tex.png"
	if ResourceLoader.exists(ball_tex_path):
		mat.albedo_texture = load(ball_tex_path) as Texture2D
		mat.albedo_color = Color.WHITE
		mat.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	else:
		mat.albedo_color = Color(0.95, 0.95, 0.93)
	_mesh_node.material_override = mat
	add_child(_mesh_node)

	# Soft radial ground shadow disc so ball height is always readable
	_shadow_node = MeshInstance3D.new()
	_shadow_node.name = "BallShadow"
	var qm := QuadMesh.new()
	qm.size = Vector2(BALL_RADIUS * 2.8, BALL_RADIUS * 2.8)
	_shadow_node.mesh = qm
	var sh_mat := StandardMaterial3D.new()
	sh_mat.albedo_texture = PlayerAnimCache.get_shadow_texture()
	sh_mat.albedo_color = Color.WHITE
	sh_mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	sh_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	sh_mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	_shadow_node.material_override = sh_mat
	_shadow_node.rotation.x = -PI * 0.5
	_shadow_node.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(_shadow_node)

func reset_to_center() -> void:
	global_position = Vector3(0.0, BALL_RADIUS, 0.0)
	vel = Vector3.ZERO
	angular_spin_y = 0.0
	owner_player = null
	state = BallState.STOPPED
	pickup_cooldown = 0.0
	is_active_play = true
	_update_shadow()

func place_at(pos: Vector3) -> void:
	global_position = Vector3(pos.x, BALL_RADIUS, pos.z)
	vel = Vector3.ZERO
	angular_spin_y = 0.0
	owner_player = null
	state = BallState.STOPPED
	pickup_cooldown = 0.0
	_update_shadow()

func set_owner_player(p: Node3D, team_idx: int) -> void:
	owner_player = p
	last_touch_player = p
	last_touch_team = team_idx
	vel = Vector3.ZERO
	flight_frames = 0
	state = BallState.ATTACHED
	offside_target = null

func kick(kick_vel: Vector3, kicker: Node3D, team_idx: int, spin_y: float = 0.0, p_flight_type: int = 0) -> void:
	owner_player = null
	last_touch_player = kicker
	last_touch_team = team_idx
	vel = kick_vel
	flight_frames = 0
	angular_spin_y = spin_y
	flight_type = clampi(p_flight_type, 0, 3)
	if absf(spin_y) > 0.05:
		_spin_period = clampi(int(round(lerpf(12.0, 4.0, clampf(absf(spin_y) / 2.5, 0.0, 1.0)))), 3, 16)
		_spin_dir = signf(spin_y)
	else:
		_spin_period = 0
		_spin_dir = 0.0
	state = BallState.AIRBORNE if kick_vel.y > 0.2 else BallState.ROLLING
	pickup_cooldown = 0.16
	offside_target = null
	SoundFX.play_sfx("kick")

func predict_landing_point(max_steps: int = 45) -> Vector3:
	# Mirrors WE3 func_8004B2E0: simulate forward (30 Hz frames) to find ball intercept/landing point
	if owner_player != null or state == BallState.STOPPED:
		return global_position
	var sim_pos := global_position
	var sim_vel := vel
	for _i in range(max_steps):
		var h := Vector2(sim_vel.x, sim_vel.z).length()
		var air := sim_pos.y > BALL_RADIUS + 0.02 or sim_vel.y > 0.0
		if air:
			var s3 := sqrt(h * h + sim_vel.y * sim_vel.y)
			if h > 0.0:
				var nh := maxf(0.0, h - s3 * h / DRAG_K)
				sim_vel.x *= nh / h
				sim_vel.z *= nh / h
			sim_vel.y -= s3 * sim_vel.y / (DRAG_K * 4.0)
			sim_vel.y -= GRAVITY * TICK
		else:
			var nh2 := maxf(0.0, h * (1.0 - 1.0 / 64.0) - h * h / DRAG_K)
			if h > 0.0:
				sim_vel.x *= nh2 / h
				sim_vel.z *= nh2 / h
		sim_pos += sim_vel * TICK
		if sim_pos.y <= BALL_RADIUS:
			sim_pos.y = BALL_RADIUS
			if absf(sim_vel.y) < 0.7:
				return sim_pos
			sim_vel.y = -sim_vel.y * 0.65
			sim_vel.x *= 0.8
			sim_vel.z *= 0.8
	return sim_pos

## Ball position after `frames` 30 Hz logic frames (func_8004B2E0 style forward simulation, no early exit).
func position_after(frames: int) -> Vector3:
	if owner_player != null:
		return global_position
	var p := global_position
	var v := vel
	for _i in range(frames):
		var h := Vector2(v.x, v.z).length()
		if p.y > BALL_RADIUS + 0.005 or v.y > 0.0:
			var s3 := sqrt(h * h + v.y * v.y)
			if h > 0.0:
				var nh := maxf(0.0, h - s3 * h / DRAG_K)
				v.x *= nh / h
				v.z *= nh / h
			v.y -= s3 * v.y / (DRAG_K * 4.0)
			v.y -= GRAVITY * TICK
		else:
			var nh2 := maxf(0.0, h * (1.0 - 1.0 / 64.0) - h * h / DRAG_K)
			if h > 0.0:
				v.x *= nh2 / h
				v.z *= nh2 / h
		p += v * TICK
		if p.y <= BALL_RADIUS:
			p.y = BALL_RADIUS
			if v.y < -1.4:
				v.y = -v.y * 0.65
				v.x *= 0.85
				v.z *= 0.85
			else:
				v.y = 0.0
	return p

## Distance (m) a ground ball rolls from speed v0 until it slows to v_min (authentic WE3 rolling friction).
static func roll_distance(v0: float, v_min: float) -> float:
	var v := v0
	var d := 0.0
	var n := 0
	while v > v_min and n < 400:
		d += v * (1.0 / 30.0)
		v = v * (1.0 - 1.0 / 64.0) - v * v / 6881.0
		n += 1
	return d

func _physics_process(delta: float) -> void:
	if pickup_cooldown > 0.0:
		pickup_cooldown = maxf(0.0, pickup_cooldown - delta)

	if owner_player != null and is_instance_valid(owner_player):
		# Follow ahead of the dribbling player's feet (further ahead when sprinting, like WE3)
		var fwd := -owner_player.global_transform.basis.z
		fwd.y = 0.0
		if fwd.length_squared() < 0.001:
			fwd = Vector3.FORWARD
		fwd = fwd.normalized()
		var p_speed := Vector2(owner_player.vel.x, owner_player.vel.z).length() if "vel" in owner_player else 0.0
		var lead_dist := lerpf(0.52, 0.76, clampf(p_speed / 7.8, 0.0, 1.0))
		var target_pos := owner_player.global_position + fwd * lead_dist
		target_pos.y = BALL_RADIUS
		var step := target_pos - global_position
		global_position = global_position.lerp(target_pos, clampf(delta * 22.0, 0.0, 1.0))
		vel = step / maxf(delta, 0.001)
		_roll_visual(vel, delta)
		_update_shadow()
		return

	# Free-flight / rolling physics: authentic WE3 ball step (func_8004BAA4), run at the 30 Hz logic rate
	var pos := global_position
	_tick_acc += delta
	var guard := 0
	while _tick_acc >= TICK and guard < 4:
		_tick_acc -= TICK
		guard += 1
		flight_frames += 1
		var in_air := pos.y > BALL_RADIUS + 0.005 or vel.y > 0.0
		var h := Vector2(vel.x, vel.z).length()
		if in_air:
			state = BallState.AIRBORNE
			# func_8004BAA4 lines 73-79: discrete 1.40625 deg heading turn every _spin_period frames
			if h > 0.1 and _spin_period > 0 and (flight_frames % _spin_period) == 0:
				var hv := Vector2(vel.x, vel.z).rotated(_spin_dir * deg_to_rad(1.40625))
				vel.x = hv.x
				vel.z = hv.y
			pos += vel * TICK
			if pos.y > BALL_RADIUS:
				var g: float = GRAVITY_BY_TYPE[clampi(flight_type, 0, 3)]
				vel.y -= g * TICK
			else:
				# Ground contact: func_8004BAA4 lines 104-163 bounce damping by flight_type
				pos.y = BALL_RADIUS
				var k := minf(8.0, h / (absf(vel.y) + 1.0))
				var damp := 4.0 * k
				var h_keep := 1.0
				var v_keep := 0.625
				match flight_type:
					0:
						h_keep = 1.0 - (64.0 - damp) / 256.0
						v_keep = 1.0 - (96.0 - damp) / 256.0
					1:
						h_keep = 1.0 - (64.0 - damp) / 128.0
						v_keep = 1.0 - (64.0 - damp) / 240.0
					2:
						h_keep = 1.0 - (64.0 - damp) / 272.0
						v_keep = 1.0 - (96.0 - damp) / 288.0
						flight_type = 0
					3:
						h_keep = 1.0 - (64.0 - damp) / 288.0
						v_keep = 1.0 - (96.0 - damp) / 304.0
						flight_type = 0
				vel.x *= h_keep
				vel.z *= h_keep
				vel.y = absf(vel.y) * v_keep
				if _spin_period > 0:
					_spin_period += 2
				if vel.y <= 8.28:
					flight_type = 0
				if vel.y < 0.69:
					vel.y = 0.0
					state = BallState.ROLLING
			# Quadratic air drag proportional to speed magnitude
			h = Vector2(vel.x, vel.z).length()
			var s3 := sqrt(h * h + vel.y * vel.y)
			if h > 0.0:
				var nh := maxf(0.0, h - s3 * h / DRAG_K)
				vel.x *= nh / h
				vel.z *= nh / h
			vel.y -= s3 * vel.y / (DRAG_K * 4.0)
		else:
			pos.y = BALL_RADIUS
			vel.y = 0.0
			angular_spin_y = 0.0
			_spin_period = 0
			pos += Vector3(vel.x, 0.0, vel.z) * TICK
			# rolling: h -= h>>6 ; h -= ((h*h)>>21)/7
			var nh2 := maxf(0.0, h * (1.0 - 1.0 / 64.0) - h * h / DRAG_K)
			if h > 0.0:
				vel.x *= nh2 / h
				vel.z *= nh2 / h
			if nh2 < 0.12:   # h_speed < 256 raw (1 unit/frame)
				vel.x = 0.0
				vel.z = 0.0
				state = BallState.STOPPED
			else:
				state = BallState.ROLLING

	# Goalpost, crossbar, goal-net & perimeter adboard collisions (func_800A8F3C)
	pos = _collide_goalposts(pos)
	pos = _collide_goal_net_and_adboards(pos)

	global_position = pos
	_roll_visual(vel, delta)
	_update_shadow()

	if is_active_play:
		_check_pitch_boundaries(pos)

func _roll_visual(v: Vector3, delta: float) -> void:
	if _mesh_node == null:
		return
	var horiz := Vector3(v.x, 0.0, v.z)
	var speed := horiz.length()
	if speed > 0.05:
		var axis := Vector3.UP.cross(horiz.normalized()).normalized()
		if axis.length_squared() > 0.01:
			_mesh_node.rotate(axis, (speed / BALL_RADIUS) * delta)

func _update_shadow() -> void:
	if _shadow_node == null:
		return
	var h := maxf(0.0, global_position.y - BALL_RADIUS)
	_shadow_node.global_position = Vector3(global_position.x + h * 0.06, 0.02, global_position.z + h * 0.08)
	var height_factor := clampf(1.0 - h * 0.10, 0.40, 1.0)
	_shadow_node.scale = Vector3(height_factor, height_factor, 1.0)

func _collide_goalposts(pos: Vector3) -> Vector3:
	var post_r := 0.06 + BALL_RADIUS
	for gx: float in [-PitchBuilder.HALF_LEN, PitchBuilder.HALF_LEN]:
		# Left and right posts
		if pos.y < PitchBuilder.GOAL_HEIGHT + BALL_RADIUS:
			for gz: float in [-PitchBuilder.GOAL_HALF_W, PitchBuilder.GOAL_HALF_W]:
				var dx := pos.x - gx
				var dz := pos.z - gz
				var dist := sqrt(dx * dx + dz * dz)
				if dist < post_r and dist > 0.0001:
					var nx := dx / dist
					var nz := dz / dist
					pos.x = gx + nx * post_r
					pos.z = gz + nz * post_r
					var dot := vel.x * nx + vel.z * nz
					if dot < 0.0:
						vel.x -= 1.65 * dot * nx
						vel.z -= 1.65 * dot * nz
						SoundFX.play_sfx("post")
		# Crossbar
		if absf(pos.z) <= PitchBuilder.GOAL_HALF_W + BALL_RADIUS:
			var dx := pos.x - gx
			var dy := pos.y - PitchBuilder.GOAL_HEIGHT
			var dist := sqrt(dx * dx + dy * dy)
			if dist < post_r and dist > 0.0001:
				var nx := dx / dist
				var ny := dy / dist
				pos.x = gx + nx * post_r
				pos.y = PitchBuilder.GOAL_HEIGHT + ny * post_r
				var dot := vel.x * nx + vel.y * ny
				if dot < 0.0:
					vel.x -= 1.65 * dot * nx
					vel.y -= 1.65 * dot * ny
					SoundFX.play_sfx("post")
	return pos

## Authentic WE3 perimeter adboard bounce (func_800A8F3C) + goal-net containment.
func _collide_goal_net_and_adboards(pos: Vector3) -> Vector3:
	# Goal net pocket: HALF_LEN..HALF_LEN+2.2m, |z| < GOAL_HALF_W, y < GOAL_HEIGHT
	var net_depth := 2.2
	if absf(pos.x) > PitchBuilder.HALF_LEN and absf(pos.x) <= PitchBuilder.HALF_LEN + net_depth + 0.6 and absf(pos.z) <= PitchBuilder.GOAL_HALF_W + 0.2 and pos.y <= PitchBuilder.GOAL_HEIGHT + 0.2:
		var max_x := PitchBuilder.HALF_LEN + net_depth - BALL_RADIUS
		if absf(pos.x) > max_x:
			pos.x = signf(pos.x) * max_x
			vel.x = -vel.x * 0.22
			vel.z *= 0.45
			vel.y *= 0.45
		if absf(pos.z) > PitchBuilder.GOAL_HALF_W - BALL_RADIUS:
			pos.z = signf(pos.z) * (PitchBuilder.GOAL_HALF_W - BALL_RADIUS)
			vel.z = -vel.z * 0.25
		if pos.y > PitchBuilder.GOAL_HEIGHT - BALL_RADIUS:
			pos.y = PitchBuilder.GOAL_HEIGHT - BALL_RADIUS
			vel.y = -absf(vel.y) * 0.25
		return pos

	# func_800A8F3C: adboard bounce when ball height <= 1.024m
	if pos.y <= 1.024:
		var ad_z := 40.0   # 0x2710 = 10000 raw = 40.0m
		var ad_x := 58.5   # 0x39F0..0x3B30 (~58.5m)
		if absf(pos.x) <= ad_x and absf(pos.z) >= ad_z:
			pos.z = signf(pos.z) * (ad_z - 0.05)
			vel.z = -vel.z * 0.5
			vel.x *= 0.5
		if absf(pos.z) <= ad_z and absf(pos.x) >= ad_x:
			pos.x = signf(pos.x) * (ad_x - 0.05)
			vel.x = -vel.x * 0.5
			vel.z *= 0.5
	return pos

func _check_pitch_boundaries(pos: Vector3) -> void:
	# Goal detection: Home (team 0) attacks +X, Away (team 1) attacks -X
	if absf(pos.z) < PitchBuilder.GOAL_HALF_W - BALL_RADIUS * 0.3 and pos.y < PitchBuilder.GOAL_HEIGHT:
		if pos.x > PitchBuilder.HALF_LEN + BALL_RADIUS:
			is_active_play = false
			goal_detected.emit(0) # Home scored!
			return
		elif pos.x < -PitchBuilder.HALF_LEN - BALL_RADIUS:
			is_active_play = false
			goal_detected.emit(1) # Away scored!
			return

	# Touchline (Throw-in)
	if absf(pos.z) > PitchBuilder.HALF_WID + BALL_RADIUS:
		is_active_play = false
		var clamped_x := clampf(pos.x, -PitchBuilder.HALF_LEN + 0.5, PitchBuilder.HALF_LEN - 0.5)
		var edge_z := signf(pos.z) * (PitchBuilder.HALF_WID - 0.3)
		out_of_bounds_detected.emit("throw_in", Vector3(clamped_x, 0.0, edge_z), 1 - last_touch_team)
		return

	# Goal line (Corner or Goal Kick)
	if absf(pos.x) > PitchBuilder.HALF_LEN + BALL_RADIUS:
		is_active_play = false
		var defending_team := 1 if pos.x > 0.0 else 0
		if last_touch_team == defending_team:
			# Corner kick for attacking team
			var corner_x := signf(pos.x) * (PitchBuilder.HALF_LEN - 0.6)
			var corner_z := signf(pos.z) * (PitchBuilder.HALF_WID - 0.6)
			out_of_bounds_detected.emit("corner", Vector3(corner_x, 0.0, corner_z), 1 - defending_team)
		else:
			# Goal kick for defending team
			var gk_x := signf(pos.x) * (PitchBuilder.HALF_LEN - 5.5)
			out_of_bounds_detected.emit("goal_kick", Vector3(gk_x, 0.0, 0.0), defending_team)
