class_name PitchBuilder
extends Node3D
## Builds the 105m x 68m FIFA-regulation 3D soccer pitch, goals, ad boards, and stadium stands.

const PITCH_LENGTH := 105.0
const PITCH_WIDTH := 68.0
const HALF_LEN := PITCH_LENGTH * 0.5  # 52.5
const HALF_WID := PITCH_WIDTH * 0.5   # 34.0
const GOAL_WIDTH := 7.32
const GOAL_HALF_W := GOAL_WIDTH * 0.5 # 3.66
const GOAL_HEIGHT := 2.44
const GOAL_DEPTH := 2.0
const LINE_WIDTH := 0.14
const LINE_Y := 0.015

var _animated_flags: Array[Node3D] = []
var _flag_time: float = 0.0

func _ready() -> void:
	# Authentic WE3 stadium (grass, lines, ad boards, stands) + 3D goals + 3D corner flags + stand flags.
	_build_extracted_stadium(GameData.selected_stadium)
	_build_goal(-HALF_LEN, -1.0)
	_build_goal(HALF_LEN, 1.0)
	_build_we3_corner_flags()
	_build_stand_flags()

func _process(delta: float) -> void:
	if _animated_flags.is_empty():
		return
	_flag_time += delta
	for i in range(_animated_flags.size()):
		var f := _animated_flags[i]
		if is_instance_valid(f):
			var base_rot: float = float(f.get_meta("base_rot_y", 0.0))
			var phase: float = float(f.get_meta("phase", float(i) * 0.7))
			f.rotation.y = base_rot + sin(_flag_time * 4.2 + phase) * 0.16

func _build_we3_corner_flags() -> void:
	# Crisp 3D WE3 corner flags (white pole + red/yellow diagonal pennant, animated in _process)
	var pole_mat := StandardMaterial3D.new()
	pole_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	pole_mat.albedo_color = Color(0.96, 0.96, 0.98)

	var flag_tex := _create_corner_flag_texture()
	var flag_mat := StandardMaterial3D.new()
	flag_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	flag_mat.albedo_texture = flag_tex
	flag_mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	flag_mat.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST

	var pole_mesh := CylinderMesh.new()
	pole_mesh.top_radius = 0.024
	pole_mesh.bottom_radius = 0.028
	pole_mesh.height = 1.55

	var cloth_mesh := QuadMesh.new()
	cloth_mesh.size = Vector2(0.42, 0.28)

	var idx := 0
	for sx: float in [-1.0, 1.0]:
		for sz: float in [-1.0, 1.0]:
			var corner_root := Node3D.new()
			corner_root.name = "CornerFlag_%d" % idx
			corner_root.position = Vector3(sx * HALF_LEN, 0.0, sz * HALF_WID)
			add_child(corner_root)

			var pole := MeshInstance3D.new()
			pole.mesh = pole_mesh
			pole.material_override = pole_mat
			pole.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
			pole.position = Vector3(0.0, 0.775, 0.0)
			corner_root.add_child(pole)

			var pivot := Node3D.new()
			pivot.position = Vector3(0.0, 1.38, 0.0)
			var base_rot := 0.35 if sx < 0.0 else PI - 0.35
			pivot.rotation.y = base_rot
			pivot.set_meta("base_rot_y", base_rot)
			pivot.set_meta("phase", float(idx) * 1.1)
			corner_root.add_child(pivot)

			var cloth := MeshInstance3D.new()
			cloth.mesh = cloth_mesh
			cloth.material_override = flag_mat
			cloth.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
			cloth.position = Vector3(0.21, 0.0, 0.0)
			pivot.add_child(cloth)

			_animated_flags.append(pivot)
			idx += 1

func _create_corner_flag_texture() -> ImageTexture:
	var img := Image.create(32, 24, false, Image.FORMAT_RGBA8)
	var red := Color(0.92, 0.14, 0.12)
	var yellow := Color(0.98, 0.86, 0.12)
	for y in range(24):
		for x in range(32):
			var c := red if ((x / 8 + y / 8) % 2 == 0) else yellow
			img.set_pixel(x, y, c)
	return ImageTexture.create_from_image(img)

