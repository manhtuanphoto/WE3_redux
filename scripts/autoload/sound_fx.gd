extends Node
## Synthesizes retro PS1-style match sound effects (whistle, kick, crowd cheer, post clang).

const SAMPLE_RATE := 22050

var _players: Dictionary = {}
var _streams: Dictionary = {}
var _bgm_player: AudioStreamPlayer = null
var _crowd_player: AudioStreamPlayer = null
var _commentary_player: AudioStreamPlayer = null
var _commentary_banks: Dictionary = {}
var _commentary_en_banks: Dictionary = {}
var _commentary_player_clips: Dictionary = {}
var _commentary_queue: Array[AudioStream] = []
var _last_comm_ms: int = 0
var _current_bgm: String = ""
var music_muted: bool = false

func _ready() -> void:
	# Verified acoustic mappings from cleanly decoded WE3 VAB (ISSPRO2A.RA @0x3800) with exact VAB tone sample rates:
	# sfx_02 (0.26s, 22050Hz, 524Hz punch) = ball kick; sfx_04 (0.63s, 22050Hz, 2600Hz decay) = goalpost clang;
	# sfx_15 (0.20s, 16564Hz, 2532Hz) = short whistle; sfx_09 (0.85s, 22050Hz, 2998Hz) = long whistle;
	# sfx_16 (5.54s, 10735Hz) = stadium goal roar; sfx_12 (2.45s, 18542Hz, PS1 loop 12740..45360) = crowd ambience.
	_streams["whistle"] = _load_sfx_or_fallback("res://assets/audio/sfx/sfx_15.wav", _gen_whistle(0.45))
	_streams["whistle_long"] = _load_sfx_or_fallback("res://assets/audio/sfx/sfx_09.wav", _gen_whistle(0.85))
	_streams["kick"] = _load_sfx_or_fallback("res://assets/audio/sfx/sfx_02.wav", _gen_kick(0.14))
	_streams["post"] = _load_sfx_or_fallback("res://assets/audio/sfx/sfx_04.wav", _gen_post_clang(0.32))
	_streams["cheer"] = _load_sfx_or_fallback("res://assets/audio/sfx/sfx_16.wav", _gen_cheer(1.6))

	for key in _streams.keys():
		var asp := AudioStreamPlayer.new()
		asp.stream = _streams[key]
		asp.volume_db = -6.0 if key != "cheer" else -8.0
		add_child(asp)
		_players[key] = asp

	_bgm_player = AudioStreamPlayer.new()
	_bgm_player.volume_db = -14.0
	_bgm_player.finished.connect(func():
		if not music_muted and _bgm_player.stream != null:
			_bgm_player.play()
	)
	add_child(_bgm_player)

	# Authentic PS1 hardware-looped stadium crowd ambience (sfx_12.wav, VAG loop flags 12740..45360)
	_crowd_player = AudioStreamPlayer.new()
	_crowd_player.volume_db = -20.0
	var crowd_wav := _load_sfx_or_fallback("res://assets/audio/sfx/sfx_12.wav", _gen_cheer(2.4))
	if crowd_wav is AudioStreamWAV:
		var dup_crowd := (crowd_wav as AudioStreamWAV).duplicate() as AudioStreamWAV
		var total_frames := dup_crowd.data.size() / 2
		dup_crowd.loop_mode = AudioStreamWAV.LOOP_FORWARD
		dup_crowd.loop_begin = mini(12740, maxi(0, total_frames - 2048))
		dup_crowd.loop_end = mini(45360, total_frames)
		_crowd_player.stream = dup_crowd
	else:
		_crowd_player.stream = crowd_wav
	add_child(_crowd_player)

	_commentary_player = AudioStreamPlayer.new()
	_commentary_player.volume_db = -4.0
	_commentary_player.finished.connect(_on_commentary_finished)
	add_child(_commentary_player)
	_commentary_banks = {
		"goal": ["res://assets/audio/commentary/isspro3a_00.wav", "res://assets/audio/commentary/isspro3b_00.wav"],
		"kickoff": ["res://assets/audio/commentary/isspro2b_00.wav", "res://assets/audio/commentary/isspro2c_00.wav"],
		"setpiece": ["res://assets/audio/commentary/isspro2b_01.wav", "res://assets/audio/commentary/isspro2c_01.wav"],
		"shot": ["res://assets/audio/commentary/isspro3a_01.wav", "res://assets/audio/commentary/isspro3b_01.wav"],
	}
	_scan_external_commentary()

