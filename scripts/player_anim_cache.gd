class_name PlayerAnimCache
extends RefCounted
## Loads and caches WE3 12-bone animations into a shared Godot AnimationLibrary.

const ANIM_MAP := {
	"idle": {"path": "res://assets/3d/animations/anim_001_21f.gltf", "loop": true},
	"run": {"path": "res://assets/3d/animations/anim_002_18f.gltf", "loop": true},
	"sprint": {"path": "res://assets/3d/animations/anim_004_14f.gltf", "loop": true},
	"sprint_alt": {"path": "res://assets/3d/animations/anim_089_14f.gltf", "loop": true},
	"dribble": {"path": "res://assets/3d/animations/anim_027_16f.gltf", "loop": true},
	"dribble_turn": {"path": "res://assets/3d/animations/anim_038_16f.gltf", "loop": true},
	"dribble_fast": {"path": "res://assets/3d/animations/anim_052_16f.gltf", "loop": true},
	"dribble_alt": {"path": "res://assets/3d/animations/anim_024_16f.gltf", "loop": true},
	"turn_brake": {"path": "res://assets/3d/animations/anim_014_16f.gltf", "loop": false},
	"pass": {"path": "res://assets/3d/animations/anim_006_10f.gltf", "loop": false},
	"pass_alt": {"path": "res://assets/3d/animations/anim_010_13f.gltf", "loop": false},
	"pass_long": {"path": "res://assets/3d/animations/anim_016_23f.gltf", "loop": false},
	"cross_alt": {"path": "res://assets/3d/animations/anim_011_14f.gltf", "loop": false},
	"header": {"path": "res://assets/3d/animations/anim_007_10f.gltf", "loop": false},
	"header_alt": {"path": "res://assets/3d/animations/anim_028_12f.gltf", "loop": false},
	"shoot": {"path": "res://assets/3d/animations/anim_015_44f.gltf", "loop": false},
	"volley": {"path": "res://assets/3d/animations/anim_029_28f.gltf", "loop": false},
	"trap": {"path": "res://assets/3d/animations/anim_035_15f.gltf", "loop": false},
	"tackle": {"path": "res://assets/3d/animations/anim_030_26f.gltf", "loop": false},
	"recover": {"path": "res://assets/3d/animations/anim_084_27f.gltf", "loop": false},
	"throw_in": {"path": "res://assets/3d/animations/anim_053_35f.gltf", "loop": false},
	"gk_catch": {"path": "res://assets/3d/animations/anim_115_18f.gltf", "loop": false},
	"gk_dive": {"path": "res://assets/3d/animations/anim_109_19f.gltf", "loop": false},
	"gk_save_alt": {"path": "res://assets/3d/animations/anim_036_25f.gltf", "loop": false},
	"gk_throw": {"path": "res://assets/3d/animations/anim_096_36f.gltf", "loop": false},
	"foul_react": {"path": "res://assets/3d/animations/anim_074_31f.gltf", "loop": false},
	"celebrate": {"path": "res://assets/3d/animations/anim_039_54f.gltf", "loop": true},
	"celebrate_alt": {"path": "res://assets/3d/animations/anim_044_47f.gltf", "loop": true},
	"celebrate_jump": {"path": "res://assets/3d/animations/anim_005_36f.gltf", "loop": true},
	"celebrate_3": {"path": "res://assets/3d/animations/anim_102_36f.gltf", "loop": true},
	"celebrate_4": {"path": "res://assets/3d/animations/anim_103_36f.gltf", "loop": true},
	"ending_celebrate": {"path": "res://assets/3d/animations/ending/end_anim_05_36f.gltf", "loop": true},
	"ending_idle": {"path": "res://assets/3d/animations/ending/end_anim_01_21f.gltf", "loop": true},
	"ending_walk": {"path": "res://assets/3d/animations/ending/end_anim_02_18f.gltf", "loop": true},
	"ending_bow": {"path": "res://assets/3d/animations/ending/end_anim_06_10f.gltf", "loop": false},
	"bicycle": {"path": "res://assets/3d/animations/anim_045_29f.gltf", "loop": false},
}

static var _cached_library: AnimationLibrary = null

static func get_library() -> AnimationLibrary:
	if _cached_library != null:
		return _cached_library

	var lib := AnimationLibrary.new()
	for anim_name in ANIM_MAP.keys():
		var info: Dictionary = ANIM_MAP[anim_name]
		var path: String = info["path"]
		if not ResourceLoader.exists(path):
			continue
		var packed := load(path) as PackedScene
		if packed == null:
			continue
		var inst := packed.instantiate()
		var ap := _find_anim_player(inst)
		if ap != null:
			var list := ap.get_animation_list()
			if not list.is_empty():
				var src_anim := ap.get_animation(list[0])
				if src_anim != null:
					var dup := src_anim.duplicate(true) as Animation
					dup.loop_mode = Animation.LOOP_LINEAR if info["loop"] else Animation.LOOP_NONE
					_smooth_animation_tracks(dup)
					lib.add_animation(anim_name, dup)
		inst.free()

	_cached_library = lib
	return _cached_library

static func _smooth_animation_tracks(anim: Animation) -> void:
	for t in range(anim.get_track_count()):
		anim.track_set_interpolation_type(t, Animation.INTERPOLATION_LINEAR)
		anim.track_set_interpolation_loop_wrap(t, true)
		if anim.track_get_type(t) == Animation.TYPE_ROTATION_3D:
			var kcount := anim.track_get_key_count(t)
			if kcount >= 2:
				var prev_q: Quaternion = anim.track_get_key_value(t, 0)
				for k in range(1, kcount):
					var q: Quaternion = anim.track_get_key_value(t, k)
					if prev_q.dot(q) < 0.0:
						q = -q
						anim.track_set_key_value(t, k, q)
					prev_q = q

static func _find_anim_player(n: Node) -> AnimationPlayer:
	if n is AnimationPlayer:
		return n
	for c in n.get_children():
		var found := _find_anim_player(c)
		if found != null:
			return found
	return null

static var _cached_shadow_tex: Texture2D = null

static func get_shadow_texture() -> Texture2D:
	if _cached_shadow_tex != null:
		return _cached_shadow_tex
	const SZ := 32
	var img := Image.create(SZ, SZ, false, Image.FORMAT_RGBA8)
	var c := (SZ - 1) * 0.5
	for y in range(SZ):
		for x in range(SZ):
			var dx := (x - c) / c
			var dy := (y - c) / c
			var r2 := dx * dx + dy * dy
			var alpha := clampf(1.0 - r2, 0.0, 1.0)
			alpha = alpha * alpha * 0.52
			img.set_pixel(x, y, Color(0.02, 0.04, 0.03, alpha))
	_cached_shadow_tex = ImageTexture.create_from_image(img)
	return _cached_shadow_tex
