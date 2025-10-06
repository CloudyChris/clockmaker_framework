# session_manager.gd
extends BaseManager

var data : SessionData = SessionData.new()
var time_accumulator : float = 0
var frames : int = 0
#-------------------------------------------------------------------------------
func load_manager() -> bool:
	data = ResourceManager.get_entry("", "/game/session/data", ResourceManager.CollectionType.NONE)
	if not data:
		data = SessionData.new()
	
	if not data:
		push_error("Session manager failed to load. No bitches")
		return false
	
	return true

func reset_manager() -> bool:
	data = null
	data.is_paused = true
	time_accumulator = 0
	return true
#-------------------------------------------------------------------------------
func play() -> void:
	if data:
		data.is_paused = false
		get_tree().paused = false
	else:
		push_error("No session data")

func pause() -> void:
	if data:
		data.is_paused = true
		get_tree().paused = true
	else:
		push_error("No session data")
#-------------------------------------------------------------------------------
func get_data() -> SessionData:
	return data
#-------------------------------------------------------------------------------
func _process(delta: float) -> void:
	if data:
		if not data.is_paused:
			if time_accumulator >= 1:
				data.session_time += 1
				time_accumulator -= 1
			time_accumulator += delta