func _create_team_flag_texture(primary: Color, secondary: Color) -> ImageTexture:
	var img := Image.create(48, 32, false, Image.FORMAT_RGBA8)
	for y in range(32):
		for x in range(48):
			var c := primary
			if y >= 10 and y < 22:
				c = secondary
			elif x < 3 or x >= 45 or y < 2 or y >= 30:
				c = primary.darkened(0.18)
			img.set_pixel(x, y, c)
	return ImageTexture.create_from_image(img)

func _build_stand_flags() -> void:
	# Waving national team flags on stadium stands (Home side x < 0, Away side x > 0)
	var stand_flags := Node3D.new()
	stand_flags.name = "StandFlags"
	add_child(stand_flags)

	var h_pri := GameData.get_team_primary_color(GameData.home_team_id)
	var h_sec := GameData.get_team_secondary_color(GameData.home_team_id)
	var a_pri := GameData.get_team_primary_color(GameData.away_team_id)
	var a_sec := GameData.get_team_secondary_color(GameData.away_team_id)

	var home_mat := StandardMaterial3D.new()
	home_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	home_mat.albedo_texture = _create_team_flag_texture(h_pri, h_sec)
	home_mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	home_mat.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST

	var away_mat := StandardMaterial3D.new()
	away_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	away_mat.albedo_texture = _create_team_flag_texture(a_pri, a_sec)
	away_mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	away_mat.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST

	var pole_mat := StandardMaterial3D.new()
	pole_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	pole_mat.albedo_color = Color(0.86, 0.88, 0.90)

	var pole_mesh := CylinderMesh.new()
	pole_mesh.top_radius = 0.05
	pole_mesh.bottom_radius = 0.06
	pole_mesh.height = 4.2

	var cloth_mesh := QuadMesh.new()
	cloth_mesh.size = Vector2(2.4, 1.5)

	# Place flags along North & South stands (visible in broadcast camera) and behind both goals
	var placements: Array[Dictionary] = []
	for x_pos: float in [-42.0, -28.0, -14.0, 14.0, 28.0, 42.0]:
		placements.append({"pos": Vector3(x_pos, 4.5, -HALF_WID - 11.5), "is_home": x_pos < 0.0, "rot": 0.25})
		placements.append({"pos": Vector3(x_pos, 9.5, -HALF_WID - 18.5), "is_home": x_pos < 0.0, "rot": -0.20})
		placements.append({"pos": Vector3(x_pos, 4.5, HALF_WID + 11.5), "is_home": x_pos < 0.0, "rot": PI - 0.25})
	for z_pos: float in [-24.0, -12.0, 12.0, 24.0]:
		placements.append({"pos": Vector3(-HALF_LEN - 12.0, 5.0, z_pos), "is_home": true, "rot": PI * 0.5 + 0.2})
		placements.append({"pos": Vector3(HALF_LEN + 12.0, 5.0, z_pos), "is_home": false, "rot": -PI * 0.5 - 0.2})

	for i in range(placements.size()):
		var item: Dictionary = placements[i]
		var pos: Vector3 = item["pos"]
		var is_home: bool = item["is_home"]
		var base_rot: float = item["rot"]

		var root := Node3D.new()
		root.position = pos
		stand_flags.add_child(root)

		var pole := MeshInstance3D.new()
		pole.mesh = pole_mesh
		pole.material_override = pole_mat
		pole.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		pole.position = Vector3(0.0, 2.1, 0.0)
		root.add_child(pole)

		var pivot := Node3D.new()
		pivot.position = Vector3(0.0, 3.4, 0.0)
		pivot.rotation.y = base_rot
		pivot.set_meta("base_rot_y", base_rot)
		pivot.set_meta("phase", float(i) * 0.85)
		root.add_child(pivot)

		var cloth := MeshInstance3D.new()
		cloth.mesh = cloth_mesh
		cloth.material_override = home_mat if is_home else away_mat
		cloth.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		cloth.position = Vector3(1.2, 0.0, 0.0)
		pivot.add_child(cloth)

		_animated_flags.append(pivot)

