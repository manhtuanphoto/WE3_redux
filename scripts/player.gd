class_name Player
extends Node3D
## Represents a single footballer with WE3 12-bone animation, kit texture, and human/AI control.

enum Role { GK, DF, MF, FW }

const WALK_SPEED := 5.76     # 48 units: WE3 run cap (func_80048AD8)
const SPRINT_SPEED := 7.44    # 62 units = 0x35 + SPEED_TABLE[7] (func_80023078)
const DRIBBLE_SPEED := 5.28   # 44 units
const ACCEL := 26.0
const TURN_SPEED := 14.0
const PICKUP_RADIUS := 0.85
const GK_REACT_FRAMES := 8       # func_80094EEC: keeper only reacts when ball timer (0x31C) >= 8
const GK_DIVE_FRAMES := 7        # func_8008F2DC: contact on dive animation frame 7
const GK_TOUCH_RADIUS := 0.512   # func_80094EEC radius 128 units * 0.004 m
const GK_DIVE_BASE := 1.0        # port estimate: dive-decision code not located, reach = base + 0.10 m per defense level
const SPEED_TABLE := [5, 5, 5, 5, 6, 7, 8, 9, 10, 16]   # D_800C6AE4
const ACCEL_TABLE := [-3, -3, -2, -2, -1, -1, 0, 0, 3, 6]  # D_800C6BF0 (s16)

@export var team_side: int = 0       # 0 = Home (attacks +X), 1 = Away (attacks -X)
@export var team_id: int = 0         # Index into GameData.teams
@export var slot_index: int = 0      # 0..10
@export var player_name: String = "Player"
@export var role: Role = Role.MF
@export var home_anchor: Vector3 = Vector3.ZERO

var is_human: bool = false
var is_controlled: bool = false
var yellow_cards: int = 0
var is_sent_off: bool = false
var vel: Vector3 = Vector3.ZERO
var stamina: float = 100.0
var action_lock_timer: float = 0.0
var tackle_timer: float = 0.0
var tackle_dir: Vector3 = Vector3.FORWARD
var shot_charge: float = 0.0
var is_charging_shot: bool = false
var _pending_kick_timer: float = 0.0
var _pending_kick_vel: Vector3 = Vector3.ZERO
var _pending_kick_spin: float = 0.0
var _pending_is_shot: bool = false
var _one_touch_action: String = ""
var _one_touch_timer: float = 0.0
var _ai_hold_timer: float = 0.0
var _tackle_cd: float = 0.0
var _gk_diving: bool = false
var _gk_dive_t: float = -1.0
var _gk_dive_aim: Vector3 = Vector3.ZERO
var _gk_dive_origin_z: float = 0.0

# --- Authentic WE3 player profile (SLPM_861.62 stat words; levels 1..9) ---
var stats: Dictionary = {"offense": 6, "defense": 5, "speed": 7, "acceleration": 7, "kick_power": 6, "technique": 6, "header_jump": 6, "stamina": 7, "curve_pass": 5}
var shirt_number: int = 0
var height_cm: int = 178
var pos_group: String = ""
var roster_slot: int = -1
var _walk_speed: float = WALK_SPEED
var _sprint_speed: float = SPRINT_SPEED
var _dribble_speed: float = DRIBBLE_SPEED
var _accel: float = ACCEL
var _stam_drain: float = 1.0
var _kick_mul: float = 1.0
var _tech_err_deg: float = 0.0

var _model_root: Node3D = null
var _anim_player: AnimationPlayer = null
var _current_anim: String = ""
var _ring_mesh: MeshInstance3D = null
var _shadow_mesh: MeshInstance3D = null
var _name_label: Label3D = null
var _ball: Ball = null
var _all_players: Array[Player] = []

func setup(p_team_side: int, p_team_id: int, p_slot: int, p_name: String, p_role: Role, p_anchor: Vector3, p_ball: Ball, p_all_players: Array[Player]) -> void:
	team_side = p_team_side
	team_id = p_team_id
	slot_index = p_slot
	player_name = p_name
	role = p_role
	home_anchor = p_anchor
	_ball = p_ball
	_all_players = p_all_players

## Applies the authentic stat record. Mapping follows the real C data flow:
##  speed (w0 nibble 5) -> max run speed, acceleration (w1 nibble 0) -> acceleration divisor,
##  technique -> random pass/shot error window [0 .. 9-technique] (func_80036230),
##  stamina -> fatigue rate, kick_power -> shot power, offense/defense -> AI shoot range / tackle strength.
func apply_profile(rec: Dictionary, p_roster_slot: int = -1) -> void:
	roster_slot = p_roster_slot
	if rec.is_empty():
		return
	var st: Variant = rec.get("stats", null)
	if typeof(st) == TYPE_DICTIONARY:
		for k in stats.keys():
			stats[k] = int(st.get(k, stats[k]))
	shirt_number = int(rec.get("shirt_number", shirt_number))
	height_cm = int(rec.get("height_cm", height_cm))
	pos_group = str(rec.get("pos_group", ""))
	var spd := float(stats["speed"])
	var acc := float(stats["acceleration"])
	var sta := float(stats["stamina"])
	var kp := float(stats["kick_power"])
	var tech := float(stats["technique"])
	# Authentic tables from SLPM_861.62 (D_800C6AE4 / D_800C6BF0, verified against asm/data/B6874.data.s):
	# max speed units = 0x35 + SPEED_TABLE[speed]; acceleration divisor = 18 - ACCEL_TABLE[acceleration] (func_80023078).
	# 250 units = 1 m, 30 logic fps -> 0.12 m/s per unit. Port calibrates to a speed-7 player and keeps WE3 ratios.
	var si := clampi(int(spd), 0, 9)
	var ai := clampi(int(acc), 0, 9)
	var speed_mul := float(53 + SPEED_TABLE[si]) / float(53 + SPEED_TABLE[7])
	var accel_div := 18.0 - float(ACCEL_TABLE[ai])
	_walk_speed = WALK_SPEED
	_sprint_speed = SPRINT_SPEED * speed_mul
	_dribble_speed = DRIBBLE_SPEED * (1.0 + (speed_mul - 1.0) * 0.8 + (tech - 6.0) * 0.006)
	_accel = ACCEL * (18.0 / accel_div)
	_stam_drain = 1.0 + (6.0 - sta) * 0.09
	_kick_mul = 1.0 + (kp - 6.0) * 0.035
	_tech_err_deg = maxf(0.0, 9.0 - tech) * 0.7

## WE3 func_80023078: below the stamina threshold max speed drops up to ~7% and acceleration divisor grows 18 -> 28.
func _fat_spd() -> float:
	return 1.0 if stamina >= 40.0 else lerpf(0.93, 1.0, clampf(stamina / 40.0, 0.0, 1.0))

func _fat_acc() -> float:
	return 1.0 if stamina >= 40.0 else lerpf(0.64, 1.0, clampf(stamina / 40.0, 0.0, 1.0))

func _apply_tech_error(dir: Vector3, mult: float = 1.0) -> Vector3:
	if _tech_err_deg <= 0.0:
		return dir
	return dir.rotated(Vector3.UP, deg_to_rad(randf_range(-_tech_err_deg, _tech_err_deg) * mult))

func _ready() -> void:
	_build_model()
	_build_indicators()

func _build_model() -> void:
	var rec_idx := (team_side * 11 + slot_index) % 22
	var rec_path := "res://assets/3d/players/record_%02d.gltf" % rec_idx
	if not ResourceLoader.exists(rec_path):
		rec_path = "res://assets/3d/players/record_00.gltf"
	var packed := load(rec_path) as PackedScene
	if packed != null:
		_model_root = packed.instantiate()
		# WE3 models face +Z; rotate 180 deg so -Z is forward in Godot
		_model_root.rotation.y = PI
		add_child(_model_root)
		_apply_kit_material(_model_root)
		_anim_player = _find_anim_player(_model_root)
		if _anim_player != null:
			var lib := PlayerAnimCache.get_library()
			if not _anim_player.has_animation_library("we3"):
				_anim_player.add_animation_library("we3", lib)
			_play_anim("idle")

