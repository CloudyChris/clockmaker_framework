@tool
class_name GraphUIElement
extends UIElement

@export var ui_graph: UICodeGraph = null
#-------------------------------------------------------------------------------
# TODO Consider making all stuff defined by a graph have another master graph that allows for output
# of the user-created graph, or else, how are we gonna build UI or influence the battle map?
# Alternatively find another solution, like potentially giving the run method vargs
func build_ui() -> void:
	ui_tree = null
	if ui_graph:
		ui_graph.run()