func _build_turf() -> void:
	# Main playable pitch + apron (120m x 82m)
	var turf_mesh := PlaneMesh.new()
	turf_mesh.size = Vector2(124.0, 86.0)

	var mat := StandardMaterial3D.new()
	mat.albedo_texture = _create_striped_grass_texture()
	mat.roughness = 0.88
	mat.metallic = 0.0
	mat.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS

	var turf := MeshInstance3D.new()
	turf.name = "PitchTurf"
	turf.mesh = turf_mesh
	turf.material_override = mat
	turf.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(turf)

func _create_striped_grass_texture() -> ImageTexture:
	var w := 512
	var h := 256
	var img := Image.create(w, h, false, Image.FORMAT_RGBA8)
	var c_light := Color(0.16, 0.48, 0.18)
	var c_dark := Color(0.12, 0.40, 0.14)
	var c_apron := Color(0.10, 0.32, 0.12)

	var stripe_count := 18
	for x in range(w):
		var u := float(x) / float(w)
		var world_x := (u - 0.5) * 124.0
		var stripe_idx := int(floor((world_x + HALF_LEN) / (PITCH_LENGTH / float(stripe_count))))
		var col := c_light if (stripe_idx % 2 == 0) else c_dark
		if absf(world_x) > HALF_LEN + 1.5:
			col = c_apron
		for y in range(h):
			var v := float(y) / float(h)
			var world_z := (v - 0.5) * 86.0
			var final_col := c_apron if absf(world_z) > HALF_WID + 1.5 else col
			img.set_pixel(x, y, final_col)
	return ImageTexture.create_from_image(img)

func _build_lines() -> void:
	var line_mat := StandardMaterial3D.new()
	line_mat.albedo_color = Color(0.95, 0.97, 0.95)
	line_mat.roughness = 0.6

	var lines_root := Node3D.new()
	lines_root.name = "PitchLines"
	add_child(lines_root)

	# Outer boundary
	_add_rect_outline(lines_root, line_mat, 0.0, 0.0, PITCH_LENGTH, PITCH_WIDTH)
	# Halfway line
	_add_line_box(lines_root, line_mat, Vector3(0.0, LINE_Y, 0.0), Vector2(LINE_WIDTH, PITCH_WIDTH))
	# Center circle + spot
	_add_circle(lines_root, line_mat, Vector3.ZERO, 9.15, 48)
	_add_disc(lines_root, line_mat, Vector3.ZERO, 0.25, 16)

	# Penalty boxes, 6-yard boxes, spots & arcs for both ends
	for side: float in [-1.0, 1.0]:
		var goal_x := side * HALF_LEN
		# 18-yard box (16.5m deep, 40.32m wide)
		var pen_center_x := goal_x - side * (16.5 * 0.5)
		_add_rect_outline(lines_root, line_mat, pen_center_x, 0.0, 16.5, 40.32)
		# 6-yard box (5.5m deep, 18.32m wide)
		var six_center_x := goal_x - side * (5.5 * 0.5)
		_add_rect_outline(lines_root, line_mat, six_center_x, 0.0, 5.5, 18.32)
		# Penalty spot (11m from goal line)
		var spot_x := goal_x - side * 11.0
		_add_disc(lines_root, line_mat, Vector3(spot_x, 0.0, 0.0), 0.22, 16)
		# Penalty arc outside 18-yard box
		var arc_start := -0.92 if side < 0.0 else PI - 0.92
		var arc_end := 0.92 if side < 0.0 else PI + 0.92
		_add_arc(lines_root, line_mat, Vector3(spot_x, 0.0, 0.0), 9.15, arc_start, arc_end, 20)

	# 4 Corner arcs
	_add_arc(lines_root, line_mat, Vector3(-HALF_LEN, 0.0, -HALF_WID), 1.0, 0.0, PI * 0.5, 10)
	_add_arc(lines_root, line_mat, Vector3(-HALF_LEN, 0.0, HALF_WID), 1.0, -PI * 0.5, 0.0, 10)
	_add_arc(lines_root, line_mat, Vector3(HALF_LEN, 0.0, -HALF_WID), 1.0, PI * 0.5, PI, 10)
	_add_arc(lines_root, line_mat, Vector3(HALF_LEN, 0.0, HALF_WID), 1.0, PI, PI * 1.5, 10)