func _apply_kit_material(root: Node) -> void:
	var kit_tex := GameData.get_team_kit_texture(team_id)
	var extra_tex := GameData.get_team_extra_kit_texture(team_id)
	var jersey_col := Color(0.95, 0.78, 0.12) if (role == Role.GK and team_side == 0) else (Color(0.18, 0.75, 0.42) if role == Role.GK else GameData.get_team_primary_color(team_id))
	var skin_col := Color(0.88, 0.68, 0.54)
	var boot_col := Color(0.10, 0.10, 0.12)

	var skin_variants := [
		"res://assets/textures/players/skin_black_hair.png",
		"res://assets/textures/players/skin_brown_hair.png",
		"res://assets/textures/players/skin_blonde_hair.png",
	]
	var skin_path: String = skin_variants[(team_id * 7 + slot_index) % 3]
	var skin_tex: Texture2D = load(skin_path) as Texture2D if ResourceLoader.exists(skin_path) else null

	# Authentic WE3 256x256 unified atlas:
	#   Top-Left  (0, 0)   = tpage 512 (tex_XX_<Team>.png: jersey, shorts, socks, #2)
	#   Top-Right (128, 0) = tpage 576 (tex_XX_blk01_128x128.png: real back numbers 3..22 + GK kit)
	#   Bot-Left  (0, 128) = tpage 384 (skin_*.png: face, hair, neck, arms, knees)
	var atlas_tex := _build_player_atlas_texture(kit_tex, extra_tex, skin_tex, jersey_col, skin_col)
	var mat_atlas := StandardMaterial3D.new()
	mat_atlas.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat_atlas.albedo_texture = atlas_tex
	mat_atlas.albedo_color = Color.WHITE
	mat_atlas.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	mat_atlas.cull_mode = BaseMaterial3D.CULL_DISABLED

	var mat_boots := StandardMaterial3D.new()
	mat_boots.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	if ResourceLoader.exists("res://assets/3d/players/boots_tex.png"):
		mat_boots.albedo_texture = load("res://assets/3d/players/boots_tex.png") as Texture2D
		mat_boots.albedo_color = Color.WHITE
		mat_boots.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	else:
		mat_boots.albedo_color = boot_col
	mat_boots.cull_mode = BaseMaterial3D.CULL_DISABLED

	_attach_boots(root, mat_boots)
	_apply_part_mats_recursive(root, mat_atlas, mat_atlas, mat_atlas, mat_atlas, mat_atlas, mat_boots)

func _build_player_atlas_texture(kit_tex: Texture2D, extra_tex: Texture2D, skin_tex: Texture2D, fallback_kit: Color, fallback_skin: Color) -> ImageTexture:
	var atlas := Image.create(256, 256, false, Image.FORMAT_RGBA8)
	atlas.fill_rect(Rect2i(0, 0, 256, 128), fallback_kit)
	atlas.fill_rect(Rect2i(0, 128, 256, 128), fallback_skin)
	# For GK (rec 0), top-left uses extra_tex (GK green/alt kit); for outfielders, top-left = kit_tex, top-right = extra_tex (real back numbers 3..22!)
	var tl_tex := extra_tex if (role == Role.GK and extra_tex != null) else kit_tex
	if tl_tex != null:
		var ki := tl_tex.get_image()
		if ki != null:
			ki.convert(Image.FORMAT_RGBA8)
			atlas.blit_rect(ki, Rect2i(0, 0, mini(128, ki.get_width()), mini(128, ki.get_height())), Vector2i(0, 0))
	if extra_tex != null:
		var ei := extra_tex.get_image()
		if ei != null:
			ei.convert(Image.FORMAT_RGBA8)
			atlas.blit_rect(ei, Rect2i(0, 0, mini(128, ei.get_width()), mini(128, ei.get_height())), Vector2i(128, 0))
	if skin_tex != null:
		var si := skin_tex.get_image()
		if si != null:
			si.convert(Image.FORMAT_RGBA8)
			atlas.blit_rect(si, Rect2i(0, 0, mini(128, si.get_width()), mini(128, si.get_height())), Vector2i(0, 128))
			atlas.blit_rect(si, Rect2i(0, 0, mini(128, si.get_width()), mini(128, si.get_height())), Vector2i(128, 128))
	return ImageTexture.create_from_image(atlas)

func _stamp_back_number(atlas: Image, num: int) -> void:
	# Sample original digit color from the template '2' (x=62..72, y=40..55) vs clean jersey edge (x=60, y=44)
	var ref_bg := atlas.get_pixel(60, 44)
	var num_col := Color(0.05, 0.35, 0.12)
	var best_diff := 0.0
	for y in range(40, 55):
		for x in range(62, 73):
			var c := atlas.get_pixel(x, y)
			var d := absf(c.r - ref_bg.r) + absf(c.g - ref_bg.g) + absf(c.b - ref_bg.b)
			if d > best_diff:
				best_diff = d
				num_col = c
	# Erase template '2' row-by-row using each row's own left/right jersey color so shading gradient is preserved!
	for y in range(39, 57):
		var c_left := atlas.get_pixel(59, y)
		var c_right := atlas.get_pixel(74, y)
		for x in range(61, 74):
			var t := float(x - 61) / 12.0
			atlas.set_pixel(x, y, c_left.lerp(c_right, t))
	const DIGITS := {
		"0": ["111", "101", "101", "101", "111"],
		"1": ["010", "110", "010", "010", "111"],
		"2": ["111", "001", "111", "100", "111"],
		"3": ["111", "001", "111", "001", "111"],
		"4": ["101", "101", "111", "001", "001"],
		"5": ["111", "100", "111", "001", "111"],
		"6": ["111", "100", "111", "101", "111"],
		"7": ["111", "001", "001", "010", "010"],
		"8": ["111", "101", "111", "101", "111"],
		"9": ["111", "101", "111", "001", "111"],
	}
	var s := str(clampi(num, 1, 99))
	var scale_px := 2
	var total_w := s.length() * (3 * scale_px) + (s.length() - 1) * 1
	var start_x := 67 - total_w / 2
	var start_y := 42
	for ci in range(s.length()):
		var glyph: Array = DIGITS.get(s[ci], DIGITS["1"])
		var ox := start_x + ci * (3 * scale_px + 1)
		for row in range(5):
			for col in range(3):
				if glyph[row][col] == "1":
					atlas.fill_rect(Rect2i(ox + col * scale_px, start_y + row * scale_px, scale_px, scale_px), num_col)

func _pick_hair_color(tid: int, slot: int) -> Color:
	const HAIR_PALETTE := [
		Color(0.08, 0.07, 0.06), # Black
		Color(0.22, 0.14, 0.08), # Dark Brown
		Color(0.38, 0.24, 0.12), # Chestnut
		Color(0.75, 0.60, 0.28), # Blonde
		Color(0.12, 0.10, 0.09), # Charcoal
	]
	return HAIR_PALETTE[(tid * 7 + slot * 3) % HAIR_PALETTE.size()]

func _create_head_hair_material(skin_col: Color, hair_col: Color) -> ShaderMaterial:
	var sh := Shader.new()
	sh.code = """
shader_type spatial;
render_mode cull_disabled;
uniform vec4 skin_color : source_color;
uniform vec4 hair_color : source_color;
varying vec3 local_pos;
void vertex() {
	local_pos = VERTEX;
}
void fragment() {
	// Top and back of 00_Head mesh are hair; front face is skin
	bool is_hair = (local_pos.y > 0.082) || (local_pos.z < -0.022 && local_pos.y > 0.005);
	ALBEDO = is_hair ? hair_color.rgb : skin_color.rgb;
	ROUGHNESS = is_hair ? 0.85 : 0.68;
}
"""
	var sm := ShaderMaterial.new()
	sm.shader = sh
	sm.set_shader_parameter("skin_color", skin_col)
	sm.set_shader_parameter("hair_color", hair_col)
	return sm

func _attach_boots(root: Node, mat_boots: Material) -> void:
	var boot_r_mesh: Variant = load("res://assets/3d/players/tmd_60_off_0E328_v8_p5.obj")
	var boot_l_mesh: Variant = load("res://assets/3d/players/tmd_61_off_0E45C_v8_p5.obj")
	var r_foot := root.find_child("08_R_Foot", true, false)
	if r_foot != null and boot_r_mesh is Mesh:
		var mi_r := MeshInstance3D.new()
		mi_r.name = "BootMesh_R"
		mi_r.mesh = boot_r_mesh
		mi_r.material_override = mat_boots
		r_foot.add_child(mi_r)
	var l_foot := root.find_child("11_L_Foot", true, false)
	if l_foot != null and boot_l_mesh is Mesh:
		var mi_l := MeshInstance3D.new()
		mi_l.name = "BootMesh_L"
		mi_l.mesh = boot_l_mesh
		mi_l.material_override = mat_boots
		l_foot.add_child(mi_l)

func _apply_part_mats_recursive(n: Node, mat_jersey: Material, mat_shorts: Material, mat_socks: Material, mat_skin: Material, mat_head: Material, mat_boots: Material) -> void:
	if n is MeshInstance3D:
		var mi := n as MeshInstance3D
		var bname := mi.name
		if bname.begins_with("00_"):
			mi.material_override = mat_head
		elif bname.begins_with("03_") or bname.begins_with("05_"):
			mi.material_override = mat_skin
		elif bname.begins_with("06_") or bname.begins_with("09_"):
			mi.material_override = mat_shorts
		elif bname.begins_with("07_") or bname.begins_with("10_"):
			mi.material_override = mat_socks
		elif bname.begins_with("08_") or bname.begins_with("11_"):
			mi.material_override = mat_boots
		else:
			mi.material_override = mat_jersey
	for c in n.get_children():
		_apply_part_mats_recursive(c, mat_jersey, mat_shorts, mat_socks, mat_skin, mat_head, mat_boots)

