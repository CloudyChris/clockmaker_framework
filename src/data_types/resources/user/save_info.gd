# save_info.gd
@tool
class_name SaveInfo
extends GameDataEntry

## play time in seconds
@export var play_time : int = 0
## screenshot taken just before saving
@export var screenshot : ImageTexture = null
## save file is a collection
@export var save_path : String = ""