func _add_rect_outline(parent: Node3D, mat: Material, cx: float, cz: float, len_x: float, wid_z: float) -> void:
	_add_line_box(parent, mat, Vector3(cx, LINE_Y, cz - wid_z * 0.5), Vector2(len_x + LINE_WIDTH, LINE_WIDTH))
	_add_line_box(parent, mat, Vector3(cx, LINE_Y, cz + wid_z * 0.5), Vector2(len_x + LINE_WIDTH, LINE_WIDTH))
	_add_line_box(parent, mat, Vector3(cx - len_x * 0.5, LINE_Y, cz), Vector2(LINE_WIDTH, wid_z))
	_add_line_box(parent, mat, Vector3(cx + len_x * 0.5, LINE_Y, cz), Vector2(LINE_WIDTH, wid_z))

func _add_line_box(parent: Node3D, mat: Material, pos: Vector3, size_xz: Vector2) -> void:
	var mesh := BoxMesh.new()
	mesh.size = Vector3(size_xz.x, 0.01, size_xz.y)
	var mi := MeshInstance3D.new()
	mi.mesh = mesh
	mi.material_override = mat
	mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	mi.position = pos
	parent.add_child(mi)

func _add_circle(parent: Node3D, mat: Material, center: Vector3, radius: float, segs: int) -> void:
	_add_arc(parent, mat, center, radius, 0.0, TAU, segs)

func _add_arc(parent: Node3D, mat: Material, center: Vector3, radius: float, a0: float, a1: float, segs: int) -> void:
	for i in range(segs):
		var t0 := lerpf(a0, a1, float(i) / float(segs))
		var t1 := lerpf(a0, a1, float(i + 1) / float(segs))
		var p0 := Vector3(cos(t0) * radius, 0.0, sin(t0) * radius)
		var p1 := Vector3(cos(t1) * radius, 0.0, sin(t1) * radius)
		var mid := center + (p0 + p1) * 0.5
		mid.y = LINE_Y
		var seg_len := p0.distance_to(p1) + LINE_WIDTH * 0.5
		var angle := atan2(p1.z - p0.z, p1.x - p0.x)
		var mesh := BoxMesh.new()
		mesh.size = Vector3(seg_len, 0.01, LINE_WIDTH)
		var mi := MeshInstance3D.new()
		mi.mesh = mesh
		mi.material_override = mat
		mi.position = mid
		mi.rotation.y = -angle
		parent.add_child(mi)

func _add_disc(parent: Node3D, mat: Material, center: Vector3, radius: float, segs: int) -> void:
	var cyl := CylinderMesh.new()
	cyl.top_radius = radius
	cyl.bottom_radius = radius
	cyl.height = 0.01
	cyl.radial_segments = segs
	var mi := MeshInstance3D.new()
	mi.mesh = cyl
	mi.material_override = mat
	mi.position = Vector3(center.x, LINE_Y, center.z)
	parent.add_child(mi)