func _build_indicators() -> void:
	# Authentic WE3 ground shadow: Day = 1 quad at 315 deg (func_8001A544); Night = 4 floodlight quads at 45/135/225/315 deg (func_8001A968)
	_shadow_mesh = MeshInstance3D.new()
	var is_night := GameData.stadium_is_night or GameData.selected_stadium == "GRDM_FN"
	var qm := QuadMesh.new()
	qm.size = Vector2(0.88, 1.28)
	var sh_mat := StandardMaterial3D.new()
	sh_mat.albedo_texture = PlayerAnimCache.get_shadow_texture()
	var base_alpha := 0.72 if GameData.stadium_is_rain else 1.0
	sh_mat.albedo_color = Color(1.0, 1.0, 1.0, base_alpha * (0.48 if is_night else 0.95))
	sh_mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	sh_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	sh_mat.cull_mode = BaseMaterial3D.CULL_DISABLED

	var angles: Array = [deg_to_rad(45.0), deg_to_rad(135.0), deg_to_rad(225.0), deg_to_rad(315.0)] if is_night else [deg_to_rad(315.0)]
	for ang in angles:
		var q_mi := MeshInstance3D.new()
		q_mi.mesh = qm
		q_mi.material_override = sh_mat
		q_mi.rotation.x = -PI * 0.5
		q_mi.rotation.z = ang
		q_mi.position = Vector3(sin(ang) * 0.22, 0.02, cos(ang) * 0.22)
		q_mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		_shadow_mesh.add_child(q_mi)
	add_child(_shadow_mesh)

	# Controlled player ring
	_ring_mesh = MeshInstance3D.new()
	var torus := TorusMesh.new()
	torus.inner_radius = 0.44
	torus.outer_radius = 0.56
	torus.rings = 16
	torus.ring_segments = 12
	_ring_mesh.mesh = torus
	var r_mat := StandardMaterial3D.new()
	r_mat.albedo_color = Color(0.15, 0.92, 1.0) if team_side == 0 else Color(1.0, 0.35, 0.25)
	r_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	_ring_mesh.material_override = r_mat
	_ring_mesh.position = Vector3(0.0, 0.03, 0.0)
	_ring_mesh.visible = false
	# Authentic WE3 overhead 1P inverted triangle marker above player's head
	var cursor := MeshInstance3D.new()
	var cone := CylinderMesh.new()
	cone.top_radius = 0.16
	cone.bottom_radius = 0.0
	cone.height = 0.24
	cone.radial_segments = 4
	cursor.mesh = cone
	cursor.material_override = r_mat
	cursor.position = Vector3(0.0, 2.25, 0.0)
	cursor.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	_ring_mesh.add_child(cursor)
	add_child(_ring_mesh)

	_name_label = Label3D.new()
	_name_label.text = player_name
	_name_label.font_size = 36
	_name_label.pixel_size = 0.008
	_name_label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	_name_label.modulate = Color(1.0, 0.95, 0.45)
	_name_label.outline_size = 6
	_name_label.position = Vector3(0.0, 2.25, 0.0)
	_name_label.visible = false
	add_child(_name_label)

func set_controlled(val: bool) -> void:
	is_controlled = val
	if _ring_mesh != null:
		_ring_mesh.visible = val
	if _name_label != null:
		_name_label.visible = val

func _play_anim(anim_key: String, blend: float = 0.12, speed_scale: float = 1.0) -> void:
	if _anim_player == null:
		return
	_anim_player.speed_scale = speed_scale
	if _current_anim == anim_key:
		return
	var full_name := "we3/" + anim_key
	if _anim_player.has_animation(full_name):
		_anim_player.play(full_name, blend)
		_current_anim = anim_key

func _find_anim_player(n: Node) -> AnimationPlayer:
	if n is AnimationPlayer:
		return n
	for c in n.get_children():
		var found := _find_anim_player(c)
		if found != null:
			return found
	return null

func has_ball() -> bool:
	return _ball != null and _ball.owner_player == self

func _physics_process(delta: float) -> void:
	if is_sent_off:
		return
	if MatchState.phase == MatchState.Phase.FULLTIME:
		vel = Vector3.ZERO
		var won := (team_side == 0 and MatchState.home_score > MatchState.away_score) or (team_side == 1 and MatchState.away_score > MatchState.home_score)
		_play_anim("ending_celebrate" if won else "idle")
		return

	if MatchState.phase == MatchState.Phase.PAUSED or MatchState.phase == MatchState.Phase.HALFTIME:
		return

	if MatchState.phase == MatchState.Phase.GOAL_CELEBRATION:
		vel = Vector3.ZERO
		if team_side == MatchState.last_touch_team:
			const CEL_ANIMS := ["celebrate", "celebrate_alt", "celebrate_jump", "celebrate_3", "celebrate_4"]
			_play_anim(CEL_ANIMS[slot_index % CEL_ANIMS.size()])
		else:
			_play_anim("idle")
		return

	if MatchState.phase != MatchState.Phase.OPEN_PLAY:
		vel = Vector3.ZERO
		_play_anim("idle")
		return

	if action_lock_timer > 0.0:
		action_lock_timer = maxf(0.0, action_lock_timer - delta)
	if _tackle_cd > 0.0:
		_tackle_cd = maxf(0.0, _tackle_cd - delta)
	if _one_touch_timer > 0.0:
		_one_touch_timer = maxf(0.0, _one_touch_timer - delta)
		if _one_touch_timer <= 0.0:
			_one_touch_action = ""

	if _pending_kick_timer > 0.0:
		_pending_kick_timer -= delta
		if _pending_kick_timer <= 0.0 and has_ball():
			_ball.kick(_pending_kick_vel, self, team_side, _pending_kick_spin)
			if _pending_is_shot:
				MatchState.shots[team_side] += 1
			else:
				MatchState.passes_completed[team_side] += 1

	if tackle_timer > 0.0:
		tackle_timer = maxf(0.0, tackle_timer - delta)
		# WE3 func_80040324.c: full slide first half, 20% grass friction decay second half
		if tackle_timer > 0.16:
			vel = tackle_dir * 8.4
		else:
			vel *= pow(0.8, delta * 30.0)
		_move_and_clamp(delta)
		_try_pickup_ball()
		if tackle_timer <= 0.0:
			_play_anim("recover", 0.08)
		return

	if is_controlled and is_human:
		_process_human(delta)
	else:
		_process_ai(delta)
		var ai_sp := Vector2(vel.x, vel.z).length()
		if ai_sp > 6.0:
			stamina = maxf(0.0, stamina - 7.0 * _stam_drain * delta)
		else:
			stamina = minf(100.0, stamina + 5.0 * delta)

	_move_and_clamp(delta)
	_try_pickup_ball()
	_update_locomotion_anim()

func _process_human(delta: float) -> void:
	var input_vec := Vector2(
		Input.get_action_strength("move_right") - Input.get_action_strength("move_left"),
		Input.get_action_strength("move_down") - Input.get_action_strength("move_up")
	)
	var move_dir := Vector3(input_vec.x, 0.0, input_vec.y)
	if move_dir.length() > 1.0:
		move_dir = move_dir.normalized()

	var sprinting := Input.is_action_pressed("sprint") and stamina > 5.0 and move_dir.length() > 0.1
	# WE3 Knock-on burst (func_80027140.c): pressing sprint while dribbling knocks ball ahead
	if has_ball() and Input.is_action_just_pressed("sprint") and stamina > 10.0 and action_lock_timer <= 0.0:
		var ko_dir := move_dir if move_dir.length() > 0.1 else -global_transform.basis.z
		ko_dir.y = 0.0
		_ball.kick(ko_dir.normalized() * 7.0, self, team_side)
		stamina = maxf(0.0, stamina - 4.0)

	if sprinting:
		stamina = maxf(0.0, stamina - 18.0 * _stam_drain * delta)
	else:
		stamina = minf(100.0, stamina + 12.0 * delta)

	var max_spd := (_sprint_speed * _fat_spd()) if sprinting else ((_dribble_speed * _fat_spd()) if has_ball() else (_walk_speed * _fat_spd()))
	if action_lock_timer > 0.0:
		max_spd *= 0.35
	# WE3 motion model: separate body/move angles, 30 Hz speed accumulation, turn caps, brake-and-turn
	_step_motion(move_dir, max_spd, sprinting, delta)

	# Ball actions when in possession
	if has_ball() and action_lock_timer <= 0.0:
		if Input.is_action_just_pressed("pass_short"):
			_perform_pass(move_dir, false, false)
		elif Input.is_action_just_pressed("pass_through"):
			_perform_pass(move_dir, true, false)
		elif Input.is_action_just_pressed("pass_loft"):
			_perform_pass(move_dir, false, true)
		elif Input.is_action_just_pressed("shoot"):
			is_charging_shot = true
			shot_charge = 0.2
		elif is_charging_shot and Input.is_action_pressed("shoot"):
			shot_charge = minf(1.0, shot_charge + 1.35 * delta)
		elif is_charging_shot and Input.is_action_just_released("shoot"):
			_perform_shoot(move_dir, shot_charge)
			is_charging_shot = false
			shot_charge = 0.0
	else:
		is_charging_shot = false
		shot_charge = 0.0
		# WE3 One-Touch buffer (SUT_MOT_CHAM_CHON_CHAN_VA_NE_BONG_WE3.md)
		if Input.is_action_just_pressed("pass_short"):
			_one_touch_action = "pass_short"
			_one_touch_timer = 0.35
		elif Input.is_action_just_pressed("pass_through"):
			_one_touch_action = "pass_through"
			_one_touch_timer = 0.35
		elif Input.is_action_just_pressed("shoot") and action_lock_timer <= 0.0:
			if _ball != null and global_position.distance_to(_ball.global_position) < 3.2 and _ball.owner_player == null:
				_one_touch_action = "shoot"
				_one_touch_timer = 0.35
			else:
				_perform_slide_tackle(move_dir)

