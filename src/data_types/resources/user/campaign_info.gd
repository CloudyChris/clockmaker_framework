# campaign_info.gd
@tool
class_name CampaignInfo
extends GameDataEntry

@export var hero_name : String = ""
## play time in seconds
@export var play_time : int = 0
## portrait of the hero
@export var picture : ImageTexture = null
## list of save info to be displayed in load menu
@export var save_list : Array[SaveInfo] = []