func _build_goal(goal_x: float, dir_out: float) -> void:
	var goal_root := Node3D.new()
	goal_root.name = "Goal_%s" % ("Left" if dir_out < 0.0 else "Right")
	add_child(goal_root)

	var post_mat := StandardMaterial3D.new()
	post_mat.albedo_color = Color(0.96, 0.96, 0.98)
	post_mat.roughness = 0.3
	post_mat.metallic = 0.2

	var post_r := 0.06
	# Left & Right posts
	for z_sign in [-1.0, 1.0]:
		var post_mesh := CylinderMesh.new()
		post_mesh.top_radius = post_r
		post_mesh.bottom_radius = post_r
		post_mesh.height = GOAL_HEIGHT
		var post := MeshInstance3D.new()
		post.mesh = post_mesh
		post.material_override = post_mat
		post.position = Vector3(goal_x, GOAL_HEIGHT * 0.5, z_sign * GOAL_HALF_W)
		goal_root.add_child(post)

	# Crossbar
	var bar_mesh := CylinderMesh.new()
	bar_mesh.top_radius = post_r
	bar_mesh.bottom_radius = post_r
	bar_mesh.height = GOAL_WIDTH + post_r * 2.0
	var bar := MeshInstance3D.new()
	bar.mesh = bar_mesh
	bar.material_override = post_mat
	bar.position = Vector3(goal_x, GOAL_HEIGHT, 0.0)
	bar.rotation.x = PI * 0.5
	goal_root.add_child(bar)

	# Back net frame & translucent net
	var net_mat := StandardMaterial3D.new()
	net_mat.albedo_color = Color(0.92, 0.95, 1.0, 0.28)
	net_mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	net_mat.cull_mode = BaseMaterial3D.CULL_DISABLED

	var back_x := goal_x + dir_out * GOAL_DEPTH
	var mid_x := goal_x + dir_out * (GOAL_DEPTH * 0.5)

	# Back net panel
	var back_panel := BoxMesh.new()
	back_panel.size = Vector3(0.03, GOAL_HEIGHT, GOAL_WIDTH)
	var back_mi := MeshInstance3D.new()
	back_mi.mesh = back_panel
	back_mi.material_override = net_mat
	back_mi.position = Vector3(back_x, GOAL_HEIGHT * 0.5, 0.0)
	goal_root.add_child(back_mi)

	# Top net panel
	var top_panel := BoxMesh.new()
	top_panel.size = Vector3(GOAL_DEPTH, 0.03, GOAL_WIDTH)
	var top_mi := MeshInstance3D.new()
	top_mi.mesh = top_panel
	top_mi.material_override = net_mat
	top_mi.position = Vector3(mid_x, GOAL_HEIGHT, 0.0)
	goal_root.add_child(top_mi)

	# Side net panels
	for z_sign in [-1.0, 1.0]:
		var side_panel := BoxMesh.new()
		side_panel.size = Vector3(GOAL_DEPTH, GOAL_HEIGHT, 0.03)
		var side_mi := MeshInstance3D.new()
		side_mi.mesh = side_panel
		side_mi.material_override = net_mat
		side_mi.position = Vector3(mid_x, GOAL_HEIGHT * 0.5, z_sign * GOAL_HALF_W)
		goal_root.add_child(side_mi)

func _build_corner_flags() -> void:
	var pole_mat := StandardMaterial3D.new()
	pole_mat.albedo_color = Color(0.95, 0.95, 0.95)
	var flag_mat := StandardMaterial3D.new()
	flag_mat.albedo_color = Color(0.95, 0.2, 0.15)

	for sx in [-1.0, 1.0]:
		for sz in [-1.0, 1.0]:
			var pole := MeshInstance3D.new()
			var cm := CylinderMesh.new()
			cm.top_radius = 0.025
			cm.bottom_radius = 0.025
			cm.height = 1.4
			pole.mesh = cm
			pole.material_override = pole_mat
			pole.position = Vector3(sx * HALF_LEN, 0.7, sz * HALF_WID)
			add_child(pole)

			var flag := MeshInstance3D.new()
			var bm := BoxMesh.new()
			bm.size = Vector3(0.32, 0.22, 0.02)
			flag.mesh = bm
			flag.material_override = flag_mat
			flag.position = Vector3(sx * HALF_LEN + (-sx * 0.16), 1.28, sz * HALF_WID)
			add_child(flag)