func _process_ai(delta: float) -> void:
	if _ball == null:
		return
	if role == Role.GK and _gk_dive_t >= 0.0:
		_gk_dive_t += delta
		if _gk_dive_t >= GK_DIVE_FRAMES * LOGIC_DT:
			_gk_dive_finish()
		return
	if action_lock_timer > 0.0 or _pending_kick_timer > 0.0:
		vel = vel.lerp(Vector3.ZERO, clampf((_accel * _fat_acc()) * delta, 0.0, 1.0))
		return

	if role == Role.GK:
		_process_gk_ai(delta)
		return

	var attack_dir := 1.0 if team_side == 0 else -1.0
	var opp_goal := Vector3(attack_dir * PitchBuilder.HALF_LEN, 0.0, 0.0)

	if has_ball():
		_ai_hold_timer += delta
		# Decision scoring: Shoot (only in good angle), Cross (only if teammate in box), Pass to real open teammate, or Dribble
		var dist_to_goal := global_position.distance_to(opp_goal)
		if dist_to_goal < (20.0 + float(stats["offense"])) and absf(global_position.z) < 18.0:
			# Aim toward far corner away from opponent GK
			var corner_mag := randf_range(1.8, 3.3)
			var corner_z := -corner_mag if global_position.z >= 0.0 else corner_mag
			var aim_pt := Vector3(attack_dir * PitchBuilder.HALF_LEN, 0.0, corner_z)
			_perform_shoot((aim_pt - global_position).normalized(), randf_range(0.62, 0.92), true)
			_ai_hold_timer = 0.0
			return

		# Wing cross into the penalty box (func_8002C28C: x >= 34.70m -> target x >= 41.92m, |z| = 8.92m)
		if _ai_hold_timer > 0.25 and (global_position.x * attack_dir) >= 34.7 and absf(global_position.z) > 14.0:
			var cross_z := 8.92 if global_position.z < 0.0 else -8.92
			var box_dir := (Vector3(attack_dir * 42.5, 0.0, cross_z) - global_position).normalized()
			if _find_pass_target(box_dir, false) != null:
				_perform_pass(box_dir, false, true)
				_ai_hold_timer = 0.0
				return

		var nearest_opp := _find_nearest_opponent(global_position)
		var opp_dist := nearest_opp.global_position.distance_to(global_position) if nearest_opp != null else 99.0

		# Real WE3 func_80033B88.c: in own defensive third (<-27.9m) under <=3.25m pressure & facing away (>=112.5 deg) -> clear to nearest touchline!
		var in_own_def_third := (global_position.x * attack_dir) < -27.9
		if in_own_def_third and opp_dist <= 3.25:
			var fwd := -global_transform.basis.z
			fwd.y = 0.0
			var facing_atk := fwd.normalized().dot(Vector3(attack_dir, 0.0, 0.0)) if fwd.length_squared() > 0.01 else 1.0
			if facing_atk < -0.38:
				var touch_z := PitchBuilder.HALF_WID if global_position.z >= 0.0 else -PitchBuilder.HALF_WID
				var clear_dir := (Vector3(global_position.x + attack_dir * 8.0, 0.0, touch_z) - global_position).normalized()
				_perform_pass(clear_dir, false, true)
				_ai_hold_timer = 0.0
				return

		# Avoid instant hot-potato passing: hold/dribble >=0.38s unless under immediate pressure (<2.6m)
		var urgent_pass := opp_dist < 2.6
		var can_pass := urgent_pass or _ai_hold_timer >= 0.38
		var should_pass := opp_dist < 5.5 or (role == Role.DF and opp_dist < 8.5)
		if can_pass and (should_pass or (role != Role.FW and _ai_hold_timer >= 0.85)):
			var open_mate := _find_best_open_teammate(attack_dir)
			if open_mate != null:
				var mate_ahead := (open_mate.global_position.x - global_position.x) * attack_dir > 5.0
				if urgent_pass or should_pass or mate_ahead:
					var use_through := open_mate.role == Role.FW and (open_mate.global_position.x * attack_dir) > (global_position.x * attack_dir) + 4.0
					_kick_pass_to_target(open_mate, use_through, false)
					_ai_hold_timer = 0.0
					return

		# Dribble toward goal with WE3 8-sector opponent avoidance (func_8005EF94)
		var dir := (opp_goal - global_position).normalized()
		if nearest_opp != null and opp_dist < 6.5:
			var best_dir := dir
			var best_score := -INF
			for s_i in range(8):
				var cand := dir.rotated(Vector3.UP, float(s_i - 4) * (PI / 4.0) * 0.5)
				var goal_dot := cand.dot(dir)
				var min_opp_d := 99.0
				for op in _all_players:
					if op.team_side != team_side and not op.is_sent_off:
						var to_op := op.global_position - (global_position + cand * 2.5)
						to_op.y = 0.0
						min_opp_d = minf(min_opp_d, to_op.length())
				var sc := goal_dot * 2.2 + minf(min_opp_d, 6.0) * 0.85
				if sc > best_score:
					best_score = sc
					best_dir = cand
			dir = best_dir
		# Open space ahead -> sprint-dribble (knock-on pace); pressured -> controlled dribble
		if opp_dist > 3.4 and stamina > 25.0:
			_step_motion(dir, _sprint_speed * 0.9 * _fat_spd(), true, delta)
		else:
			_step_motion(dir, _dribble_speed * _fat_spd(), false, delta)
		return

	_ai_hold_timer = 0.0

	# Off-ball AI (role-aware + WE3 func_800807F0 path-cutting + func_800691A8 tactical shift & offside pull)
	var team_has_ball: bool = _ball.owner_player != null and (_ball.owner_player as Player).team_side == team_side
	var team_in_attack: bool = team_has_ball or (_ball.owner_player == null and _ball.last_touch_team == team_side)
	var tactic_lvl := GameData.home_tactic_level if team_side == 0 else GameData.away_tactic_level
	var offside_trap := GameData.home_offside_trap if team_side == 0 else GameData.away_offside_trap
	var tactic_shift := (-3.072 if tactic_lvl == 0 else (6.144 if tactic_lvl == 2 else 3.072)) * attack_dir
	var target_pos := home_anchor

	if _ball.owner_player == null and _is_closest_presser():
		target_pos = _ball.predict_landing_point(20)
	elif team_in_attack:
		var offside_lim := _get_offside_line_x(attack_dir)
		match role:
			Role.DF:
				var df_min_x := -PitchBuilder.HALF_LEN + 6.0 if attack_dir > 0.0 else -6.0
				var df_max_x := 6.0 if attack_dir > 0.0 else PitchBuilder.HALF_LEN - 6.0
				target_pos.x = clampf(lerpf(home_anchor.x, _ball.global_position.x - attack_dir * 16.0, 0.45) + tactic_shift * 0.5, df_min_x, df_max_x)
				target_pos.z = lerpf(home_anchor.z, _ball.global_position.z, 0.25)
			Role.MF:
				target_pos.x = clampf(lerpf(home_anchor.x, _ball.global_position.x + attack_dir * 17.0, 0.6) + tactic_shift, -PitchBuilder.HALF_LEN + 8.0, PitchBuilder.HALF_LEN - 14.0)
				target_pos.z = lerpf(home_anchor.z, _ball.global_position.z, 0.35)
			Role.FW:
				# Attack space behind midfield while staying onside (func_800691A8)
				var desired_x := lerpf(home_anchor.x, _ball.global_position.x + attack_dir * 28.0, 0.85) + tactic_shift
				if attack_dir > 0.0:
					target_pos.x = clampf(minf(desired_x, offside_lim - 1.2), -PitchBuilder.HALF_LEN + 10.0, PitchBuilder.HALF_LEN - 6.0)
				else:
					target_pos.x = clampf(maxf(desired_x, offside_lim + 1.2), -PitchBuilder.HALF_LEN + 6.0, PitchBuilder.HALF_LEN - 10.0)
				target_pos.z = home_anchor.z * 0.85
				# func_800691A8 case 5: spread wide opposite the ball-carrier's flank
				if _ball.owner_player != null and signf(target_pos.z) == signf(_ball.owner_player.global_position.z) and absf(target_pos.z) > 4.0:
					target_pos.z = -target_pos.z
	else:
		if _is_closest_presser():
			if _ball.owner_player != null:
				var carrier := _ball.owner_player as Player
				var dist_to_carrier := global_position.distance_to(carrier.global_position)
				if dist_to_carrier > 3.2:
					# WE3 func_800807F0.c: cut off lane ~2.05m ahead when chasing from distance
					var carrier_fwd := -carrier.global_transform.basis.z
					carrier_fwd.y = 0.0
					target_pos = carrier.global_position + carrier_fwd.normalized() * 2.05
				else:
					# Close directly onto the ball to challenge/steal!
					target_pos = _ball.global_position
			else:
				target_pos = _ball.predict_landing_point(20)
		else:
			# Defensive shape by role (+ offside trap step-up for DF)
			var shift_ratio := 0.28 if role == Role.DF else 0.42
			target_pos.x = lerpf(home_anchor.x, _ball.global_position.x - attack_dir * 10.0, shift_ratio)
			if role == Role.DF and offside_trap:
				target_pos.x += attack_dir * 5.5
			if role == Role.DF:
				# Keep DF behind the ball when defending
				if attack_dir > 0.0:
					target_pos.x = minf(target_pos.x, _ball.global_position.x - 4.0)
				else:
					target_pos.x = maxf(target_pos.x, _ball.global_position.x + 4.0)
			target_pos.z = lerpf(home_anchor.z, _ball.global_position.z, 0.35)

	# Pass-lane avoidance (func_8003172C.c): sidestep +-90 deg if teammate's fast pass is crossing close
	if _ball.owner_player == null and _ball.last_touch_team == team_side and _ball.vel.length() > 7.5:
		var to_me := global_position - _ball.global_position
		to_me.y = 0.0
		if to_me.length() < 4.1 and to_me.length() > 0.4:
			var bdir := Vector3(_ball.vel.x, 0.0, _ball.vel.z).normalized()
			if bdir.dot(to_me.normalized()) > 0.85 and not _is_closest_presser():
				var side_vec := Vector3(-bdir.z, 0.0, bdir.x)
				var sign_s := 1.0 if side_vec.dot(to_me) >= 0.0 else -1.0
				target_pos = global_position + side_vec * (sign_s * 1.5)

	# Gentle teammate separation so players never stack on top of each other
	for mate in _all_players:
		if mate != self and mate.team_side == team_side and not mate.is_sent_off:
			var sep := global_position - mate.global_position
			sep.y = 0.0
			var d := sep.length()
			if d > 0.01 and d < 1.8:
				target_pos += (sep / d) * (1.8 - d)

	var to_target := target_pos - global_position
	to_target.y = 0.0
	var diff_mult: float = [0.82, 0.95, 1.06][clampi(GameData.ai_difficulty, 0, 2)]
	if to_target.length() > 0.6:
		var dir := to_target.normalized()
		var spd: float = ((_sprint_speed * _fat_spd()) * 0.90 if _is_closest_presser() else (_walk_speed * _fat_spd()) * 0.92) * diff_mult
		_step_motion(dir, spd, spd > (_walk_speed * _fat_spd()) * 1.04, delta)
	else:
		_step_motion(Vector3.ZERO, 0.0, false, delta)
		_face_direction((_ball.global_position - global_position).normalized(), delta)