func _scan_external_commentary() -> void:
	const EVENTS := ["goal", "kickoff", "shot", "save", "post", "foul", "yellow_card", "red_card", "offside", "corner", "throw_in", "free_kick", "penalty", "halftime", "fulltime", "setpiece"]
	for ev in EVENTS:
		var dir_p := "res://assets/audio/commentary/en/%s/" % ev
		var files := _list_audio_files(dir_p)
		if not files.is_empty():
			_commentary_en_banks[ev] = files
	var p_dir := "res://assets/audio/commentary/players/"
	for p_path in _list_audio_files(p_dir):
		var key := p_path.get_file().get_basename().to_lower()
		_commentary_player_clips[key] = p_path

func _list_audio_files(dir_path: String) -> Array[String]:
	var out: Array[String] = []
	var dir := DirAccess.open(dir_path)
	if dir == null:
		return out
	dir.list_dir_begin()
	var fn := dir.get_next()
	while fn != "":
		if not dir.current_is_dir() and not fn.ends_with(".import"):
			var low := fn.to_lower()
			if low.ends_with(".wav") or low.ends_with(".ogg") or low.ends_with(".mp3"):
				out.append(dir_path + fn)
		fn = dir.get_next()
	dir.list_dir_end()
	out.sort()
	return out

func _on_commentary_finished() -> void:
	if not _commentary_queue.is_empty() and _commentary_player != null and is_inside_tree():
		var next_s: AudioStream = _commentary_queue.pop_front()
		if next_s != null:
			_commentary_player.stream = next_s
			_commentary_player.play()

func play_commentary(event_key: String, player_name: String = "") -> void:
	if _commentary_player == null or not is_inside_tree():
		return
	var high_prio := event_key in ["goal", "red_card", "penalty", "halftime", "fulltime"]
	var now_ms := Time.get_ticks_msec()
	if not high_prio:
		if _commentary_player.playing or (now_ms - _last_comm_ms) < 1600:
			return
	else:
		_commentary_queue.clear()

	# Prefer external English pack (assets/audio/commentary/en/<event>/), fallback to Japanese WE3 clips
	var list: Array = _commentary_en_banks.get(event_key, [])
	if list.is_empty():
		var fb_key := event_key if _commentary_banks.has(event_key) else ("setpiece" if event_key in ["corner", "throw_in", "free_kick", "foul", "offside"] else "")
		if fb_key != "":
			list = _commentary_banks.get(fb_key, [])
	var ev_stream: AudioStream = null
	if not list.is_empty():
		var path: String = list[randi() % list.size()]
		if ResourceLoader.exists(path):
			ev_stream = load(path) as AudioStream

	var name_stream: AudioStream = null
	if player_name != "":
		var norm := player_name.strip_edges().to_lower().replace(" ", "_")
		if _commentary_player_clips.has(norm):
			var np: String = _commentary_player_clips[norm]
			if ResourceLoader.exists(np):
				name_stream = load(np) as AudioStream

	var seq: Array[AudioStream] = []
	if event_key == "goal":
		if ev_stream != null: seq.append(ev_stream)
		if name_stream != null: seq.append(name_stream)
	else:
		if name_stream != null: seq.append(name_stream)
		if ev_stream != null: seq.append(ev_stream)
	if seq.is_empty():
		return

	_last_comm_ms = now_ms
	_commentary_player.stream = seq[0]
	_commentary_player.play()
	for i in range(1, seq.size()):
		_commentary_queue.append(seq[i])

func _load_sfx_or_fallback(path: String, fallback: AudioStream) -> AudioStream:
	if ResourceLoader.exists(path):
		var s := load(path) as AudioStream
		if s != null:
			return s
	return fallback