func _build_extracted_stadium(stadium_name: String) -> void:
	var stadium := Node3D.new()
	stadium.name = "ExtractedStadium_" + stadium_name
	add_child(stadium)

	var mesh_dir := "res://assets/3d/stadiums/%s/" % stadium_name
	var tex_dir := _resolve_stadium_tex_dir(stadium_name)
	var textures := _load_stadium_textures(tex_dir)

	var dir := DirAccess.open(mesh_dir)
	if dir == null:
		_build_stadium_surroundings()
		return

	var obj_files: Array[String] = []
	dir.list_dir_begin()
	var fname := dir.get_next()
	while fname != "":
		if not dir.current_is_dir() and fname.ends_with(".obj") and not fname.ends_with(".import"):
			obj_files.append(fname)
		fname = dir.get_next()
	dir.list_dir_end()
	obj_files.sort()

	if obj_files.is_empty():
		_build_stadium_surroundings()
		return

	# Authentic WE3 scale (part_00 spans 123.2m x 92.4m = 105x68 pitch + apron)
	var stad_scale := Vector3.ONE
	for idx in range(obj_files.size()):
		var obj_path := mesh_dir + obj_files[idx]
		var mesh_res: Variant = load(obj_path)
		if not (mesh_res is Mesh):
			continue
		var aabb: AABB = (mesh_res as Mesh).get_aabb()
		var is_goal_mesh := aabb.size.x < 10.0 and aabb.size.y < 4.0 and aabb.size.z < 1.0
		if is_goal_mesh:
			# Handled cleanly by 3D _build_goal() (avoids opaque flat plane across goal mouth)
			continue

		var is_pitch_part := obj_files[idx].begins_with("part_00")
		# Use native .obj + stadium.mtl + exact PS1 UV/CLUT textures imported by Godot
		var m := mesh_res as Mesh
		for s in range(m.get_surface_count()):
			var smat := m.surface_get_material(s)
			if smat is StandardMaterial3D:
				var std := smat as StandardMaterial3D
				std.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
				std.cull_mode = BaseMaterial3D.CULL_DISABLED
				std.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
				if is_pitch_part:
					std.transparency = BaseMaterial3D.TRANSPARENCY_DISABLED
				else:
					std.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA_SCISSOR
					std.alpha_scissor_threshold = 0.5

		# Primary mesh
		var mi := MeshInstance3D.new()
		mi.mesh = mesh_res
		mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		mi.rotation.y = PI * 0.5
		mi.scale = stad_scale
		mi.position.y = 0.0
		stadium.add_child(mi)

		# Mirror only half-stadium parts (aabb.end.z <= 8.0) onto the opposite half
		if aabb.end.z <= 8.0:
			var mi_mirror := MeshInstance3D.new()
			mi_mirror.mesh = mesh_res
			mi_mirror.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
			mi_mirror.rotation.y = -PI * 0.5
			mi_mirror.scale = stad_scale
			mi_mirror.position.y = 0.0
			stadium.add_child(mi_mirror)

func _resolve_stadium_tex_dir(stadium_name: String) -> String:
	if stadium_name == "GRDM_FN":
		return "res://assets/textures/stadiums/GDC_FNS/"
	if stadium_name == "GRDM_I":
		return "res://assets/textures/stadiums/GDC_I/"
	var suffix := stadium_name.replace("GRDM_", "")
	var candidates := [
		"res://assets/textures/stadiums/GDC_%sDS/" % suffix,
		"res://assets/textures/stadiums/GDC_%sNS/" % suffix,
		"res://assets/textures/stadiums/GDC_CDS/",
	]
	for c in candidates:
		if DirAccess.dir_exists_absolute(c):
			return c
	return "res://assets/textures/stadiums/GDC_CDS/"

func _load_stadium_textures(tex_dir: String) -> Array[Texture2D]:
	var out: Array[Texture2D] = []
	var dir := DirAccess.open(tex_dir)
	if dir == null:
		return out
	var files: Array[String] = []
	dir.list_dir_begin()
	var f := dir.get_next()
	while f != "":
		if not dir.current_is_dir() and f.ends_with(".png") and not f.ends_with(".import"):
			files.append(f)
		f = dir.get_next()
	dir.list_dir_end()
	files.sort()
	for fname in files:
		var tex := load(tex_dir + fname) as Texture2D
		if tex != null:
			out.append(tex)
	return out