func _process_gk_ai(delta: float) -> void:
	var own_goal_x := -PitchBuilder.HALF_LEN if team_side == 0 else PitchBuilder.HALF_LEN
	var inward := 1.0 if team_side == 0 else -1.0
	var goal_center := Vector3(own_goal_x, 0.0, 0.0)

	if has_ball():
		_ai_hold_timer += delta
		vel = vel.lerp(Vector3.ZERO, clampf((_accel * _fat_acc()) * delta, 0.0, 1.0))
		_face_direction(Vector3(inward, 0.0, 0.0), delta)
		if _ai_hold_timer < 0.90:
			return
		_ai_hold_timer = 0.0
		# Distribute ball to an open teammate (or punt upfield if none)
		var open_mate := _find_best_open_teammate(inward)
		if open_mate != null:
			_kick_pass_to_target(open_mate, false, true)
		else:
			var clear_dir := Vector3(inward, 0.0, randf_range(-0.25, 0.25)).normalized()
			_perform_pass(clear_dir, false, true)
		return
	_ai_hold_timer = 0.0

	var ball_pos := _ball.global_position
	# WE3 bisector positioning (GOALKEEPER_AND_PITCH.md)
	var to_ball := ball_pos - goal_center
	to_ball.y = 0.0
	var bisector_dist := clampf(to_ball.length() * 0.14, 1.4, 4.2)
	var bisector_dir := to_ball.normalized() if to_ball.length_squared() > 0.01 else Vector3(inward, 0.0, 0.0)
	var target := goal_center + bisector_dir * bisector_dist
	target.z = clampf(target.z, -3.4, 3.4)

	if not ((_ball.owner_player == null) and ((_ball.vel.x * inward) < -9.0)):
		_gk_diving = false
	# WE3 func_8008F2DC keeper dive: reacts >= 8 frames after the kick, aims at the ball position 7 frames ahead
	var shot_coming := (_ball.owner_player == null) and ((_ball.vel.x * inward) < -5.76) and absf(ball_pos.x - own_goal_x) < 24.0 and _ball.last_touch_team != team_side
	if shot_coming and _gk_dive_t < 0.0 and _ball.flight_frames >= GK_REACT_FRAMES:
		var aim := _ball.position_after(GK_DIVE_FRAMES)
		var gap := Vector2(aim.x - global_position.x, aim.z - global_position.z).length()
		var in_mouth := absf(aim.z) < PitchBuilder.GOAL_HALF_W + 1.2 and (aim.x - own_goal_x) * inward < 2.5 and aim.y < 2.6
		if in_mouth and gap <= GK_DIVE_BASE + 0.10 * float(stats["defense"]):
			_gk_start_dive(aim)
			return

	# Rush out if ball is loose inside the 18-yard box and close
	elif _ball.owner_player == null and absf(ball_pos.x - own_goal_x) < 14.0 and absf(ball_pos.z) < 12.0:
		target = ball_pos

	var to_t := target - global_position
	to_t.y = 0.0
	if to_t.length() > 0.2:
		var dir := to_t.normalized()
		var spd := (_sprint_speed * _fat_spd()) * 0.95 if shot_coming else (_walk_speed * _fat_spd()) * 1.05
		_step_motion(dir, spd, spd > (_walk_speed * _fat_spd()) * 1.04, delta)
	else:
		_step_motion(Vector3.ZERO, 0.0, false, delta)
	_face_direction((ball_pos - global_position).normalized(), delta)

const LOGIC_DT := 1.0 / 30.0       # WE3 logic frame
const U2MS := 0.12                 # 1 speed unit/frame = 0.004 m * 30 fps
const ANG_U := TAU / 256.0         # WE3 angle unit (0..255 = 360 deg)
var _mv_accum: float = 0.0
var _speed_u: float = 0.0          # actor+0x3A0
var _move_yaw: float = 0.0         # actor+0x3A4 (travel heading, same convention as rotation.y)
var _braking: bool = false
var _last_set_vel: Vector3 = Vector3.ZERO

func _step_motion(dir: Vector3, cap_ms: float, sprint: bool, delta: float) -> void:
	# Re-sync the WE3 speed/heading state if other systems (tackles, restarts, locks) changed `vel`
	if vel.distance_to(_last_set_vel) > 0.02:
		var vflat := Vector2(vel.x, vel.z)
		_speed_u = vflat.length() / U2MS
		if vflat.length() > 0.05:
			_move_yaw = atan2(-vel.x, -vel.z)
	_mv_accum += delta
	var steps := 0
	while _mv_accum >= LOGIC_DT and steps < 4:
		_mv_accum -= LOGIC_DT
		steps += 1
		_motion_tick(dir, cap_ms / U2MS, sprint)
	if _mv_accum > LOGIC_DT:
		_mv_accum = 0.0
	var fwd := Vector3(-sin(_move_yaw), 0.0, -cos(_move_yaw))
	vel = fwd * (_speed_u * U2MS)
	_last_set_vel = vel

## One 30 Hz logic tick of the WE3 run / sprint steering (func_80048AD8, func_80054A68, func_800259F0).
func _motion_tick(dir: Vector3, cap_u: float, sprint: bool) -> void:
	_braking = false
	if dir.length_squared() < 0.0025 or cap_u <= 0.0:
		_speed_u = maxf(0.0, _speed_u - 8.0)          # coast-down: 8 units/frame
		return
	var target_yaw := atan2(-dir.x, -dir.z)
	var diff := absf(angle_difference(_move_yaw, target_yaw))
	if _speed_u < 1.0:
		# standing start: snap travel heading to input, body turns at its own rate
		_move_yaw = target_yaw
		diff = 0.0
	var body_diff := absf(angle_difference(rotation.y, target_yaw))
	if not sprint:
		if diff > 1.5708 and _speed_u >= 32.0:
			# >90 deg reversal at speed: skid (speed -= 8) before turning
			_speed_u = maxf(0.0, _speed_u - 8.0)
			_braking = true
			return
		rotation.y = rotate_toward(rotation.y, target_yaw, 12.0 * ANG_U)
		_move_yaw = rotate_toward(_move_yaw, target_yaw, 24.0 * ANG_U)
		var cap := cap_u if (body_diff < 0.05 or diff < 33.0 * ANG_U) else minf(cap_u, 34.0)
		_speed_u = minf(cap, _speed_u + cap * 0.25)
	else:
		var div := maxf(2.0, 18.0 - float(ACCEL_TABLE[clampi(int(stats["acceleration"]), 0, 9)]) - 2.0)
		if diff < 33.0 * ANG_U:
			# straight dash: heading may only swing 4 units/frame
			rotation.y = rotate_toward(rotation.y, target_yaw, 4.0 * ANG_U)
			_move_yaw = rotate_toward(_move_yaw, target_yaw, 4.0 * ANG_U)
			_speed_u = clampf(_speed_u + cap_u / div, 30.0, cap_u)
		elif diff >= 96.0 * ANG_U and _speed_u >= cap_u - 0.5:
			# hard reversal at top speed -> anim 7 skid
			_speed_u = maxf(0.0, _speed_u - 10.0)
			_braking = true
		else:
			rotation.y = rotate_toward(rotation.y, target_yaw, 12.0 * ANG_U)
			_move_yaw = rotate_toward(_move_yaw, target_yaw, 24.0 * ANG_U)
			_speed_u = minf(34.0, _speed_u + 8.0)