func _input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		if (event as InputEventKey).keycode == KEY_M:
			music_muted = not music_muted
			if _bgm_player != null:
				_bgm_player.stream_paused = music_muted
			if _crowd_player != null:
				_crowd_player.stream_paused = music_muted

func start_crowd_ambience() -> void:
	if _crowd_player != null and is_inside_tree():
		_crowd_player.set_meta("active", true)
		if not _crowd_player.playing and not music_muted:
			_crowd_player.play()

func stop_crowd_ambience() -> void:
	if _crowd_player != null:
		_crowd_player.set_meta("active", false)
		_crowd_player.stop()

func stop_music() -> void:
	_current_bgm = ""
	if _bgm_player != null:
		_bgm_player.stop()

func play_music(track_name: String) -> void:
	if _bgm_player == null or _current_bgm == track_name:
		return
	var path := "res://assets/audio/%s.wav" % track_name
	if not ResourceLoader.exists(path):
		return
	var stream := load(path) as AudioStream
	if stream != null:
		_current_bgm = track_name
		_bgm_player.stream = stream
		_bgm_player.play()
		_bgm_player.stream_paused = music_muted

func play_sfx(sfx_name: String) -> void:
	if _players.has(sfx_name):
		var asp: AudioStreamPlayer = _players[sfx_name]
		if asp != null and is_inside_tree():
			asp.play()

func _make_wav(samples: PackedByteArray) -> AudioStreamWAV:
	var wav := AudioStreamWAV.new()
	wav.format = AudioStreamWAV.FORMAT_8_BITS
	wav.mix_rate = SAMPLE_RATE
	wav.stereo = false
	wav.data = samples
	return wav

func _gen_whistle(dur: float) -> AudioStreamWAV:
	var count := int(dur * SAMPLE_RATE)
	var data := PackedByteArray()
	data.resize(count)
	for i in range(count):
		var t := float(i) / float(SAMPLE_RATE)
		var env := minf(1.0, t * 25.0) * minf(1.0, (dur - t) * 18.0)
		var mod := 1.0 + 0.04 * sin(TAU * 42.0 * t)
		var s := sin(TAU * 2850.0 * mod * t) * 0.65 + sin(TAU * 3120.0 * t) * 0.35
		var val := clampi(int(s * env * 95.0), -127, 127)
		data[i] = (val + 256) & 0xFF
	return _make_wav(data)

func _gen_kick(dur: float) -> AudioStreamWAV:
	var count := int(dur * SAMPLE_RATE)
	var data := PackedByteArray()
	data.resize(count)
	for i in range(count):
		var t := float(i) / float(SAMPLE_RATE)
		var env := exp(-t * 28.0)
		var freq := lerpf(170.0, 52.0, minf(1.0, t * 14.0))
		var s := sin(TAU * freq * t) * 0.8 + (randf() * 2.0 - 1.0) * 0.2 * exp(-t * 45.0)
		var val := clampi(int(s * env * 115.0), -127, 127)
		data[i] = (val + 256) & 0xFF
	return _make_wav(data)

func _gen_post_clang(dur: float) -> AudioStreamWAV:
	var count := int(dur * SAMPLE_RATE)
	var data := PackedByteArray()
	data.resize(count)
	for i in range(count):
		var t := float(i) / float(SAMPLE_RATE)
		var env := exp(-t * 11.0)
		var s := sin(TAU * 520.0 * t) * 0.6 + sin(TAU * 785.0 * t) * 0.4
		var val := clampi(int(s * env * 100.0), -127, 127)
		data[i] = (val + 256) & 0xFF
	return _make_wav(data)

func _gen_cheer(dur: float) -> AudioStreamWAV:
	var count := int(dur * SAMPLE_RATE)
	var data := PackedByteArray()
	data.resize(count)
	var prev := 0.0
	for i in range(count):
		var t := float(i) / float(SAMPLE_RATE)
		var env := minf(1.0, t * 4.0) * minf(1.0, (dur - t) * 1.8)
		var noise := randf() * 2.0 - 1.0
		prev = lerpf(prev, noise, 0.22)
		var val := clampi(int(prev * env * 110.0), -127, 127)
		data[i] = (val + 256) & 0xFF
	return _make_wav(data)