func _build_stadium_surroundings() -> void:
	var stadium := Node3D.new()
	stadium.name = "Stadium"
	add_child(stadium)

	# Perimeter ad boards along north & south touchlines and behind goals
	var ad_colors := [
		Color(0.10, 0.28, 0.65),
		Color(0.75, 0.12, 0.16),
		Color(0.90, 0.68, 0.10),
		Color(0.12, 0.52, 0.26)
	]
	for i in range(12):
		var mat := StandardMaterial3D.new()
		mat.albedo_color = ad_colors[i % ad_colors.size()]
		var bm := BoxMesh.new()
		bm.size = Vector3(8.5, 1.0, 0.25)
		var x_pos := -48.0 + float(i) * 8.7
		for z_pos in [-HALF_WID - 4.2, HALF_WID + 4.2]:
			var mi := MeshInstance3D.new()
			mi.mesh = bm
			mi.material_override = mat
			mi.position = Vector3(x_pos, 0.5, z_pos)
			stadium.add_child(mi)

	# Tiered concrete/crowd stands around all 4 sides
	var stand_mat := StandardMaterial3D.new()
	stand_mat.albedo_color = Color(0.22, 0.24, 0.28)
	stand_mat.roughness = 0.9

	var crowd_mat := StandardMaterial3D.new()
	crowd_mat.albedo_color = Color(0.35, 0.30, 0.36)
	crowd_mat.roughness = 0.95

	# North & South main stands
	for sz in [-1.0, 1.0]:
		for tier in range(3):
			var bm := BoxMesh.new()
			bm.size = Vector3(114.0, 2.2, 6.0)
			var mi := MeshInstance3D.new()
			mi.mesh = bm
			mi.material_override = crowd_mat if tier < 2 else stand_mat
			mi.position = Vector3(0.0, 1.1 + float(tier) * 2.2, sz * (HALF_WID + 10.0 + float(tier) * 5.0))
			stadium.add_child(mi)

	# East & West end stands
	for sx in [-1.0, 1.0]:
		for tier in range(2):
			var bm := BoxMesh.new()
			bm.size = Vector3(6.0, 2.2, 76.0)
			var mi := MeshInstance3D.new()
			mi.mesh = bm
			mi.material_override = crowd_mat if tier == 0 else stand_mat
			mi.position = Vector3(sx * (HALF_LEN + 10.0 + float(tier) * 5.0), 1.1 + float(tier) * 2.2, 0.0)
			stadium.add_child(mi)

	# 4 Corner floodlight towers
	var tower_mat := StandardMaterial3D.new()
	tower_mat.albedo_color = Color(0.75, 0.78, 0.82)
	var lamp_mat := StandardMaterial3D.new()
	lamp_mat.albedo_color = Color(1.0, 0.98, 0.88)
	lamp_mat.emission_enabled = true
	lamp_mat.emission = Color(1.0, 0.96, 0.82)
	lamp_mat.emission_energy_multiplier = 2.5

	for sx in [-1.0, 1.0]:
		for sz in [-1.0, 1.0]:
			var pole := MeshInstance3D.new()
			var cm := CylinderMesh.new()
			cm.top_radius = 0.35
			cm.bottom_radius = 0.55
			cm.height = 24.0
			pole.mesh = cm
			pole.material_override = tower_mat
			pole.position = Vector3(sx * (HALF_LEN + 14.0), 12.0, sz * (HALF_WID + 14.0))
			stadium.add_child(pole)

			var bank := MeshInstance3D.new()
			var bm := BoxMesh.new()
			bm.size = Vector3(4.5, 2.2, 0.6)
			bank.mesh = bm
			bank.material_override = lamp_mat
			stadium.add_child(bank)
			bank.look_at_from_position(Vector3(sx * (HALF_LEN + 14.0), 24.2, sz * (HALF_WID + 14.0)), Vector3.ZERO, Vector3.UP)