func _gk_start_dive(aim: Vector3) -> void:
	_gk_dive_aim = aim
	_gk_dive_t = 0.0
	var target := Vector3(aim.x, 0.0, aim.z)
	var to_t := target - global_position
	to_t.y = 0.0
	vel = to_t / (GK_DIVE_FRAMES * LOGIC_DT)
	_speed_u = 0.0
	_face_direction(Vector3(1.0 if team_side == 0 else -1.0, 0.0, 0.0), 1.0)
	_update_mirror_foot()
	_play_anim("gk_dive", 0.04)

## Frame 7 of the dive: contact test (func_80094EEC) then catch / spill (func_80094DC4).
func _gk_dive_finish() -> void:
	vel = Vector3.ZERO
	_gk_dive_t = -1.0
	if _ball.owner_player != null:
		return
	var d2 := Vector2(_ball.global_position.x - global_position.x, _ball.global_position.z - global_position.z).length()
	if d2 < GK_TOUCH_RADIUS and _ball.global_position.y < 2.6:
		_gk_touch_ball()
	else:
		action_lock_timer = 0.55   # beaten: lies on the ground

## func_80094DC4: true = spill (ball pops up), false = hold. Level = keeper rating 0..9 (defense stat).
func _gk_should_spill(speed_raw: float, timer: int) -> bool:
	var level := int(stats["defense"])
	if timer < 29 - level and speed_raw >= 0x3000:
		return true
	if timer >= 48:
		return false
	var d := (level - 7) * 3072
	var r := (randi() % absi(d)) if d != 0 else 0
	var threshold := (timer - 22) * 768 + 0xA000
	return float(r + threshold) < speed_raw

func _gk_touch_ball() -> void:
	var b := _ball
	var hspeed := Vector2(b.vel.x, b.vel.z).length()
	var speed_raw := hspeed / U2MS * 256.0
	if _gk_should_spill(speed_raw, b.flight_frames):
		MatchState.gk_spills[team_side] += 1
		var heading := Vector3(b.vel.x, 0.0, b.vel.z)
		heading = heading.normalized() if heading.length_squared() > 0.001 else Vector3(1.0 if team_side == 0 else -1.0, 0.0, 0.0)
		# func_800951C4(heading + triangular jitter +-16 units, power 16..23 units/frame, lift 16)
		var jitter := float(randi() % 17 - randi() % 17)
		heading = heading.rotated(Vector3.UP, jitter * ANG_U)
		var push := float(16 + randi() % 8) * U2MS
		b.kick(heading * push + Vector3(0.0, 16.0 * 736.0 / 256.0 * U2MS, 0.0), self, team_side)
		action_lock_timer = 0.5
		_play_anim("gk_dive", 0.04)
	else:
		MatchState.gk_saves[team_side] += 1
		b.set_owner_player(self, team_side)
		MatchState.last_touch_team = team_side
		MatchState.last_touch_player_name = player_name
		action_lock_timer = 0.3
		_play_anim("gk_catch", 0.04)

func _is_closest_presser() -> bool:
	var target_pos: Vector3 = _ball.global_position
	if _ball.owner_player == null:
		target_pos = _ball.predict_landing_point(18)
		# Avoid chasing our own pass right after kicking
		if action_lock_timer > 0.1:
			return false
	var my_dist := global_position.distance_squared_to(target_pos)
	for p in _all_players:
		if p != self and p.team_side == team_side and p.role != Role.GK and not p.is_sent_off:
			if p.is_controlled:
				if _ball.owner_player == null and p.global_position.distance_squared_to(target_pos) < 12.25:
					return false
				continue
			if p.tackle_timer > 0.0 or p.action_lock_timer > 0.1:
				continue
			if p.global_position.distance_squared_to(target_pos) < my_dist:
				return false
	return true

func _find_nearest_opponent(from_pos: Vector3) -> Player:
	# WE3 func_800494C0.c: track closest_1 and closest_2 backup if closest_1 is busy/tackling
	var best: Player = null
	var second: Player = null
	var best_d2 := INF
	var second_d2 := INF
	for p in _all_players:
		if p.team_side != team_side:
			var d2 := p.global_position.distance_squared_to(from_pos)
			if d2 < best_d2:
				second = best
				second_d2 = best_d2
				best = p
				best_d2 = d2
			elif d2 < second_d2:
				second = p
				second_d2 = d2
	if best != null and (best.tackle_timer > 0.0 or best.action_lock_timer > 0.25) and second != null:
		return second
	return best

func _find_best_open_teammate(attack_dir: float) -> Player:
	var best: Player = null
	var best_score := -INF
	for p in _all_players:
		if p == self or p.team_side != team_side or p.role == Role.GK or p.is_sent_off:
			continue
		var dist := global_position.distance_to(p.global_position)
		if dist < 4.5 or dist > 38.0:
			continue
		if _is_player_offside(p):
			continue
		var forward_gain := (p.global_position.x - global_position.x) * attack_dir
		var opp := _find_nearest_opponent(p.global_position)
		var space := opp.global_position.distance_to(p.global_position) if opp != null else 15.0
		var lane_bonus := 4.0 if _is_pass_lane_clear(global_position, p.global_position) else -8.0
		# In the attacking half, progression matters more than safety (AI pushes into the box)
		var fwd_w := 0.55 if (global_position.x * attack_dir) > 8.0 else 0.25
		var score := forward_gain * fwd_w + minf(space, 12.0) * 0.6 - dist * 0.12 + lane_bonus
		if score > best_score:
			best_score = score
			best = p
	return best

func _is_pass_lane_clear(from_pos: Vector3, to_pos: Vector3) -> bool:
	var seg := to_pos - from_pos
	seg.y = 0.0
	var seg_len := seg.length()
	if seg_len < 1.0:
		return true
	var seg_dir := seg / seg_len
	for p in _all_players:
		if p.team_side != team_side and not p.is_sent_off:
			var rel := p.global_position - from_pos
			rel.y = 0.0
			var proj := rel.dot(seg_dir)
			if proj > 1.0 and proj < seg_len - 1.0:
				var perp := (rel - seg_dir * proj).length()
				if perp < 1.8:
					return false
	return true

func _get_offside_line_x(atk_dir: float) -> float:
	var opp_xs: Array[float] = []
	for p in _all_players:
		if p.team_side != team_side and not p.is_sent_off:
			opp_xs.append(p.global_position.x)
	if opp_xs.size() < 2:
		return PitchBuilder.HALF_LEN * atk_dir
	opp_xs.sort()
	if atk_dir > 0.0:
		return opp_xs[opp_xs.size() - 2]
	return opp_xs[1]

func _solve_pass_speed(dist_m: float, min_arrival_spd: float) -> float:
	# func_8002EB3C: smallest push that still arrives with >= min speed, simulated with the real rolling friction.
	# Authentic pass push is clamped to 112 units/frame = 13.44 m/s.
	var cap := 13.44 * _kick_mul
	var v := min_arrival_spd + 1.0
	while v < cap:
		if Ball.roll_distance(v, min_arrival_spd) >= dist_m:
			return v
		v += 0.3
	return cap

## Lob: pick flight time from distance (flight limited by pass power), vertical speed from gravity.
func _lob_velocity(dir: Vector3, dist_m: float) -> Vector3:
	var t := clampf(dist_m / 12.5, 0.9, 2.7)
	var vy := Ball.GRAVITY * t * 0.5
	var h := minf(dist_m / t * 1.05, 20.0 * _kick_mul)
	var flat := Vector3(dir.x, 0.0, dir.z).normalized()
	return flat * h + Vector3(0.0, vy, 0.0)

func _kick_pass_to_target(target: Player, is_through: bool, is_loft: bool) -> void:
	var lead := target.vel * (0.45 if is_through else 0.18)
	var dest := target.global_position + lead
	var to_dest := dest - global_position
	to_dest.y = 0.0
	var dist := to_dest.length()
	var kick_dir := to_dest.normalized() if dist > 0.01 else -global_transform.basis.z
	var min_arrive := 4.8 if is_through else 3.0
	var speed := _solve_pass_speed(dist, min_arrive)
	kick_dir = _apply_tech_error(kick_dir)
	var kick_vel := kick_dir * speed
	if is_loft:
		kick_vel = _lob_velocity(kick_dir, dist)
	_face_direction(kick_dir, 1.0)
	_ball.kick(kick_vel, self, team_side)
	if _is_player_offside(target):
		_ball.offside_target = target
		_ball.offside_defending_team = 1 - team_side
	MatchState.passes_completed[team_side] += 1
	action_lock_timer = 0.22
	_play_anim("pass_long" if is_loft else "pass", 0.06)

func _find_pass_target(aim_dir: Vector3, is_through: bool) -> Player:
	var dir := aim_dir if aim_dir.length_squared() > 0.05 else -global_transform.basis.z
	dir.y = 0.0
	dir = dir.normalized()

	var best: Player = null
	var best_score := -INF
	for p in _all_players:
		if p == self or p.team_side != team_side:
			continue
		var to_p := p.global_position - global_position
		to_p.y = 0.0
		var dist := to_p.length()
		if dist < 2.0 or dist > 48.0:
			continue
		var align := dir.dot(to_p / dist)
		if align < 0.15:
			continue
		var score := align * 2.2 - (dist / 45.0)
		if is_through and p.role == Role.FW:
			score += 0.4
		if score > best_score:
			best_score = score
			best = p
	return best

