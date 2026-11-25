class_name Referee
extends Node3D
## Main referee (follows play) or Touchline Assistant Referee (tracks ball X with flag).

var is_linesman: bool = false
var touchline_z: float = 34.8
var _ball: Ball = null
var _model_root: Node3D = null
var _anim_player: AnimationPlayer = null
var _current_anim: String = ""
var vel: Vector3 = Vector3.ZERO

func setup(p_ball: Ball, p_is_linesman: bool = false, p_touchline_z: float = 34.8) -> void:
	_ball = p_ball
	is_linesman = p_is_linesman
	touchline_z = p_touchline_z

func _ready() -> void:
	var ref_path := "res://assets/3d/referee/referee_01.gltf" if is_linesman else "res://assets/3d/referee/referee_00.gltf"
	if not ResourceLoader.exists(ref_path):
		ref_path = "res://assets/3d/players/record_00.gltf"
	var packed := load(ref_path) as PackedScene
	if packed != null:
		_model_root = packed.instantiate()
		_model_root.rotation.y = PI
		add_child(_model_root)
		_apply_referee_kit(_model_root)
		_anim_player = _find_anim_player(_model_root)
		if _anim_player != null:
			var lib := PlayerAnimCache.get_library()
			if not _anim_player.has_animation_library("we3"):
				_anim_player.add_animation_library("we3", lib)
			_play_anim("idle")

func _apply_referee_kit(root: Node) -> void:
	# Authentic WE3 128x256 atlas for referee_00..03.gltf (top 128x128 = black ref kit, bottom 128x128 = skin/face)
	var atlas := Image.create(128, 256, false, Image.FORMAT_RGBA8)
	atlas.fill_rect(Rect2i(0, 0, 128, 128), Color(0.14, 0.15, 0.18))
	atlas.fill_rect(Rect2i(0, 128, 128, 128), Color(0.86, 0.68, 0.54))
	if ResourceLoader.exists("res://assets/textures/players/skin_black_hair.png"):
		var st := load("res://assets/textures/players/skin_black_hair.png") as Texture2D
		if st != null and st.get_image() != null:
			var si := st.get_image()
			si.convert(Image.FORMAT_RGBA8)
			atlas.blit_rect(si, Rect2i(0, 0, mini(128, si.get_width()), mini(128, si.get_height())), Vector2i(0, 128))
	var mat_atlas := StandardMaterial3D.new()
	mat_atlas.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat_atlas.albedo_texture = ImageTexture.create_from_image(atlas)
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
		mat_boots.albedo_color = Color(0.08, 0.08, 0.10)
	mat_boots.cull_mode = BaseMaterial3D.CULL_DISABLED

	_attach_boots(root, mat_boots)
	_apply_rec(root, mat_atlas, mat_atlas, mat_atlas, mat_atlas, mat_boots)

func _attach_boots(root: Node, mat_boots: Material) -> void:
	var boot_r_mesh: Variant = load("res://assets/3d/players/tmd_60_off_0E328_v8_p5.obj")
	var boot_l_mesh: Variant = load("res://assets/3d/players/tmd_61_off_0E45C_v8_p5.obj")
	var r_foot := root.find_child("08_R_Foot", true, false)
	if r_foot != null and boot_r_mesh is Mesh:
		var mi_r := MeshInstance3D.new()
		mi_r.name = "BootMesh_R"
		mi_r.mesh = boot_r_mesh
		mi_r.material_override = mat_boots
		mi_r.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		r_foot.add_child(mi_r)
	var l_foot := root.find_child("11_L_Foot", true, false)
	if l_foot != null and boot_l_mesh is Mesh:
		var mi_l := MeshInstance3D.new()
		mi_l.name = "BootMesh_L"
		mi_l.mesh = boot_l_mesh
		mi_l.material_override = mat_boots
		mi_l.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		l_foot.add_child(mi_l)

func _apply_rec(n: Node, mat_shirt: Material, mat_shorts: Material, mat_socks: Material, mat_skin: Material, mat_boots: Material) -> void:
	if n is MeshInstance3D:
		var mi := n as MeshInstance3D
		mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		if mi.name.begins_with("00_") or mi.name.begins_with("03_") or mi.name.begins_with("05_"):
			mi.material_override = mat_skin
		elif mi.name.begins_with("06_") or mi.name.begins_with("09_"):
			mi.material_override = mat_shorts
		elif mi.name.begins_with("07_") or mi.name.begins_with("10_"):
			mi.material_override = mat_socks
		elif mi.name.begins_with("08_") or mi.name.begins_with("11_") or mi.name.begins_with("BootMesh"):
			mi.material_override = mat_boots
		else:
			mi.material_override = mat_shirt
	for c in n.get_children():
		_apply_rec(c, mat_shirt, mat_shorts, mat_socks, mat_skin, mat_boots)

func _physics_process(delta: float) -> void:
	if _ball == null or MatchState.phase in [MatchState.Phase.PAUSED, MatchState.Phase.HALFTIME, MatchState.Phase.FULLTIME]:
		return

	var bpos := _ball.global_position
	var target := Vector3.ZERO
	if is_linesman:
		target = Vector3(clampf(bpos.x, -PitchBuilder.HALF_LEN + 2.0, PitchBuilder.HALF_LEN - 2.0), 0.0, touchline_z)
	else:
		# Main referee keeps ~10m diagonal offset from ball
		var offset := Vector3(-6.0 if bpos.x > 0.0 else 6.0, 0.0, -8.0 if bpos.z > 0.0 else 8.0)
		target = Vector3(
			clampf(bpos.x + offset.x, -PitchBuilder.HALF_LEN + 5.0, PitchBuilder.HALF_LEN - 5.0),
			0.0,
			clampf(bpos.z + offset.z, -PitchBuilder.HALF_WID + 4.0, PitchBuilder.HALF_WID - 4.0)
		)

	var to_t := target - global_position
	to_t.y = 0.0
	if to_t.length() > 0.8:
		var dir := to_t.normalized()
		var spd := 5.4 if to_t.length() > 6.0 else 3.8
		vel = vel.lerp(dir * spd, clampf(12.0 * delta, 0.0, 1.0))
		global_position += vel * delta
		var target_yaw := atan2(-dir.x, -dir.z)
		rotation.y = lerp_angle(rotation.y, target_yaw, clampf(10.0 * delta, 0.0, 1.0))
		_play_anim("run", 0.12)
	else:
		vel = vel.lerp(Vector3.ZERO, clampf(12.0 * delta, 0.0, 1.0))
		var look_dir := (bpos - global_position)
		look_dir.y = 0.0
		if look_dir.length_squared() > 0.01:
			var target_yaw := atan2(-look_dir.x, -look_dir.z)
			rotation.y = lerp_angle(rotation.y, target_yaw, clampf(10.0 * delta, 0.0, 1.0))
		_play_anim("idle", 0.12)

func _play_anim(anim_key: String, blend: float = 0.12) -> void:
	if _anim_player == null or _current_anim == anim_key:
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