func _perform_pass(aim_dir: Vector3, is_through: bool, is_loft: bool) -> void:
	var target := _find_pass_target(aim_dir, is_through)
	var kick_dir := aim_dir if aim_dir.length_squared() > 0.05 else -global_transform.basis.z
	kick_dir.y = 0.0
	kick_dir = kick_dir.normalized()

	var speed := 11.0
	var pass_dist := 22.0
	if target != null:
		var lead := target.vel * (0.45 if is_through else 0.18)
		var dest := target.global_position + lead
		var to_dest := dest - global_position
		to_dest.y = 0.0
		if to_dest.length() > 0.5:
			kick_dir = to_dest.normalized()
			pass_dist = to_dest.length()
			speed = _solve_pass_speed(pass_dist, 4.8 if is_through else 3.0)

	kick_dir = _apply_tech_error(kick_dir)
	var kick_vel := kick_dir * speed
	if is_loft:
		kick_vel = _lob_velocity(kick_dir, pass_dist)

	_face_direction(kick_dir, 1.0)
	_ball.kick(kick_vel, self, team_side)
	# Offside check: game-side heuristic (second-last defender). NOTE: func_8001F2D0 is the match-end/result setup, not the offside rule; the real rule was not located in the C decomp
	if target != null and _is_player_offside(target):
		_ball.offside_target = target
		_ball.offside_defending_team = 1 - team_side
	MatchState.passes_completed[team_side] += 1
	action_lock_timer = 0.22
	_play_anim("pass_long" if is_loft else "pass", 0.06)

func _is_player_offside(target: Player) -> bool:
	if MatchState.phase != MatchState.Phase.OPEN_PLAY:
		return false
	var atk_dir := 1.0 if team_side == 0 else -1.0
	var tx := target.global_position.x * atk_dir
	var bx := _ball.global_position.x * atk_dir
	if tx <= 0.0 or tx <= bx:
		return false
	# Find 2nd-last opponent along attack direction
	var opp_xs: Array[float] = []
	for p in _all_players:
		if p.team_side != team_side and not p.is_sent_off:
			opp_xs.append(p.global_position.x * atk_dir)
	opp_xs.sort()
	opp_xs.reverse()
	var line_x := opp_xs[1] if opp_xs.size() >= 2 else PitchBuilder.HALF_LEN
	return tx > line_x + 0.5

func _perform_aerial_contest() -> void:
	var atk_dir := 1.0 if team_side == 0 else -1.0
	var attack_x := PitchBuilder.HALF_LEN * atk_dir
	var head_stat := float(stats.get("head", 6))
	var is_header := _ball.global_position.y >= 1.25
	_play_anim("header" if is_header else "volley", 0.05)
	action_lock_timer = 0.36
	MatchState.last_touch_team = team_side
	MatchState.last_touch_player_name = player_name

	if absf(attack_x - global_position.x) < 24.0 and absf(global_position.z) < 16.0:
		# Attacking aerial strike on goal
		MatchState.shot_attempts[team_side] += 1
		var target_z := randf_range(-2.6, 2.6) * clampf(1.2 - 0.06 * head_stat, 0.5, 1.2)
		var dir := (Vector3(attack_x, 0.0, target_z) - global_position)
		dir.y = 0.0
		dir = dir.normalized()
		_face_direction(dir, 1.0)
		var spd := (12.0 + 0.45 * head_stat) * _kick_mul
		var lift := 1.8 if is_header else 2.6
		_ball.kick(dir * spd + Vector3(0.0, lift, 0.0), self, team_side, randf_range(-1.5, 1.5))
	else:
		# Midfield / defensive aerial clearance downfield
		var clear_dir := Vector3(atk_dir, 0.0, randf_range(-0.35, 0.35)).normalized()
		_face_direction(clear_dir, 1.0)
		var spd := 11.0 + 0.35 * head_stat
		_ball.kick(clear_dir * spd + Vector3(0.0, 4.2, 0.0), self, team_side)

func _perform_shoot(aim_dir: Vector3, power: float, exact_aim: bool = false) -> void:
	MatchState.shot_attempts[team_side] += 1
	var attack_x := PitchBuilder.HALF_LEN if team_side == 0 else -PitchBuilder.HALF_LEN
	var goal_center := Vector3(attack_x, 0.0, 0.0)
	var base_dir := (goal_center - global_position).normalized()
	if aim_dir.length_squared() > 0.05:
		if exact_aim:
			base_dir = aim_dir.normalized()   # AI placing the shot in a chosen corner
		else:
			base_dir = (base_dir * 0.65 + aim_dir.normalized() * 0.35).normalized()
	base_dir.y = 0.0
	base_dir = _apply_tech_error(base_dir, 1.5)

	# WE3 func_80040324: shot push 128..143 units/frame (15.4-17.2 m/s) at full power; low lift
	var spd := lerpf(10.8, 17.2, clampf(power, 0.2, 1.0)) * _kick_mul
	var loft := lerpf(1.2, 4.5, clampf(power, 0.2, 1.0))
	var kick_vel := base_dir * spd + Vector3(0.0, loft, 0.0)
	var spin := randf_range(-2.5, 2.5)

	var ball_h := _ball.global_position.y
	_update_mirror_foot()
	_face_direction(base_dir, 1.0)
	_pending_kick_vel = kick_vel
	_pending_kick_spin = spin
	_pending_is_shot = true
	_pending_kick_timer = 0.14
	action_lock_timer = 0.34
	if ball_h > 1.35:
		_play_anim("header", 0.05)
	elif ball_h > 0.75:
		_play_anim("volley", 0.05)
	else:
		_play_anim("shoot", 0.06)
	SoundFX.play_commentary("shot", player_name)

func _update_mirror_foot() -> void:
	if _model_root == null or _ball == null:
		return
	var local_x := global_transform.basis.x.dot(_ball.global_position - global_position)
	_model_root.scale.x = -1.0 if local_x < 0.0 else 1.0

func _perform_slide_tackle(aim_dir: Vector3) -> void:
	var dir := aim_dir if aim_dir.length_squared() > 0.05 else -global_transform.basis.z
	dir.y = 0.0
	tackle_dir = dir.normalized()
	tackle_timer = 0.36
	action_lock_timer = 0.55
	_face_direction(tackle_dir, 1.0)
	_play_anim("tackle", 0.05)

func _try_pickup_ball() -> void:
	if _ball == null or _ball.pickup_cooldown > 0.0:
		return
	if role == Role.GK and _gk_dive_t >= 0.0:
		return   # mid-dive: contact is decided only on dive frame 7 (func_80094EEC)
	var max_h := 2.25 if role == Role.GK else (2.15 if _ball.owner_player == null else 1.35)
	var radius := 1.25 if role == Role.GK else PICKUP_RADIUS
	if _ball.global_position.y > max_h:
		return
	var dist := Vector2(global_position.x - _ball.global_position.x, global_position.z - _ball.global_position.z).length()
	if dist > radius:
		return

	var incoming_speed := _ball.vel.length()
	# Goalkeeper touching a fast shot: authentic hold/spill decision (func_80094DC4)
	if role == Role.GK and _ball.owner_player == null and incoming_speed > 5.76 and _ball.last_touch_team != team_side and _gk_dive_t < 0.0:
		_gk_touch_ball()
		return
	if role == Role.GK and _ball.owner_player == null:
		if incoming_speed > 10.0:
			_play_anim("gk_dive", 0.05)
			action_lock_timer = 0.35
		elif incoming_speed > 4.0:
			_play_anim("gk_catch", 0.05)
			action_lock_timer = 0.25

	if _ball.owner_player == null:
		if _ball.offside_target == self:
			var def_team := _ball.offside_defending_team
			_ball.offside_target = null
			_ball.out_of_bounds_detected.emit("offside", global_position, def_team)
			return
		# Authentic WE3 aerial header / volley contest (func_80035A5C / func_8003A174)
		if role != Role.GK and _ball.global_position.y >= 0.95:
			_perform_aerial_contest()
			return
		# Authentic WE3 high-speed ball deflection / heavy touch (func_8004DEFC)
		var tech_stat := float(stats.get("technique", 6))
		var defl_thresh := 11.5 + tech_stat * 0.65
		if role != Role.GK and incoming_speed > defl_thresh and _ball.flight_frames < 32:
			var bdir := Vector3(_ball.vel.x, 0.0, _ball.vel.z)
			var normal := (global_position - _ball.global_position)
			normal.y = 0.0
			var out_dir := (-bdir.normalized() if bdir.length_squared() > 0.01 else -global_transform.basis.z)
			if normal.length_squared() > 0.001:
				out_dir = (out_dir * 0.6 + (-normal.normalized()) * 0.4).normalized()
			out_dir = out_dir.rotated(Vector3.UP, randf_range(-0.45, 0.45))
			var out_spd := clampf(incoming_speed * 0.38 + Vector2(vel.x, vel.z).length() * 0.3, 3.5, 8.5)
			_ball.kick(out_dir * out_spd + Vector3(0.0, randf_range(1.2, 2.6), 0.0), self, team_side)
			MatchState.last_touch_team = team_side
			MatchState.last_touch_player_name = player_name
			_play_anim("trap", 0.05)
			action_lock_timer = 0.24
			return
		if role != Role.GK and incoming_speed > 8.5:
			_play_anim("trap", 0.05)
			action_lock_timer = 0.16
		_ball.set_owner_player(self, team_side)
		MatchState.last_touch_team = team_side
		MatchState.last_touch_player_name = player_name
		# Execute buffered One-Touch action immediately on arrival!
		if _one_touch_timer > 0.0 and _one_touch_action != "":
			var act := _one_touch_action
			_one_touch_action = ""
			_one_touch_timer = 0.0
			var fwd := -global_transform.basis.z
			if act == "pass_short":
				_perform_pass(fwd, false, false)
			elif act == "pass_through":
				_perform_pass(fwd, true, false)
			elif act == "shoot":
				_perform_shoot(fwd, 0.72)
	elif _ball.owner_player != self and (_ball.owner_player as Player).team_side != team_side:
		# Steal / tackle challenge (with WE3 foul + yellow/red card check)
		var carrier := _ball.owner_player as Player
		# Stand-up challenges are discrete attempts (WE3 AI tackles on a cooldown), not a per-frame roll
		if tackle_timer <= 0.0:
			if _tackle_cd > 0.0:
				return
			_tackle_cd = 0.7
		# func_80040DD4: the slide's contact check only becomes active from animation frame 5; mode 1/2 contact
		# takes the ball cleanly (possession), mode 0 contact just knocks it away (poke).
		var sliding := tackle_timer > 0.0
		if sliding and tackle_timer > 0.36 - 5.0 * LOGIC_DT:
			return
		var foul_severity := _eval_we3_foul_severity(carrier) if sliding else 0
		var win_chance := 1.0 if sliding else 0.34
		win_chance *= clampf((0.7 + 0.06 * float(stats["defense"])) / (0.7 + 0.06 * float(carrier.stats["technique"])), 0.6, 1.5)
		if foul_severity == 0 and randf() < win_chance:
			carrier.action_lock_timer = 0.45
			MatchState.last_touch_team = team_side
			MatchState.last_touch_player_name = player_name
			if sliding:
				_ball.set_owner_player(self, team_side)   # clean win
			else:
				# poke: power = max(72 units, own speed + 8 + rand(4)) along own heading +/- 16 units, lift rand(3)
				var fwd := -global_transform.basis.z
				fwd.y = 0.0
				fwd = fwd.normalized().rotated(Vector3.UP, float(randi() % 17 - randi() % 17) * ANG_U)
				var push := maxf(72.0, _speed_u + 8.0 + float(randi() % 4)) * U2MS
				_ball.kick(fwd * push + Vector3(0.0, float(randi() % 3) * 2.875 * U2MS, 0.0), self, team_side)
		elif sliding and (foul_severity > 0 or dist > 0.58):
			# Authentic WE3 foul & card (func_80052720)
			if foul_severity == 0:
				foul_severity = 1
			tackle_timer = 0.0
			action_lock_timer = 0.65
			_play_anim("foul_react", 0.05)
			_apply_foul_card(foul_severity)
			# Check if foul occurred inside defending team's 18-yard box -> Penalty Kick!
			var def_goal_x := -PitchBuilder.HALF_LEN if team_side == 0 else PitchBuilder.HALF_LEN
			if absf(carrier.global_position.x - def_goal_x) < 16.5 and absf(carrier.global_position.z) < 20.16:
				var pen_x := def_goal_x + (11.0 if team_side == 0 else -11.0)
				_ball.out_of_bounds_detected.emit("penalty", Vector3(pen_x, 0.0, 0.0), carrier.team_side)
			else:
				_ball.out_of_bounds_detected.emit("foul", carrier.global_position, carrier.team_side)

## Authentic WE3 foul & card evaluator (func_80052720 + jtbl_80011DA4).
## Returns: 0 = clean, 1 = foul only, 2 = yellow card, 3 = straight red card.
func _eval_we3_foul_severity(carrier: Player) -> int:
	if MatchState.phase != MatchState.Phase.OPEN_PLAY:
		return 0
	if role == Role.GK or team_side == carrier.team_side:
		return 0

	# Team booking cap: suppress new cards if >= 4 teammates are already booked
	var booked_count := 0
	var p_parent := get_parent()
	if p_parent != null:
		for ch in p_parent.get_children():
			if ch is Player and (ch as Player).team_side == team_side:
				var pl := ch as Player
				if pl.yellow_cards > 0 or pl.is_sent_off:
					booked_count += 1
	var suppress_card := (booked_count >= 4 and yellow_cards == 0)

	var to_other := carrier.global_position - global_position
	to_other.y = 0.0
	var c_fwd := -carrier.global_transform.basis.z
	c_fwd.y = 0.0
	var delta_u := 128
	if to_other.length_squared() > 0.0001 and c_fwd.length_squared() > 0.0001:
		var ref_dir := c_fwd.normalized().rotated(Vector3.UP, -8.0 * ANG_U)
		var ang_rad := absf(ref_dir.signed_angle_to(to_other.normalized(), Vector3.UP))
		delta_u = clampi(int(round(ang_rad / ANG_U)), 0, 128)

	var bucket := delta_u >> 4
	match bucket:
		0:
			# .L80052900: straight from behind (0..21 deg)
			if suppress_card:
				return 1
			if randi() % 4 == 0 and yellow_cards == 0:
				return 3
			return 2
		1, 2:
			# .L8005295C: angled from behind (22..66 deg)
			var r := randi() % 8
			if r == 0 or r == 2 or r == 6 or suppress_card:
				return 1
			return 2
		3, 4, 5:
			# .L800529A8: side challenge (67.5..133.6 deg)
			if randi() % 100 < 60:
				if suppress_card or randi() % 12 != 0:
					return 1
				return 2
		_:
			# .L800529E0: front challenge (135..180 deg)
			if randi() % 100 < 50:
				return 1

	# penalty_box_check: defending penalty area (15.6m from goal line, |z| < 19.4m)
	var def_goal_x := -PitchBuilder.HALF_LEN if team_side == 0 else PitchBuilder.HALF_LEN
	if absf(global_position.x - def_goal_x) <= 15.6 and absf(global_position.z) < 19.408:
		return 1
	return 0

func _apply_foul_card(severity: int) -> void:
	if severity == 3:
		is_sent_off = true
		visible = false
		global_position = Vector3(0.0, -10.0, -50.0)
		MatchState.log_event("RED CARD! %s sent off!" % player_name)
		SoundFX.play_commentary("red_card", player_name)
	elif severity == 2:
		yellow_cards += 1
		if yellow_cards >= 2:
			is_sent_off = true
			visible = false
			global_position = Vector3(0.0, -10.0, -50.0)
			MatchState.log_event("2nd YELLOW -> RED CARD! %s sent off!" % player_name)
			SoundFX.play_commentary("red_card", player_name)
		else:
			MatchState.log_event("YELLOW CARD: %s" % player_name)
			SoundFX.play_commentary("yellow_card", player_name)

func _move_and_clamp(delta: float) -> void:
	global_position += vel * delta
	global_position.x = clampf(global_position.x, -PitchBuilder.HALF_LEN - 2.0, PitchBuilder.HALF_LEN + 2.0)
	global_position.z = clampf(global_position.z, -PitchBuilder.HALF_WID - 2.0, PitchBuilder.HALF_WID + 2.0)
	global_position.y = 0.0
	if _shadow_mesh != null:
		_shadow_mesh.visible = not is_sent_off
		var stretch := 1.65 if (tackle_timer > 0.0 or _gk_dive_t >= 0.0) else 1.0
		_shadow_mesh.scale = Vector3(1.0, 1.0, stretch)

func _face_direction(dir: Vector3, delta: float) -> void:
	_face_direction_rate(dir, delta, TURN_SPEED)

func _face_direction_rate(dir: Vector3, delta: float, rate: float) -> void:
	var flat := Vector3(dir.x, 0.0, dir.z)
	if flat.length_squared() < 0.001:
		return
	var target_yaw := atan2(-flat.x, -flat.z)
	rotation.y = lerp_angle(rotation.y, target_yaw, clampf(rate * delta, 0.0, 1.0))

func _update_locomotion_anim() -> void:
	if action_lock_timer > 0.0 or tackle_timer > 0.0 or _pending_kick_timer > 0.0:
		return
	if _braking and _speed_u > 8.0:
		_play_anim("turn_brake", 0.05)
		return
	if _model_root != null and _model_root.scale.x != 1.0:
		_model_root.scale.x = 1.0
	var spd := Vector2(vel.x, vel.z).length()
	if spd < 0.35 or (_current_anim == "idle" and spd < 0.65):
		_play_anim("idle", 0.18, 1.0)
	elif has_ball():
		_play_anim("dribble", 0.14, clampf(spd / 5.2, 0.85, 1.25))
	elif spd > 6.6 or (_current_anim == "sprint" and spd > 5.8):
		_play_anim("sprint", 0.16, 1.15)
	else:
		_play_anim("run", 0.16, clampf(spd / 5.6, 0.85, 1.15))
